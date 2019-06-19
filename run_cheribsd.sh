#!/bin/sh

if [ -z "${ARCHNAME}" ]; then
    output_file_base=${OUTPUT_FILE:-/tmp/stats}
    archname=unknown
else
    output_file_base=${OUTPUT_FILE:-/tmp/stats-${ARCHNAME}}
    archname=${ARCHNAME}
fi

if [ $# -eq 0 ]; then
    # TODO: prepare-benchmark-environment.sh
    service syslogd stop
    service devd stop
    ps aux
    # CHECK nothing bad is running:
    sleep 5

    # Export RTLD debug flags to check that we compiled RTLD without debug support:
    export LD_CHERI_DEBUG=1
    export LD_CHERI_DEBUG_VERBOSE=1
    export LD_DEBUG=1
    export LD_DEBUG_VERBOSE=1

    # First with lazy binding
    unset LD_BIND_NOW
    unset LD_CHERI_BIND_NOW
    echo "Running benchmark with lazy binding"
    sh -x "$0" lazy

    # Now without lazy binding
    export LD_CHERI_BIND_NOW=1
    export LD_BIND_NOW=1
    echo "Running benchmark with BIND_NOW"
    sh -x "$0" bind-now
    exit 0
else
    echo "Running with suffix: $1"
    archname="${archname}-$1"
    output_file_base="${output_file_base}-$1"
fi


output_file="${output_file_base}.csv"

do_test() {
     # echo "Testing" "$@"
     for i in 1 2 3 4 6 7 8 9 10; do
         beri_count_stats -a "$archname" -f csv -o "$output_file" "$@" 2>/dev/null
     done
}

for i in test_qsort*; do
  echo $i
  if [ -x "$i" ]; then
    echo "Running $i: `date`"
    do_test ./$i 1000
    echo "Completed $i: `date`"
    date
  fi
done
