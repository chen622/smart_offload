#!/usr/bin/env bash

#set -e

DOWNLOAD_DIR="/root/download"
DPDK_FILE_NAME="dpdk-21.11.1.tar.xz"
DPDK_URL="https://fast.dpdk.org/rel/${DPDK_FILE_NAME}"
PIP_DEPENDENCIES="meson ninja pyelftools"

echo "Installing DPDK..."

pip3 install ${PIP_DEPENDENCIES}

mkdir -p ${DOWNLOAD_DIR}
cd ${DOWNLOAD_DIR}

wget ${DPDK_URL}
tar xJf ${DPDK_FILE_NAME}
cd dpdk-*/

meson build
cd build
ninja
ninja install

echo "/usr/local/lib" >> /etc/ld.so.conf.d/libc.conf
echo "/usr/local/lib64" >> /etc/ld.so.conf.d/libc.conf
ldconfig

pkg-config --exists libdpdk