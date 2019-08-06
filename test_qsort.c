/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright 2019 Alex Richadson
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory (Department of Computer Science and
 * Technology) under DARPA contract HR0011-18-C-0016 ("ECATS"), as part of the
 * DARPA SSITH research programme.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <err.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sysexits.h>
#include "simple_benchmarks_common.h"

extern void benchmark_qsort(void *a, size_t n, size_t es, int (*cmp)(const void *, const void *));

static bool ascending;
extern long compare_long(long l1, long l2);
static int comparator(const void* a1, const void* a2) {
  const long* l1 = (const long*)a1;
  const long* l2 = (const long*)a2;
  // ensure that we access one global variable in the callback:
  // FIXME: should call another library functions to show how bad trampolines are
  return (int)(ascending ? compare_long(*l1,  *l2) : compare_long(*l2, *l1));
}

static void benchmark_one_run(long* buffer, long count) {
  benchmark_qsort(buffer, count, sizeof(long), comparator);
}

/*
 * Ensure that the benchmark buffer is nicely aligned to avoid differences between
 * CHERI and MIPS (since Cheri128 will increase the alignment of the buffer).
 * This might result in cachelines being evicted that shouldn't be.
 */
_Alignas(65536) static long benchmark_buffer[65536];
#ifndef nitems
#define nitems(array) (sizeof(array) / sizeof(array[0]))
#endif

int main(int argc, char** argv) {
  // if (argc < 2) {
  //  errx(EX_USAGE, "usage: <num_iterations> [ascending/descending] [bufsize]");
  // }
#ifdef __mips__
  long iterations = 500;
#else
  long iterations = 10000;
#endif
  if (argc > 1)
    iterations = atol(argv[1]);
  if (iterations < 1)
    errx(EX_USAGE, "iterations must be greater than 1: %ld", iterations);

#ifdef __mips__
  size_t bufsize = 5000; /* Takes to long with a 65K buffer */
#else
  size_t bufsize = nitems(benchmark_buffer);
#endif

  if (argc > 2) {
    if (*argv[2] != 'a' && *argv[2] != 'd')
      errx(EX_USAGE, "second argument must be 'a' or 'd': %s", argv[2]);
    ascending = *argv[2] == 'a';
  }
  if (argc > 3)
    bufsize = (size_t)atol(argv[3]);
  if (bufsize < 1 || bufsize > nitems(benchmark_buffer))
    errx(EX_USAGE, "bufsize must be between 1 and %ld: %ld", nitems(benchmark_buffer), iterations);
  // ensure buffer is already sorted:
  for (size_t idx = 0; idx < nitems(benchmark_buffer); idx++) {
    // worst case: set all to the same value!
    benchmark_buffer[idx] = ascending ? idx : nitems(benchmark_buffer) - idx;
  }
#if 0
  // sort once to ensure that the buffer is sorted:
  benchmark_qsort(benchmark_buffer, bufsize, sizeof(long), comparator);
  for (size_t idx = 0; idx < nitems(benchmark_buffer); idx++) {
    printf("buffer[%zd] = %ld\n", idx, benchmark_buffer[idx]);
  }
#endif
  // Now do the benchmark
  COLLECT_STATS(stats_at_start);
  for (long n = 0; n < iterations; n++) {
    benchmark_one_run(benchmark_buffer, bufsize);
  }
  COLLECT_STATS(stats_at_end);
  REPORT_STATS("-benchmark-loop", &stats_at_end, &stats_at_start);
}
