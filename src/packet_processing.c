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
#include "smart_offload.h"
#include "hash_key.h"
#include "flow_meta.h"

static __thread zlog_category_t *zc = 0;
static __thread struct rte_hash *flow_hash_table = 0;
static rte_xmm_t mask0;

static inline void print_ether_addr(const char *ex, struct rte_ether_addr *eth_addr, char *result) {
    char buf[RTE_ETHER_ADDR_FMT_SIZE];
    rte_ether_format_addr(buf, RTE_ETHER_ADDR_FMT_SIZE, eth_addr);
    sprintf(result, "%s%s", ex, buf);
}

static inline void format_ipv4_addr(const char *ex, const uint32_t ip, char *result) {
    sprintf(result, "%s%d.%d.%d.%d", ex,
            (ip >> 24) & 0x000000ff,
            (ip >> 16) & 0x000000ff,
            (ip >> 8) & 0x000000ff,
            (ip) & 0x000000ff);
}

static inline void dump_pkt_info(union ipv4_5tuple_host *key, uint16_t qi, char *result) {
    char pkt_info[2][100] = {{0},
                             {0},};

    uint32_t src_ip = rte_be_to_cpu_32(key->ip_src);
    format_ipv4_addr(" - src_ip=", src_ip, pkt_info[0]);
    uint32_t dst_ip = rte_be_to_cpu_32(key->ip_dst);
    format_ipv4_addr(" - dst_ip=", dst_ip, pkt_info[1]);
    uint16_t src_port = rte_be_to_cpu_16(key->port_src);
    uint16_t dst_port = rte_be_to_cpu_16(key->port_dst);

    snprintf(result, 250, "%s:%u%s:%u - queue=0x%x", pkt_info[0], src_port, pkt_info[1], dst_port,
             (unsigned int) qi);
}

static __rte_always_inline void get_ipv4_5tuple(struct rte_mbuf *m0, __m128i mask0, union ipv4_5tuple_host *key) {
    __m128i tmpdata0 = _mm_loadu_si128(
            rte_pktmbuf_mtod_offset(m0, __m128i *,
                                    sizeof(struct rte_ether_hdr) +
                                    offsetof(struct rte_ipv4_hdr, time_to_live)));

    key->xmm = _mm_and_si128(tmpdata0, mask0);
}

/**
 * Check the packet type and process it with corresponding method.
 *
 * @param m The packet raw data.
 * @param queue_index The index of queue which the packet comes from.
 * @param key The key used to hash.
 */
static inline int packet_processing(struct rte_mbuf *m, uint16_t queue_index) {
    int ret = 0;
    union ipv4_5tuple_host key = {.xmm=0};
    mask0 = (rte_xmm_t) {.u32 = {BIT_8_TO_15, ALL_32_BITS,
                                 ALL_32_BITS, ALL_32_BITS}};
    if (m->packet_type & RTE_PTYPE_L3_IPV4 && m->packet_type & RTE_PTYPE_L4_TCP) {
        get_ipv4_5tuple(m, mask0.x, &key);
        char pkt_info[250];
        dump_pkt_info(&key, queue_index, pkt_info);
        struct flow_meta *data = 0;
        ret = rte_hash_lookup_data(flow_hash_table, &key, (void **) &data);
        zlog_debug(zc, "packet(%u)(%d): %s", m->pkt_len, ret, pkt_info);
        if (ret == -ENOENT) { // A flow that has not yet appeared
            data = create_flow_meta(m->pkt_len);
            ret = rte_hash_add_key_data(flow_hash_table, &key, data);
            if (ret != 0) {
                zlog_error(zc, "can not add pkt:%s into flow table, return error %d", pkt_info, ret);
                return 1;
            } else {
                zlog_debug(zc, "success add a flow to flow hash table");
                return 0;
            }
        } else if (ret >= 0) { // A flow that has appeared before
            data->packet_amount++;
            zlog_debug(zc, "capture a packet which belong to a flow in flow table, which already have %d packets",
                       data->packet_amount);
            return 0;
        } else {
            zlog_error(zc, "unhandled error occur");
            return -1;
        }

    }
    return 0;
}


int process_loop(void *args) {
    unsigned lcore_id;
    zc = zlog_get_category("worker");
    lcore_id = rte_lcore_id();
    uint16_t queue_id = *(uint16_t *) args;
    zlog_info(zc, "worker #%u for queue #%u start working!", lcore_id, queue_id);

    char table_name[RTE_HASH_NAMESIZE];
    snprintf(table_name, sizeof(table_name), "flow_hash_table_%u", lcore_id);
    /* create hash map */
    struct rte_hash_parameters flow_hash_table_parameter = {
            .name = table_name,
            .entries = MAX_HASH_ENTRIES,
            .key_len = sizeof(union ipv4_5tuple_host),
            .hash_func = ipv4_hash_crc,
            .hash_func_init_val = 0,
    };
    flow_hash_table = rte_hash_create(&flow_hash_table_parameter);
    if (flow_hash_table == NULL) {
        char err_msg[MAX_ERROR_MESSAGE_LENGTH];
        snprintf(err_msg, MAX_ERROR_MESSAGE_LENGTH, "unable to create flow hash table on worker #%u", lcore_id);
        smto_exit(EXIT_FAILURE, err_msg);
    }


    /* pre-allocate the local variable */
    struct rte_mbuf *mbufs[32] = {0};
    struct rte_flow_error error;
    uint16_t port_id = 0;
    uint16_t nb_rx;
    uint16_t nb_tx;
    uint16_t packet_index;

    while (!force_quit) {
        nb_rx = rte_eth_rx_burst(port_id, queue_id, mbufs, 32);
        if (nb_rx) {
            for (packet_index = 0; packet_index < nb_rx; packet_index++) {
                struct rte_mbuf *m = mbufs[packet_index];
                packet_processing(m, queue_id);
            }
            nb_tx = rte_eth_tx_burst(port_id, queue_id,
                                     mbufs, nb_rx);
        }
        /* Free any unsent packets. */
        if (unlikely(nb_tx < nb_rx)) {
            uint16_t buf;
            for (buf = nb_tx; buf < nb_rx; buf++)
                rte_pktmbuf_free(mbufs[buf]);
        }
    }

    const void *key = 0;
    void *data = 0;
    uint32_t *next = 0;
    uint32_t current = rte_hash_iterate(flow_hash_table, &key, &data, next);
    for (; current != -ENOENT; current = rte_hash_iterate(flow_hash_table, &key, &data, next)) {
        int32_t del_key = rte_hash_del_key(flow_hash_table, key);
        rte_hash_free_key_with_position(flow_hash_table, del_key);
        rte_free(data);
    }
    // TODO: check the memory of key and value has free or not
    rte_hash_free(flow_hash_table);
    return 0;
}
