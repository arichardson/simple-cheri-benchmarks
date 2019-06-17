cmake_minimum_required(VERSION 3.14)
project(simple-cheri-benchmarks)
set(MACOSX_RPATH ON)
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)  # Use $ORIGIN when possible

include(CheckCCompilerFlag)
check_c_compiler_flag(-fstack-protector-all STACK_PROTECTOR_ALL)

# Add -pie to binaries
include(CheckPIESupported)
check_pie_supported()

# Default to -O2
add_compile_options(-O2)

add_library(qsort SHARED qsort.c)
add_executable(test_qsort test_qsort.c)
target_link_libraries(test_qsort PRIVATE qsort)


function(add_qsort_test _suffix) # _flags
    add_library(qsort_${_suffix} SHARED qsort.c)
    target_compile_options(qsort_${_suffix} PUBLIC ${ARGN})
    add_executable(test_qsort_${_suffix} test_qsort.c)
    target_compile_options(test_qsort_${_suffix} PRIVATE -pedantic -Wall -Wextra ${ARGN})
    target_link_libraries(test_qsort_${_suffix} PRIVATE qsort_${_suffix})
endfunction()

if(STACK_PROTECTOR_ALL)
    add_qsort_test(spa -fstack-protector-all)
endif()


# https://developers.redhat.com/blog/2018/03/21/compiler-and-linker-flags-gcc/
# -fstack-protector-strong completely supersedes the earlier stack protector
# options. It only instruments functions that have addressable local variables
# or use alloca. Other functions cannot be subject to direct stack buffer
# overflows and are not instrumented. This greatly reduces the performance and
# code size impact of the stack protector.
check_c_compiler_flag(-fstack-protector-strong STACK_PROTECTOR_STRONG)
if(STACK_PROTECTOR_STRONG)
    add_qsort_test(sps -fstack-protector-strong)
endif()


# Intel CET: decodes to nops on older (i.e. currently all) CPUs
# Similar to ARM BTI
check_c_compiler_flag(-fcf-protection CF_PROTECTION)
if(CF_PROTECTION)
    add_qsort_test(fcf -fcf-protection)
endif()

check_c_compiler_flag(-mcet CET)
if(CET AND CF_PROTECTION)
    add_qsort_test(fcf -mcet -fcf-protection)
endif()
