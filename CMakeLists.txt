cmake_minimum_required(VERSION 3.14)
project(simple-cheri-benchmarks)
set(MACOSX_RPATH ON)
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)  # Use $ORIGIN when possible

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
SET(CMAKE_INSTALL_RPATH "$ORIGIN")

if (APPLE)
    message(STATUS "APPLE TARGET")
endif()
if (CMAKE_CROSSCOMPILING)
    message(STATUS "CMAKE_CROSSCOMPILING")
endif()

if("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "CHERI")
    message(STATUS "BUILDING FOR CHERI")
    set(CHERI 1)
    if("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "256-bit")
      set(BENCHMARK_ARCH "cheri-256")
    elseif("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "128-bit")
      set(BENCHMARK_ARCH "cheri-128")
    elseif("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "64-bit")
      set(BENCHMARK_ARCH "cheri-64")
    else()
      set(BENCHMARK_ARCH "cheri")
    endif()
elseif("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "MIPS")
    message(STATUS "BUILDING FOR CHERI")
    set(MIPS 1)
    set(BENCHMARK_ARCH "mips")
else()
    set(BENCHMARK_ARCH "unknown")
endif()
message(STATUS "BENCHMARK_ARCH is ${BENCHMARK_ARCH}")


include(CheckCCompilerFlag)
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

function(add_objdump _target)
    if (EXISTS "$ENV{CHERI_SDK}/llvm-objdump")
        add_custom_command(TARGET ${_target} POST_BUILD
            COMMAND "$ENV{CHERI_SDK}/llvm-objdump" ARGS -no-leading-addr -d $<TARGET_FILE:${_target}> > "${CMAKE_CURRENT_BINARY_DIR}/${_target}.dump"
            BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/${_target}.dump" VERBATIM
            COMMENT "Creating dump of ${_target}")
    endif()
endfunction()

function(add_qsort_test _suffix) # _flags
    add_library(qsort_${_suffix} SHARED qsort.c compare_long.c)
    target_compile_options(qsort_${_suffix} PUBLIC -Wall ${ARGN})
    target_link_options(qsort_${_suffix} PRIVATE ${ARGN})
    add_executable(test_qsort_${_suffix} test_qsort.c)
    target_compile_options(test_qsort_${_suffix} PRIVATE -pedantic -Wall -Wextra ${ARGN})
    target_link_options(test_qsort_${_suffix} PRIVATE ${ARGN})
    target_link_libraries(test_qsort_${_suffix} PRIVATE qsort_${_suffix})
    add_objdump(test_qsort_${_suffix})
    if(MIPS OR CHERI)
        target_link_libraries(test_qsort_${_suffix} PRIVATE statcounters)
    endif()
    install(TARGETS test_qsort_${_suffix} qsort_${_suffix} RUNTIME DESTINATION bin LIBRARY DESTINATION bin)
endfunction()

add_qsort_test(default)

set(ALL_SUPPORTED_FLAGS)

macro(check_security_flag _flag _varname)
    check_c_compiler_flag(${_flag} ${_varname})
    if(${_varname})
        list(APPEND ALL_SUPPORTED_FLAGS ${_flag})
    endif()
endmacro()

macro(add_test_with_flag _suffix _varname _flag)
    check_security_flag(${_flag} ${_varname})
    if(${_varname})
        add_qsort_test(${_suffix} ${_flag} ${ARGN})
    endif()
endmacro()


# Do not bother compiling with stack-protector for CHERI (the compiler rejects
# -fstack-protector-all but allows -fstack-protector-strong
if (NOT CHERI)
    add_test_with_flag(spa STACK_PROTECTOR_ALL -fstack-protector-all)

    # https://developers.redhat.com/blog/2018/03/21/compiler-and-linker-flags-gcc/
    # -fstack-protector-strong completely supersedes the earlier stack protector
    # options. It only instruments functions that have addressable local variables
    # or use alloca. Other functions cannot be subject to direct stack buffer
    # overflows and are not instrumented. This greatly reduces the performance and
    # code size impact of the stack protector.
    add_test_with_flag(sps STACK_PROTECTOR_STRONG -fstack-protector-strong)
endif()

# Skip CET and retpoline when targeting MIPS or CHERI:
if (NOT MIPS AND NOT CHERI)
    # Intel CET: decodes to nops on older (i.e. currently all) CPUs
    # Similar to ARM BTI
    add_test_with_flag(fcf CF_PROTECTION -fcf-protection)

    check_security_flag(-fcf-protection=return FCF_RETURN)
    check_security_flag(-fcf-protection=full FCF_FULL)
    check_security_flag(-mshstk SHSTK)
    check_security_flag(-mcet CET)
    if(CET AND CF_PROTECTION)
        add_qsort_test(fcf_mcet -mcet -fcf-protection)
    endif()
    if(SHSTK AND FCF_RETURN)
        add_qsort_test(fcf_return -mshstk -fcf-protection=return)
    endif()
    if(SHSTK AND FCF_FULL)
        add_qsort_test(fcf_full -mshstk -fcf-protection=full)
    endif()

    add_test_with_flag(retpoline RETPOLINE -mretpoline)
    add_test_with_flag(retpoline_gcc_ib RETPOLINE_GCC_IB -mindirect-branch=thunk)
    add_test_with_flag(retpoline_gcc_fr RETPOLINE_GCC_FR -mfunction-return=thunk)
    if (RETPOLINE_GCC_IB AND RETPOLINE_GCC_FR )
        add_qsort_test(retpoline_gcc -mindirect-branch=thunk -mfunction-return=thunk)
    endif()
endif()

# LLVM safe-stack (split spills and address-taken variables)
add_test_with_flag(safestack SAFESTACK -fsanitize=safe-stack)

message(STATUS "All supported flags: ${ALL_SUPPORTED_FLAGS}")
if (-fcf-protection IN_LIST ALL_SUPPORTED_FLAGS AND -mindirect-branch=thunk IN_LIST ALL_SUPPORTED_FLAGS)
    # These flags are incompatible for GCC
    set(FCF_FLAGS ${ALL_SUPPORTED_FLAGS})
    set(RETPOLINE_FLAGS ${ALL_SUPPORTED_FLAGS})
    list(FILTER RETPOLINE_FLAGS EXCLUDE REGEX "-fcf-protection.*")
    list(FILTER FCF_FLAGS EXCLUDE REGEX "-mindirect-branch=.*")
    list(FILTER FCF_FLAGS EXCLUDE REGEX "-mfunction-return=.*")
    add_qsort_test(all_retpoline ${RETPOLINE_FLAGS})
    add_qsort_test(all_fcf ${FCF_FLAGS})
else()
    if(ALL_SUPPORTED_FLAGS)
        add_qsort_test(all ${ALL_SUPPORTED_FLAGS})
    else()
        message(STATUS "Extra mitigation flags empty -> not builing another binary")
    endif()
endif()

if (FALSE)
    # TODO: also include asan benchmarks
    add_test_with_flag(asan ASAN -fsanitize=address)
    add_test_with_flag(ubsan UBSAN -fsanitize=undefined)
endif()

# Try building static:
set(CMAKE_TRY_COMPILE_TARGET_TYPE "EXECUTABLE")
set(CMAKE_REQUIRED_FLAGS "-static")
set(CMAKE_REQUIRED_LINK_OPTIONS "-static")
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("int main() { return 0; }" CAN_LINK_STATIC)
message(STATUS "Can link static: ${CAN_LINK_STATIC}")

#if (CAN_LINK_STATIC)
#    add_library(qsort_static STATIC qsort.c compare_long.c)
#    target_compile_options(qsort_static PUBLIC -Wall -static)
#    add_executable(test_qsort_static test_qsort.c)
#    target_compile_options(test_qsort_static PRIVATE -pedantic -Wall -Wextra -static)
#    target_link_options(test_qsort_static PRIVATE -static)
#    target_link_libraries(test_qsort_static PRIVATE qsort_static)
#    if(MIPS OR CHERI)
#        target_link_libraries(test_qsort_static PRIVATE statcounters)
#    endif()
#    add_objdump(test_qsort_static)
#    install(TARGETS test_qsort_static qsort_static RUNTIME DESTINATION bin LIBRARY DESTINATION bin)
#endif()

if (CAN_LINK_STATIC)
    add_library(qsort_static STATIC qsort.c compare_long.c)
    target_compile_options(qsort_static PUBLIC -Wall -static -O3)
    add_executable(test_qsort_static test_qsort.c)
    target_compile_options(test_qsort_static PRIVATE -pedantic -Wall -Wextra -static -O3)
    target_link_options(test_qsort_static PRIVATE -static)
    target_link_libraries(test_qsort_static PRIVATE qsort_static)
    if(MIPS OR CHERI)
        target_link_libraries(test_qsort_static PRIVATE statcounters)
    endif()
    add_objdump(test_qsort_static)
    install(TARGETS test_qsort_static qsort_static RUNTIME DESTINATION bin LIBRARY DESTINATION bin)
endif()


function(add_simple_benchmark _target) # _srcs
    add_executable(${_target} ${ARGN})
    target_compile_options(${_target} PRIVATE -pedantic -Wall -Wextra)
    add_objdump(${_target})
    if(MIPS OR CHERI)
        target_link_libraries(${_target} PRIVATE statcounters)
    endif()
    install(TARGETS ${_target} RUNTIME DESTINATION bin LIBRARY DESTINATION bin)
endfunction()

## Simple malloc benchmark:
add_simple_benchmark(malloc_bench_shared malloc_bench.c)
if (CAN_LINK_STATIC)
    add_simple_benchmark(malloc_bench_static malloc_bench.c)
    target_compile_options(malloc_bench_static PRIVATE -static)
    target_link_options(malloc_bench_static PRIVATE -static)
endif()

configure_file(malloc_benchmark.sh.in ${CMAKE_CURRENT_BINARY_DIR}/malloc_benchmark.sh @ONLY)
configure_file(run_cheribsd.sh.in ${CMAKE_CURRENT_BINARY_DIR}/run_cheribsd.sh @ONLY)
configure_file(run_jenkins-bluehive.sh ${CMAKE_CURRENT_BINARY_DIR}/run_jenkins-bluehive.sh @ONLY)
install(PROGRAMS
    ${CMAKE_CURRENT_BINARY_DIR}/malloc_benchmark.sh
    ${CMAKE_CURRENT_BINARY_DIR}/run_cheribsd.sh
    ${CMAKE_CURRENT_BINARY_DIR}/run_jenkins-bluehive.sh
    run_linux_perf.sh
    DESTINATION bin)

# TO run and get cycle counds on linux: perf stat -d -d -d --sync --repeat 20
# CSV-ish output: --field-separator=,

# for i in test_qsort*; do echo $i; perf stat -d -d -d --sync --field-separator=, --repeat 15 -o results_$i.csv ./$i 8000; done


#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()
