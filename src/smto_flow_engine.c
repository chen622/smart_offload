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

struct rte_flow *create_general_offload_flow(uint16_t port_id,
                                             struct smto_flow_key *flow_key,
                                             struct rte_flow_error *error) {
  /// The rte flow will be created
  struct rte_flow *flow = 0;
  /// The basic attribute of rte flow
  struct rte_flow_attr attr = {
      .group = 1, ///< Set the rule on the main group.
      .ingress = 1,///< Rx flow.
      .priority = 0,
  };
  /// The specific pattern of ipv4 header
  struct rte_flow_item_ipv4 ipv4_pattern_spec = {
      .hdr = {
          .src_addr = flow_key->tuple.ip1,
          .dst_addr = flow_key->tuple.ip2,
          .next_proto_id = flow_key->tuple.proto
      }
  };
  /// The mask to match ipv4 header
  struct rte_flow_item_ipv4 ipv4_pattern_mask = {
      .hdr = {
          .src_addr = RTE_BE32(0xffffffff),
          .dst_addr = RTE_BE32(0xffffffff),
          .next_proto_id = 0xff
      }
  };
  /// Define the pattern to match the packet
  struct rte_flow_item pattern[] = {
      [L2] = {
          .type = RTE_FLOW_ITEM_TYPE_ETH,
      },
      [L3] = {
          .type = RTE_FLOW_ITEM_TYPE_IPV4,
          .spec = &ipv4_pattern_spec,
          .mask = &ipv4_pattern_mask,
      },
      [L4] = {
          .type = RTE_FLOW_ITEM_TYPE_VOID,
      },
      [END] = {
          .type = RTE_FLOW_ITEM_TYPE_END
      }

  };
  if (flow_key->tuple.proto == IPPROTO_TCP) {
    pattern[L4].type = RTE_FLOW_ITEM_TYPE_TCP;
    struct rte_flow_item_tcp tcp_pattern = {
        .hdr = {
            .src_port = flow_key->tuple.port1,
            .dst_port = flow_key->tuple.port2,
        }
    };
    pattern[L4].spec = &tcp_pattern;
  } else if (flow_key->tuple.proto == IPPROTO_UDP) {
    pattern[L4].type = RTE_FLOW_ITEM_TYPE_UDP;
    struct rte_flow_item_udp udp_pattern = {
        .hdr = {
            .src_port = flow_key->tuple.port1,
            .dst_port = flow_key->tuple.port2,
        }
    };
    pattern[L4].spec = &udp_pattern;
  } else {
    zlog_error(smto_cb->logger, "unsupported l4 proto type %u", flow_key->tuple.proto);
    return flow;
  }

  /// Define an action to change the dst of ipv4
  struct rte_flow_action_set_ipv4 ipv4_new_dst = {
      .ipv4_addr = RTE_IPV4(5, 5, 5, 5)
  };
  /// Define an action to send packet to hairpin queue
  struct rte_flow_action_queue hairpin_queue = {
      .index = HAIRPIN_QUEUE_INDEX,
  };
  /// Define a counter to count the quantity of packet
  struct rte_flow_action_count dedicated_counter = {
  };

  /// Define an action to set a hook which will be executed when the flow time out
  struct rte_flow_action_age age_action = {
      .context = flow_key,
      .timeout = FLOW_TIMEOUT_SECOND
  };

  /// Define the action pipeline
  struct rte_flow_action actions[] = {
      {
          .type = RTE_FLOW_ACTION_TYPE_SET_IPV4_DST,
          .conf = &ipv4_new_dst
      },
      {
          .type = RTE_FLOW_ACTION_TYPE_COUNT,
          .conf = &dedicated_counter,
      },
      {
          .type = RTE_FLOW_ACTION_TYPE_AGE,
          .conf = &age_action
      },
      {
          .type = RTE_FLOW_ACTION_TYPE_QUEUE,
          .conf = &hairpin_queue
      },
      {
          .type = RTE_FLOW_ACTION_TYPE_END,
      }
  };

  flow = rte_flow_create(port_id, &attr, pattern, actions, error);
  return flow;
}

int create_flow_loop(void *args) {
  void *flow_rules[5];
  uint32_t result = 0;
  uint32_t remain = 0;
  struct smto_flow_key *flow_key = NULL;
  char pkt_info[MAX_PKT_INFO_LENGTH];

  zlog_info(smto_cb->logger, "worker%d for flow engine start working!", rte_lcore_id());
  while (smto_cb->is_running) {
    result = rte_ring_dequeue_burst(smto_cb->flow_rules_ring, flow_rules, 5, &remain);
    for (uint32_t i = 0; i < result; ++i) {
      flow_key = (struct smto_flow_key *) flow_rules[i];
      struct rte_flow_error error;
      struct rte_flow *flow = create_general_offload_flow(smto_cb->ports[0], flow_key, &error);
      if (flow == NULL) {
        dump_pkt_info(&flow_key->tuple, smto_cb->ports[0], -1, pkt_info, MAX_PKT_INFO_LENGTH);
        zlog_error(smto_cb->logger, "failed to create a flow(%s): %s", pkt_info, error.message);
        flow_key->is_offload = NOT_OFFLOAD;
        continue;
      }
      flow_key->flow = flow;
      flow_key->is_offload = OFFLOAD_SUCCESS;

      if (smto_cb->mode == SINGLE_PORT_MODE) { /// Create a flow for symmetrical flow on the same port.
        flow = create_general_offload_flow(smto_cb->ports[0], flow_key->symmetrical_flow_key, &error);
        if (flow == NULL) {
          dump_pkt_info(&flow_key->tuple, smto_cb->ports[0], -1, pkt_info, MAX_PKT_INFO_LENGTH);
          zlog_error(smto_cb->logger, "failed to create a flow(%s): %s", pkt_info, error.message);
          flow_key->symmetrical_flow_key->is_offload = NOT_OFFLOAD;
          continue;
        }
      } else if (smto_cb->mode == DOUBLE_PORT_MODE) { /// Create a flow for symmetrical flow on the other port.
        flow = create_general_offload_flow(smto_cb->ports[1], flow_key->symmetrical_flow_key, &error);
        if (flow == NULL) {
          dump_pkt_info(&flow_key->tuple, smto_cb->ports[1], -1, pkt_info, MAX_PKT_INFO_LENGTH);
          zlog_error(smto_cb->logger, "failed to create a flow(%s): %s", pkt_info, error.message);
          flow_key->symmetrical_flow_key->is_offload = NOT_OFFLOAD;
          continue;
        }
      }
      flow_key->symmetrical_flow_key->flow = flow;
      flow_key->symmetrical_flow_key->is_offload = OFFLOAD_SUCCESS;
    }
  }
  return 0;
}