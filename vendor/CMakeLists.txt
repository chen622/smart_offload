include(ExternalProject)

set(VENDOR_ROOT ${CMAKE_BINARY_DIR}/vendor)
set(VENDOR_BUILD ${CMAKE_BINARY_DIR}/vendor/vbuild)

set(ZLOG_URL ${CMAKE_CURRENT_SOURCE_DIR}/zlog-1.2.15.zip)
ExternalProject_Add(ZLOG
        PREFIX ${VENDOR_ROOT}/zlog
        URL ${ZLOG_URL}
        CONFIGURE_COMMAND pwd
        BUILD_IN_SOURCE 1
        BUILD_COMMAND make PREFIX=${VENDOR_BUILD}
        INSTALL_COMMAND make PREFIX=${VENDOR_BUILD} install
        INSTALL_DIR ${VENDOR_BUILD}
        )

