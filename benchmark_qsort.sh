#!/bin/sh

run_benchmark() {
  #env | grep STATCOU
  beri_count_stats -a "$STATCOUNTERS_ARCHNAME" -f csv -p "$STATCOUNTERS_PROGNAME-full" -o "$STATCOUNTERS_OUTPUT" "$@"
}

set -xe
unset LD_BIND_NOT; export LD_CHERI_BIND_NOW=1
unset LD_NOT; export LD_BIND_NOW=1
#echo "Running benchmark with BIND_NOW"
STATCOUNTERS_ARCHNAME="$STATCOUNTERS_ARCHNAME-bind-now" run_benchmark ./test_qsort_default

unset LD_CHERI_BIND_NOW; unset LD_CHERI_BIND_NOT
unset LD_BIND_NOW; unset LD_BIND_NOT
#echo "Running benchmark with lazy binding"
STATCOUNTERS_ARCHNAME="$STATCOUNTERS_ARCHNAME-bind-lazy" run_benchmark ./test_qsort_default

# This one just takes an absurdly long time and is probably not useful
#unset LD_CHERI_BIND_NOW; export LD_CHERI_BIND_NOT=1
#unset LD_BIND_NOW; export LD_BIND_NOT=1
#echo "Running benchmark with BIND_NOT"
#STATCOUNTERS_ARCHNAME="$STATCOUNTERS_ARCHNAME-bind-not" run_benchmark ./test_qsort_default

#echo "Running static benchmark"
STATCOUNTERS_ARCHNAME="$STATCOUNTERS_ARCHNAME-static" run_benchmark ./test_qsort_static
