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
#include <sys/time.h>
#include "flow_management.h"


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
    uint16_t port_id;

    /* Setup environment of DPDK */
    ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        smto_exit(EXIT_FAILURE, "invalid EAL arguments");
    }

    /* Listen to the shutdown event */
    force_quit = false;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Initialize the memory pool of dpdk */
    struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", NUM_MBUFS, CACHE_SIZE, 0,
                                                            RTE_MBUF_DEFAULT_BUF_SIZE,
                                                            rte_socket_id());
    if (mbuf_pool == NULL) {
        smto_exit(EXIT_FAILURE, "mem init failed");

    }

    port_id = rte_eth_find_next_owned_by(0, RTE_ETH_DEV_NO_OWNER);
    init_port(port_id, mbuf_pool);
    setup_one_port_hairpin(port_id);


    struct rte_flow *flow = 0;
    struct rte_flow_error flow_error = {0};
    struct timeval start, end;
    gettimeofday(&start, NULL);
    long flow_count = 0;
    union ipv4_5tuple_host tuple = {
            .port_src = 10,
            .port_dst = 1,
            .proto = IPPROTO_TCP,
            .ip_src = RTE_IPV4(1, 1, 1, 1),
            .ip_dst = RTE_IPV4(2, 2, 2, 2)
    };
    zlog_category_t *zc = zlog_get_category("worker");
    while (!force_quit) {
        flow = create_offload_rte_flow(port_id, NULL, &tuple, zc, &flow_error);
        tuple.port_dst = (tuple.port_dst + 1) % 50000;
        tuple.ip_src = tuple.ip_src++;
        if (flow == NULL) {
            force_quit = true;
            printf("err: %s\n", flow_error.message);
        }
        flow_count++;
    }
    gettimeofday(&end, NULL);
    long time_use = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
    printf("total has %ld flows and take %ld us, average %f us", flow_count, time_use,
           (double) time_use / (double) flow_count);
    return 1;
}