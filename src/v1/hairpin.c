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


/**
 * Setup hairpin queue.
 *
 * @param port_id The first port.
 * @param prev_port_id The second port.
 * @param port_num Used to mark is the odd port.
 * @return
 *   - 0 : Success
 */
static int setup_hairpin_queues(uint16_t port_id, uint16_t prev_port_id, bool same_port, uint16_t port_num) {
    /* The id of peer port */
    uint16_t peer_port_id = RTE_MAX_ETHPORTS;
    int ret;
    char err_msg[MAX_ERROR_MESSAGE_LENGTH];

    struct rte_eth_hairpin_conf hairpin_conf = {
            .peer_count = 1,
            .manual_bind = 1,
            .tx_explicit = 1,
    };
    if (same_port) {
        hairpin_conf.manual_bind = 0;
        hairpin_conf.tx_explicit = 0;
    }
    struct rte_eth_dev_info dev_info = {0};
    struct rte_eth_dev_info peer_dev_info = {0};
    struct rte_eth_rxq_info rxq_info = {0};
    struct rte_eth_txq_info txq_info = {0};

    uint16_t total_rxq_quantity, general_rxq_quantity, general_txq_quantity, peer_general_rxq_quantity, peer_general_txq_quantity;

    /* get the information of port */
    ret = rte_eth_dev_info_get(port_id, &dev_info);
    if (ret) {
        snprintf(err_msg, MAX_ERROR_MESSAGE_LENGTH, "cannot get device info, port id: %u\n", port_id);
        smto_exit(EXIT_FAILURE, err_msg);
    }
    general_rxq_quantity = dev_info.nb_rx_queues - HAIRPIN_QUEUES_QUANTITY;
    general_txq_quantity = dev_info.nb_tx_queues - HAIRPIN_QUEUES_QUANTITY;
    total_rxq_quantity = dev_info.nb_rx_queues;

    /* get the port to bind with */
    if (same_port) { /* bind with same port */
        peer_port_id = port_id;
    } else if (port_num & 0x1) { /* if this port is odd, it will bind with last port */
        peer_port_id = prev_port_id;
    } else {
        peer_port_id = rte_eth_find_next_owned_by(port_id + 1,
                                                  RTE_ETH_DEV_NO_OWNER);
        if (peer_port_id >= RTE_MAX_ETHPORTS)
            peer_port_id = port_id;
    }

    /* get the information of peer port */
    ret = rte_eth_dev_info_get(peer_port_id, &peer_dev_info);
    if (ret) {
        snprintf(err_msg, MAX_ERROR_MESSAGE_LENGTH, "cannot get peer device info, port id: %u\n", port_id);
        smto_exit(EXIT_FAILURE, err_msg);
    }
    peer_general_rxq_quantity = peer_dev_info.nb_rx_queues - HAIRPIN_QUEUES_QUANTITY;
    peer_general_txq_quantity = peer_dev_info.nb_tx_queues - HAIRPIN_QUEUES_QUANTITY;

    /* create hairpin queues on both ports*/
    for (uint16_t hairpin_queue = general_rxq_quantity, peer_hairpin_queue = peer_general_txq_quantity;
         hairpin_queue < total_rxq_quantity;
         hairpin_queue++, peer_hairpin_queue++) {
        hairpin_conf.peers[0].port = peer_port_id;
        hairpin_conf.peers[0].queue = peer_hairpin_queue;
        ret = rte_eth_rx_hairpin_queue_setup(
                port_id, hairpin_queue,
                rxq_info.nb_desc, &hairpin_conf);
        if (ret != 0)
            return ret;
    }

    for (uint16_t hairpin_queue = general_txq_quantity, peer_hairpin_queue = peer_general_rxq_quantity;
         hairpin_queue < total_rxq_quantity;
         hairpin_queue++, peer_hairpin_queue++) {
        hairpin_conf.peers[0].port = peer_port_id;
        hairpin_conf.peers[0].queue = peer_hairpin_queue;
        ret = rte_eth_tx_hairpin_queue_setup(
                port_id, hairpin_queue,
                txq_info.nb_desc, &hairpin_conf);
        if (ret != 0)
            return ret;
    }
    return 0;
}


/**
 * Bind all TX hairpin queues to the pair port RX queues.
 *
 * @param port_id
 * @param direction
 * @return
 */
