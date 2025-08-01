# Copyright Contributors to the OpenImageIO project.
# SPDX-License-Identifier: Apache-2.0
# https://github.com/AcademySoftwareFoundation/OpenImageIO

###########################################################################
#
# This file contains compiler-related detection, options, and actions.
#
# Each option declaration is kept close to the related logic for that
# option.
#
###########################################################################


###########################################################################
# Print some basic status about the system and compiler
#
message (VERBOSE "CMAKE_SYSTEM_NAME      = ${CMAKE_SYSTEM_NAME}")
message (VERBOSE "CMAKE_SYSTEM_VERSION   = ${CMAKE_SYSTEM_VERSION}")
message (VERBOSE "CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}")
message (STATUS  "CMAKE_CXX_COMPILER     = ${CMAKE_CXX_COMPILER}")
message (STATUS  "CMAKE_CXX_COMPILER_ID  = ${CMAKE_CXX_COMPILER_ID}")
message (VERBOSE "CMAKE_CXX_COMPILE_FEATURES = ${CMAKE_CXX_COMPILE_FEATURES}")


###########################################################################
# C++ language standard
#
set (CMAKE_CXX_MINIMUM 17)
set (CMAKE_CXX_STANDARD 17 CACHE STRING
     "C++ standard to build with (17, 20, etc.) Minimum is ${CMAKE_CXX_MINIMUM}.")
set (DOWNSTREAM_CXX_STANDARD 17 CACHE STRING
     "C++ minimum standard to impose on downstream clients")
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)
message (STATUS "Building with C++${CMAKE_CXX_STANDARD}, downstream minimum C++${DOWNSTREAM_CXX_STANDARD}")
if (CMAKE_CXX_STANDARD VERSION_LESS CMAKE_CXX_MINIMUM)
    message (FATAL_ERROR "C++${CMAKE_CXX_STANDARD} is not supported, minimum is C++${CMAKE_CXX_MINIMUM}")
endif ()
# Remember the -std flags we need will be used later for custom Cuda builds
set (CSTD_FLAGS "")
if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG OR CMAKE_COMPILER_IS_INTEL)
    set (CSTD_FLAGS "-std=c++${CMAKE_CXX_STANDARD}")
endif ()


###########################################################################
# Figure out which compiler we're using
#

if (CMAKE_COMPILER_IS_GNUCC)
    execute_process (COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
                     OUTPUT_VARIABLE GCC_VERSION
                     OUTPUT_STRIP_TRAILING_WHITESPACE)
    message (VERBOSE "Using gcc ${GCC_VERSION} as the compiler")
else ()
    set (GCC_VERSION 0)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER MATCHES "[Cc]lang"
    OR CMAKE_CXX_COMPILER_ID MATCHES "IntelLLVM")
    # If using any flavor of clang, set CMAKE_COMPILER_IS_CLANG. If it's
    # Apple's variety, set CMAKE_COMPILER_IS_APPLECLANG and
    # APPLECLANG_VERSION_STRING, otherwise for generic clang set
    # CLANG_VERSION_STRING.
    set (CMAKE_COMPILER_IS_CLANG 1)
    EXECUTE_PROCESS( COMMAND ${CMAKE_CXX_COMPILER} --version OUTPUT_VARIABLE clang_full_version_string )
    if (clang_full_version_string MATCHES "Apple")
        set (CMAKE_CXX_COMPILER_ID "AppleClang")
        set (CMAKE_COMPILER_IS_APPLECLANG 1)
        string (REGEX REPLACE ".* version ([0-9]+\\.[0-9]+).*" "\\1" APPLECLANG_VERSION_STRING ${clang_full_version_string})
        set (ANY_CLANG_VERSION_STRING ${APPLECLANG_VERSION_STRING})
        message (VERBOSE "The compiler is Clang: ${CMAKE_CXX_COMPILER_ID} version ${APPLECLANG_VERSION_STRING}")
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "IntelLLVM")
        set (CMAKE_COMPILER_IS_INTELCLANG 1)
        string (REGEX MATCH "[0-9]+(\\.[0-9]+)+" INTELCLANG_VERSION_STRING ${clang_full_version_string})
        set (ANY_CLANG_VERSION_STRING ${INTELCLANG_VERSION_STRING})
        message (VERBOSE "The compiler is Intel Clang: ${CMAKE_CXX_COMPILER_ID} version ${INTELCLANG_VERSION_STRING}")
    else ()
        set (CMAKE_COMPILER_IS_GENERICCLANG 1)
        string (REGEX REPLACE ".* version ([0-9]+\\.[0-9]+).*" "\\1" CLANG_VERSION_STRING ${clang_full_version_string})
        set (ANY_CLANG_VERSION_STRING ${CLANG_VERSION_STRING})
        message (VERBOSE "The compiler is Clang: ${CMAKE_CXX_COMPILER_ID} version ${CLANG_VERSION_STRING}")
    endif ()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    set (CMAKE_COMPILER_IS_INTEL 1)
    message (VERBOSE "Using Intel as the compiler")
