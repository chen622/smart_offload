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
#include <rte_malloc.h>
#include <sys/time.h>
#include "v1/flow_management.h"
#include "v1/flow_meta.h"
#include "v1/smart_offload.h"
#include "v1/flow_event.h"

#ifdef RELEASE
char *zlog_conf = "/etc/smart_offload/zlog.conf";
#else
char *zlog_conf = "conf/zlog.conf";
#endif

static void signal_handler(int signum) {
  if (signum == SIGINT || signum == SIGTERM) {
    dzlog_info("Signal %d received, preparing to exit...",
               signum);
    force_quit = true;
  }
}

int main(int argc, char **argv) {
  /* General return value */
  int ret;
  char err_msg[MAX_ERROR_MESSAGE_LENGTH];
  /* Quantity of ports */
  uint16_t port_quantity;
  /* Quantity of slave works */
  uint16_t worker_quantity;
  uint16_t port_id = 0;

  /* Setup environment of DPDK */
  ret = rte_eal_init(argc, argv);
  if (ret < 0) {
    smto_exit(EXIT_FAILURE, "invalid EAL arguments");
  }

  /* Setup zlog */
  ret = dzlog_init(zlog_conf, "main");
  if (ret) {
    smto_exit(EXIT_FAILURE, "zlog init failed");
  }

    /* Check the instruction */
#if defined(__SSE2__)
  dzlog_debug("Find SSE2 support");
#else
#error No vector engine (SSE, NEON, ALTIVEC) available, check your toolchain
#endif

  /* Listen to the shutdown event */
  force_quit = false;
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  dzlog_debug("Running on socket #%d", rte_socket_id());

  /* Check the quantity of network ports */
  port_quantity = rte_eth_dev_count_avail();
  if (port_quantity < 1) {
    smto_exit(EXIT_FAILURE, "no enough Ethernet ports found");
  } else if (port_quantity > 2) {
    dzlog_warn("%d ports detected, but we only use two", port_quantity);
  }

  /* Check the quantity of workers */
  worker_quantity = rte_lcore_count();
  if (worker_quantity != GENERAL_QUEUES_QUANTITY + 1) {
    snprintf(err_msg, MAX_ERROR_MESSAGE_LENGTH,
             "worker quantity does not match the queue quantity, it should be %u rather than %u",
             GENERAL_QUEUES_QUANTITY + 1, worker_quantity);
    smto_exit(EXIT_FAILURE, err_msg);
  }

  /* Initialize the memory pool of dpdk */
  struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", NUM_MBUFS, CACHE_SIZE, 0,
                                                          RTE_MBUF_DEFAULT_BUF_SIZE,
                                                          rte_socket_id());
  if (mbuf_pool == NULL) {
    smto_exit(EXIT_FAILURE, "cannot init mbuf pool");
  }

  /* Config port and setup hairpin mode */
  if (port_quantity == 1) {
    dzlog_debug("ONE port hairpin mode");
    port_id = rte_eth_find_next_owned_by(0, RTE_ETH_DEV_NO_OWNER);
    init_port(port_id, mbuf_pool);
    setup_one_port_hairpin(port_id);
  } else {
    dzlog_debug("TWO port hairpin mode unsupported");
    smto_exit(EXIT_FAILURE, "WO port hairpin mode unsupported");
    /* Initialize the network port and do some configure */
    RTE_ETH_FOREACH_DEV(port_id) {
      init_port(port_id, mbuf_pool);
    }
    setup_two_port_hairpin();
  }

  struct rte_flow *flow = 0;
  struct rte_flow_error flow_error = {0};
  flow = create_default_jump_flow(port_id, &flow_error);
  if (flow == NULL) {
    snprintf(err_msg, MAX_ERROR_MESSAGE_LENGTH,
             "the default jump flow create failed: %s", flow_error.message);
    smto_exit(EXIT_FAILURE, err_msg);
  }
  flow = create_default_rss_flow(port_id, GENERAL_QUEUES_QUANTITY, &flow_error);
  if (flow == NULL) {
    snprintf(err_msg, MAX_ERROR_MESSAGE_LENGTH,
             "the default rss flow create failed: %s", flow_error.message);
    smto_exit(EXIT_FAILURE, err_msg);
  }

  /* Create flow hash map */
  struct rte_hash_parameters flow_hash_map_parameter = {
      .name = "flow_hash_table",
      .entries = MAX_HASH_ENTRIES,
      .key_len = sizeof(union ipv4_5tuple_host),
      .hash_func = ipv4_hash_crc,
      .hash_func_init_val = 0,
      .extra_flag = RTE_HASH_EXTRA_FLAGS_RW_CONCURRENCY_LF
  };
  struct rte_hash *flow_hash_map = rte_hash_create(&flow_hash_map_parameter);
  if (flow_hash_map == NULL) {
    snprintf(err_msg, MAX_ERROR_MESSAGE_LENGTH, "unable to create flow hash table");
    smto_exit(EXIT_FAILURE, err_msg);
  }

  if (register_aged_event(port_id, flow_hash_map)) {
    smto_exit(EXIT_FAILURE, "cannot register from age timeout event");
  }

  uint16_t lcore_id = 0;
  uint16_t index = 0;
  struct worker_parameter worker_params[GENERAL_QUEUES_QUANTITY];
  RTE_LCORE_FOREACH_WORKER(lcore_id) {
    worker_params[index].port_id = port_id;
    worker_params[index].queue_id = index;
    worker_params[index].flow_hash_map = flow_hash_map;
#ifndef VM
    rte_power_init(lcore_id);
    ret = rte_power_set_freq(lcore_id, 2);
#endif
    if (ret < 0) {
      dzlog_warn("worker #%u does not running at the fixed frequency", lcore_id);
    }
    rte_eal_remote_launch(process_loop, &worker_params[index], lcore_id);
    index++;
  }

  rte_eal_mp_wait_lcore();


  /* free the memory of flow metadata and the flow hash map*/
  int32_t key_count = rte_hash_count(flow_hash_map);
  dzlog_debug("%d flow keys has been added into flow hash map", key_count);
  if (key_count > 0) {
    char format_key[MAX_ERROR_MESSAGE_LENGTH];
    const void *key = 0;
    void *data = 0;
    uint32_t *next = 0;
    uint32_t current = rte_hash_iterate(flow_hash_map, &key, &data, next);
    for (; current != -ENOENT; current = rte_hash_iterate(flow_hash_map, &key, &data, next)) {
      int32_t del_key = rte_hash_del_key(flow_hash_map, key);
      ret = rte_hash_free_key_with_position(flow_hash_map, del_key);
      if (ret) {
        dump_pkt_info((union ipv4_5tuple_host *) key, -1, format_key, MAX_ERROR_MESSAGE_LENGTH);
        dzlog_error("cannot free hash key(%s)", format_key);
      }
      rte_free(data);
    }
  }
  rte_hash_free(flow_hash_map);

  smto_exit(EXIT_SUCCESS, "SUCCESS! All core stop running!");
}
