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

#ifndef SMART_OFFLOAD_HASH_KEY_H
#define SMART_OFFLOAD_HASH_KEY_H

/* check the instruction support for hash function */
#if defined(RTE_ARCH_X86) || defined(__ARM_FEATURE_CRC32)
#define EM_HASH_CRC 1
#endif

#ifdef EM_HASH_CRC

#include <rte_hash_crc.h>

#define DEFAULT_HASH_FUNC       rte_hash_crc
#else
#include <rte_jhash.h>
#define DEFAULT_HASH_FUNC       rte_jhash
#endif

/**
 * The width of wide instruction for load data.
 */
typedef __m128i xmm_t;

#define IPV6_ADDR_LEN 16

struct ipv4_5tuple {
    uint32_t ip_dst;
    uint32_t ip_src;
    uint16_t port_dst;
    uint16_t port_src;
    uint8_t proto;
} __rte_packed;

/**
 * The key of the flow hash table.
 */
union ipv4_5tuple_host {
    struct {
        uint8_t pad0;
        uint8_t proto;
        uint16_t pad1;
        uint32_t ip_src;
        uint32_t ip_dst;
        uint16_t port_src;
        uint16_t port_dst;
    };
    xmm_t xmm;
};


/**
 * The hash function to generate the real hash key.
 *
 * @param data The key used as the key of flow table.
 * @param data_len The length of key.
 * @param init_val
 * @return The real hash key.
 */
static inline uint32_t ipv4_hash_crc(const void *data, __rte_unused uint32_t data_len,
                                     uint32_t init_val) {
    const union ipv4_5tuple_host *k;
    uint32_t t;
    const uint32_t *p;

    k = data;
    t = k->proto;
    p = (const uint32_t *) &k->port_src;

#ifdef EM_HASH_CRC
    init_val = rte_hash_crc_4byte(t, init_val);
    init_val = rte_hash_crc_4byte(k->ip_src, init_val);
    init_val = rte_hash_crc_4byte(k->ip_dst, init_val);
    init_val = rte_hash_crc_4byte(*p, init_val);
#else
    init_val = rte_jhash_1word(t, init_val);
    init_val = rte_jhash_1word(k->ip_src, init_val);
    init_val = rte_jhash_1word(k->ip_dst, init_val);
    init_val = rte_jhash_1word(*p, init_val);
#endif

    return init_val;
}

static void convert_ipv4_5tuple(struct ipv4_5tuple *key1,
                                union ipv4_5tuple_host *key2) {
    key1->ip_dst = rte_be_to_cpu_32(key2->ip_dst);
    key1->ip_src = rte_be_to_cpu_32(key2->ip_src);
    key1->port_dst = rte_be_to_cpu_16(key2->port_dst);
    key1->port_src = rte_be_to_cpu_16(key2->port_src);
    key1->proto = key2->proto;
}

#endif //SMART_OFFLOAD_HASH_KEY_H
