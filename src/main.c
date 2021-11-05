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
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#include "smart_offload.h"

#ifdef RELEASE
char *zlog_conf = "/etc/natexp/zlog.conf";
#else
char *zlog_conf = "conf/zlog.conf";
#endif


static void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        dzlog_debug("\n\nSignal %d received, preparing to exit...\n",
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

    zlog_fini();

    uint16_t port_id;
    RTE_ETH_FOREACH_DEV(port_id) {
        rte_eth_dev_stop(port_id);
        rte_eth_dev_close(port_id);
    }
    /* clean up the EAL */
    rte_eal_cleanup();

    rte_exit(exit_code, "%s", format);
}

int main(int argc, char **argv) {
    /* General return value */
    int ret;
    char *err_msg;
    /* Quantity of ports */
    uint16_t port_quantity;

    /* setup the environment of DPDK */
    ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        smto_exit(EXIT_FAILURE, "invalid EAL arguments\n");
    }

    /* setup zlog */
    ret = dzlog_init("conf/zlog.conf", "main");
    if (ret) {
        smto_exit(EXIT_FAILURE, "zlog init failed\n");
    }

    /* listen to the shutdown event */
    force_quit = false;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* check the quantity of network ports */
    port_quantity = rte_eth_dev_count_avail();
    if (port_quantity < 2) {
        smto_exit(EXIT_FAILURE, "no enough Ethernet ports found\n");
    } else if (port_quantity > 2) {
        dzlog_warn("%d ports detected, but we only use two\n", port_quantity);
    }

    /* initialize the memory pool of dpdk */
    struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", 40960, 128, 0,
                                                            RTE_MBUF_DEFAULT_BUF_SIZE,
                                                            rte_socket_id());
    if (mbuf_pool == NULL) {
        smto_exit(EXIT_FAILURE, "cannot init mbuf pool\n");
    }

    uint16_t port_id;
    uint16_t prev_port_id = RTE_MAX_ETHPORTS;
    uint16_t port_num = 0;
    RTE_ETH_FOREACH_DEV(port_id) {
        /* initialize the network port and do some configure */
        init_port(port_id, mbuf_pool);

//        /* setup hairpin queues */
//        ret = setup_hairpin_queues(port_id, prev_port_id,
//                                   port_num);
//        if (ret) {
//            sprintf(err_msg, "fail to setup hairpin queues"
//                             " on port: %u\n", port_id);
//            smto_exit(EXIT_FAILURE, err_msg);
//        }
//        port_num++;
//        prev_port_id = port_id;

        /* start the port */
        ret = rte_eth_dev_start(port_id);
        if (ret < 0) {
            sprintf(err_msg, "fail to start network device: err=%d, port=%u\n",
                    ret, port_id);
            smto_exit(EXIT_FAILURE, err_msg);
        }

        /* check the port status */
        assert_link_status(port_id);
    }

    uint16_t lcore_id;
    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        /* Simpler equivalent. 8< */
        rte_eal_remote_launch(packet_processing, NULL, lcore_id);
        /* >8 End of simpler equivalent. */
    }

    smto_exit(EXIT_SUCCESS, ":: SUCCESS! all core stop running!\n");
}
