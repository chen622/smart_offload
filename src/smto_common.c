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

#include "smto_comon.h"

const char *smto_error_string(enum smto_error_code err_code) {
  switch (err_code) {
    case SMTO_SUCCESS: return "success";
    case SMTO_ERROR_HUGE_PAGE_MEMORY_ALLOCATION: return "failed to allocate huge page memory";
    case SMTO_ERROR_MEMORY_ALLOCATION: return "failed to allocate memory allocation";
    case SMTO_ERROR_NO_ENOUGH_WORKER: return "no enough worker";
    case SMTO_ERROR_NO_AVAILABLE_PORTS: return "no available ports";
    case SMTO_ERROR_DEVICE_CONFIGURE: return "device configure error";
    case SMTO_ERROR_QUEUE_SETUP: return "failed to setup queue";
    case SMTO_ERROR_DEVICE_START: return "failed to start device";
    case SMTO_ERROR_FLOW_CREATE: return "failed to create flow";
    case SMTO_ERROR_FLOW_QUERY: return "failed to query flow";
    case SMTO_ERROR_EVENT_REGISTER: return "failed to register event";
    case SMTO_ERROR_HASH_MAP_CREATION: return "failed to create hash map";
    case SMTO_ERROR_HASH_MAP_OPERATION: return "failed to operate hash map";
    case SMTO_ERROR_WORKER_LAUNCH: return "failed to launch worker";
    case SMTO_ERROR_RING_CREATION: return "failed to create ring";
    case SMTO_ERROR_UNSUPPORTED_PACKET_TYPE: return "unsupported packet type";
    case SMTO_ERROR_UNKNOWN: return "unknown error";
    default: return "unsupported error code";
  }

}