endif ()
if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
    set (COMPILER_IS_GCC_OR_ANY_CLANG TRUE)
endif ()


###########################################################################
# Turn on more detailed warnings and optionally consider warnings as errors
#
if (NOT ${PROJECT_NAME}_SUPPORTED_RELEASE OR DEFINED ENV{${PROJECT_NAME}_CI})
    # For development branches (i.e., not a supported release), or when
    # running CI, default to treating all warnings as errors.
    option (STOP_ON_WARNING "Stop building if there are any compiler warnings" ON)
else ()
    # For release branches not doing a CI build default to just printing
    # warnings but not letting them stop the build.
    option (STOP_ON_WARNING "Stop building if there are any compiler warnings" OFF)
endif()
option (EXTRA_WARNINGS "Enable lots of extra pedantic warnings" OFF)
if (NOT MSVC)
    add_compile_options (-Wall -Wformat -Wformat=2)
    if (EXTRA_WARNINGS)
        add_compile_options (-Wextra)
    endif ()
    if (STOP_ON_WARNING)
        add_compile_options (-Werror)
    endif ()
endif ()


###########################################################################
# Control symbol visibility
#
# We try hard to make default symbol visibility be "hidden", except for
# symbols that are part of the public API, which should be marked in the
# source code with a special decorator, OIIO_API.
#
# Additionally, there is a hidesymbols.map file that on some platforms may
# give more fine-grained control for hiding symbols, because sometimes
# dependent libraries may not be well behaved and need extra hiding.
#
set (CMAKE_CXX_VISIBILITY_PRESET "hidden" CACHE STRING "Symbol visibility (hidden or default")
option (VISIBILITY_INLINES_HIDDEN "Hide symbol visibility of inline functions" ON)
set (VISIBILITY_MAP_FILE "${PROJECT_SOURCE_DIR}/src/build-scripts/hidesymbols.map" CACHE FILEPATH "Visibility map file")
set (CMAKE_C_VISIBILITY_PRESET ${CMAKE_CXX_VISIBILITY_PRESET})
if (${CMAKE_CXX_VISIBILITY_PRESET} STREQUAL "hidden" AND VISIBILITY_MAP_FILE AND
    (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG) AND
    (CMAKE_SYSTEM_NAME MATCHES "Linux|kFreeBSD" OR CMAKE_SYSTEM_NAME STREQUAL "GNU"))
    # Linux/FreeBSD/Hurd: also hide all the symbols of dependent libraries
    # to prevent clashes if an app using this project is linked against
    # other versions of our dependencies.
    set (VISIBILITY_MAP_COMMAND "-Wl,--version-script=${VISIBILITY_MAP_FILE}")
endif ()


###########################################################################
# Compiler-specific and platform-specific options.
#
# Here is where we add a whole bunch of options for specific compilers or
# platforms. Usually this is to suppress false-positive compiler warnings.
#
if (CMAKE_COMPILER_IS_CLANG OR CMAKE_COMPILER_IS_APPLECLANG)
    # Clang-specific options
    add_compile_options ("-Wno-unused-function")
    add_compile_options ("-Wno-overloaded-virtual")
    add_compile_options ("-Wno-unneeded-internal-declaration")
    add_compile_options ("-Wno-unused-private-field")
    add_compile_options ("-Wno-tautological-compare")
    # disable warning about unused command line arguments
    add_compile_options ("-Qunused-arguments")
    # Don't warn if we ask it not to warn about warnings it doesn't know
    add_compile_options ("-Wunknown-warning-option")
    if (CLANG_VERSION_STRING VERSION_GREATER_EQUAL 3.6 OR
        APPLECLANG_VERSION_STRING VERSION_GREATER 6.1)
        add_compile_options ("-Wno-unused-local-typedefs")
    endif ()
    if (CLANG_VERSION_STRING VERSION_GREATER_EQUAL 3.9)
        # Don't warn about using unknown preprocessor symbols in `#if`
        add_compile_options ("-Wno-expansion-to-defined")
    endif ()
    if (CMAKE_GENERATOR MATCHES "Xcode")
        add_compile_options ("-Wno-shorten-64-to-32")
    endif ()
