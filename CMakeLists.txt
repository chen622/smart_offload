cmake_minimum_required(VERSION 3.16)
project(smart_offload)

set(CMAKE_C_FLAGS "-std=gnu11")

# configure build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
    message(STATUS "No CMAKE_BUILD_TYPE selected, defaulting to ${CMAKE_BUILD_TYPE}")
else ()
    message(STATUS "Predefined CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE} ")
endif ()

if (VM)
    add_definitions(-DVM)
endif ()

if (CMAKE_BUILD_TYPE MATCHES Release)
    set(CMAKE_C_FLAGS_RELEASE "-O3")
    add_definitions(-DRELEASE)
    configure_file(conf/zlog-debug.conf ${CMAKE_CURRENT_BINARY_DIR}/conf/zlog.conf COPYONLY)
    # configure_file(conf/zlog.conf /etc/smart_offload/zlog.conf COPYONLY)
    # file(MAKE_DIRECTORY /opt/tsg/smart_offload/log)
else ()
    configure_file(conf/zlog-debug.conf ${CMAKE_CURRENT_BINARY_DIR}/conf/zlog.conf COPYONLY)
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
endif ()

add_definitions(-DALLOW_EXPERIMENTAL_API)

# avoid Apple Silicon impact
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")

# build third-party package
include(${CMAKE_SOURCE_DIR}/vendor/CMakeLists.txt)

include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/include/deps)

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

add_subdirectory(src)
add_subdirectory(test)