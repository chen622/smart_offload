# smart_offload

[中文](https://github.com/chen622/smart_offload/blob/main/README-ZH.md)

## 1. Main Feature

- Pass the packet into process thread, and create an offloading rte_flow after n packets.
- The offloading rte_flow can count the packets, set timeout callback, use hairpin to forward packets.
- Delete the rte_flow if there is no corresponding packet for 10 seconds.

## 2. Module Design

The diagram is written with Chinese, you can see the English comment of functions in the code.
![module](https://img.ccm.ink/smart_offload.jpg)

## 3. Quick Start

### Requirements：
- DPDK 20.11.3.0.4
- x86 server with Mellanox/Nvidia smart-nic.

### Run
```bash
mkdir build && cd build
cmake ..
make
# -l Specify the core; -a Specify the port
./smart_offload -l 1-9 -a 82:00.0
```