endif ()

if (CMAKE_COMPILER_IS_GNUCC AND NOT (CMAKE_COMPILER_IS_CLANG OR CMAKE_COMPILER_IS_APPLECLANG))
    # gcc specific options
    add_compile_options ("-Wno-unused-local-typedefs")
    add_compile_options ("-Wno-unused-result")
    if (NOT ${GCC_VERSION} VERSION_LESS 7.0)
        add_compile_options ("-Wno-aligned-new")
        add_compile_options ("-Wno-noexcept-type")
    endif ()
endif ()

if (INTELCLANG_VERSION_STRING VERSION_GREATER_EQUAL 2022.1.0)
    # New versions of icx warn about changing certain floating point options
    add_compile_options ("-Wno-overriding-t-option")
endif ()

if (MSVC)
    # Microsoft specific options
    add_compile_options (/W1)
    add_compile_options (/MP)
    add_compile_definitions (_CRT_SECURE_NO_DEPRECATE)
    add_compile_definitions (_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions (_CRT_NONSTDC_NO_WARNINGS)
    add_compile_definitions (_SCL_SECURE_NO_WARNINGS)
    add_compile_definitions (JAS_WIN_MSVC_BUILD)
endif (MSVC)

if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD"
    AND ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "i386")
    # For FreeBSD, minimum arch of i586 is needed for atomic cpu instructions
    add_compile_options (-march=i586)
endif ()

# Fast-math mode may go faster, but it breaks IEEE and also makes inconsistent
# results on different compilers/platforms, so we don't use it by default.
option (ENABLE_FAST_MATH "Use fast math (may break IEEE fp rules)" OFF)
if (ENABLE_FAST_MATH)
    if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
        add_compile_options ("-ffast-math")
    elseif (MSVC)
        add_compile_options ("/fp:fast")
    endif ()
else ()
    if (CMAKE_COMPILER_IS_INTELCLANG)
        # Intel icx is fast-math by default, so if we don't want that, we need
        # to explicitly disable it.
        add_compile_options ("-fno-fast-math")
    endif ()
endif ()

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
    # this allows native instructions to be used for sqrtf instead of a function call
    add_compile_options ("-fno-math-errno")
endif ()


###########################################################################
# Use ccache if found
#
# This can really speed up compilation by caching object files that have
# been compiled previously with identical arguments and inputs. Putting this
# logic here makes it work even if the user is unaware of ccache. If it's
# not found on the system, it will simply be silently not used.
option (USE_CCACHE "Use ccache if found" ON)
find_program (CCACHE_EXE ccache)
if (CCACHE_EXE AND USE_CCACHE)
    if (CMAKE_COMPILER_IS_CLANG AND USE_QT AND (NOT DEFINED ENV{CCACHE_CPP2}))
        message (STATUS "Ignoring ccache because clang + Qt + env CCACHE_CPP2 is not set")
    else ()
        if (NOT ${CXX_COMPILER_LAUNCHER} MATCHES "ccache")
            set (CXX_COMPILER_LAUNCHER ${CCACHE_EXE} ${CXX_COMPILER_LAUNCHER})
        endif ()
        if (NOT ${C_COMPILER_LAUNCHER} MATCHES "ccache")
            set (C_COMPILER_LAUNCHER ${CCACHE_EXE} ${C_COMPILER_LAUNCHER})
        endif ()
        message (STATUS "ccache enabled: ${CCACHE_EXE}")
    endif ()
endif ()


