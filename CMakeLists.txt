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

#add_library(qsort SHARED qsort.c)
#add_executable(test_qsort test_qsort.c)
#target_link_libraries(test_qsort PRIVATE qsort)
#add_custom_command(TARGET test_qsort PRE_LINK
#    COMMAND "$ENV{CHERI_SDK}/llvm-objdump" ARGS -d test_qsort > "${CMAKE_CURRENT_BINARY_DIR}/test_qsort.dump"
#    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/test_qsort.dump" VERBATIM
#    COMMENT "Creating dump of test_qsort")

function(add_qsort_test _suffix) # _flags
    add_library(qsort_${_suffix} SHARED qsort.c)
    target_compile_options(qsort_${_suffix} PUBLIC -Wall ${ARGN})
    target_link_options(qsort_${_suffix} PRIVATE ${ARGN})
    add_executable(test_qsort_${_suffix} test_qsort.c)
    target_compile_options(test_qsort_${_suffix} PRIVATE -pedantic -Wall -Wextra ${ARGN})
    target_link_options(test_qsort_${_suffix} PRIVATE ${ARGN})
    target_link_libraries(test_qsort_${_suffix} PRIVATE qsort_${_suffix})
    #if (EXISTS "$ENV{CHERI_SDK}/llvm-objdump")
        add_custom_command(TARGET test_qsort_${_suffix} POST_BUILD
            COMMAND "$ENV{CHERI_SDK}/llvm-objdump" ARGS -no-leading-addr -d test_qsort_${_suffix} > "${CMAKE_CURRENT_BINARY_DIR}/test_qsort_${_suffix}.dump"
            BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/test_qsort_${_suffix}.dump" VERBATIM
            COMMENT "Creating dump of test_qsort_${_suffix}")
    #endif()
    install(TARGETS test_qsort_${_suffix} qsort_${_suffix} RUNTIME DESTINATION bin LIBRARY DESTINATION bin)
endfunction()

add_qsort_test(default)

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
    add_qsort_test(fcf_mcet -mcet -fcf-protection)
endif()

check_c_compiler_flag(-mshstk SHSTK)
if(SHSTK AND CF_PROTECTION)
    add_qsort_test(fcf_return -mshstk -fcf-protection=return)
endif()

check_c_compiler_flag(-fcf-protection=full FCF_FULL)
if(FCF_FULL)
    add_qsort_test(fcf_full -mshstk -fcf-protection=full)
endif()

check_c_compiler_flag(-mretpoline RETPOLINE)
if(RETPOLINE)
    add_qsort_test(retpoline -mretpoline)
endif()

if (FALSE)
    # TODO: get asan benchmarks
    check_c_compiler_flag(-fsanitize=address ASAN)
    if(ASAN)
        add_qsort_test(asan -fsanitize=address)
    endif()
    check_c_compiler_flag(-fsanitize=undefined UBSAN)
    if(UBSAN)
        add_qsort_test(ubsan -fsanitize=undefined)
    endif()
endif()
