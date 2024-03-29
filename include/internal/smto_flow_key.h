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
#include <stdbool.h>
#else
#include <rte_jhash.h>
#endif

/// Used to allocate memory for dumping five tuple.
#define MAX_PKT_INFO_LENGTH 50

/// Used to mask useless bits when extract packet info.
#define ALL_32_BITS 0xffffffff
#define BIT_8_TO_15 0x0000ff00

typedef __m128i xmm_t;

enum offload_status {
  NOT_OFFLOAD = 0,
  OFFLOADING = 1,
  OFFLOAD_SUCCESS = 2,
};

struct smto_flow_key {
  union {
    struct rdarm_five_tuple tuple; ///< The tuple to identify a flow.
    struct {
      uint8_t pad0;
      uint8_t proto;
      uint16_t pad1;
      uint32_t ip_src;
      uint32_t ip_dst;
      uint16_t port_src;
      uint16_t port_dst;
    }; ///< The struct to load tuple from mbuf.
    xmm_t xmm;
  };
  struct rdarm_five_tuple modify_tuple;
  struct rte_flow *flow;
  volatile uint64_t create_at; ///< Use the number of cycles of CPU as the time.
  volatile uint32_t flow_size; ///< Total size of packets in this flow.
  volatile uint32_t packet_amount; ///< Total amount of packets in a flow.
//  uint16_t new_port;
  enum offload_status is_offload; ///< Has created rte_flow to offload flow or not
  struct smto_flow_key *symmetrical_flow_key;
};

/**
 * Return a format string of ipv4 5-tuple.
 *
 * @param key The key want to print.
 * @param port_id The port id of this pkt.
 * @param qi The id of queue, negative means ignore.
 * @param result The result string.
 * @param result_size The max length of result string.
 */
void dump_pkt_info(struct rdarm_five_tuple *key, uint16_t port_id, int qi, char *result, int result_length);

#endif //SMART_OFFLOAD_INCLUDE_INTERNAL_SMTO_FLOW_KEY_H_
