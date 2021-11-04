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

/* Shutdown flag */
volatile bool force_quit;


/* Shutdown event has been triggered */
static void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        printf("\n\nSignal %d received, preparing to exit...\n",
               signum);
        force_quit = true;
    }
}

void smto_exit(int exit_code, const char *format) {
    zlog_fini();
    rte_exit(exit_code, "%s", format);
}

int main(int argc, char **argv) {
    /* General return value */
    int ret;
    /* Quantity of ports */
    uint16_t port_amount;
    zlog_category_t *c;

    ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        smto_exit(EXIT_FAILURE, "invalid EAL arguments\n");
    }

    ret = zlog_init("conf/zlog.conf");
    if (ret) {
        smto_exit(EXIT_FAILURE, "zlog init failed\n");
    }

    c = zlog_get_category("main");
    if (!c) {
        smto_exit(EXIT_FAILURE, "zlog main category get failed\n");
    }

    force_quit = false;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    port_amount = rte_eth_dev_count_avail();
    if (port_amount < 2) {
        smto_exit(EXIT_FAILURE, "no enough Ethernet ports found\n");
    } else if (port_amount > 2) {
        zlog_warn(c, "%d ports detected, but we only use two\n", port_amount);
    }

    struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", 40960, 128, 0,
                                                            RTE_MBUF_DEFAULT_BUF_SIZE,
                                                            rte_socket_id());
    if (mbuf_pool == NULL) {
        smto_exit(EXIT_FAILURE, "cannot init mbuf pool\n");
    }

    uint16_t port_id;
    RTE_ETH_FOREACH_DEV(port_id) {
    }

    uint16_t lcore_id;
    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        /* Simpler equivalent. 8< */
        rte_eal_remote_launch(packet_processing, NULL, lcore_id);
        /* >8 End of simpler equivalent. */
    }

    smto_exit(EXIT_SUCCESS, "all core stop running\n");
}
