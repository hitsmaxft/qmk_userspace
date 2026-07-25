# C18 KEY 适配 AP2D BLE 2.13：实现状态

本页记录在完整调研报告之后新增的静态证据、QMK 实现和验证结果。原始报告保留
不变，以便继续通过其 `SHA256SUMS.txt` 复核。

## 新增静态证据

针对官方 AP2D KEY 3.08 的 ARM 反汇编复核得到：

- `0x9316` 构造 Consumer 上报，UART 业务字段仍为 `0x10/0x08`，数据长度为
  8 字节；四个槽位是小端 `uint16_t Usage`。
- `0x7DF4` 是 slot 动作发送路径。动作 1 构造 `0x40/0x01`，动作 2 构造
  `0x40/0x04`，两者都只发送一个 slot 字节。
- 同一路径在 slot 动作前发送两字节 payload：
  - 广播：`0x20/0x0B slot,1`；
  - 连接：`0x20/0x24 slot,2`。
- `0x82DE` 处理的 `0x21/0x22` 属于 AP2D 对象/厂商业务回调及四块 9 字节
  槽数据访问。它不是“AP2D 配对/连接 UART opcode”的充分证据。
- KEY 3.08 在 `0x138C0` 保存压缩初始化数据描述，固件自己的 `0x13700`
  解压器会恢复 RAM `0x20000000..0x20000473`。恢复后的
  `0x20000414` 是 11 项 UART 主组分发表：

```text
01 -> 99A1   02 -> C1FD   03 -> C27B   10 -> 7EA9
11 -> DADD   12 -> C259   20 -> AB79   30 -> 8B01
40 -> B115   50 -> B5B1   60 -> 800D
```

该表由 `tools/reverse/annepro2/recover_ap2d_data.py` 调用固件自身解压器恢复，
不是按相邻常量猜出的地址。

因此首版继续使用已验证的 `0x40/0x01` 广播和 `0x40/0x04` 连接命令，不把
`0x21/0x22` 猜测为 BLE slot 命令。原 QMK 在 slot 字节后额外写出的一个
`0x00` 不计入帧头声明长度，也不出现在 3.08 构造器的数据长度中，现已移除。

## 已实现

QMK 分支：`codex/annepro2-ble213-backport`

基线后的首个 backport 提交：`d1b9d6df06 Add Anne Pro 2 BLE firmware profiles`

slot 状态差异修正：`da28ab855d Match AP2D BLE slot state commands`

slot 事务顺序与 LED 范围修正：
`8109af3c33 Match AP2D slot transaction ordering`

可测试状态机与切槽隔离：
`a2e6e9585f Test Anne Pro 2 BLE slot state machine`

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
- slot 状态通知按 profile 编码：BLE 2.05 保留已实测的
  `0x20/0x0B slot,0/1`；BLE 2.13 使用 AP2D 3.08 汇编确认的
  `0x20/0x0B slot,1` 与 `0x20/0x24 slot,2`。状态通知先于主命令发送，仍只
  在动作边沿发送一次，不随 `0x40/0x01` 或 `0x40/0x04` 重试。
- 四槽事务已提取为无 QMK/ChibiOS 依赖的状态机。UART、EEPROM 和 host
  driver 只是执行状态机给出的动作，host 测试与固件使用同一份迁移逻辑。
- 快速切槽只保留最后一次意图。新意图入队时立即停止旧事务重试并切回 USB
  路由；等待 1 秒静默窗口后才发送新槽的状态帧和主命令。窗口内到达的旧
  ACK 因状态不匹配而丢弃。
- 命令 ACK 只进入握手等待，不能切换 QMK host driver；只有收到
  `0x20/0x0C` HID ready 握手后才保存槽位并切到 BLE。
- connect 握手超时只执行一次有界 wakeup + 启动广播恢复；第二次超时停止，
  不无限广播。用户主动长按广播继续遵守原决定，不自动超时回滚。
- unpair、显式 USB 切换和 profile 切换会清除 held/pending/retry/timeout
  状态；适配层同时清空 UART 半帧，避免旧 parser 数据进入下一次事务。
- 增加无硬件依赖的 host 测试，覆盖 Consumer release、1–4 个 Usage、
  golden vectors、slot 状态 golden vectors、profile/slot 全组合、校验损坏
  和越界输入；状态机测试覆盖启动恢复、tap/hold、命令重试、ACK/握手分离、
  快速切槽、超时恢复、解绑、状态清理和 32 位计时器回绕。

UART `0x40/0x01`、`0x40/0x04` ACK 不包含 KEY 侧 transaction ID 或可确认的
槽号。静默窗口能隔离通常的迟到 ACK，但无法从协议上证明在新命令发出后才
到达的同 opcode 旧 ACK 属于哪个事务。由于 ACK 本身不启用输入路由，风险已
收窄；最终仍须以四槽实机日志验证。

## 明确排除的 LED 路径

AP2D 取消了 C18 的独立 LED MCU，KEY MCU 直接驱动 RGB。`0x8426`、
`0xBE1A`、`0xBE5A`、`0xBE60` 属于 AP2D 自身的 HID Output/RGB 状态实现，
不能作为 C18 LED MCU 的替代代码直接回移。按当前范围：

- 不移植 AP2D 的锁定灯、RGB、LED Output callback 或 suspend 灯控；
- 不在 BLE UART parser 中猜测 LED group/opcode；
- C18 原有 LED MCU 与板级实现保持不变；
- 已删除曾经隔离实现、但未接入 UART 的 1/2 字节 LED decoder 和测试。

旧 QMK 的 `ble_capslock_t` 会把任意 11 字节 RX 帧的末字节当作 Caps 状态，
容易被命令 ACK 和握手帧污染，而且 host driver 实际始终返回 0；该伪兼容
路径仍保持删除。debug 构建继续记录每个完整 RX 帧，供未来独立研究使用。

Vendor Report ID 2 的方向适配也保持关闭，直到业务 UART opcode 被确认。

## 构建

所有命令从 userspace 根目录通过 direnv 环境执行：

```sh
direnv exec . just annepro2-test
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
- 当前普通构建为 43,736 字节；日志构建为 42,532 字节。日志构建更小是因为
  userspace 已按原约定关闭一组较大的 RGB 效果。

这些结果只证明编码器、持久化格式和 QMK 构建成立，不证明 BLE 2.13 已能在
C18 BLE 板安全启动，也不证明 radio、bond 或四主机切换已通过硬件
验收。

## 下一步门禁

1. 按 [BLE 2.13 交叉刷写门禁](ble213-crossflash-gate.md) 使用 CC254x
   调试接口保存 256 KiB 全 flash 和 2 KiB Information Page，并验证两次读取
   哈希一致。当前 `annepro2_tools` 的 BLE 路径没有严格 ACK、readback 或
   erase 边界证明，禁止直接使用。
2. 依次验证键盘 press/release、Consumer、清除配对、四个 slot 的
   广播/连接/超时/迟到事件。
3. Vendor Report ID 2 的方向适配保持关闭，直到业务 UART opcode 被确认。
4. 验证通过后再考虑把 BLE 2.13 profile 纳入上游 PR；在此之前它保持实验性。
