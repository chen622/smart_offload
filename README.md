# smart_offload

## 一、主要功能

- 将流经的数据包上送到处理线程中，并在第 n 个包后下发对应的流表进行卸载
- 下发的流表可以使网卡对数据包进行计数、超时判断、hairpin转发等操作
- 能够定期对已卸载的流进行数量、大小的统计
- 对于一段时间未使用的流表进行删除处理

## 二、模块设计

![module](https://img.ccm.ink/smart_offload.jpg)

## 三、待验证部分

- [ ]  对流下发的速率上限进行测试
- [ ]  对流的数量上限进行测试
- [ ]  对空pattern的rte_flow使用rss
- [ ]  一组hairpin是否可以实现双向通信
- [ ]  如何实现定时查询 age？回调？alarm触发？


## 四、快速上手

### 配置要求：
- DPDK 20.11.3.0.4
- 配备 Mellanox/Nvidia 智能网卡的 x86 服务器

### 运行指令
```bash
mkdir build && cd build
cmake ..
make
./smart_offload -l 0-8
```