#include <sys/cdefs.h>
#include <sys/param.h>

#if __has_include(<statcounters.h>)
#include <statcounters.h>
#define HAVE_STATCOUNTERS 1
#ifdef __mips__
#include <statcounters_mips.h>
#endif
#define COLLECT_STATS(name) statcounters_bank_t name; statcounters_sample(&name)
static inline void REPORT_STATS(const char* phase, statcounters_bank_t* end, statcounters_bank_t* start) {
  statcounters_bank_t diff;
  statcounters_diff(&diff, end, start);
  statcounters_dump_with_phase(&diff, phase);
}

#else
#define HAVE_STATCOUNTERS 0
#define COLLECT_STATS(name)
#define REPORT_STATS(phase, end, start)
#endif

#ifndef nitems
#define nitems(array) (sizeof(array) / sizeof(array[0]))
#endif

static inline uint64_t get_inst_user(void) {
#ifdef __mips__
  return statcounters_get_inst_user_count();
#else
  return 0;
#endif
}
