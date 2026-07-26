# BLE 2.13 开发问题与解决记录

本页记录 C18 KEY 适配、BLE IAP 工具和实机验证中已经遇到的问题。结论按
“静态证据、传输结果、运行结果”分层，避免把构建或 status-zero 回复误写成
射频与 flash readback 结论。

## IAP 地址不是镜像文件偏移

最初按“BLE 文件从 offset 0 开始”把 erase/write base 设为 `0`，传输停滞。
C18 IAP 的 firmware-layout 回复实际为 main、LED、BLE 都报告 `0x4000`。

解决：

- 工具先读取 layout，再选择 target base；
- 不再为 BLE 硬编码 `0` 或 main 默认值；
- `--base` 只用于诊断，若与设备报告值不一致就拒绝执行；
- 官方 `0x26000` 文件从 image offset 0 读取，但 transport 地址从
  `0x4000` 到 `0x29FE0`。

`0x4000` 是 KEY IAP 协议地址，不足以证明 CC254x 的物理擦除映射。

## 旧工具会把失败流程继续当作完成

旧版 flasher 对回复只做打印，写块失败后继续，缺少目标、命令、状态和超时
约束。BLE erase 较慢时也可能无界等待。

解决版 `hitsmaxft/AnnePro2-Tools@158aa04`：

- 回复必须匹配 destination/source、command 和 key；
- erase/write status 必须为零；
- 普通回复 5 秒、erase 30 秒超时；
- 任何错误立即中止并返回非零 exit status；
- `--probe` 只读显示 layout 和 mode；
- `--boot` 按 ObinsKit 行为向 main 发送 mode 2，不等待会消失的回复。

Nix 包装固定在 `hitsmaxft/nix-annepro2-tools@3a1602d`，userspace flake
再固定该提交，避免不同机器取得旧工具。

## status-zero 不是 flash readback

官方 BLE 2.13 的 erase 和 4,864 次 write 都收到匹配的 status-zero 回复，
随后固件启动、macOS 连接并完成普通键盘输入。这是强于“命令已发送”的运行
证据，但 IAP 没有已验证的读取命令。

因此分别记录：

- IAP 请求传输：通过；
- BLE 启动、连接、普通按键：通过；
- flash 逐字节 readback 与物理擦除边界：未验证。

## 命令 ACK 不是 BLE 已连接

`0x40/0x01` 或 `0x40/0x04` ACK 只证明 BLE 副控接受命令。旧状态机若在 ACK
后切换 QMK host driver，会把键盘报告发往尚未完成连接的 BLE 路由。

解决：

- ACK 只进入 handshake-wait 状态；
- 只有 BLE 发来的 `0x20/0x0C` HID-ready 才持久化 slot 并切换路由；
- 重复 HID-ready 可幂等应答，不重复切换；
- 一次实测从广播到 HID-ready 约 6.09 秒。

## 旧 Caps 结构既会误判又没有输出

旧 QMK 把所有 11-byte BLE RX 帧覆盖到末字节为 `caps_lock` 的结构，因此
`40/01`、`40/04` ACK 和 `20/0C` handshake 都可能误改该变量；与此同时
`host_driver.keyboard_leds` 固定返回 0，实际又不会把状态交给 QMK 灯控。

继续反汇编后得到可实现的共同契约：

- C18 KEY 2.36.3 的 `0xC45E–0xC4CC` 从标准 LED Output byte 提取 bit 1，
  归一化为 `0/1` 并构造 `20/07`；
- AP2D KEY 3.08 的 group `0x20`/opcode `0x07` 调用 Caps 状态函数；
- BLE 2.13 实机连接时已捕获 `20/07 00`。

解决：

- 只接受完整的 `7B 12 35 00 03 00 00 7D 20 07 00/01`；
- 错误长度、routing、group/opcode 和 value `>1` 不改变状态；
- 协议回复继续保留原 value；
- QMK BLE host driver 返回标准 Caps LED bit，新 route 前清零防止跨 host
  残留；
- `macvim` keymap 只调用 C18 既有外置 LED MCU sticky-key 命令，不移植
  AP2D 的 GPIO/RGB 实现；
- host 测试覆盖合法开关和所有上述拒绝路径，实体灯仍单独实测。

## 快速切槽会混入旧事务