###########################################################################
# Build time debugging aid: time all compile & link commands
#
# Note, though, that this is not especially helpful when doing a parallel
# build. If you wish to time individual compile commands, it's best to also
# set `-j 1` or CMAKE_BUILD_PARALLEL_LEVEL to 1.
option (TIME_COMMANDS "Time each compile and link command" OFF)
if (TIME_COMMANDS)
    set (CXX_COMPILER_LAUNCHER ${CMAKE_COMMAND} -E time ${CXX_COMPILER_LAUNCHER})
    set (C_COMPILER_LAUNCHER ${CMAKE_COMMAND} -E time ${C_COMPILER_LAUNCHER})
endif ()


###########################################################################
# Option to force use of libc++ (the LLVM project's alternate C++ standard
# library). Currently this only has an effect if using clang as the
# compiler. Maybe it would also work for g++? Investigate.
option (USE_LIBCPLUSPLUS "Compile with clang libc++" OFF)
if (USE_LIBCPLUSPLUS AND CMAKE_COMPILER_IS_CLANG)
    message (STATUS "Using libc++")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif ()


###########################################################################
# For gcc >= 5, allow an option to force which version of the C++ ABI to
# use (mostly this affects the implementation of std::string).
#
# FIXME: In theory, this should also be needed for clang, if compiling with
# the gcc libstdc++ toolchain. In practice, I could not get things to build
# with clang properly when using this option, and I haven't yet seen a case
# where it's needed. We can return to this and fix for clang if it becomes a
# legit problem later.
#
set (GLIBCXX_USE_CXX11_ABI "" CACHE STRING "For gcc, use the new C++11 library ABI (0|1)")
if (CMAKE_COMPILER_IS_GNUCC AND ${GCC_VERSION} VERSION_GREATER_EQUAL 5.0)
    if (NOT ${GLIBCXX_USE_CXX11_ABI} STREQUAL "")
        add_compile_definitions (_GLIBCXX_USE_CXX11_ABI=${GLIBCXX_USE_CXX11_ABI})
    endif ()
endif ()


###########################################################################
# SIMD and machine architecture options.
#
# The USE_SIMD option may be set to a comma-separated list of machine /
# instruction set options, such as "avx3,f16c". The list will be parsed and
# the proper compiler directives added to generate code for those ISA
# capabilities.
#
set_cache (USE_SIMD "" "Use SIMD directives (0, sse2, sse3, ssse3, sse4.1, sse4.2, avx, avx2, avx512f, f16c, aes)")
set (SIMD_COMPILE_FLAGS "")
message (STATUS "Compiling with SIMD level ${USE_SIMD}")
if (NOT USE_SIMD STREQUAL "")
    if (USE_SIMD STREQUAL "0")
        set (SIMD_COMPILE_FLAGS ${SIMD_COMPILE_FLAGS} "-DOIIO_NO_SIMD=1")
    else ()
        set(_highest_msvc_arch 0)
        string (REPLACE "," ";" SIMD_FEATURE_LIST "${USE_SIMD}")
        foreach (feature ${SIMD_FEATURE_LIST})
            message (VERBOSE "SIMD feature: ${feature}")
            if (MSVC OR CMAKE_COMPILER_IS_INTEL)
                if (feature STREQUAL "sse2")
                    list (APPEND SIMD_COMPILE_FLAGS "/D__SSE2__")
                endif ()
                if (feature STREQUAL "sse4.1")
                    list (APPEND SIMD_COMPILE_FLAGS "/D__SSE2__" "/D__SSE4_1__")
                endif ()
                if (feature STREQUAL "sse4.2")
                    list (APPEND SIMD_COMPILE_FLAGS "/D__SSE2__" "/D__SSE4_2__")
                endif ()
                if (feature STREQUAL "avx" AND _highest_msvc_arch LESS 1)
                    set(_highest_msvc_arch 1)
                endif ()
                if (feature STREQUAL "avx2" AND _highest_msvc_arch LESS 2)
                    set(_highest_msvc_arch 2)
                endif ()
                if (feature STREQUAL "avx512f" AND _highest_msvc_arch LESS 3)
                    set(_highest_msvc_arch 3)
                endif ()
            else ()
                list (APPEND SIMD_COMPILE_FLAGS "-m${feature}")
            endif ()
            if (feature STREQUAL "fma" AND (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG))
                # If fma is requested, for numerical accuracy sake, turn it
                # off by default except when we explicitly use madd. At some
                # future time, we should look at this again carefully and
                # see if we want to use it more widely by ffp-contract=fast.
                add_compile_options ("-ffp-contract=off")
            endif ()
        endforeach()

        # Only add a single /arch flag representing the highest level of support.
        if (MSVC OR CMAKE_COMPILER_IS_INTEL)
            if (_highest_msvc_arch EQUAL 1)
                list (APPEND SIMD_COMPILE_FLAGS "/arch:AVX")
            endif ()
            if (_highest_msvc_arch EQUAL 2)
                list (APPEND SIMD_COMPILE_FLAGS "/arch:AVX2")
            endif ()
            if (_highest_msvc_arch EQUAL 3)
                list (APPEND SIMD_COMPILE_FLAGS "/arch:AVX512")
            endif ()
        endif ()
        unset(_highest_msvc_arch)
    endif ()
    add_compile_options (${SIMD_COMPILE_FLAGS})
