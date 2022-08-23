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

#include "internal/smto_setup.h"

extern struct smto *smto_cb;


int init_port(uint16_t port_id) {
  int ret = 0;

  /// Get the information of this port
  struct rte_eth_dev_info dev_info;
  ret = rte_eth_dev_info_get(port_id, &dev_info);
  if (ret != 0) {
    zlog_error(smto_cb->logger, "failed to get the device info of port %d: %s", port_id, rte_strerror(ret));
    return SMTO_ERROR_DEVICE_CONFIGURE;
  }

  /// Check the numa node
  if (dev_info.device->numa_node != rte_socket_id()) {
    zlog_warn(smto_cb->logger, "the device %d is not on the same NUMA node as the application", port_id);
  }

  /// Configure the network port
  struct rte_eth_conf port_conf = {
      .rxmode = {
          .split_hdr_size = 0,
      },
      .txmode = {
          .offloads =
          DEV_TX_OFFLOAD_VLAN_INSERT |
              DEV_TX_OFFLOAD_IPV4_CKSUM |
              DEV_TX_OFFLOAD_UDP_CKSUM |
              DEV_TX_OFFLOAD_TCP_CKSUM |
              DEV_TX_OFFLOAD_SCTP_CKSUM |
              DEV_TX_OFFLOAD_TCP_TSO,
      },
  };
  port_conf.txmode.offloads &= dev_info.tx_offload_capa;

  /// The additional one is used for hairpin
  ret = rte_eth_dev_configure(port_id, GENERAL_QUEUES_QUANTITY + 1, GENERAL_QUEUES_QUANTITY + 1, &port_conf);
  if (ret != 0) {
    zlog_error(smto_cb->logger, "can not change the configuration of port %d: %s", port_id, rte_strerror(ret));
    return SMTO_ERROR_DEVICE_CONFIGURE;
  }

  for (int i = 0; i < GENERAL_QUEUES_QUANTITY; ++i) {
    ret = rte_eth_rx_queue_setup(port_id,
                                 i,
                                 QUEUE_DESC_NUMBER,
                                 rte_eth_dev_socket_id(port_id),
                                 &dev_info.default_rxconf,
                                 smto_cb->pkt_mbuf_pool);
    if (ret < 0) {
      zlog_error(smto_cb->logger, "can not setup the rx queue of port %d: %s", port_id, rte_strerror(ret));
      return SMTO_ERROR_QUEUE_SETUP;
    }
  }

  for (int i = 0; i < GENERAL_QUEUES_QUANTITY; ++i) {
    ret = rte_eth_tx_queue_setup(port_id,
                                 i,
                                 QUEUE_DESC_NUMBER,
                                 rte_eth_dev_socket_id(port_id),
                                 &dev_info.default_txconf);
    if (ret < 0) {
      zlog_error(smto_cb->logger, "can not setup the tx queue of port %d: %s", port_id, rte_strerror(ret));
      return SMTO_ERROR_QUEUE_SETUP;
    }
  }

  /// Setting the RX port to promiscuous mode
  ret = rte_eth_promiscuous_enable(port_id);
  if (ret != 0) {
    zlog_error(smto_cb->logger, "can not enable the promiscuous mode of port %d: %s", port_id, rte_strerror(ret));
    return SMTO_ERROR_DEVICE_CONFIGURE;
  }

  return SMTO_SUCCESS;

//  /// Starting the port
//  ret = rte_eth_dev_start(port_id);
//  if (ret < 0) {
//    zlog_error(smto_cb->logger, "can not start the port %d: %s", port_id, rte_strerror(ret));
//    return SMTO_ERROR_DEVICE_START;
//  }
//
//  return assert_link_status(port_id);
}

/**
 * Setup hairpin queue.
 *
 * @param port_id The first port.
 * @param peer_port_id The second port.
 * @param port_num Used to mark is the odd port.
 * @return
 *   - 0 : Success
 */
