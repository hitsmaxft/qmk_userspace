# C18D 固定适配 AP2D BLE 2.13：实现状态

本页记录完整调研报告之后新增的静态证据、QMK 实现和验证结果。调研报告中的
范围修正与实施文件一并更新，并通过其 `SHA256SUMS.txt` 复核。

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
`0x00` 不计入帧头声明长度，也不出现在 AP2D KEY 3.08 构造器的数据长度中；
因此 C18D BLE 2.13 目标不发送它。C18 BLE 2.05 目标继续保留该历史多配对
兼容字节；两套完整物理帧由互斥的型号协议对象分别生成和测试。

## 已实现

QMK 分支：`codex/annepro2-ble213-backport`

相对 QMK `origin/master` 的当前提交序列：

- `9a11173bf8 Harden Anne Pro 2 BLE connection state handling`
- `27d55e7b52 Add Anne Pro 2 C18 BLE firmware profiles`
- `7d3d33a8aa Extract and test Anne Pro 2 BLE state parsing`
- `7c344d4e7d Scope the BLE backport to Anne Pro 2 C18`
- `e3dfb6829d Handle Anne Pro 2 BLE Caps Lock state`
- `01e6d3f18d Add Anne Pro 2 BLE status diagnostics`
- `11c08ff0dc Fix Anne Pro 2D BLE 2.13 slot selection`

- `annepro2/c18` 固定链接 BLE 2.05 协议对象；`annepro2/c18d` 固定链接 BLE
  2.13 协议对象。C2D 复用 C18D 的 2.13 协议，C15 继续使用原
  `annepro2_ble.c`。
- 公共层只包含 UART parser、连接事务状态机、HID transport 和 slot-only
  EEPROM codec。公共 API 中没有 BLE profile enum、get/set 或运行时分支。
- EEPROM 只保存带型号 tag 的最后成功 slot。C18、C18D、C2D 的 tag 不同，
  不能跨型号读取；旧双-profile EEPROM 记录在首次启动时失效并被重写为无 slot。
- Consumer 编码：
  - BLE 2.05：4 字节位图，补齐亮度增减两个原来遗漏的 bit。
  - BLE 2.13：8 字节、最多四个小端 16 位 Usage。
  - 无法表达的 2.05 Usage 不截断，发送 release-all 并在 debug 构建记录。
- 已删除 `KC_AP2_BLE205`、`KC_AP2_BLE213` 以及构建环境中的 profile 选择；
  型号是唯一协议选择边界。
- slot 状态通知按型号编码：BLE 2.05 保留已实测的
  `0x20/0x0B slot,0/1`；BLE 2.13 使用 AP2D 3.08 汇编确认的
  `0x20/0x0B slot,1` 与 `0x20/0x24 slot,2`。状态通知先于主命令发送，仍只
  在动作边沿发送一次，不随 `0x40/0x01` 或 `0x40/0x04` 重试。
- 新恢复的 AP2D 3.08 slot 选择前导只编译进 C18D/C2D：
  `c0/17` 查询；不同槽时发送 `40/17 slot`、`02/01 01`、等待 20 ms、
  `02/01 02`、再等待 20 ms，最后才投递已有状态帧和主命令。BLE 2.05
  不进入此状态机，公共 BLE state 实现未改动。
- 四槽事务已提取为无 QMK/ChibiOS 依赖的状态机。UART、EEPROM 和 host
  driver 只是执行状态机给出的动作，host 测试与固件使用同一份迁移逻辑。
- 快速切槽只保留最后一次意图。新意图入队时立即停止旧事务重试并切回 USB
  路由；等待 1 秒静默窗口后才发送新槽的状态帧和主命令。窗口内到达的旧
  ACK 因状态不匹配而丢弃。
- 命令 ACK 只进入握手等待，不能切换 QMK host driver；只有收到
  `0x20/0x0C` HID ready 握手后才保存槽位并切到 BLE。
- connect 握手超时只执行一次有界 wakeup + 启动广播恢复；第二次超时停止，
  不无限广播。用户主动长按广播继续遵守原决定，不自动超时回滚。
- unpair 和显式 USB 切换会清除 held/pending/retry/timeout
  状态；适配层同时清空 UART 半帧，避免旧 parser 数据进入下一次事务。
