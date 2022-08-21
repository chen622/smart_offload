/*
 * MIT License
 * 
 * Copyright (c) 2021 Chenming C (ccm@ccm.ink)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#include "v1/flow_event.h"

/**
 * Used to queue the result of a flow counter.
 *
 * @param port_id The port id of flow.
 * @param flow The target flow.
 * @param query_counter The result.
 * @param error The error occur when querying a flow.
 * @return
 *      - On success, zero.
 *      - On failure, a negative value.
 */
static int query_counter(uint16_t port_id, struct rte_flow *flow, struct rte_flow_query_count *counter,
                         struct rte_flow_error *error) {
    /* Define the query action */
    struct rte_flow_action_count dedicated_counter = {
    };
    struct rte_flow_action actions[2];
    actions[1].type = RTE_FLOW_ACTION_TYPE_END;
    actions[0].type = RTE_FLOW_ACTION_TYPE_COUNT;
    actions[0].conf = &dedicated_counter;
    if (rte_flow_query(port_id, flow, actions, counter, error)) {
        zlog_category_t *zc = zlog_get_category("flow_event");
        zlog_error(zc, "cannot get the result of counter: %s", error->message);
        return -1;
    }
    return 0;
}

void delete_timeout_flows(void *params) {
    /* Get parameters and free memory */
    zlog_category_t *zc = zlog_get_category("flow_event");
    uint16_t port_id = ((struct delete_flow_params *) params)->port_id;
    struct rte_hash *flow_hash_map = ((struct delete_flow_params *) params)->flow_hash_map;
    rte_free(params);

    void **flow_keys = 0;
    int timeout_quantity = 0;
    struct rte_flow_error flow_error = {0};
    int ret = 0;

    /* Get the amount of timeout flows */
    timeout_quantity = rte_flow_get_aged_flows(port_id, NULL, 0, &flow_error);
    if (timeout_quantity == 0) {
        return;
    }

    /* Get the timeout flows */
    flow_keys = rte_zmalloc("timeout_flow_keys", sizeof(void *) * timeout_quantity, 0);
    if (flow_keys == NULL) {
        zlog_error(zc, "cannot allocate memory for timeout flow keys");
        return;
    }
    ret = rte_flow_get_aged_flows(port_id, flow_keys, timeout_quantity, &flow_error);
    if (ret != timeout_quantity) {
        rte_free(flow_keys);
        zlog_error(zc, "port:%d get aged flows count(%d) != total(%d)", port_id, ret, timeout_quantity);
        return;
    }

    union ipv4_5tuple_host *flow_key = 0;
    struct flow_meta *flow_data = 0;
    char pkt_info[MAX_ERROR_MESSAGE_LENGTH];
    for (int i = 0; i < timeout_quantity; ++i) {
        if (!flow_keys[i]) {
            zlog_error(zc, "get NULL flow key");
            continue;
        }
        flow_key = (union ipv4_5tuple_host *) flow_keys[i];
        dump_pkt_info(flow_key, -1, pkt_info, MAX_ERROR_MESSAGE_LENGTH);
        ret = rte_hash_lookup_data(flow_hash_map, flow_key, (void **) &flow_data);
        if (ret == -ENOENT) { // the key doesn't appear in the map
            zlog_error(zc, "cannot find a packet(%s) when deleting", pkt_info);
        } else if (ret >= 0) { // the key in the map
            /* Delete the flow from the flow map */
            int32_t del_key = rte_hash_del_key(flow_hash_map, flow_key);
            ret = rte_hash_free_key_with_position(flow_hash_map, del_key);
            if (ret) {
                zlog_error(zc, "cannot free hash key(%s)", pkt_info);
            }
            if (flow_data->flow == NULL) {
                zlog_error(zc, "cannot get the rte_flow of packet(%s)", pkt_info);
            } else {
                /* Query the counter of the timeout flow */
                struct rte_flow_query_count counter = {0};
                ret = query_counter(port_id, flow_data->flow, &counter, &flow_error);
                if (ret) {
                    zlog_error(zc, "cannot query the counter of a timeout flow: %s", flow_error.message);
                } else {
                    zlog_info(zc,
                              "flow(%s) timeout, total has %lu packets, offload has %lu packets and up has %u packets.",
                              pkt_info,
                              flow_data->packet_amount + counter.hits, counter.hits, flow_data->packet_amount);
                }

                /* Delete the flow from nic */
                ret = rte_flow_destroy(port_id, flow_data->flow, &flow_error);
                if (ret) {
                    zlog_error(zc, "cannot remove a offload rte_flow from nic: %s", flow_error.message);
                } else {
                    zlog_info(zc, "flow(%s) has been delete because timeout", pkt_info);
                }
            }
            rte_free(flow_data);
        } else {
            dump_pkt_info(flow_key, -1, pkt_info, MAX_ERROR_MESSAGE_LENGTH);
            zlog_error(zc, "invalid parameter(%s) for search timeout flow", pkt_info);
        }
    }
    rte_free(flow_keys);
}

/**
 * Execute when a flow timeout. This function will be running in a interrupt thread, so it should run as fast as possible.
 *
 * @param port_id The port id of flow.
 * @param type The type of event.
 * @param flow_hash_map The hash map to save flows.
 * @param ret_param Unused.
 *      - On success, zero.
 *      - On failure, a negative value.
 */
static int aged_event_callback(uint16_t port_id, enum rte_eth_event_type type,
                               void *flow_hash_map, void *ret_param) {
    RTE_SET_USED(ret_param);
    if (type == RTE_ETH_EVENT_FLOW_AGED) {
        struct delete_flow_params *params = rte_zmalloc("delete_flow_params", sizeof(struct delete_flow_params), 0);
        params->port_id = port_id;
        params->flow_hash_map = flow_hash_map;
        if (rte_eal_alarm_set(1000, delete_timeout_flows, (void *) params)) {
            zlog_category_t *zc = zlog_get_category("flow_event");
            zlog_error(zc, "cannot run delete flow function synchronously");
            return -1;
        }
    }
    return 0;
}


int register_aged_event(uint16_t port_id, struct rte_hash *flow_hash_map) {
    return rte_eth_dev_callback_register(port_id, RTE_ETH_EVENT_FLOW_AGED,
                                         aged_event_callback, flow_hash_map);
}