static int setup_hairpin_queues(uint16_t port_id, uint16_t peer_port_id) {
  int ret;

  struct rte_eth_hairpin_conf hairpin_conf = {
      .peer_count = 1,
      .manual_bind = 1,
      .tx_explicit = 1,
  };
  if (port_id == peer_port_id) {
    hairpin_conf.manual_bind = 0;
    hairpin_conf.tx_explicit = 0;
  }

  /// create hairpin queues on both ports
  hairpin_conf.peers[0].port = peer_port_id;
  hairpin_conf.peers[0].queue = HAIRPIN_QUEUE_INDEX;
  ret = rte_eth_tx_hairpin_queue_setup(
      port_id, HAIRPIN_QUEUE_INDEX,
      0, &hairpin_conf);
  if (ret != 0) {
    zlog_error(smto_cb->logger, "can not setup the hairpin tx queue of port %d: %s", port_id, rte_strerror(ret));
    return ret;
  }

  hairpin_conf.peers[0].port = peer_port_id;
  hairpin_conf.peers[0].queue = HAIRPIN_QUEUE_INDEX;
  ret = rte_eth_rx_hairpin_queue_setup(
      peer_port_id, HAIRPIN_QUEUE_INDEX,
      0, &hairpin_conf);
  if (ret != 0) {
    zlog_error(smto_cb->logger, "can not setup the hairpin rx queue of port %d: %s", peer_port_id, rte_strerror(ret));
    return ret;
  }
  return SMTO_SUCCESS;
}

int setup_one_port_hairpin(int port_id) {
  int ret;

  /// Setup hairpin queues
  ret = setup_hairpin_queues(port_id, port_id);
  if (ret != 0) {
    return SMTO_ERROR_HAIRPIN_SETUP;
  }
  return SMTO_SUCCESS;
}

int destroy_hash_map() {
  if (smto_cb->flow_hash_map != NULL) {
    int ret = 0;
    int key_count = rte_hash_count(smto_cb->flow_hash_map);
    zlog_debug(smto_cb->logger, "%d flow keys has been added into flow hash map", key_count);
    if (key_count > 0) {
      const void *key = 0;
      void *data = 0;
      uint32_t *next = 0;
      uint32_t current = rte_hash_iterate(smto_cb->flow_hash_map, &key, &data, next);
      for (; current != -ENOENT; current = rte_hash_iterate(smto_cb->flow_hash_map, &key, &data, next)) {
        int del_key_position = rte_hash_del_key(smto_cb->flow_hash_map, key);
        if (del_key_position < 0) {
          zlog_error(smto_cb->logger,
                     "failed to delete the key from flow hash map: %s",
                     rte_strerror(del_key_position));
          continue;
        }
        ret = rte_hash_free_key_with_position(smto_cb->flow_hash_map, del_key_position);
        if (ret) {
          zlog_error(smto_cb->logger, "cannot free a flow key: %s", rte_strerror(ret));
        }
        rte_free(data);
      }
    }
    rte_hash_free(smto_cb->flow_hash_map);
  }
  return SMTO_SUCCESS;
}

int assert_link_status(uint16_t port_id) {
  struct rte_eth_link link = {0};
  uint8_t rep_cnt = MAX_REPEAT_TIMES;
  int ret = 0;

  do {
    ret = rte_eth_link_get(port_id, &link);
    if (ret == 0 && link.link_status == RTE_ETH_LINK_UP) {
      break;
    }
    rte_delay_ms(CHECK_INTERVAL);
  } while (--rep_cnt);

  if (ret < 0) {
    zlog_error(smto_cb->logger, "failed to get link status of port %u: %s", port_id, rte_strerror(ret));
    return SMTO_ERROR_DEVICE_START;
  }
  if (link.link_status == RTE_ETH_LINK_DOWN) {
    zlog_error(smto_cb->logger, "link is still down of port %u", port_id);
    return SMTO_ERROR_DEVICE_START;
  }
  return SMTO_SUCCESS;
}