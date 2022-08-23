include(ExternalProject)

set(VENDOR_ROOT ${CMAKE_BINARY_DIR}/vendor)
set(VENDOR_BUILD ${CMAKE_BINARY_DIR}/vendor/vbuild)

file(MAKE_DIRECTORY ${VENDOR_BUILD}/include)

# build third-party package zlog
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
add_library(zlog SHARED IMPORTED)
add_dependencies(zlog ZLOG)
set_target_properties(zlog PROPERTIES
        IMPORTED_LOCATION ${VENDOR_BUILD}/lib/libzlog.a
        INTERFACE_INCLUDE_DIRECTORIES ${VENDOR_BUILD}/include)

ExternalProject_Add(RDARM
        PREFIX ${VENDOR_ROOT}/rdarm
        URL ${CMAKE_CURRENT_SOURCE_DIR}/vendor/rdarm-e8b5c3fb.zip
        INSTALL_DIR ${VENDOR_BUILD}
        BUILD_BYPRODUCTS ${VENDOR_BUILD}/lib/librdarm.a
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${VENDOR_BUILD} -DCMAKE_BUILD_TYPE=Release -DNO_NAT_TEST=True -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        BUILD_IN_SOURCE 1
        )
add_library(rdarm SHARED IMPORTED)
add_dependencies(rdarm RDARM)
set_target_properties(rdarm PROPERTIES
        IMPORTED_LOCATION ${VENDOR_BUILD}/lib/librdarm.a
        INTERFACE_INCLUDE_DIRECTORIES ${VENDOR_BUILD}/include)

include_directories(${VENDOR_BUILD}/include)

# import dpdk library
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBDPDK REQUIRED libdpdk)

# import thread
find_package(Threads REQUIRED)


link_directories(${LIBDPDK_LIBRARY_DIRS})

include_directories(${LIBDPDK_INCLUDE_DIRS})
