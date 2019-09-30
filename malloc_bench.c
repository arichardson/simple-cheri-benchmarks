/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright 2019 Alex Richardson
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
#include <inttypes.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include "simple_benchmarks_common.h"

struct allocated_item {
  void *ptr;
  size_t size;
};
// allocate something between zero and 64K
#define SMALL_ALLOC_SIZE 256
#define MAX_ALLOC_SIZE 65536

static size_t get_next_alloc_size(bool small) {
  long random_value = random();
  return small ? random_value % SMALL_ALLOC_SIZE
               : random_value % MAX_ALLOC_SIZE;
}

static unsigned verbosity = 0;
#define dbg(...) do { if (verbosity) { printf(__VA_ARGS__); } } while(0)
#define dbg_verbose(...) do { if (verbosity > 1) { printf(__VA_ARGS__); } } while(0)

#define MAX_ALLOCATIONS 4096
struct allocated_item allocations[MAX_ALLOCATIONS];

static void run_benchmark(size_t num_elements) {
  if (num_elements > nitems(allocations))
    errx(EX_USAGE, "Invalid number of items: %zd", num_elements);
  memset(allocations, 0, sizeof(allocations));
  COLLECT_STATS(stats_start);
  for (size_t i = 0; i < nitems(allocations); i++) {
    // Make a large allocation every fourth time:
    allocations[i].size = get_next_alloc_size(i % 4 != 3);
    allocations[i].ptr = malloc(allocations[i].size);
    dbg_verbose("malloc()'d %ld bytes: %#p\n", allocations[i].size, allocations[i].ptr);
  }
  COLLECT_STATS(stats_after_malloc);
  // free every third pointer:
  for (size_t i = 0; i < nitems(allocations); i += 3) {
    free(allocations[i].ptr);
    dbg_verbose("free'd() %ld bytes: %#p\n", allocations[i].size, allocations[i].ptr);
    allocations[i].ptr = NULL;
  }
  COLLECT_STATS(stats_after_partial_free);
  // realloc the remaining pointers to a new random size:
  for (size_t i = 0; i < nitems(allocations); i++) {
    // Large allocation size for every eigth allocation:
    size_t new_size = get_next_alloc_size(i % 8 != 7);
    if (!allocations[i].ptr) {
      // Add a new allocation in the free'd slots
      allocations[i].size = new_size;
      allocations[i].ptr = calloc(new_size, 1);
      dbg_verbose("calloc()'d %ld bytes: %#p\n", allocations[i].size,
          allocations[i].ptr);
      continue;
    }
    // otherwise resize the allocations to a new size
    void *new_pointer = realloc(allocations[i].ptr, new_size);
    dbg_verbose("Relloc'd %ld -> %ld bytes: %#p -> %#p\n", allocations[i].size,
        new_size, allocations[i].ptr, new_pointer);
    allocations[i].size = new_size;
    allocations[i].ptr = new_pointer;
  }
  COLLECT_STATS(stats_after_realloc);
  // finally free everything:
  for (size_t i = 0; i < nitems(allocations); i++) {
    free(allocations[i].ptr);
    dbg_verbose("free'd() %ld bytes: %#p\n", allocations[i].size, allocations[i].ptr);
    allocations[i].ptr = NULL;
  }
  COLLECT_STATS(stats_at_end);
#if HAVE_STATCOUNTERS
  dbg("User instructions for malloc() loop:     %12" PRId64 "\n", stats_after_malloc.inst_user - stats_start.inst_user);
  dbg("User instructions for free() loop:       %12" PRId64 "\n", stats_after_partial_free.inst_user - stats_after_malloc.inst_user);
  dbg("User instructions for realloc() loop:    %12" PRId64 "\n", stats_after_realloc.inst_user - stats_after_partial_free.inst_user);
  dbg("User instructions for final free() loop: %12" PRId64 "\n", stats_at_end.inst_user - stats_after_realloc.inst_user);
  dbg("User instructions (total):               %12" PRId64 "\n", stats_at_end.inst_user - stats_start.inst_user);
#endif
  REPORT_STATS("-initial-malloc", &stats_after_malloc, &stats_start);
  REPORT_STATS("-partial-free", &stats_after_partial_free, &stats_after_malloc);
  REPORT_STATS("-realloc", &stats_after_realloc, &stats_after_partial_free);
  REPORT_STATS("-complete-free", &stats_at_end, &stats_after_realloc);
  REPORT_STATS("-full-benchmark", &stats_at_end, &stats_start);
}

#ifdef __GLIBC__
extern char *program_invocation_short_name;
static inline const char* getprogname(void) {
  return program_invocation_short_name;
}
#endif

static void usage(int exitcode) {
  fprintf(stderr, "%s [-v/-q] [--elements N] [--seed N]\n", getprogname());
  exit(exitcode);
}

static const struct option options[] = {
	{ "elements", required_argument, NULL, 'e' },
	{ "help", no_argument, NULL, 'h' },
	{ "quiet", no_argument, NULL, 'q' },
	{ "seed", required_argument, NULL, 's' },
	{ "verbose", no_argument, NULL, 'v' },
	{ NULL, 0, NULL, 0 }
};

int main(int argc, char **argv) {
  /* Start option string with + to avoid parsing after first non-option */
  size_t num_elements = MAX_ALLOCATIONS;
  long seed = 0xc;
  int opt;
  while ((opt = getopt_long(argc, argv, "+e:s:vq", options, NULL)) != -1) {
    switch (opt) {
    case 'q':
      verbosity = 0;
      break;
    case 'v':
      verbosity++;
      break;
    case 's':
      seed = atol(optarg);
      break;
    case 'e':
      num_elements = atol(optarg);
      if (num_elements < 1 || num_elements > MAX_ALLOCATIONS) {
        fprintf(stderr, "Cannot test more than %d elements!\n", MAX_ALLOCATIONS);
        usage(1);
      }
      break;
    case 'h':
      usage(0);
      break;
    default:
      usage(1);
    }
  }
  argc -= optind;
  argv += optind;
  srandom(seed);
  printf("Running benchmark with %zd allocations...\n", num_elements);
  run_benchmark(num_elements);
  printf("Done running benchmark\n");
  return (0);
}
