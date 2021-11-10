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

#include "flow_management.h"

enum layer_name {
    L2,
    L3,
    L4,
    TUNNEL,
    L2_INNER,
    L3_INNER,
    L4_INNER,
    END
};


struct rte_flow *create_offload_rte_flow(uint16_t port_id, zlog_category_t *zc, struct rte_flow_error *error) {
    struct rte_flow *flow = 0;
    struct rte_flow_attr attr = { /* Holds the flow attributes. */
            .group = 0, /* set the rule on the main group. */
            .ingress = 1,/* Rx flow. */
            .priority = 0,};
    struct rte_flow_item pattern[] = {
            [L2] = {
                    .type = RTE_FLOW_ITEM_TYPE_ETH,
            },
            [L3] = {
                    .type = RTE_FLOW_ITEM_TYPE_IPV4,
            },
            [L4] = {
                    .type = RTE_FLOW_ITEM_TYPE_TCP,
            }
    };

    /* create flow on first port and first hairpin queue. */
    RTE_ASSERT(port_id != RTE_MAX_ETHPORTS);
    struct rte_eth_dev_info dev_info;
    int ret = rte_eth_dev_info_get(port_id, &dev_info);
    if (ret) {
        zlog_warn(zc, "can not get dev info of peer port");
        return NULL;
    }
    uint16_t hairpin_queue_id;
    for (hairpin_queue_id = 0; hairpin_queue_id < dev_info.nb_rx_queues; hairpin_queue_id++) {
        struct rte_eth_dev *dev = &rte_eth_devices[port_id];
        if (rte_eth_dev_is_rx_hairpin_queue(dev, hairpin_queue_id)) {
            break;
        }
    }

    struct rte_flow_action_set_ipv4 ipv4 = {
            .ipv4_addr = RTE_IPV4(5, 5, 5, 5)
    };
    struct rte_flow_action_queue hairpin_queue = {
            .index = hairpin_queue_id,
    };
    struct rte_flow_action actions[] = {
            [0] = {
                    .type = RTE_FLOW_ACTION_TYPE_SET_IPV4_DST,
                    .conf = &ipv4
            },
            [1] = {
                    .type = RTE_FLOW_ACTION_TYPE_QUEUE,
                    .conf = &hairpin_queue
            },
            [2] = {
                    .type = RTE_FLOW_ACTION_TYPE_END,
            }
    };
    flow = rte_flow_create(port_id, &attr, pattern, actions, error);
    return flow;
}

