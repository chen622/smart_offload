include(ExternalProject)

set(VENDOR_ROOT ${CMAKE_BINARY_DIR}/vendor)
set(VENDOR_BUILD ${CMAKE_BINARY_DIR}/vendor/vbuild)

file(MAKE_DIRECTORY ${VENDOR_BUILD}/include)

ExternalProject_Add(ZLOG
        PREFIX ${VENDOR_ROOT}/zlog
        URL https://github.com/HardySimpson/zlog/archive/refs/tags/1.2.15.tar.gz
        URL_MD5 155f841543f947bf21112ae4328e7152
        CONFIGURE_COMMAND pwd
        BUILD_IN_SOURCE 1
        BUILD_COMMAND make PREFIX=${VENDOR_BUILD}
        INSTALL_COMMAND make PREFIX=${VENDOR_BUILD} install
        INSTALL_DIR ${VENDOR_BUILD}
        BUILD_BYPRODUCTS ${VENDOR_BUILD}/lib/libzlog.a
        )

# import dpdk library
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBDPDK REQUIRED libdpdk)

# import thread
find_package(Threads REQUIRED)

# build third-party package zlog
add_library(zlog SHARED IMPORTED)
add_dependencies(zlog ZLOG)
set_target_properties(zlog PROPERTIES
        IMPORTED_LOCATION ${VENDOR_BUILD}/lib/libzlog.a
        INTERFACE_INCLUDE_DIRECTORIES ${VENDOR_BUILD}/include)

link_directories(${LIBDPDK_LIBRARY_DIRS})

include_directories(${LIBDPDK_INCLUDE_DIRS})