- UART parser 校验 24 位 payload length 的高字节、32 字节 payload 上限和
  `0x7D` delimiter；20 ms 内没有补齐的半帧会过期，下一帧可从 `0x7B`
  重新同步。payload 内部的 `0x7B` 不会被误认为新帧。
- C18 KEY 2.36.3 与 AP2D KEY 3.08 静态路径共同确认 `0x20/0x07` 的 value
  是 Caps Lock 布尔状态。driver 只接受完整精确帧和 `0/1`，映射到 QMK
  host LED bit 1；任意 value 的原值协议回复继续保留，新 route 前清除旧状态。
- 增加无硬件依赖的 host 测试，覆盖 Consumer release、1–4 个 Usage、
  golden vectors、slot 状态 golden vectors、型号隔离、slot 全组合、校验损坏
  和越界输入；状态机测试覆盖启动恢复、tap/hold、命令重试、ACK/握手分离、
  四个 slot、快速切槽、超时恢复、解绑、状态清理和 32 位计时器回绕；
  parser 测试覆盖完整帧、噪声、断帧、无效长度/delimiter 和计时器回绕。

UART `0x40/0x01`、`0x40/0x04` ACK 不包含 KEY 侧 transaction ID 或可确认的
槽号。静默窗口能隔离通常的迟到 ACK，但无法从协议上证明在新命令发出后才
到达的同 opcode 旧 ACK 属于哪个事务。由于 ACK 本身不启用输入路由，风险已
收窄；最终仍须以四槽实机日志验证。

## C18 锁定灯边界

AP2D 取消了 C18 的独立 LED MCU，KEY MCU 直接驱动 RGB。`0x8426`、
`0xBE1A`、`0xBE5A`、`0xBE60` 属于 AP2D 自身的 HID Output/RGB 状态实现，
不能作为 C18 LED MCU 的替代代码直接回移。按当前范围：

- 不移植 AP2D 的 RGB、GPIO、LED Output callback 或 suspend 灯控；
- 只采用两代固件共同确认的逻辑 ABI：`20/07 00/01` 是 Caps 状态；
- C18 原有 LED MCU 与板级实现保持不变；
- QMK driver 通过标准 `host_driver.keyboard_leds` 暴露 Caps bit，当前
  `macvim` keymap 再调用 C18 既有 `sticky key` 命令显示物理灯位。

旧 QMK 的 `ble_capslock_t` 会把任意 11 字节 RX 帧的末字节当作 Caps 状态，
容易被命令 ACK 和握手帧污染，而且 host driver 实际始终返回 0；该伪兼容
路径仍保持删除。新的严格 decoder 有 host 测试覆盖合法开/关、错误长度、
routing、group/opcode、非法 value 和空指针。

Vendor Report ID 2 的方向适配也保持关闭，直到业务 UART opcode 被确认。

## 构建

所有命令从 userspace 根目录通过 direnv 环境执行：

```sh
direnv exec . just annepro2-test
direnv exec . just annepro2
direnv exec . just annepro2-c18d
direnv exec . just annepro2-c15
direnv exec . just annepro2-log
direnv exec . just annepro2-c18d-log
```

`annepro2_c18_macvim.bin` 与 `annepro2_c18d_macvim.bin` 是不同目标产物，不再
复制版本专用别名。调试层只保留显式 USB 路由键，不含协议切换键。

## 已完成的软件验证

- host 测试以 `-std=c11 -Wall -Wextra -Werror` 编译并通过。
- `annepro2`（C18）构建通过，二进制 46,824 字节；`annepro2-c18d` 构建通过，
  二进制 47,360 字节。构建依赖分别只包含 `c18/annepro2_ble_protocol.c` 或
  `c18d/annepro2_ble_protocol.c` 与 2.13 slot 状态机。
- C15 default 继续链接原 BLE 驱动并构建通过，二进制 37,508 字节；C2D
  复用固定 2.13 协议并构建为 25,448 字节。
- C18 与 C18D 的型号专用 Mouse `0x60/0x04 + 8B` 编码、全零 release、
  能力限制和裁剪 host 测试通过；两型号的 QMK 鼠标键仍需实机验证。
- C18 的 `ap2_led.*`、`protocol.*`、`rgb_driver.*` 与 QMK 分支基线逐字节
  无差异。
- 新增/修改的 C18/C18D、parser、protocol、slot config、state 和 host test 文件通过
  `clang-format --dry-run --Werror`；两个仓库通过 `git diff --check`。
