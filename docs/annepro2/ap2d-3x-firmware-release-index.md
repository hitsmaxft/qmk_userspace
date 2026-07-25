# Anne Pro 2D 3.x 固件样本索引

本页记录 C18 KEY 兼容 AP2D BLE 2.13 工作所使用的官方版本链。完整的逆向
证据、协议差异、实现方案和风险门禁见
[C18 BLE 2.13 完整调研报告](research/C18_BLE213_research/00_INDEX.md)。

## 官方版本链

版本信息来自 Hexcore 的 AP2D `latest.json`。仓库保存了分析时取得的
[manifest 副本](research/C18_BLE213_research/source_manifests/annepro2d_latest.json)。

| AP2D KEY | BLE | 日期 | KEY SHA-256 | BLE SHA-256 |
|---|---|---|---|---|
| 3.04 | 2.09 | 2022-11-20 | `d2007cd4…00a54` | `36322742…c0ef3` |
| 3.05 | 2.09 | 2022-11-22 | `e3b5b40e…d81b2` | `36322742…c0ef3` |
| 3.06 | 2.10 | 2023-01-05 | `8914176a…4b01` | `3494ffed…0c8eea` |
| 3.08 | 2.13 | 2023-03-20 | `ab7a91fe…f7c1` | `1b904ae9…c73d` |

官方 3.08 说明包含：同一主机绑定两台 AP2D、macOS 蓝牙 Caps Lock 灯、
USB suspend，以及多设备切换修复；升级后要求删除旧配对并重新绑定。

## 本地样本

下载的二进制已归档在 `assets/ap2_fw` submodule 的以下目录：

```text
assets/ap2_fw/annepro2d/firmware/3.04/
assets/ap2_fw/annepro2d/firmware/3.05/
assets/ap2_fw/annepro2d/firmware/3.06/
assets/ap2_fw/annepro2d/firmware/3.08/
```

每个目录包含对应的 `annepro2_discovery_KEY_APP.bin` 和
`annepro2_discovery_ble.bin`。完整文件名、大小和 SHA-256 见完整报告的
[证据与版本链](research/C18_BLE213_research/01_EVIDENCE_AND_VERSIONS.md)。

## 已确认的兼容边界

- C18 与 AP2D KEY 使用相同的 PA4/PA5、AF6、115200 bit/s BLE UART 基础
  链路，并共享 `0x7B…0x7D` 帧族。
- 标准键盘 Report ID 1 保持一致。
- BLE 2.13 的 Vendor Report ID 2 交换了 IN/OUT Usage 方向。
- Consumer Report ID 3 从 C18 的 4 字节位图变为 4 个小端 `uint16_t`
  Usage，共 8 字节。
- 锁定灯输入需要兼容单字节 LED bits 和 `[Report ID, LED bits]`。
- BLE 2.13 会校验并重建与工厂 IEEE 地址关联的安全记录，因此升级后需要
  清除旧配对并重新绑定。

这些结论来自报告所列的固件字节、反汇编和描述符证据。它们不能证明 AP2D
BLE 2.13 可以安全交叉刷入 C18 BLE 板，也不能替代无线连接和四主机切换的
实机验收。

## 实现约束

首版只修改 C18 KEY 源码，BLE 二进制保持原样。同一源码内置
`C18_BLE205` 和 `C18_BLE213` 两个显式 profile，并生成默认值不同的构建。
版本选择必须持久化且可恢复；没有证据的 UART 命令不得用探测包猜测。

四主机状态机必须等待断开/连接结果、隔离迟到事件，并在超时后回到稳定状态。
AP2D 3.08 中出现的 `0x21/0x22` 和四条 9 字节记录仍需通过完整调用链或 UART
抓包确认语义，不能仅凭常量直接移植。
