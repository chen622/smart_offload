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

#ifndef SMART_OFFLOAD_SRC_SMTO_UTILS_H_
#define SMART_OFFLOAD_SRC_SMTO_UTILS_H_
#include <zlog.h>
#include <pthread.h>
#include <memory.h>
#include <stdlib.h>
#include <stdint.h>

#define TURN 2


/**
 * @brief Make statistics of time and print average, 50%, 90%, 99% and max value.
 *
 * @param logger The logger to print result.
 * @param time_array The array of time.
 * @param size The size of array.
 * @param prefix_name The prefix name of output result.
 */
void time_stat(zlog_category_t *logger, uint64_t time_array[], int size, char *prefix_name);

#endif //SMART_OFFLOAD_SRC_SMTO_UTILS_H_