endif ()


###########################################################################
# Preparation to test for compiler/language features
if (NOT VERBOSE)
    set (CMAKE_REQUIRED_QUIET 1)
endif ()
include (CMakePushCheckState)
include (CheckCXXSourceCompiles)
include (CheckCXXSourceRuns)
include (CheckLibraryExists)


###########################################################################
# Check if we need libatomic on this platform.  We shouldn't on mainstream
# x86/x86_64, but might on some other platforms.
#
if (NOT MSVC AND NOT APPLE AND NOT ANDROID)
    cmake_push_check_state ()
    check_cxx_source_compiles(
       "#include <atomic>
        #include <cstdint>
        std::atomic<uint64_t> x {0};
        int main() {
            uint64_t i = x.load(std::memory_order_relaxed);
            return 0;
        }"
        COMPILER_SUPPORTS_ATOMIC_WITHOUT_LIBATOMIC)
    cmake_pop_check_state ()
    if (NOT COMPILER_SUPPORTS_ATOMIC_WITHOUT_LIBATOMIC)
        check_library_exists (atomic __atomic_load_8 "" LIBATOMIC_WORKS)
        if (LIBATOMIC_WORKS)
            list (APPEND GCC_ATOMIC_LIBRARIES "-latomic")
            message (STATUS "Compiler needs libatomic, added")
        else ()
            message (FATAL_ERROR "Compiler needs libatomic, but not found")
        endif ()
    else ()
        message (VERBOSE "Compiler supports std::atomic, no libatomic necessary")
    endif ()
endif ()


###########################################################################
# Code coverage options
#
option (CODECOV "Build code coverage tests" OFF)
if (CODECOV AND (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG))
    message (STATUS "Compiling for code coverage analysis")
    add_compile_options (-ftest-coverage -fprofile-arcs)
    add_link_options (-ftest-coverage -fprofile-arcs)
    add_compile_definitions (${PROJ_NAME}_CODE_COVERAGE=1)
endif ()


###########################################################################
# Runtime profiling
#
set_cache (PROFILER "" "Build executables with profiler support (choices: gperftools)")
if (PROFILER STREQUAL "gperftools")
    find_library(PROFILER_LIBRARIES NAMES profiler)
    message (STATUS "Compiling for profiling with ${PROFILER}, found ${PROFILER_LIBRARIES}")
endif ()


###########################################################################
# Build profiling
#
set_option (${PROJ_NAME}_BUILD_PROFILER "Profile the build process" OFF)
if (${PROJ_NAME}_BUILD_PROFILER AND CMAKE_COMPILER_IS_CLANG)
    add_compile_options (-ftime-trace)
    message (STATUS "Profiling the build process")
endif ()


###########################################################################
# Sanitizer options
#
set_cache (SANITIZE "" "Build code using sanitizer (address, thread, undefined)")
if (SANITIZE AND (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG))
    message (STATUS "Compiling for sanitizer=${SANITIZE}")
    string (REPLACE "," ";" SANITIZE_FEATURE_LIST ${SANITIZE})
    foreach (feature ${SANITIZE_FEATURE_LIST})
        message (STATUS "  sanitize feature: ${feature}")
        add_compile_options (-fsanitize=${feature})
        add_link_options (-fsanitize=${feature})
    endforeach()
    add_compile_options (-g -fno-omit-frame-pointer)
    if (CMAKE_COMPILER_IS_GNUCC)
        # turn on glibcxx extra annotations to find vector writes past end
        add_compile_definitions (_GLIBCXX_SANITIZE_VECTOR=1)
    endif ()
    add_compile_definitions (${PROJECT_NAME}_SANITIZE=1)