- debug 固件启动时输出 `QMK_GIT_HASH` 与可用的
  `QMK_USERSPACE_VERSION`，并在 2 秒后只重复一次以跨过 USB console
  重新枚举窗口；后续实机日志可以绑定到实际构建来源。
- 可选 BLE 状态回调向 keymap 报告 advertising、connecting、connected 和
  有界恢复失败。C18 `macvim` 只调用现有外置 LED MCU API，在物理
  slot 1–4 上显示蓝/黄/红提示；没有移植 AP2D 直驱 LED/RGB。
- `qmk lint` 对 `annepro2/c18`、`annepro2/c18d` 和 `annepro2/c2d` 全部通过。

完整命令、构建哈希与逐需求证据见
[C18/C18D 固定协议验证矩阵](ble213-validation-matrix.md)。

## 已完成的实机验证

- 修复版 `annepro2-tools` 从 C18 IAP 读取到 BLE transport base `0x4000`，
  完成 erase 和 4,864 个 32 字节 write；每条回复都匹配
  target/command/key 且 status 为零。
- 官方 AP2D BLE 2.13 镜像在 C18 上启动。
- slot 1 广播后，USB console 收到 `0x20/0x0C` HID-ready；QMK 只在该事件
  后切换到 BLE route。命令 ACK 没有被误当作连接成功。
- 从广播到 HID-ready 的一次样本延迟约 6.09 秒。
- macOS 显示 `HEXCORE AnnePro 2D`、`BLE-1.5.2` 且已连接；用户确认普通
  键盘输入正常。旧的 `AnnePro2 / BLE-1.5.0` 条目来自 macOS 配对缓存。

这些结果证明 BLE 2.13 已在目标板上完成启动和连接。2026-07-27 又刷入
userspace `378e305896` / QMK `01e6d3f18d` 的正式 BLE 2.13 KEY；拔除 USB
后，操作者准确输入 `ap2ble-1234567890-qwerty`，确认音量加、音量减和静音，
并以两次 Caps 操作得到 `ABCabc`，C18 外置 LED MCU 的红灯同步亮灭。因此
当前正式版的普通键、三种实际媒体键、Caps 主机状态和实体锁定灯通过实机验证。

IAP 协议没有可用的 flash readback，所以 status-zero 传输仍不能解释成逐字节
写回验证。随后四槽回归发现 slot 1 可以加密并输入，而 slot 2–4 能连接后
立刻在 SMP 失败。macOS 记录显示三者分别使用地址尾字节 `F9/FA/FB`，但
`enc-state: OFF`，并由 peer 返回 pairing failed reason 4/status 4805。
重新反汇编发现先前只移植了 slot 最终命令，遗漏 AP2D 3.08 的
query/select/prepare 前导事务；代码已按 2.13-only 边界补齐。2026-07-28，
操作者清空四个 slot 的 bond 后，分别与四台主机重新配对成功，并完成快速
交叉切槽、连接超时和压力测试。BLE 2.05 回归仍需单独验收。之前没有
revision 的日志只保留为 UART 协议证据，不覆盖当前精确 debug 镜像。

核心 backport 的实机使用官方 BLE 2.13 原始镜像。另行提供的
`HEXCORE AnnePro 2C` 固定宽度名称变体只用于兼容模式显示，不参与上述结论，
也不覆盖官方镜像。

2026-07-28，操作者在上述逐项证据基础上明确认定 C18 KEY 对官方 BLE 2.13
的 backport 完全验证通过，范围包括普通键盘、媒体键、Caps/实体锁定灯、
四槽配对、四主机切换、连接超时、压力测试和断电恢复。缺少启动 revision
行只影响精确构建归档，不再阻塞 2.13 功能验收。

## 下一步门禁

1. 用 C18 + BLE 2.05 重复普通键盘、媒体、Caps、配对、四槽和断电恢复；确认
   日志与二进制中都不存在 2.13 slot 前导。
2. 用 C18D + BLE 2.13 重复同一矩阵并保存 build revision；拆分前 C18 实机
   结果不能直接归属新的 C18D 二进制。
3. 若要把刷写从“status-zero 传输”提升到完整验证，仍需 CC254x 调试接口保存
   256 KiB flash 与 Information Page，并完成写后 readback。
4. Vendor Report ID 2 的方向适配保持关闭，直到业务 UART opcode 被确认。
