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


#include "smto.h"
#include "internal/smto_setup.h"
#include "internal/smto_flow_engine.h"
#include "internal/smto_event.h"
#include "internal/smto_flow_key.h"

const uint32_t SRC_IP = RTE_IPV4(5, 1, 1, 1);

/// The global control block of SmartOffload.
struct smto *smto_cb = 0;

/// The parameters for each worker threads.
struct worker_parameter *worker_params;

int init_smto(struct smto **smto) {
  int ret = 0;
  *smto = calloc(sizeof(struct smto), 1);
  if ((*smto) == NULL) {
    return SMTO_ERROR_MEMORY_ALLOCATION;
  }
  smto_cb = *smto;

  smto_cb->logger = zlog_get_category("smto");

  /// Check the quantity of workers
  uint32_t worker_quantity = rte_lcore_count();
  if (worker_quantity > GENERAL_QUEUES_QUANTITY + 2) {
    zlog_warn(smto_cb->logger,
              "worker quantity(%u) greater than required(queue quantity(%u) + flow_engine(1) + main(1)), the remaining worker will remain idle",
              worker_quantity,
              GENERAL_QUEUES_QUANTITY);
  } else if (worker_quantity < GENERAL_QUEUES_QUANTITY + 2) {
    zlog_error(smto_cb->logger,
               "worker quantity(%u) should be greater than or equal to required(queue quantity(%u) + flow_engine(1) + main(1))",
               worker_quantity,
               GENERAL_QUEUES_QUANTITY);
    ret = SMTO_ERROR_NO_ENOUGH_WORKER;
    goto err;
  }

  /// Check the quantity of ports
  uint16_t port_quantity = rte_eth_dev_count_avail();
  if (port_quantity < 1) {
    ret = SMTO_ERROR_NO_AVAILABLE_PORTS;
    goto err;
  } else if (port_quantity > 2) {
    zlog_warn(smto_cb->logger, "%d ports detected, but only use first two", port_quantity);
  }

  /// Initialize the memory pool
  smto_cb->pkt_mbuf_pool = rte_pktmbuf_pool_create("smto_pool", NUM_MBUFS, CACHE_SIZE, 0,
                                                   RTE_MBUF_DEFAULT_BUF_SIZE,
                                                   rte_socket_id());
  if (smto_cb->pkt_mbuf_pool == NULL) {
    zlog_error(smto_cb->logger, "failed to create memory pool: %s", rte_strerror(rte_errno));
    ret = SMTO_ERROR_HUGE_PAGE_MEMORY_ALLOCATION;
    goto err;
  }

  /// Config port and setup hairpin mode
  if (port_quantity == 1) {
    zlog_info(smto_cb->logger, "single port mode");
    smto_cb->mode = SINGLE_PORT_MODE;
    smto_cb->ports[0] = rte_eth_find_next_owned_by(0, RTE_ETH_DEV_NO_OWNER);
    init_port(smto_cb->ports[0]);
    ret = setup_one_port_hairpin(smto_cb->ports[0]);
    if (ret != 0) {
      zlog_error(smto_cb->logger, "failed to setup hairpin for port %d", smto_cb->ports[0]);
      goto err;
    }
    /// Start the ports
    ret = rte_eth_dev_start(smto_cb->ports[0]);
    if (ret < 0) {
      zlog_error(smto_cb->logger,
                 "failed to start network device: err: %s, port=%u",
                 rte_strerror(ret),
                 smto_cb->ports[0]);
      goto err;
    }
    /// Check the port status
    if (assert_link_status(smto_cb->ports[0])) {
      ret = SMTO_ERROR_DEVICE_START;
      goto err;
    }
  } else {
    zlog_info(smto_cb->logger, "dual port mode");
    smto_cb->mode = DOUBLE_PORT_MODE;
    smto_cb->ports[0] = rte_eth_find_next_owned_by(0, RTE_ETH_DEV_NO_OWNER);
    smto_cb->ports[1] = rte_eth_find_next_owned_by(smto_cb->ports[0] + 1, RTE_ETH_DEV_NO_OWNER);
    init_port(smto_cb->ports[0]);
    init_port(smto_cb->ports[1]);
    ret = setup_two_port_hairpin(smto_cb->ports[0], smto_cb->ports[1]);
    if (ret != 0) {
      zlog_error(smto_cb->logger,
                 "failed to setup dual port hairpin for port %d and %d",
                 smto_cb->ports[0],
                 smto_cb->ports[1]);
      goto err;
    }
    ret = rte_eth_dev_start(smto_cb->ports[0]);
    if (ret < 0) {
      zlog_error(smto_cb->logger,
                 "failed to start network device: err: %s, port=%u",
                 rte_strerror(ret),
                 smto_cb->ports[0]);
      goto err;
    }
    ret = rte_eth_dev_start(smto_cb->ports[1]);
    if (ret < 0) {
      zlog_error(smto_cb->logger,
                 "failed to start network device: err: %s, port=%u",
                 rte_strerror(ret),
                 smto_cb->ports[1]);
      goto err;
    }

    /* Let's find our peer RX ports, TXQ -> RXQ. */
    ret = rte_eth_hairpin_bind(smto_cb->ports[0], smto_cb->ports[1]);
    if (ret) {
      zlog_error(smto_cb->logger,
                 "can not bind the hairpin queues of port %d-%d: %s",
                 smto_cb->ports[0],
                 smto_cb->ports[1],
                 rte_strerror(ret));
      goto err;
    }
    /* Let's find our peer TX ports, RXQ -> TXQ. */
    ret = rte_eth_hairpin_bind(smto_cb->ports[1], smto_cb->ports[0]);
    if (ret) {
      zlog_error(smto_cb->logger,
                 "can not bind the hairpin queues of port %d-%d: %s",
                 smto_cb->ports[0],
                 smto_cb->ports[1],
                 rte_strerror(ret));
      goto err;
    }
  }

  /// Create default jump and rss flow
  struct rte_flow *flow = create_default_jump_flow(smto_cb->ports[0]);
  if (flow == NULL) {
    ret = SMTO_ERROR_FLOW_CREATE;
    goto err;
  }
  struct rte_flow_error flow_error;
  flow = create_default_rss_flow(smto_cb->ports[0]);
  if (flow == NULL) {
    ret = SMTO_ERROR_FLOW_CREATE;
    goto err1;
  }

  /// Create flow hash map
  struct rte_hash_parameters flow_hash_map_parameter = {
      .name = "flow_hash_table",
      .entries = MAX_HASH_ENTRIES,
      .key_len = sizeof(struct rdarm_five_tuple),
#ifdef EM_HASH_CRC
      .hash_func = rte_hash_crc,
#else
      .hash_func = rte_jhash,
#endif
      .hash_func_init_val = 622,
      .extra_flag = RTE_HASH_EXTRA_FLAGS_RW_CONCURRENCY_LF
  };
  smto_cb->flow_hash_map = rte_hash_create(&flow_hash_map_parameter);
  if (smto_cb->flow_hash_map == NULL) {
    ret = SMTO_ERROR_HASH_MAP_CREATION;
    goto err1;
  }

  /// Create ring for flow rules from worker to flow engine
  ssize_t ring_size = rte_ring_get_memsize(MAX_RING_ENTRIES);
  smto_cb->flow_rules_ring = rte_calloc("flow_rule_ring", ring_size, 1, 0);
  if (smto_cb->flow_rules_ring == NULL) {
    zlog_error(smto_cb->logger, "failed to allocate memory for flow rule ring");
    ret = SMTO_ERROR_HUGE_PAGE_MEMORY_ALLOCATION;
    goto err2;
  }
  ret = rte_ring_init(smto_cb->flow_rules_ring, "flow_rule_ring", MAX_RING_ENTRIES, RING_F_MP_RTS_ENQ | RING_F_SC_DEQ);
  if (ret != 0) {
    zlog_error(smto_cb->logger, "failed to initialize flow rule ring: %s", rte_strerror(rte_errno));
    ret = SMTO_ERROR_RING_CREATION;
    rte_free(smto_cb->flow_rules_ring);
    goto err2;
  }

  /// Create ring for port pool
  ring_size = rte_ring_get_memsize(MAX_HASH_ENTRIES);
  smto_cb->port_pool = rte_calloc("port_pool", ring_size, 1, 0);
  if (smto_cb->port_pool == NULL) {
    zlog_error(smto_cb->logger, "failed to allocate memory for port pool ring");
    ret = SMTO_ERROR_HUGE_PAGE_MEMORY_ALLOCATION;
    goto err3;
  }
  ret = rte_ring_init(smto_cb->port_pool, "port_pool", MAX_HASH_ENTRIES, RING_F_MP_RTS_ENQ | RING_F_SC_DEQ);
  if (ret != 0) {
    zlog_error(smto_cb->logger, "failed to initialize port pool ring: %s", rte_strerror(rte_errno));
    ret = SMTO_ERROR_RING_CREATION;
    rte_free(smto_cb->port_pool);
    goto err3;
  }
  for (int i = 1; i < MAX_HASH_ENTRIES; ++i) {
    ret = rte_ring_enqueue(smto_cb->port_pool, (void *) (uintptr_t) i);
    if (ret != 0) {
      zlog_error(smto_cb->logger, "failed to enqueue port pool ring: %s", rte_strerror(rte_errno));
      ret = SMTO_ERROR_RING_CREATION;
      goto err4;
    }
  }

  /// Register age timeout event
  if (register_aged_event(smto_cb->ports[0]) != 0) {
    ret = SMTO_ERROR_EVENT_REGISTER;
    goto err4;
  }

  smto_cb->is_running = true;

  uint16_t lcore_id, index = 0;
  worker_params = calloc(sizeof(struct worker_parameter), GENERAL_QUEUES_QUANTITY);
  RTE_LCORE_FOREACH_WORKER(lcore_id) {
    if (index < GENERAL_QUEUES_QUANTITY) { // The worker to process packets
      worker_params[index].port_id = smto_cb->ports[0];
      worker_params[index].queue_id = index;

      if (rte_eal_remote_launch(process_loop, &worker_params[index], lcore_id) != 0) {
        ret = SMTO_ERROR_WORKER_LAUNCH;
        goto err5;
      }
    } else if (smto_cb->mode == DOUBLE_PORT_MODE && index < 2 * GENERAL_QUEUES_QUANTITY) {
      worker_params[index].port_id = smto_cb->ports[1];
      worker_params[index].queue_id = index - GENERAL_QUEUES_QUANTITY;

      if (rte_eal_remote_launch(process_loop, &worker_params[index], lcore_id) != 0) {
        ret = SMTO_ERROR_WORKER_LAUNCH;
        goto err5;
      }
    } else if ((smto_cb->mode == SINGLE_PORT_MODE && index == GENERAL_QUEUES_QUANTITY)
        || (smto_cb->mode == DOUBLE_PORT_MODE && index == GENERAL_QUEUES_QUANTITY * 2)) { // The worker to create flow
      if (rte_eal_remote_launch(create_flow_loop, NULL, lcore_id) != 0) {
        ret = SMTO_ERROR_WORKER_LAUNCH;
        goto err5;
      }
    } else {
      zlog_info(smto_cb->logger, "unused worker: %d", lcore_id);
    }
    index++;
  }
  return SMTO_SUCCESS;

  err5:
  free(worker_params);
  smto_cb->is_running = false;
  unregister_aged_event(smto_cb->ports[0]);
  rte_eal_mp_wait_lcore();
  err4:
  rte_free(smto_cb->port_pool);
  err3:
  rte_free(smto_cb->flow_rules_ring);
  err2:
  destroy_hash_map();
  err1:
  rte_flow_flush(smto_cb->ports[0], &flow_error);
  if (flow_error.type != RTE_FLOW_ERROR_TYPE_NONE) {
    zlog_error(smto_cb->logger, "failed to flush flow: %s", flow_error.message);
  }
  err:
  free(smto_cb);
  return ret;
}

void destroy_smto(struct smto *smto) {
  smto->is_running = false;
  /// Wait for all the workers to exit
  unregister_aged_event(smto->ports[0]);
  rte_eal_mp_wait_lcore();

  /// Destroy flow hash map
  destroy_hash_map();

  /// Stop the port
  uint16_t port_id;
  RTE_ETH_FOREACH_DEV(port_id) {
    struct rte_flow_error error = {0};
    int ret = rte_flow_flush(port_id, &error);
    if (ret) {
      zlog_error(smto->logger, "cannot flush rte flow on port#%u: %s", port_id, error.message);
    }
    rte_eth_dev_stop(port_id);
    rte_eth_dev_close(port_id);
  }

  free(worker_params);
  rte_eal_cleanup();
  zlog_fini();
  free(smto);
}