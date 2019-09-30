#include <sys/cdefs.h>
#include <sys/param.h>
#include <stdint.h>

#if __has_include(<statcounters.h>)
#include <statcounters.h>
#define HAVE_STATCOUNTERS 1
#ifdef __mips__
#if __has_include(<statcounters_mips.h>)
#include <statcounters_mips.h>
static inline uint64_t get_inst_user(void) {
  return statcounters_get_inst_user_count();
}
#else
static inline uint64_t get_inst_user(void) {
  return 0;
}
#endif // __has_include(<statcounters_mips.h>)
#endif //  __mips__
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
static inline uint64_t get_inst_user(void) {
  return 0;
}
#endif

#ifndef nitems
#define nitems(array) (sizeof(array) / sizeof(array[0]))
#endif
