cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)

set(PROJECT_PRESENT_NAME PM)
string(TOLOWER ${PROJECT_PRESENT_NAME} PROJECT)
project (${PROJECT} LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "-std=gnu99")

cmake_policy(SET CMP0069 NEW)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(LibEvent REQUIRED)

option(BUILD_EXAMPLE_APP "Build the example application" OFF)


include(FetchContent)

# LIBEVENT OPTIONS #
set (EVENT__LIBRARY_TYPE    STATIC CACHE INTERNAL
    "Set library type to SHARED/STATIC/BOTH (default SHARED for MSVC, otherwise BOTH)") # SHARED/STATIC/BOTH
set (EVENT__DISABLE_SAMPLES ON CACHE INTERNAL "Disable sample files")
set (EVENT__DISABLE_BENCHMARK ON CACHE INTERNAL
    "Defines if libevent should build without the benchmark executables")
set (EVENT__DISABLE_TESTS   ON CACHE INTERNAL "If tests should be compiled or not")
set (EVENT__DISABLE_REGRESS ON CACHE INTERNAL "Disable the regress tests")
# set (EVENT__DISABLE_OPENSSL ON CACHE INTERNAL "Turn off OPENSSL")
set (EVENT__DISABLE_OPENSSL OFF CACHE INTERNAL "Turn off OPENSSL")
set (EVENT__DISABLE_MBEDTLS ON CACHE INTERNAL "Turn on MBEDTLS")

# set (EVENT__ENABLE_VERBOSE_DEBUG ON CACHE INTERNAL "Enables verbose debugging")

# LIBEVENT OPTIONS #

FetchContent_Declare(
    log.c
    URL https://github.com/SSharshunov/log.c/archive/refs/tags/latest.tar.gz
)

FetchContent_Declare(
    libevent
    # GIT_REPOSITORY https://github.com/libevent/libevent.git
    # GIT_TAG fe9dc8f614db0520027e8e2adb95769193d4f0a3
    URL https://github.com/libevent/libevent/releases/download/release-2.2.1-alpha/libevent-2.2.1-alpha-dev.tar.gz
)

FetchContent_Declare(
    libhl
    URL https://github.com/xant/libhl/archive/refs/tags/libhl-3.1.tar.gz
)

FetchContent_Declare(
    libyaml
    URL https://github.com/yaml/libyaml/archive/refs/tags/0.2.5.tar.gz
)

FetchContent_Declare(
    smolrtsp
    # URL https://github.com/OpenIPC/smolrtsp/archive/refs/tags/v0.1.3.tar.gz
    GIT_REPOSITORY https://github.com/OpenIPC/smolrtsp.git
    GIT_TAG 9f708a4
)

FetchContent_Declare(
    smolrtsp-libevent
    GIT_REPOSITORY https://github.com/OpenIPC/smolrtsp-libevent
    GIT_TAG 7451e74
)

FetchContent_MakeAvailable(log.c libevent libhl libyaml smolrtsp smolrtsp-libevent)

if(BUILD_EXAMPLE_APP)
    add_subdirectory(example)
endif(BUILD_EXAMPLE_APP)