UART ACK 没有 transaction ID，也没有可确认的 slot 字段。slot 2 后立即
slot 1 时，旧 ACK 或半帧可能落入新事务，表现为需要再按一次或等待超时。

解决：

- 新 slot 只保留最后一次意图；
- 立即停止旧重试并切回 USB route；
- 清空 UART 半帧；
- 等待 1 秒静默窗口，再发送新 slot 状态帧和主命令；
- connect timeout 只执行一次有界恢复，不无限广播。

这降低了旧事务污染，但协议本身仍无法给 ACK 增加 transaction ID，四槽快速
切换需要继续做实机压力测试。

## UART 半帧可能永久污染后续事件

早期 parser 只看 payload length 的低 8 位，且没有帧间超时。如果 UART 丢失
一部分字节，后续合法帧会被继续拼到旧缓冲；非零 length 高字节也可能被静默
截断。结果是 ACK 或 `0x20/0x0C` 明明到达，却无法被状态机识别。

解决：

- 独立出无 ChibiOS 依赖的流式 parser；
- 校验完整 24 位 length，高字节必须为零，payload 上限为 32 字节；
- 校验 header 的 `0x7D` delimiter；
- 20 ms 没有新字节就丢弃半帧，随后允许从 `0x7B` 重同步；
- 保留 payload 内部合法的 `0x7B`，不能在帧中间盲目重启；
- host 测试覆盖噪声、断帧、无效长度、错误 delimiter 和 32 位 timer 回绕。

## 把 C18 backport 链接进 C15 会耗尽 RAM

第一版把 parser、profile 和完整状态机放在 Anne Pro 2 公共 `rules.mk` 路径，
C15 链接时报：

```text
cannot move location counter backwards (from 20002030 to 20001ffc)
```

C15 的 RAM magic 位于 `0x20001FFC`，新增静态状态越过了可用 RAM 上界。这也
说明“源码能被两个型号共同看到”不等于“两个型号都应承担同一实现”。

解决：

- C18 的 BLE 2.05/2.13 适配移到
  `keyboards/annepro2/c18/annepro2_ble.c`；
- parser/profile/state 只由 `c18/rules.mk` 链接；
- C15 继续链接原 `annepro2_ble.c`，只补齐公共调用所需的无状态兼容 API；
- 增加 `just annepro2-c15` 门禁，当前 default 构建为 37,504 字节。

## 构建成功不能证明当前二进制通过实机

BLE 2.13 的启动、slot 1 配对、HID-ready 和普通输入来自状态机演进期间的实机
记录。之后增加的 parser 加固和 C18/C15 范围隔离虽然通过 host 测试与构建，
但生成的是新的精确二进制。

处理原则：

- 旧日志继续作为 UART 语义和方案方向的证据；
- 不把它写成当前 `e3dfb6829d` 固件已经完成硬件回归；
- 当前精确二进制仍需重测普通键盘、媒体、四槽和外部 LED MCU；
- 验收状态集中记录在
  [C18 KEY 双 BLE 首版验证矩阵](ble213-validation-matrix.md)。

## macOS 名称缓存会造成误判

刷入 2.13 后，系统设置曾同时保留旧的 `AnnePro2 / BLE-1.5.0` 未连接条目；
新连接详情实际显示 `HEXCORE AnnePro 2D / BLE-1.5.2`。

解决：

- 把 USB console 的广播、HID-ready、route 和 report 日志与 macOS
  “Connected”状态配对判断；
- 名称验证时先考虑 macOS 配对缓存与刷新延迟；
- 必要时删除旧配对后重新扫描，不能只看历史条目。

## USB console 只能由一个进程占用

已有 `qmk console` 运行时再启动一个实例会报无法打开设备，容易被误判为
固件 USB 故障。一次中断后的 wrapper 已退出，但实际 Python
`.qmk-wrapped console -t -d AC20:8009:1` 继续存活并独占 HID；只搜索
`just qmk console` 或 shell wrapper 会漏掉它。终止该精确进程后，同一 KEY
的 console 立即成功连接，证明问题来自监听器泄露而不是固件接口。

处理流程：

1. 用 `ps`/`pgrep` 同时搜索 `.qmk-wrapped.*console`、`hid_listen`、wrapper
   和 `just qmk console`；
2. 先确认 PID、启动时间和完整命令，只终止确定的遗留 listener，不使用宽泛
   `killall`；
3. 硬件测试优先直接运行
   `direnv exec . qmk console -t -d AC20:8009:1`，结束时发送 Ctrl-C；
