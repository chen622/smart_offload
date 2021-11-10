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

#include <rte_power.h>
#include "smart_offload.h"

#ifdef RELEASE
char *zlog_conf = "/etc/natexp/zlog.conf";
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

void smto_exit(int exit_code, const char *format) {
    if (exit_code == EXIT_SUCCESS) {
        dzlog_info("%s", format);
    } else {
        dzlog_error("%s", format);
    }

    destroy_hairpin();

    uint16_t port_id;
    RTE_ETH_FOREACH_DEV(port_id) {
        struct rte_flow_error *error;
        rte_flow_flush(port_id, error);
        if (error != NULL) {
            dzlog_error("can not flush rte flow on port#%u", port_id);
        }
        rte_eth_dev_stop(port_id);
        rte_eth_dev_close(port_id);
    }

    uint16_t lcore_id = 0;
    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        rte_power_exit(lcore_id);
    }

    /* clean up the EAL */
    rte_eal_cleanup();
    zlog_fini();
    rte_exit(exit_code, "%s", format);
}

int main(int argc, char **argv) {
    /* General return value */
    int ret;
    char err_msg[MAX_ERROR_MESSAGE_LENGTH];
    /* Quantity of ports */
    uint16_t port_quantity;
    /* Quantity of slave works */
    uint16_t worker_quantity;

    /* Setup environment of DPDK */
    ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        smto_exit(EXIT_FAILURE, "invalid EAL arguments");
    }

    /* Setup zlog */
    ret = dzlog_init("conf/zlog.conf", "main");
    if (ret) {
        smto_exit(EXIT_FAILURE, "zlog init failed");
    }

    /* Listen to the shutdown event */
    force_quit = false;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Check the quantity of network ports */
    port_quantity = rte_eth_dev_count_avail();
    if (port_quantity < 2) {
        smto_exit(EXIT_FAILURE, "no enough Ethernet ports found");
    } else if (port_quantity > 2) {
        dzlog_warn("%d ports detected, but we only use two", port_quantity);
    }

    /* Check the quantity of workers*/
    worker_quantity = rte_lcore_count();
    if (worker_quantity != GENERAL_QUEUES_QUANTITY + 1) {
        snprintf(err_msg, MAX_ERROR_MESSAGE_LENGTH,
                 "worker quantity does not match the queue quantity, it should be %u rather than %u",
                 GENERAL_QUEUES_QUANTITY + 1, worker_quantity);
        smto_exit(EXIT_FAILURE, err_msg);
    }

    /* Initialize the memory pool of dpdk */
    struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", 40960, 128, 0,
                                                            RTE_MBUF_DEFAULT_BUF_SIZE,
                                                            rte_socket_id());
    if (mbuf_pool == NULL) {
        smto_exit(EXIT_FAILURE, "cannot init mbuf pool");
    }

    uint16_t port_id;
    /* Initialize the network port and do some configure */
    RTE_ETH_FOREACH_DEV(port_id) {
        init_port(port_id, mbuf_pool);
    }

    setup_hairpin();

    uint16_t lcore_id = 0;
    uint16_t index = 0;
    uint16_t queue_ids[GENERAL_QUEUES_QUANTITY];
    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        queue_ids[index] = index;
        rte_power_init(lcore_id);
//        uint32_t freqs[30];
//        uint32_t total = rte_power_freqs(lcore_id, freqs, 30);
//        for (int i = 0; i < 30; ++i) {
//            printf("\t%u", freqs[i]);
//        }
//        printf("\ntotal:%u", total);
        ret = rte_power_set_freq(lcore_id, 2);
        if (ret < 0) {
            dzlog_warn("worker #%u does not running at the fixed frequency", lcore_id);
        }
        rte_eal_remote_launch(process_loop, &queue_ids[index], lcore_id);
        index++;
    }

    rte_eal_mp_wait_lcore();

    smto_exit(EXIT_SUCCESS, "SUCCESS! All core stop running!");
}
