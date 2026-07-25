# Anne Pro 2 文档索引

- [C18 BLE 2.13 完整调研报告](research/C18_BLE213_research/00_INDEX.md)：
  官方样本、BLE 2.13 内部实现、C18/AP2D 差异、双 profile 协议规格、
  四槽状态机、验证门禁和反汇编复核入口。
- [C18 KEY 双 BLE 升级方案](C18_KEY_dual_BLE_2.05_2.13_upgrade_plan_zh.md)：
  独立保存的实现规划原稿。
- [AP2D 3.x 固件样本索引](ap2d-3x-firmware-release-index.md)：官方
  3.04–3.08 KEY/BLE 版本链、本地样本路径、已确认边界和实现约束。
- [AP2D BLE 2.13 backport 状态](ap2d-ble213-backport-status.md)：新增静态
  证据、双 profile 实现、构建方法、维护键位和剩余硬件门禁。
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

当前 backport 的主体固定为 C18 KEY 源码，BLE 2.13 二进制保持原样。首版需要
同时支持 C18 BLE 2.05 和 AP2D BLE 2.13，并覆盖键盘、Consumer、配对和四主机
切换。AP2D 已取消 C18 的独立 LED MCU，因此 AP2D 的锁定灯、RGB 和 LED 输出
实现不属于本次 backport；C18 灯控保持原有板级路径。尚未由完整调用链或抓包
确认的 UART 命令必须保持实验状态。构建成功不能替代 radio、bond、HID 或
交叉刷写安全性的实机验证。