endif ()


###########################################################################
# Safety/security hardening options
#
# Explanation of levels:
#  0 : do nothing, not recommended.
#  1 : enable features that have no (or nearly no) performance impact,
#      recommended default for optimized, shipping code.
#  2 : enable features that trade off performance for security, recommended
#      for debugging or deploying in security-sensitive environments.
#  3 : enable features that have a significant performance impact, only
#      recommended for debugging.
#
# Some documentation:
# https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.html
# https://www.gnu.org/software/libc/manual/html_node/Source-Fortification.html
# https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_macros.html
# https://libcxx.llvm.org/Hardening.html
#
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set (${PROJ_NAME}_HARDENING_DEFAULT 3)
else ()
    set (${PROJ_NAME}_HARDENING_DEFAULT 1)
endif ()
set_cache (${PROJ_NAME}_HARDENING ${${PROJ_NAME}_HARDENING_DEFAULT}
           "Turn on security hardening features 0, 1, 2, 3")
# Implementation:
if (HARDENING GREATER_EQUAL 1)
    # Features that should not detectably affect performance
    if (COMPILER_IS_GCC_OR_ANY_CLANG)
        # Enable PIE and pie to build as position-independent executables,
        # needed for address space randomiztion used by some kernels.
        add_compile_options (-fPIE -pie)
        add_link_options (-fPIE -pie)
        # Protect against stack overwrites. Is allegedly not a performance
        # tradeoff.
        add_compile_options (-fstack-protector-strong)
        add_link_options (-fstack-protector-strong)
    endif ()
    # Defining _FORTIFY_SOURCE provides buffer overflow checks in modern gcc &
    # clang with some compiler-assisted deduction of buffer lengths) for the
    # many C functions such as memcpy, strcpy, sprintf, etc. See:
    add_compile_definitions (_FORTIFY_SOURCE=${${PROJ_NAME}_HARDENING})
    # Setting _LIBCPP_HARDENING_MODE enables various hardening features in
    # clang/llvm's libc++ 18.0 and later.
    add_compiler_definitions (_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST)
endif ()
if (HARDENING GREATER_EQUAL 2)
    # Features that might impact performance measurably
    add_compile_definitions (_GLIBCXX_ASSERTIONS)
    add_compiler_definitions (_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE)
endif ()
if (HARDENING GREATER_EQUAL 3)
    # Debugging features that might impact performance significantly
    add_compile_definitions (_GLIBCXX_DEBUG)
    add_compiler_definitions (_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG)
endif ()


###########################################################################
# clang-tidy options
#
# clang-tidy is a static analyzer that is part of the LLVM tools. It has a
# variety of the usual bug and security tests, linting, and also tests for
# things like finding (and correcting!) use of older language constructs.
#
# If clang-tidy is found and enabled, a "clang-tidy" build target will be
# enabled. The set of tests can be customized both here and through
# the .clang-tidy file that is part of this project.
#
option (CLANG_TIDY "Enable clang-tidy" OFF)
set (CLANG_TIDY_CHECKS "-*" CACHE STRING "clang-tidy checks to perform (none='-*')")
set (CLANG_TIDY_ARGS "" CACHE STRING "clang-tidy args")
option (CLANG_TIDY_FIX "Have clang-tidy fix source" OFF)
if (CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy"
                 DOC "Path to clang-tidy executable")
    message (STATUS "CLANG_TIDY_EXE ${CLANG_TIDY_EXE}")
    if (CLANG_TIDY_EXE)
        set (CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
        if (CLANG_TIDY_ARGS)
            list (APPEND CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_ARGS})
        endif ()
        if (CLANG_TIDY_CHECKS)
            list (APPEND CMAKE_CXX_CLANG_TIDY -checks="${CLANG_TIDY_CHECKS}")
        endif ()
        execute_process (COMMAND ${CMAKE_CXX_CLANG_TIDY} -list-checks
                         OUTPUT_VARIABLE tidy_checks
                         OUTPUT_STRIP_TRAILING_WHITESPACE)
        if (CLANG_TIDY_FIX)
            list (APPEND CMAKE_CXX_CLANG_TIDY "-fix")
        endif ()
        message (STATUS "clang-tidy command line is: ${CMAKE_CXX_CLANG_TIDY}")
        message (STATUS "${tidy_checks}")
    else ()
        message (STATUS "Cannot run clang-tidy as requested")
    endif ()
    # Hint: run with CLANG_TIDY_ARGS=-list-checks to list all the checks
