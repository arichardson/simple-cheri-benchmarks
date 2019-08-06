#! /bin/sh
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright 2017 Alexandre Joannou
# Copyright 2019 Alex Richardson
#
# This software was developed by SRI International and the University of
# Cambridge Computer Laboratory (Department of Computer Science and
# Technology) under DARPA contract HR0011-18-C-0016 ("ECATS"), as part of the
# DARPA SSITH research programme.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
set -xe

####################
# global variables #
####################

OUTFILE=$(pwd)/statcounters.csv
DISCARD_RUN=2
SAMPLE_RUN=10
BENCHMARK_LIST="test_qsort_static test_qsort_default malloc_bench_shared malloc_bench_static"
ARCH="@BENCHMARK_ARCH@"

###################
# options parsing #
###################

usage(){
    printf  "Usage: %s [-h] [-a arch] [-b benchmarks] [-o outfile] [-d discardrun] [-r samplerun]\n" "$(basename $0)"
    printf  "\t%-15s\t%s\n" "-h" "display this help message"
    printf  "\t%-15s\t%s\n" "-a arch" "Use arch as the architecture value in the csv (default: $ARCH)"
    printf  "\t%-15s\t%s\n" "-b benchmarks" "run the following benchmarks (default: $BENCHMARK_LIST)"
    printf  "\t%-15s\t%s\n" "-o outfile" "output statcounters in outfile (pah relative to $(pwd)) (default: $OUTFILE)"
    printf  "\t%-15s\t%s\n" "-d discardrun" "number of runs to discard (default: $DISCARD_RUN)"
    printf  "\t%-15s\t%s\n" "-r samplerun" "number of runs to sample (default: $SAMPLE_RUN)"
    printf  "\t%-15s\t%s\n" "cpuname" "the cpu for which to run, one of {mips,cheri128,cheri256}"
}
while getopts "ha:b:o:d:r:" opt; do
    case $opt in
        h)
            usage
            exit 0
            ;;
        a)
            ARCH="$OPTARG"
            echo "Benchmark architecture set to \"$ARCH\""
            ;;
        b)
            BENCHMARK_LIST="$OPTARG"
            echo "Benchmark list set to \"$BENCHMARK_LIST\""
            ;;
        o)
            OUTFILE=$(pwd)/$OPTARG
            echo "statcounters output file set to $OUTFILE"
            ;;
        d)
            if [ $(echo "$OPTARG" | grep -E "^[0-9]+$") ]; then
                DISCARD_RUN=$OPTARG
                echo "discarding $DISCARD_RUN run(s)"
            else
                echo "-d argument $OPTARG must be a positive integer"
                exit 1
            fi
            ;;
        r)
            if [ $(echo $OPTARG | grep -E "^[0-9]+$") ]; then
                SAMPLE_RUN=$OPTARG
                echo "sampling $SAMPLE_RUN run(s)"
            else
                echo "-r argument $OPTARG must be a positive integer"
                exit 1
            fi
            ;;
        \?)
            echo "Invalid option: -$OPTARG" >&2
            exit 1
            ;;
    esac
done
shift $(($OPTIND - 1))

# For the mips-asan case:
export LD_LIBRARY_PATH="$(pwd)/lib"
# Should not be needed since we use rpath
# export LD_CHERI_LIBRARY_PATH="$(pwd)/lib"

################
# benchmarking #
################
echo "---> Enabling stat counters csv mode"
export STATCOUNTERS_FORMAT=csv
if [ -n "$ARCH" ]; then
  export STATCOUNTERS_ARCHNAME="$ARCH"
fi
export BENCHMARK_ROOT=$PWD

for bench in $BENCHMARK_LIST; do
    echo "---> Running $bench benchmark $(date)"
    echo "---> Set statcounters archname to $STATCOUNTERS_ARCHNAME"
    echo "---> Set statcounters progname to $STATCOUNTERS_PROGNAME"
    echo "---> Unsetting statcounters output file for discarded runs"
    export STATCOUNTERS_OUTPUT=/dev/null
    export STATCOUNTERS_PROGNAME="$bench"
    i=0
    while [ $i != "$DISCARD_RUN" ]; do
        beri_count_stats -a "$STATCOUNTERS_ARCHNAME" -o /dev/null "$BENCHMARK_ROOT/$bench" || echo "Failed to run $BENCHMARK_ROOT/$bench"
        i=$(($i+1))
    done
    echo "... discarded $DISCARD_RUN run(s)"
    export STATCOUNTERS_OUTPUT=$OUTFILE
    echo "---> Reset statcounters output file to $STATCOUNTERS_OUTPUT for sampled runs"
    i=0
    while [ $i != $SAMPLE_RUN ]; do
        beri_count_stats -a "$STATCOUNTERS_ARCHNAME" -o "$OUTFILE" "$BENCHMARK_ROOT/$bench" || echo "Failed to run $BENCHMARK_ROOT/$bench"
        echo "... $bench benchmark run $i done"
        i=$(($i+1))
    done
    echo "---> Done running $bench benchmark $(date)"
done

echo "===== DONE RUNNING BENCHMARKS ====="
cat "$STATCOUNTERS_OUTPUT"

exit 0
