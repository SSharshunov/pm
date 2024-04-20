cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)

set(xPROJECT PM)
string(TOLOWER ${xPROJECT} PROJECT)
project (${PROJECT} LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "-std=gnu99")

cmake_policy(SET CMP0069 NEW)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(LibEvent REQUIRED)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS
        "${CMAKE_C_FLAGS} -s -Os -ffunction-sections -Wl,--gc-sections -DNDEBUG -Wextra -Wno-builtin-macro-redefined")
    add_definitions(-DLOG_LEVEL=2)
else()
    if(BUILD_SHARED_LIBS)
        set(CMAKE_C_FLAGS
            "${CMAKE_C_FLAGS} -O1 -g -fsanitize=address -fno-omit-frame-pointer -Wno-builtin-macro-redefined")
            # "${CMAKE_C_FLAGS} -O1 -g -fno-omit-frame-pointer -Wno-builtin-macro-redefined")
    endif()
    add_definitions(-DLOG_LEVEL=0 -DDEBUG)
endif()

add_definitions(-DTHREAD_SAFE)

if(NOT SKIP_VERSION)
    set(VERSION_SRC ${CMAKE_CURRENT_BINARY_DIR}/version.c)
    # Add a custom command that produces version.c, plus a dummy output that's not
    # actually produced, in order to force version.cmake to always be re-run
    # before the build
    add_custom_command(
        OUTPUT ${VERSION_SRC} ${CMAKE_CURRENT_BINARY_DIR}/_version.c
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.cmake)
else()
    add_definitions(-DSKIP_VERSION)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_definitions(
    -DBUILD_TYPE="${CMAKE_BUILD_TYPE}"
    -DPROJECT_NAME="${xPROJECT}"
    -DPROJECT_FOLDER="${PROJECT}"
    -DLOG_USE_COLOR
    -DYAML
    -DVIDEO_FPS=25
)

include(FetchContent)

# JSON-C OPTIONS #
set (BUILD_STATIC_LIBS ON CACHE INTERNAL "Default to building static libraries")
set (BUILD_SHARED_LIBS OFF CACHE INTERNAL "Default to building shared libraries")
set (BUILD_TESTING OFF CACHE INTERNAL "")
set (BUILD_APPS OFF CACHE INTERNAL "Default to building apps")

# JSON-C OPTIONS #

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

# FetchContent_Declare(
#     json-c
#     URL https://github.com/json-c/json-c/archive/refs/tags/json-c-0.17-20230812.tar.gz
# )

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

# FetchContent_Declare(
#     argparse
#     URL https://github.com/cofyc/argparse/archive/refs/tags/v1.1.0.tar.gz
# )

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

set (PROJECT_SRC
    # src/log.c
    "${log.c_SOURCE_DIR}/src/log.c"
    # src/utils/ping.c
    src/pm.c
    example/main.c
    ${libhl_SOURCE_DIR}/src/linklist.c
    ${libhl_SOURCE_DIR}/src/hashtable.c
    ${VERSION_SRC}
)

add_executable(${PROJECT} ${PROJECT_SRC})
# target_compile_options(${PROJECT} PRIVATE -Wno-invalid-pch)

target_include_directories(${PROJECT}
    PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/plugins
    ${PROJECT_SOURCE_DIR}/src/utils
    PRIVATE
    "${log.c_SOURCE_DIR}/src"
    # "${json-c_BINARY_DIR}"
    # "${json-c_SOURCE_DIR}"
    "${libevent_BINARY_DIR}/include"
    "${libevent_SOURCE_DIR}/include"
    "${libyaml_SOURCE_DIR}/include"
    "${libhl_SOURCE_DIR}/src"
    # "${argparse_SOURCE_DIR}"
    "${smolrtsp-libevent_SOURCE_DIR}/include"
    "${smolrtsp_SOURCE_DIR}/include"
)

target_link_libraries(${PROJECT} PUBLIC
    # json-c
    yaml
    event
    stdc++ pthread gcc_s dl m c rt
)

set_target_properties(${PROJECT} PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)

