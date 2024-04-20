cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)

set(PROJECT_PRESENT_NAME "PM Example")
set(PROJECT "pm_example")
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
    endif()
    add_definitions(-DLOG_LEVEL=0 -DDEBUG)
endif()

add_definitions(-DTHREAD_SAFE)

message("CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")
message("PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")

if(NOT SKIP_VERSION)
    set(VERSION_SRC ${CMAKE_CURRENT_BINARY_DIR}/version.c)
    # Add a custom command that produces version.c, plus a dummy output that's not
    # actually produced, in order to force version.cmake to always be re-run
    # before the build
    add_custom_command(
        OUTPUT ${VERSION_SRC} ${CMAKE_CURRENT_BINARY_DIR}/_version.c
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/version.cmake)
else()
    add_definitions(-DSKIP_VERSION)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_definitions(
    -DBUILD_TYPE="${CMAKE_BUILD_TYPE}"
    -DPROJECT_NAME="${PROJECT_PRESENT_NAME}"
    -DPROJECT_FOLDER="pm"
    -DLOG_USE_COLOR
    -DYAML
    -DVIDEO_FPS=25
)

set (PROJECT_SRC
    "${log.c_SOURCE_DIR}/src/log.c"
    ../src/pm.c
    main.c
    ${libhl_SOURCE_DIR}/src/linklist.c
    ${libhl_SOURCE_DIR}/src/hashtable.c
    ${VERSION_SRC}
)

add_executable(${PROJECT} ${PROJECT_SRC})

target_include_directories(${PROJECT}
    PUBLIC
    ${PROJECT_SOURCE_DIR}/../src
    PRIVATE
    "${log.c_SOURCE_DIR}/src"
    "${libevent_BINARY_DIR}/include"
    "${libevent_SOURCE_DIR}/include"
    "${libyaml_SOURCE_DIR}/include"
    "${libhl_SOURCE_DIR}/src"
    "${smolrtsp-libevent_SOURCE_DIR}/include"
    "${smolrtsp_SOURCE_DIR}/include"
)

target_link_libraries(${PROJECT} PUBLIC
    yaml
    event
    stdc++ pthread gcc_s dl m c rt
)

set_target_properties(${PROJECT} PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)


if(CMAKE_BUILD_TYPE STREQUAL "Release")

else()
    target_link_options(${PROJECT} PRIVATE -fsanitize=address)
endif()
# target_link_options(${PROJECT} PRIVATE -fsanitize=address)
