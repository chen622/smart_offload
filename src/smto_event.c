/*
 * MIT License
 * 
 * Copyright (c) 2022 Chenming C (ccm@ccm.ink)
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

#include "internal/smto_event.h"
#include "internal/smto_flow_key.h"

extern struct smto *smto_cb;

int query_counter(uint16_t port_id, struct rte_flow *flow, struct rte_flow_query_count *counter,
                  struct rte_flow_error *error) {
  struct rte_flow_action_count count_action = {
  };
  struct rte_flow_action actions[2] = {
      [0] = {
          .type = RTE_FLOW_ACTION_TYPE_COUNT,
          .conf = &count_action,
      },
      [1] = {
          .type = RTE_FLOW_ACTION_TYPE_END
      }
  };
  if (rte_flow_query(port_id, flow, actions, counter, error)) {
    zlog_category_t *zc = zlog_get_category("flow_event");
    zlog_error(zc, "cannot get the result of counter: %s", error->message);
    return SMTO_ERROR_FLOW_QUERY;
  }
  return 0;
}

/**
 * Delete the timeout flows which are aged.
 *
 * @param params Port ID.
 */
void delete_timeout_flows(void *params) {
  int ret = 0;

  intptr_t port_id = (intptr_t) params;
  void *flow_keys[TIMEOUT_FLOW_BATCH_SIZE]; ///< Array of timeout flows.
  int timeout_quantity = 0; ///< Quantity of timeout flows.
  struct rte_flow_error flow_error = {0};

  /// Get the timeout flows
  timeout_quantity = rte_flow_get_aged_flows(port_id, flow_keys, TIMEOUT_FLOW_BATCH_SIZE, &flow_error);
  if (flow_error.type != RTE_FLOW_ERROR_TYPE_NONE) {
    zlog_error(smto_cb->logger, "failed to get timeout flows: %s", flow_error.message);
    return;
  }
  if (timeout_quantity == 0) {
    return;
  }

  struct smto_flow_key *flow_key = 0; ///< Used to iterate the timeout flows.
  char flow_key_str[PKT_INFO_MAX_LENGTH] = {0}; ///< Used to save the flow key string.
  for (int i = 0; i < timeout_quantity; ++i) {
    if (!flow_keys[i]) {
      zlog_error(smto_cb->logger, "get timeout flows failed: flow_key is NULL");
      continue;
    }
    flow_key = (struct smto_flow_key *) flow_keys[i];
    uint16_t queue_index = -1;
    dump_pkt_info(&flow_key->tuple, queue_index, flow_key_str, PKT_INFO_MAX_LENGTH);
    if (flow_key->flow == NULL) {
      zlog_error(smto_cb->logger, "cannot get the rte_flow of packet(%s)", flow_key_str);
    } else {

      /// Query the counter of the timeout flow
      struct rte_flow_query_count counter = {0};
      ret = query_counter(port_id, flow_key->flow, &counter, &flow_error);
      if (ret != 0) {
        zlog_error(smto_cb->logger, "cannot query the counter of a timeout flow: %s", flow_error.message);
      } else {
        zlog_info(smto_cb->logger,
                  "flow(%s) timeout, total has %lu packets, fast-path has %lu packets and slow-path has %u packets.",
                  flow_key_str,
                  flow_key->packet_amount + counter.hits, counter.hits, flow_key->packet_amount);
        flow_key->packet_amount += counter.hits;
        flow_key->flow_size += counter.bytes;
      }

      /// Delete the flow from nic
      ret = rte_flow_destroy(port_id, flow_key->flow, &flow_error);
      if (ret) {
        zlog_error(smto_cb->logger, "cannot remove a offload rte_flow from nic: %s", flow_error.message);
      } else {
        zlog_info(smto_cb->logger, "flow(%s) has been delete because timeout", flow_key_str);
      }
    }
  }
}

/**
 * Execute when a flow timeout. This function will be running in a interrupt thread, so it should run as fast as possible.
 *
 * @param port_id The port id of flow.
 * @param type The type of event.
 * @param nil Unused.
 * @param ret_param Unused.
 *      - On success, zero.
 *      - On failure, a negative value.
 */
static int aged_event_callback(uint16_t port_id, enum rte_eth_event_type type,
                               void *nil, void *ret_param) {
  RTE_SET_USED(ret_param);
  if (type == RTE_ETH_EVENT_FLOW_AGED) {
    if (rte_eal_alarm_set(1000, delete_timeout_flows, (void *) (intptr_t) port_id)) {
      zlog_category_t *zc = zlog_get_category("flow_event");
      zlog_error(zc, "cannot run delete flow function asynchronously: %s", rte_strerror(rte_errno));
      return -1;
    }
  }
  return 0;
}

int register_aged_event(uint16_t port_id) {
  return rte_eth_dev_callback_register(port_id, RTE_ETH_EVENT_FLOW_AGED,
                                       aged_event_callback, NULL);
}

int unregister_aged_event(uint16_t port_id) {
  rte_eal_alarm_cancel(delete_timeout_flows, (void *) (intptr_t) port_id);
  return rte_eth_dev_callback_unregister(port_id, RTE_ETH_EVENT_FLOW_AGED,
                                  aged_event_callback, NULL);
}