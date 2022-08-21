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


#include "internal/smto_flow_engine.h"

extern struct smto *smto_cb;

enum layer_name {
  L2,
  L3,
  L4,
  END
};

struct rte_flow *create_default_jump_flow(uint16_t port_id) {
  struct rte_flow *flow = 0;
  struct rte_flow_attr attr = {
      .group = 0,
      .ingress = 1,
      .priority = 0,};

  struct rte_flow_item pattern[] = {
      [L2] = {
          .type = RTE_FLOW_ITEM_TYPE_ETH,
      },
      [L3] = {
          .type = RTE_FLOW_ITEM_TYPE_IPV4,
      },
      [L4] = {
          .type = RTE_FLOW_ITEM_TYPE_VOID,
      },
      [END] = {
          .type = RTE_FLOW_ITEM_TYPE_END
      }

  };
  struct rte_flow_action_jump jump = {.group = 1};
  struct rte_flow_action actions[] = {
      [0] = {
          .type = RTE_FLOW_ACTION_TYPE_JUMP,
          .conf = &jump},
      [1] = {
          .type = RTE_FLOW_ACTION_TYPE_END,
          .conf = NULL}
  };
  struct rte_flow_error error = {0};
  flow = rte_flow_create(port_id, &attr, pattern, actions, &error);
  if (flow == NULL) {
    zlog_error(smto_cb->logger, "failed to create a default jump flow: %s", error.message);
    return NULL;
  }
  return flow;
}

struct rte_flow *create_default_rss_flow(uint16_t port_id) {
  struct rte_flow *flow = 0;
  struct rte_flow_attr attr = {
      .group = 1,
      .ingress = 1,
      .priority = 1,
  };


  /// Define the pattern to match the packet
  struct rte_flow_item pattern[] = {
      [L2] = {
          .type = RTE_FLOW_ITEM_TYPE_ETH,
      },
      [L3] = {
          .type = RTE_FLOW_ITEM_TYPE_IPV4,
      },
      [L4] = {
          .type = RTE_FLOW_ITEM_TYPE_VOID,
      },
      [END] = {
          .type = RTE_FLOW_ITEM_TYPE_END
      }

  };


  /// A matrix can be used to do symmetric hash
  uint8_t symmetric_rss_key[] = {
      0x6D, 0x5A, 0x6D, 0x5A,
      0x6D, 0x5A, 0x6D, 0x5A,
      0x6D, 0x5A, 0x6D, 0x5A,
      0x6D, 0x5A, 0x6D, 0x5A,
      0x6D, 0x5A, 0x6D, 0x5A,
      0x6D, 0x5A, 0x6D, 0x5A,
      0x6D, 0x5A, 0x6D, 0x5A,
      0x6D, 0x5A, 0x6D, 0x5A,
      0x6D, 0x5A, 0x6D, 0x5A,
      0x6D, 0x5A, 0x6D, 0x5A,
  };

  uint16_t queue_schedule[GENERAL_QUEUES_QUANTITY];
  for (uint16_t i = 0; i < GENERAL_QUEUES_QUANTITY; i++) {
    queue_schedule[i] = i;
  }

  struct rte_flow_action_rss rss = {
      .level = 1, ///< RSS should be done on inner header
      .queue = queue_schedule, ///< Set the selected target queues
      .queue_num = GENERAL_QUEUES_QUANTITY, ///< The number of queues
      .types =  ETH_RSS_IP,
      .key = symmetric_rss_key,
      .key_len = 40};

  struct rte_flow_action actions[] = {
      [0] = {
          .type = RTE_FLOW_ACTION_TYPE_RSS,
          .conf = &rss},
      [1] = {
          .type = RTE_FLOW_ACTION_TYPE_END,
          .conf = NULL}
  };

  struct rte_flow_error error;
  flow = rte_flow_create(port_id, &attr, pattern, actions, &error);
  if (flow == NULL) {
    zlog_error(smto_cb->logger, "failed to create a default rss flow: %s", error.message);
    return NULL;
  }

  return flow;
}

int create_flow_loop(void *args) {
  return 0;
}