endif ()


###########################################################################
# clang-format options
#
# clang-format is a source code reformatter that is part of the LLVM tools.
# It can be used to check adherence to project code formatting rules and
# correct any deviations. If clang-format is found on the system, a
# "clang-format" build target will trigger a reformatting.
#
# Note: skip all of this checking, setup, and cmake-format target if this
# is being built as a subproject.
if (PROJECT_IS_TOP_LEVEL)
    set (CLANG_FORMAT_EXE_HINT "" CACHE PATH "clang-format executable's directory (will search if not specified")
    set (CLANG_FORMAT_INCLUDES "src/*.h" "src/*.cpp testsuite/*.cpp testsuite/*.h"
        CACHE STRING "Glob patterns to include for clang-format")
    set (CLANG_FORMAT_EXCLUDES "*pugixml*" "*SHA1*" "*/farmhash.cpp"
                               "src/dpx.imageio/libdpx/*"
                               "src/cineon.imageio/libcineon/*"
                               "src/dds.imageio/bcdec.h"
                               "src/gif.imageio/gif.h"
                               "src/libutil/stb_sprintf.h"
         CACHE STRING "Glob patterns to exclude for clang-format")
    find_program (CLANG_FORMAT_EXE
                  NAMES clang-format bin/clang-format
                  HINTS ${CLANG_FORMAT_EXE_HINT} ENV CLANG_FORMAT_EXE_HINT
                        ENV LLVM_DIRECTORY
                  NO_DEFAULT_PATH
                  DOC "Path to clang-format executable")
    find_program (CLANG_FORMAT_EXE NAMES clang-format bin/clang-format)
    if (CLANG_FORMAT_EXE)
        message (STATUS "clang-format found: ${CLANG_FORMAT_EXE}")
        # Start with the list of files to include when formatting...
        file (GLOB_RECURSE FILES_TO_FORMAT ${CLANG_FORMAT_INCLUDES})
        # ... then process any list of excludes we are given
        foreach (_pat ${CLANG_FORMAT_EXCLUDES})
            file (GLOB_RECURSE _excl ${_pat})
            list (REMOVE_ITEM FILES_TO_FORMAT ${_excl})
        endforeach ()
        #message (STATUS "clang-format file list: ${FILES_TO_FORMAT}")
        file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/.clang-format
              DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
        add_custom_target (clang-format
            COMMAND "${CLANG_FORMAT_EXE}" -i -style=file ${FILES_TO_FORMAT} )
    else ()
        message (STATUS "clang-format not found.")
    endif ()
endif ()

###########################################################################
# Another way to sneak in custom compiler and DSO linking flags.
#
set (EXTRA_CPP_ARGS "" CACHE STRING "Extra C++ command line definitions")
if (EXTRA_CPP_ARGS)
    message (STATUS "Extra C++ args: ${EXTRA_CPP_ARGS}")
    add_compile_options (${EXTRA_CPP_ARGS})
endif()
set (EXTRA_DSO_LINK_ARGS "" CACHE STRING "Extra command line definitions when building DSOs")


###########################################################################
# Set the versioning for shared libraries.
#
if (${PROJECT_NAME}_SUPPORTED_RELEASE AND NOT SKBUILD)
    # Supported releases guarantee ABI back-compatibility within the release
    # family, so SO versioning is major.minor.
    set (SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
         CACHE STRING "Set the SO version for dynamic libraries")
else ()
    # Main development branch makes no ABI stability guarantee, so we make the
    # SO naming capture down to the major.minor.patch level.
    set (SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
         CACHE STRING "Set the SO version for dynamic libraries")
endif ()
message(VERBOSE "Setting SOVERSION to: ${SOVERSION}")


