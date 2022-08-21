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

#ifndef SMART_OFFLOAD_INCLUDE_INTERNAL_SMTO_FLOW_KEY_H_
#define SMART_OFFLOAD_INCLUDE_INTERNAL_SMTO_FLOW_KEY_H_

#include <stdint.h>
#include <nmmintrin.h>
#include <rdarm.h>

/// Check the instruction support for hash function.
#if defined(RTE_ARCH_X86) || defined(__ARM_FEATURE_CRC32)
#define EM_HASH_CRC 1
#endif

#ifdef EM_HASH_CRC
#include <rte_hash_crc.h>
#else
#include <rte_jhash.h>
#endif

/// Used to allocate memory for dumping five tuple.
#define PKT_INFO_MAX_LENGTH 50

typedef __m128i xmm_t;

/// The structure used to extract from packet.
union ipv4_five_tuple {
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

struct smto_flow_key {
  struct rdarm_five_tuple tuple;
  struct rte_flow *flow;
  volatile uint64_t create_at; ///< Use the number of cycles of CPU as the time.
  volatile uint32_t flow_size; ///< Total size of packets in this flow.
  volatile uint32_t packet_amount; ///< Total amount of packets in a flow.
  bool is_offload; ///< Has created rte_flow to offload flow or not
};


/**
 * Return a format string of ipv4 5-tuple.
 *
 * @param key The key want to print.
 * @param qi The id of queue.
 * @param result The result string.
 * @param result_size The max length of result string.
 */
void dump_pkt_info(struct rdarm_five_tuple *key, uint16_t qi, char *result, int result_length);


#endif //SMART_OFFLOAD_INCLUDE_INTERNAL_SMTO_FLOW_KEY_H_
