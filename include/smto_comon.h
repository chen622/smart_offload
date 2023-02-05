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

#ifndef SMART_OFFLOAD_INCLUDE_SMTO_COMMON_H_
#define SMART_OFFLOAD_INCLUDE_SMTO_COMMON_H_

#define GET_NANOSECOND(start_time) (rte_rdtsc() - start_time) / (double) rte_get_tsc_hz() * 1000 * 1000 * 1000


enum smto_error_code {
  SMTO_SUCCESS = 0,
  SMTO_ERROR_MEMORY_ALLOCATION,
  SMTO_ERROR_HUGE_PAGE_MEMORY_ALLOCATION,
  SMTO_ERROR_NO_ENOUGH_WORKER,
  SMTO_ERROR_NO_AVAILABLE_PORTS,
  SMTO_ERROR_DEVICE_CONFIGURE,
  SMTO_ERROR_QUEUE_SETUP,
  SMTO_ERROR_HAIRPIN_SETUP,
  SMTO_ERROR_HAIRPIN_BIND,
  SMTO_ERROR_DEVICE_START,
  SMTO_ERROR_FLOW_CREATE,
  SMTO_ERROR_FLOW_QUERY,
  SMTO_ERROR_EVENT_REGISTER,
  SMTO_ERROR_HASH_MAP_CREATION,
  SMTO_ERROR_HASH_MAP_OPERATION,
  SMTO_ERROR_WORKER_LAUNCH,
  SMTO_ERROR_RING_CREATION,
  SMTO_ERROR_RING_OPERATION,
  SMTO_ERROR_UNSUPPORTED_PACKET_TYPE,
  SMTO_ERROR_UNKNOWN = -100,
};

enum smto_mode {
  SINGLE_PORT_MODE = 0,
  DOUBLE_PORT_MODE
};

/**
 * @brief Get a readable message.
 */
const char *smto_error_string(enum smto_error_code error);

#endif //SMART_OFFLOAD_INCLUDE_SMTO_COMMON_H_
