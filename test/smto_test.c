#include <zlog.h>
#include <rte_eal.h>
#include <signal.h>

#include "smto.h"

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

static struct smto *smto_cb;

bool is_running = true;

static void signal_handler(int signum) {
  if (signum == SIGINT || signum == SIGTERM) {
    dzlog_info("Signal %d received, preparing to exit...",
               signum);
    is_running = false;
  }
}

int main(int argc, char **argv) {
  int ret = 0;

  ret = zlog_init("conf/zlog.conf");
  if (ret) {
    printf("zlog init failed\n");
    return -1;
  };

  zlog_category_t *logger = zlog_get_category("main");
  ret = rte_eal_init(argc, argv);
  if (ret < 0) {
    zlog_error(logger, "invalid EAL arguments\n");
    ret = -2;
    goto rte_err;
  }

  ret = init_smto(&smto_cb);
  if (ret != SMTO_SUCCESS) {
    zlog_error(logger, "init smto failed: %s\n", smto_error_string(ret));
    ret = -3;
    goto smto_err;
  }

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  zlog_info(logger, "SmartOffload start running!");

  is_running = true;
  while(is_running);

  destroy_smto(smto_cb);

  zlog_info(logger, "SmartOffload stop running!");
  smto_err:
  rte_eal_cleanup();
  rte_err:
  zlog_fini();
  exit(ret);
}