4. 退出后再次搜索进程，确保 listener 没有残留。

随后修复 `scripts/qmk-worktree.sh`：实际 QMK 命令通过可追踪 PID 运行；
wrapper 的 `INT`、`TERM`、`HUP` trap 会先终止并回收该进程，再删除临时
worktree。实机使用 `just qmk console -d AC20:8009 -t` 连接成功后发送
Ctrl-C，wrapper 返回 130；复查没有 `.qmk-wrapped console`，也没有新增
缓存 worktree。相同 wrapper 的 BLE 2.13 正常构建随后通过。

console 日志证明 UART/QMK 路由和 report 提交，不单独证明无线主机已收到
按键。

debug 固件现在会在启动时打印 `QMK_GIT_HASH` 和可用的
`QMK_USERSPACE_VERSION`。USB 产品名不会随构建变化，不能用它判断硬件上是否
已经运行最新固件；后续日志必须先保存 revision 行，再关联测试结果。

为减少人工扫日志造成的误判，userspace 增加
`tools/reverse/annepro2/audit_console_log.py`。它归纳 build、slot、命令
ACK、command-to-route 延迟、keyboard/consumer/Caps 和 parser fault，并可
要求 QMK/userspace revision。显式指定的 revision 不存在时命令返回失败；
`OBSERVED` 仍只表示 KEY↔BLE UART/状态机路径，不能代替 macOS/RF 观察。

最近一次操作者确认 IAP 刷写后蓝牙正常使用；macOS 当前分类随后确认
`HEXCORE AnnePro 2D / BLE-1.5.2` 位于 `device_connected`。清理遗留
console 后当前 KEY 的日志接口也可打开，但因监听晚于启动且没有采集新的操作，
仍没有 revision 行或功能日志。这只能再次确认 BLE 2.13 方案可运行，不能
证明当前精确 KEY 或 2C 名称变体。处理原则是保持可用 BLE 不动，需要完整
回归时先用受控 console 取证；只有 revision 不符时才补刷日志 KEY。

## 2C 名称必须保持固定宽度

兼容模式名称定义为 `HEXCORE AnnePro 2C`。BLE 2.13 中广播模板原文是拼写
异常的 `HXECORE AnnePro 2D`，GAP 名称是 `HEXCORE AnnePro 2D`；两处都是
18 字节。

解决：

- 只在 `0x02E5` 和 `0x60FB` 做 18 字节等长替换；
- advertising length `0x13`、type `0x09` 和 GAP NUL 不变；
- 精确校验只有四个字节变化；
- 官方输入保持只读，生成产物不提交 Git。

这个 2C 镜像是可选显示变体，不属于“BLE 2.13 二进制保持原样”的核心
backport 验证。核心路径继续使用官方 SHA-256
`1b904ae9cd8bf6835c0b77c72618256b701a2c3b74dc04e9dddb8a388bdfc73d`。

## Nix 环境问题

反向工具曾依赖 host 上的 Rust、Node、Ghidra、LLVM 工具，导致 agent 与用户
终端结果不一致；旧 LLVM 12 LLDB 在 aarch64-darwin 也不可用，cargo vendor
聚合哈希流程还遇到 crates.io API 403。

解决：

- userspace flake 将实际用到的 Rust、Node/asar、压缩、Ghidra、Unicorn、
  cross-binutils 和当前 Darwin LLDB 标记为 agent-only 工具；
- 所有 QMK 与分析命令通过 `direnv exec .`；
- Nix flasher 使用本地 `Cargo.lock` 的逐依赖校验，不依赖旧聚合 vendor
  下载流程；
- 包装仓库通过四平台 flake 求值，并在 aarch64-darwin 实际构建。

为避免进入 IAP 后手工拼接 target、base 和文件路径，userspace 另提供：

- `just annepro2-iap-probe`：只读确认 layout/mode；
- `just annepro2-flash-ble213-official`：精确校验并只写 BLE target；
- `just annepro2-flash-ble213-2c`：从官方输入重建四字节名称变体后只写 BLE；
- `just annepro2-flash-key <artifact>`：同一 IAP 会话写 KEY 并最终重启。

BLE recipe 故意不自动 boot，这样 BLE 与匹配 KEY 可以在一次 IAP 会话内完成；
只有最后的 KEY recipe 发送 main mode 2。
