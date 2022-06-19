# Smart Offload

[![Build](https://github.com/chen622/smart_offload/actions/workflows/build.yml/badge.svg)](https://github.com/chen622/smart_offload/actions/workflows/build.yml)
[![](https://img.shields.io/badge/%E6%96%87%E6%A1%A3-%E4%B8%AD%E6%96%87%E7%89%88-blue)](https://github.com/chen622/smart_offload/blob/main/README-ZH.md)
[![](https://img.shields.io/badge/document-English-blue)](https://github.com/chen622/smart_offload/blob/main/README.md)

## 1. Main Feature

- Pass the packet into process thread, and create an offloading rte_flow after n packets.
- The offloading rte_flow can count the packets, set timeout callback, use hairpin to forward packets.
- Delete the rte_flow if there is no corresponding packet for 10 seconds.

## 2. Module Design

The diagram is written with Chinese, you can see the English comment of functions in the code.
![module](https://img.ccm.ink/smart_offload.jpg)

## 3. Quick Start

### Requirements：
- DPDK 20.11.5
- x86 server with Mellanox/Nvidia SmartNic.

### Run
```bash
mkdir build && cd build
cmake ..
make
# -l Specify the core; -a Specify the port
./smart_offload -l 1-9 -a 82:00.0
```

## 4. Questions

### Unable to set Power Management Environment

Can not set frequency of CPU cores, you can a definition `-DVM=true` to disable this feature.