###########################################################################
# BUILD_SHARED_LIBS, if turned off, will disable building of .so/.dll
# dynamic libraries and instead only build static libraries.
#
option (BUILD_SHARED_LIBS "Build shared libraries (set to OFF to build static libs)" ON)
if (NOT BUILD_SHARED_LIBS)
    add_compile_definitions (${PROJ_NAME}_STATIC_DEFINE=1)
endif ()


###########################################################################
# LINKSTATIC, if enabled, will cause us to favor linking static versions
# of library dependencies, if they are available.
#
option (LINKSTATIC  "Link with static external libraries when possible" OFF)
if (LINKSTATIC)
    #set (_orig_link_suffixes ${CMAKE_FIND_LIBRARY_SUFFIXES})
    message (STATUS "Statically linking external libraries when possible")
    if (WIN32)
        set (CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    else ()
        set (CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    endif ()
endif ()


###########################################################################
# Windows: which MSVC runtime library should we use (to override default)?
# Note that all dependencies need the same choice. We leave this as the
# default, but allow it to be set by the environment if not explicit, which
# the built-in CMAKE_MSVC_RUNTIME_LIBRARY does not do on its own.
#
if (WIN32)
    set_from_env (CMAKE_MSVC_RUNTIME_LIBRARY)
endif ()


###########################################################################
# Any extra logic to be run only for CI builds goes here.
# We expect our own CI runs to define env variable ${PROJECT_NAME}_CI
#
if (DEFINED ENV{${PROJECT_NAME}_CI})
    add_compile_definitions (${PROJ_NAME}_CI=1 BUILD_CI=1)
    if (APPLE)
        # Keep Mono framework from being incorrectly searched for include
        # files on GitHub Actions CI.
        set(CMAKE_FIND_FRAMEWORK LAST)
    endif ()
endif ()


###########################################################################
# Rpath handling at the install step
#
# set (MACOSX_RPATH ON)
if (CMAKE_SKIP_RPATH)
    # We need to disallow the user from truly setting CMAKE_SKIP_RPATH, since
    # we want to run the generated executables from the build tree in order to
    # generate the manual page documentation.  However, we make sure the
    # install rpath is unset so that the install tree is still free of rpaths
    # for linux packaging purposes.
    set (CMAKE_SKIP_RPATH FALSE)
    unset (CMAKE_INSTALL_RPATH)
else ()
    if (NOT CMAKE_INSTALL_RPATH)
        if(APPLE)
            set(BASEPOINT @loader_path)
        else()
            set(BASEPOINT $ORIGIN)
        endif()
        set (CMAKE_INSTALL_RPATH ${BASEPOINT}
                                 ${BASEPOINT}/${CMAKE_INSTALL_LIBDIR}
                                 ${BASEPOINT}/../${CMAKE_INSTALL_LIBDIR})
    endif ()
    # add the automatically determined parts of the RPATH that
    # point to directories outside the build tree to the install RPATH
    set (CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    message (VERBOSE "CMAKE_INSTALL_RPATH = ${CMAKE_INSTALL_RPATH}")
endif ()


###########################################################################
# Generate compile_commands.json for use by editors and tools.
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)


###########################################################################
# Macro to install targets to the appropriate locations.  Use this instead
# of the install(TARGETS ...) signature. Note that it adds it to the
# export targets list for when we generate config files.
#
# Usage:
#
#    install_targets (target1 [target2 ...])
#
macro (install_targets)
    # Define options that can be passed to the macro
    set(options NAMELINK_SKIP)

    # Parse the arguments passed to the macro
    cmake_parse_arguments(INSTALL_TARGETS "${options}" "" "" "${ARGN}")

    # Check if NAMELINK_SKIP is specified
    if (INSTALL_TARGETS_NAMELINK_SKIP)
        # Set namelink_option to NAMELINK_SKIP if specified
        set(namelink_option NAMELINK_SKIP)
    else()
        # Leave namelink_option empty if NAMELINK_SKIP is not specified
        set(namelink_option)
    endif()

    # Use the install command with the appropriate options
    install (TARGETS ${INSTALL_TARGETS_UNPARSED_ARGUMENTS}
             EXPORT ${PROJ_NAME}_EXPORTED_TARGETS
             RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT user
             LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT user ${namelink_option}
             ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT developer)
endmacro()
