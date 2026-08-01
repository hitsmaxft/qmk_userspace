# Anne Pro 2 文档索引

- [C18 BLE 2.13 完整调研报告](research/C18_BLE213_research/00_INDEX.md)：
  官方样本、BLE 2.13 内部实现、C18/AP2D 差异和历史双配置方案、
  四槽状态机、验证门禁和反汇编复核入口。
- [C18 KEY 双 BLE 升级方案](C18_KEY_dual_BLE_2.05_2.13_upgrade_plan_zh.md)：
  已被 C18/C18D 型号拆分取代的实现规划原稿。
- [AP2D 3.x 固件样本索引](ap2d-3x-firmware-release-index.md)：官方
  3.04–3.08 KEY/BLE 版本链、本地样本路径、已确认边界和实现约束。
- [BLE 2.13 AnnePro 2C 名称变体](ble213-name-variant.md)：保持固件布局不变
  的固定宽度兼容名称补丁、生成命令与验证边界。
- [BLE 2.13 开发问题与解决记录](ble213-development-notes.md)：IAP base、
  strict ACK、连接握手、切槽隔离、macOS 缓存、console 与 Nix 环境。
- [AP2D BLE 2.13 backport 状态](ap2d-ble213-backport-status.md)：新增静态
  证据、C18/C18D 固定协议实现、构建方法和剩余硬件门禁。
- [C18/C18D 固定 BLE 型号验证矩阵](ble213-validation-matrix.md)：逐项区分
  源码、host 测试、构建、既有实机证据和当前仍缺的硬件验收。
- [BLE 2.13 交叉刷写门禁](ble213-crossflash-gate.md)：官方镜像包络、
  CC2541 flash/Information Page/SNV 边界、现有刷写工具缺陷及私有备份检查。
- [BLE 固件与 UART 协议](ble-firmware-and-uart-protocol.md)：官方主控/CC254x
  镜像布局、UART framing、`20/0c` 握手请求/回复与未确认边界。
- [QMK BLE 可靠性修复](ble-reliability-pr.md)：旧 200 ms 补丁为何撤回、
  event-driven host driver 切换和上游 PR 草案。
- [USB Console 验证](ble-usb-console-validation.md)：debug 构建、日志字段、
  handshake/ACK/断链验证矩阵。
- [二次 connect 实验记录](ble-double-connect-experiment.md)：归档未刷入的
  200 ms 双 connect 实验、已知竞态、回滚基线和后续 single-flight 方向。

当前实现把 C18 BLE 2.05 与 AP2D BLE 2.13 分别固定到 `annepro2/c18` 和
`annepro2/c18d`，不提供 EEPROM 协议切换。BLE 2.13 二进制保持原样。AP2D
已取消 C18 的独立 LED MCU，因此不移植 AP2D 的 GPIO/RGB/LED
Output 实现；两代共同使用的 `20/07` Caps 逻辑状态则通过 QMK 标准 LED
接口桥接到 C18 原有外置 LED MCU，并单独做实机验收。

截至 2026-07-28，操作者已将 C18 KEY 对官方 BLE 2.13 的功能 backport 整体
验收为通过，覆盖普通键盘、媒体键、Caps/实体锁定灯、四槽重新配对、四主机
切换、连接超时、压力测试和断电恢复。这些证据来自拆分前实现；当前
C18/C18D 软件门禁已经通过，拆分后的两个型号仍需分别完成实机非回归。

尚未由完整调用链或抓包确认的 UART 命令必须保持实验状态。构建成功不能替代
radio、bond、HID、外部 LED MCU 或交叉刷写安全性的实机验证。
