include(ExternalProject)

set(VENDOR_ROOT ${CMAKE_BINARY_DIR}/vendor)
set(VENDOR_BUILD ${CMAKE_BINARY_DIR}/vendor/vbuild)

ExternalProject_Add(ZLOG
        PREFIX ${VENDOR_ROOT}/zlog
        URL https://github.com/HardySimpson/zlog/archive/refs/tags/1.2.15.tar.gz
        URL_MD5 155f841543f947bf21112ae4328e7152
        CONFIGURE_COMMAND pwd
        BUILD_IN_SOURCE 1
        BUILD_COMMAND make PREFIX=${VENDOR_BUILD}
        INSTALL_COMMAND make PREFIX=${VENDOR_BUILD} install
        INSTALL_DIR ${VENDOR_BUILD}
        )