static int hairpin_port_bind(uint16_t port_id, int direction) {
    int i, ret = 0;
    uint16_t peer_ports[RTE_MAX_ETHPORTS];
    int peer_ports_num = 0;

    peer_ports_num = rte_eth_hairpin_get_peer_ports(port_id,
                                                    peer_ports, RTE_MAX_ETHPORTS, direction);
    if (peer_ports_num < 0)
        return peer_ports_num; /* errno. */
    for (i = 0; i < peer_ports_num; i++) {
        ret = rte_eth_hairpin_bind(port_id, peer_ports[i]);
        if (ret)
            return ret;
    }
    return ret;
}

/**
 * Unbind peer hairpin ports.
 *
 * @param port_id The port need to be unbind.
 * @return
 *   - <0 : do not have peer port
 *   - 0 : Success
 */
static int hairpin_port_unbind(uint16_t port_id) {
    uint16_t pair_port_list[RTE_MAX_ETHPORTS];
    int pair_port_num, i;

    /* unbind current port's hairpin TX queues. */
    rte_eth_hairpin_unbind(port_id, RTE_MAX_ETHPORTS);
    /* find all peer TX queues bind to current ports' RX queues. */
    pair_port_num = rte_eth_hairpin_get_peer_ports(port_id,
                                                   pair_port_list, RTE_MAX_ETHPORTS, 0);
    if (pair_port_num < 0)
        return pair_port_num;

    for (i = 0; i < pair_port_num; i++) {
        rte_eth_hairpin_unbind(pair_port_list[i], port_id);
    }
    return 0;
}

void setup_two_port_hairpin() {
    int ret;
    uint16_t port_id;
    char err_msg[MAX_ERROR_MESSAGE_LENGTH];

    /* setup hairpin queues */
    uint16_t prev_port_id = RTE_MAX_ETHPORTS;
    uint16_t port_num = 0;
    RTE_ETH_FOREACH_DEV(port_id) {
        ret = setup_hairpin_queues(port_id, prev_port_id, false, port_num);
        if (ret) {
            snprintf(err_msg, MAX_ERROR_MESSAGE_LENGTH, "failed to setup hairpin queues"
                                                        " on port: %u", port_id);
            smto_exit(EXIT_FAILURE, err_msg);
        }
        port_num++;
        prev_port_id = port_id;
    }

    /* start the ports */
    RTE_ETH_FOREACH_DEV(port_id) {
        ret = rte_eth_dev_start(port_id);
        if (ret < 0) {
            snprintf(err_msg, MAX_ERROR_MESSAGE_LENGTH, "failed to start network device: err=%d, port=%u",
                     ret, port_id);
            smto_exit(EXIT_FAILURE, err_msg);
        }
        /* check the port status */
        assert_link_status(port_id);
    }

    /* bind the peer ports */
    RTE_ETH_FOREACH_DEV(port_id) {
        /* Let's find our peer RX ports, TXQ -> RXQ. */
        ret = hairpin_port_bind(port_id, 1);
        if (ret) {
            snprintf(err_msg, MAX_ERROR_MESSAGE_LENGTH, "failed to bind port#%u.tx with peer's rx", port_id);
            smto_exit(EXIT_FAILURE, err_msg);
        }
        /* Let's find our peer TX ports, RXQ -> TXQ. */
        ret = hairpin_port_bind(port_id, 0);
        if (ret) {
            snprintf(err_msg, MAX_ERROR_MESSAGE_LENGTH, "failed to bind port#%u.rx with peer's tx", port_id);
            smto_exit(EXIT_FAILURE, err_msg);
        }
    }
}

void setup_one_port_hairpin(int port_id) {
    int ret;
    char err_msg[MAX_ERROR_MESSAGE_LENGTH];

    /* setup hairpin queues */
    ret = setup_hairpin_queues(port_id, port_id, true, 0);
    if (ret) {
        snprintf(err_msg, MAX_ERROR_MESSAGE_LENGTH, "failed to setup hairpin queues"
                                                    " on port: %u", port_id);
        smto_exit(EXIT_FAILURE, err_msg);
    }

    /* start the ports */
    ret = rte_eth_dev_start(port_id);
    if (ret < 0) {
        snprintf(err_msg, MAX_ERROR_MESSAGE_LENGTH, "failed to start network device: err=%d, port=%u",
                 ret, port_id);
        smto_exit(EXIT_FAILURE, err_msg);
    }
    /* check the port status */
    assert_link_status(port_id);
}


int destroy_hairpin() {
    uint16_t port_id;
    int ret, error = 0;

    RTE_ETH_FOREACH_DEV(port_id) {
        ret = hairpin_port_unbind(port_id);
        if (ret) {
            dzlog_error("error on unbind hairpin port: %u", port_id);
            error = ret;
        }
    }
    return error;
}