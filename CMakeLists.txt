cmake_minimum_required(VERSION 3.16)
project(smart_offload)

set(CMAKE_C_STANDARD 11)

# configure build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
    message(STATUS "No CMAKE_BUILD_TYPE selected, defaulting to ${CMAKE_BUILD_TYPE}")
else ()
    message(STATUS "Predefined CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE} ")
endif ()

if (CMAKE_BUILD_TYPE MATCHES Release)
    set(CMAKE_C_FLAGS_RELEASE "-O3")
    add_definitions(-DRELEASE)
else ()
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
endif ()

# avoid Apple Silicon impact
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")

# import dpdk library
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBDPDK REQUIRED libdpdk)

# import thread
find_package(Threads REQUIRED)

# build third-party package zlog
add_subdirectory(vendor)
ExternalProject_Get_Property(ZLOG INSTALL_DIR)
include_directories(${INSTALL_DIR}/include)
add_library(zlog SHARED IMPORTED)
set_target_properties(
        zlog
        PROPERTIES IMPORTED_LOCATION
        ${INSTALL_DIR}/lib/libzlog.a
)
configure_file(conf/zlog-debug.conf ${CMAKE_CURRENT_BINARY_DIR}/conf/zlog.conf COPYONLY)

include_directories(include)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)

add_subdirectory(src)
