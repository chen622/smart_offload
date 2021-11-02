include(ExternalProject)

set(VENDOR_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(VENDOR_INSTALL ${VENDOR_ROOT}/install)

set(ZLOG_URL ${VENDOR_ROOT}/zlog-1.2.15.zip)
ExternalProject_Add(ZLOG
        PREFIX ${VENDOR_ROOT}/zlog
        URL ${ZLOG_URL}
        CONFIGURE_COMMAND pwd
        BUILD_IN_SOURCE 1
        BUILD_COMMAND make PREFIX=${VENDOR_INSTALL}
        INSTALL_COMMAND make PREFIX=${VENDOR_INSTALL} install
        INSTALL_DIR ${VENDOR_INSTALL}
        )

