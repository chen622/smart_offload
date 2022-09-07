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

#include <stdint.h>
#include <rte_eal.h>
#include <zlog.h>
#include <rte_mbuf.h>

#include "smto.h"
#include "internal/smto_utils.h"

#define ALLOCATE_AMOUNT 100000
#define ALLOCATE_SIZE 1024

int main(int argc, char **argv) {
  int ret;
  ret = zlog_init("conf/zlog.conf");
  if (ret) {
    printf("zlog init failed\n");
    return -1;
  };

  zlog_category_t *logger = zlog_get_category("main");
  ret = rte_eal_init(argc, argv);
  if (ret < 0) {
    zlog_error(logger, "invalid EAL arguments\n");
    goto err;
  }


  /// Initialize the memory pool
  struct rte_mempool
      *mbuf_pool = rte_mempool_create("mbuf_pool", ALLOCATE_AMOUNT, ALLOCATE_SIZE, RTE_MEMPOOL_CACHE_MAX_SIZE, 0,
                                      NULL, NULL, NULL, NULL,
                                      rte_socket_id(), RTE_MEMPOOL_F_NO_SPREAD);
  if (mbuf_pool == NULL) {
    zlog_error(logger, "mem init failed: %s", rte_strerror(rte_errno));
    goto err2;
  }

  char *ptr[ALLOCATE_AMOUNT];
  uint64_t used_time[ALLOCATE_AMOUNT];
  for (int i = 0; i < ALLOCATE_AMOUNT; ++i) {
    uint64_t start = rte_rdtsc();
    rte_mempool_get(mbuf_pool, (void **) &(ptr[i]));
    used_time[i] = GET_NANOSECOND(start);
  }
  rte_mempool_free(mbuf_pool);
  time_stat(logger, used_time, ALLOCATE_AMOUNT, "memory pool");

  rte_delay_us_sleep(1000);
  for (int i = 0; i < ALLOCATE_AMOUNT; ++i) {
    uint64_t start = rte_rdtsc();
    ptr[i] = rte_zmalloc("test", ALLOCATE_SIZE, 0);
//    ptr[i] = calloc(1, ALLOCATE_SIZE);
    used_time[i] = GET_NANOSECOND(start);
  }
  for (int i = 0; i < ALLOCATE_AMOUNT; ++i) {
    rte_free(ptr[i]);
  }
  time_stat(logger, used_time, ALLOCATE_AMOUNT, "memory alloc");

  err2:
  rte_eal_cleanup();
  err:
  zlog_fini();
  return ret;
}