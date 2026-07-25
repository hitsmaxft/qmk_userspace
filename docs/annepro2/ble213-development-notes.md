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

解决版 `hitsmaxft/AnnePro2-Tools@3a0b490`：

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
固件 USB 故障。

解决：继续读取原 console 会话；需要重启时先明确结束旧进程。console 日志
证明 UART/QMK 路由和 report 提交，不单独证明无线主机已收到按键。

## 2C 名称必须保持固定宽度

兼容模式名称定义为 `HEXCORE AnnePro 2C`。BLE 2.13 中广播模板原文是拼写
异常的 `HXECORE AnnePro 2D`，GAP 名称是 `HEXCORE AnnePro 2D`；两处都是
18 字节。

解决：

- 只在 `0x02E5` 和 `0x60FB` 做 18 字节等长替换；
- advertising length `0x13`、type `0x09` 和 GAP NUL 不变；
- 精确校验只有四个字节变化；
- 官方输入保持只读，生成产物不提交 Git。

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
