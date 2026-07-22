# Anne Pro 2 文档索引

- [BLE 固件与 UART 协议](ble-firmware-and-uart-protocol.md)：官方主控/CC254x
  镜像布局、UART framing、`20/0c` 握手请求/回复与未确认边界。
- [QMK BLE 可靠性修复](ble-reliability-pr.md)：旧 200 ms 补丁为何撤回、
  event-driven host driver 切换和上游 PR 草案。
- [USB Console 验证](ble-usb-console-validation.md)：debug 构建、日志字段、
  handshake/ACK/断链验证矩阵。

当前最重要的未完成项是从 BLE 固件或 PA5 抓包确认断链 counterpart，以及确认
Caps Lock 回包的精确 group/opcode。构建成功不能替代 radio、bond 或 HID 实机
验证。
