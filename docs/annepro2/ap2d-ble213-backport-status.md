# C18 KEY 适配 AP2D BLE 2.13：实现状态

本页记录在完整调研报告之后新增的静态证据、QMK 实现和验证结果。原始报告保留
不变，以便继续通过其 `SHA256SUMS.txt` 复核。

## 新增静态证据

针对官方 AP2D KEY 3.08 的 ARM 反汇编复核得到：

- `0x9316` 构造 Consumer 上报，UART 业务字段仍为 `0x10/0x08`，数据长度为
  8 字节；四个槽位是小端 `uint16_t Usage`。
- `0x7DF4` 是 slot 动作发送路径。动作 1 构造 `0x40/0x01`，动作 2 构造
  `0x40/0x04`，两者都只发送一个 slot 字节。
- 同一路径在 slot 动作前发送 `0x20/0x0B` 或 `0x20/0x24` 状态消息。
- `0x82DE` 处理的 `0x21/0x22` 属于 AP2D 对象/厂商业务回调及四块 9 字节
  槽数据访问。它不是“AP2D 配对/连接 UART opcode”的充分证据。

因此首版继续使用已验证的 `0x40/0x01` 广播和 `0x40/0x04` 连接命令，不把
`0x21/0x22` 猜测为 BLE slot 命令。原 QMK 在 slot 字节后额外写出的一个
`0x00` 不计入帧头声明长度，也不出现在 3.08 构造器的数据长度中，现已移除。

## 已实现

QMK 分支：`codex/annepro2-ble213-backport`

基线后的首个 backport 提交：`d1b9d6df06 Add Anne Pro 2 BLE firmware profiles`

- 同一 C18 KEY 源码内置 `C18_BLE205` 和 `AP2D_BLE213` profile。
- Consumer 编码：
  - BLE 2.05：4 字节位图，补齐亮度增减两个原来遗漏的 bit。
  - BLE 2.13：8 字节、最多四个小端 16 位 Usage。
  - 无法表达的 2.05 Usage 不截断，发送 release-all 并在 debug 构建记录。
- EEPROM 使用 magic、版本和校验字节保存 profile 与 slot；旧的 `0..4` slot
  格式仍可读取并在下一次写入时迁移。
- 切换 profile 时清除自动连接 slot，避免把另一 BLE 模块的 bond slot
  当作当前模块的有效状态。
- 新增 `KC_AP2_BLE205`、`KC_AP2_BLE213` 维护键码。
- 增加无硬件依赖的 host 测试，覆盖 Consumer golden vectors、两种 LED
  payload、profile/slot 全组合、校验损坏和越界输入。

锁定灯的 1/2 字节 decoder 已实现并测试，但尚未接到 UART event handler。
AP2D KEY 的 `0x8426` 证明两种 HID Output 长度都被接受，却不足以单独确定
BLE→KEY UART 的外层 group/opcode；接入前仍需继续追完整调用链或抓包。

Vendor Report ID 2 的方向适配也保持关闭，直到业务 UART opcode 被确认。

## 构建

所有命令从 userspace 根目录通过 direnv 环境执行：

```sh
direnv exec . just annepro2
direnv exec . just annepro2-ble213
direnv exec . just annepro2-log
direnv exec . just annepro2-ble213-log
```

前两个分别把 BLE 2.05 和 BLE 2.13 设为“无有效 EEPROM 记录时”的默认
profile。已经保存的 profile 优先于构建默认值。

维护切换键位位于调试层：按住 `MO(9)`，再按住该层的 `Tab`（`MO(10)`），
然后按：

- `1`：保存 `C18_BLE205`；
- `2`：保存 `AP2D_BLE213`；
- `3`：切回 USB 路由。

profile 切换会清除自动连接 slot。随后应按目标 slot 重新连接或长按重新配对。

## 已完成的软件验证

- host 测试以 `-std=c11 -Wall -Wextra -Werror` 编译并通过。
- `annepro2`、`annepro2-ble213`、`annepro2-log`、
  `annepro2-ble213-log` 四种构建均通过。
- 普通构建为 44,164 字节；日志构建为 43,540 字节。日志构建更小是因为
  userspace 已按原约定关闭一组较大的 RGB 效果。

这些结果只证明编码器、持久化格式和 QMK 构建成立，不证明 BLE 2.13 已能在
C18 BLE 板安全启动，也不证明 radio、bond、锁定灯或四主机切换已通过硬件
验收。

## 下一步门禁

1. 继续追踪 BLE 2.13 LED Output 与 Vendor Report 的 UART 外层命令。
2. 在可恢复 BLE bootloader、information page、IEEE/RF 和 SNV 的条件下完成
   BLE 2.13 交叉刷写门禁。
3. 依次验证键盘 press/release、Consumer、锁定灯、清除配对、四个 slot 的
   广播/连接/超时/迟到事件。
4. 验证通过后再考虑把 BLE 2.13 profile 纳入上游 PR；在此之前它保持实验性。
