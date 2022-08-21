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

#include "v1/smart_offload.h"

void init_port(int port_id, struct rte_mempool *mbuf_pool) {
    int ret;
    char err_msg[MAX_ERROR_MESSAGE_LENGTH];

    struct rte_eth_dev_info dev_info;
    struct rte_eth_rxconf rxq_conf;
    struct rte_eth_txconf txq_conf;
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

    /* get basic info of network card */
    ret = rte_eth_dev_info_get(port_id, &dev_info);
    if (ret) {
        snprintf(err_msg, MAX_ERROR_MESSAGE_LENGTH, "Error during getting device (port %u) info: ", port_id);
        smto_exit(EXIT_FAILURE, err_msg);
    }

    dzlog_debug("initializing port: %d", port_id);

    /* check offload abilities of network card */
    port_conf.txmode.offloads &= dev_info.tx_offload_capa;

    /* configure the network card */
    ret = rte_eth_dev_configure(port_id,
                                GENERAL_QUEUES_QUANTITY + HAIRPIN_QUEUES_QUANTITY,
                                GENERAL_QUEUES_QUANTITY + HAIRPIN_QUEUES_QUANTITY, &port_conf);
    if (ret < 0) {
        snprintf(err_msg, MAX_ERROR_MESSAGE_LENGTH, "cannot configure device: err=%d, port=%u", ret, port_id);
        smto_exit(EXIT_FAILURE, err_msg);
    }

    /* configure receive port and receive queues */
    rxq_conf = dev_info.default_rxconf;
    rxq_conf.offloads = port_conf.rxmode.offloads;
    for (int i = 0; i < GENERAL_QUEUES_QUANTITY; i++) {
        ret = rte_eth_rx_queue_setup(port_id, i, QUEUE_DESC_NUMBER, rte_eth_dev_socket_id(port_id), &rxq_conf,
                                     mbuf_pool);
        if (ret < 0) {
            snprintf(err_msg, MAX_ERROR_MESSAGE_LENGTH, "Rx queue setup failed: err=%d, port=%u", ret, port_id);
            smto_exit(EXIT_FAILURE, err_msg);
        }
    }

    /* configure transmit port and transmit queues */
    txq_conf = dev_info.default_txconf;
    txq_conf.offloads = port_conf.txmode.offloads;
    for (int i = 0; i < GENERAL_QUEUES_QUANTITY; i++) {
        ret = rte_eth_tx_queue_setup(port_id, i, QUEUE_DESC_NUMBER, rte_eth_dev_socket_id(port_id), &txq_conf);
        if (ret < 0) {
            snprintf(err_msg, MAX_ERROR_MESSAGE_LENGTH, "Tx queue setup failed: err=%d, port=%u", ret, port_id);
            smto_exit(EXIT_FAILURE, err_msg);
        }
    }

    ret = rte_eth_promiscuous_enable(port_id);
    if (ret) {
        snprintf(err_msg, MAX_ERROR_MESSAGE_LENGTH, "promiscuous mode enable failed: err=%s, port=%u",
                 rte_strerror(-ret), port_id);
        smto_exit(EXIT_FAILURE, err_msg);
    }

    dzlog_debug("initializing port: %d done", port_id);
}

void assert_link_status(uint16_t port_id) {
    struct rte_eth_link link;
    uint8_t rep_cnt = MAX_REPEAT_TIMES;
    int link_get_err = -EINVAL;

    memset(&link, 0, sizeof(link));
    do {
        link_get_err = rte_eth_link_get(port_id, &link);
        if (link_get_err == 0 && link.link_status == ETH_LINK_UP)
            break;
        rte_delay_ms(CHECK_INTERVAL);
    } while (--rep_cnt);

    if (link_get_err < 0) {
        char *err_msg;
        snprintf(err_msg, MAX_ERROR_MESSAGE_LENGTH, "get link status is failing: %s", rte_strerror(-link_get_err));
        smto_exit(EXIT_FAILURE, err_msg);
    }
    if (link.link_status == ETH_LINK_DOWN) {
        smto_exit(EXIT_FAILURE, "link is still down");
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
        struct rte_flow_error error = {0};
        int ret = rte_flow_flush(port_id, &error);
        if (ret) {
            dzlog_error("cannot flush rte flow on port#%u: %s", port_id, error.message);
        }
        rte_eth_dev_stop(port_id);
        rte_eth_dev_close(port_id);
    }
#ifndef VM
    uint16_t lcore_id = 0;
    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        rte_power_exit(lcore_id);
    }
#endif

    /* clean up the EAL */
    rte_eal_cleanup();
    zlog_fini();
    rte_exit(exit_code, "%s", format);
}

