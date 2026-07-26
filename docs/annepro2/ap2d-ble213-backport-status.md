# C18 KEY 适配 AP2D BLE 2.13：实现状态

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
`0x00` 不计入帧头声明长度，也不出现在 3.08 构造器的数据长度中，现已移除。

## 已实现

QMK 分支：`codex/annepro2-ble213-backport`

相对 QMK `origin/master` 的当前提交序列：

- `9a11173bf8 Harden Anne Pro 2 BLE connection state handling`
- `27d55e7b52 Add Anne Pro 2 C18 BLE firmware profiles`
- `7d3d33a8aa Extract and test Anne Pro 2 BLE state parsing`
- `7c344d4e7d Scope the BLE backport to Anne Pro 2 C18`
- `e3dfb6829d Handle Anne Pro 2 BLE Caps Lock state`

- 同一 C18 KEY 源码内置 `C18_BLE205` 和 `AP2D_BLE213` profile。
- 完整 BLE 2.05/2.13 实现只由 C18 的 `rules.mk` 链接；C15 继续使用原
  `annepro2_ble.c`，不会承担 parser/profile/state 的 RAM 开销。
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
- UART parser 校验 24 位 payload length 的高字节、32 字节 payload 上限和
  `0x7D` delimiter；20 ms 内没有补齐的半帧会过期，下一帧可从 `0x7B`
  重新同步。payload 内部的 `0x7B` 不会被误认为新帧。
- C18 KEY 2.36.3 与 AP2D KEY 3.08 静态路径共同确认 `0x20/0x07` 的 value
  是 Caps Lock 布尔状态。driver 只接受完整精确帧和 `0/1`，映射到 QMK
  host LED bit 1；任意 value 的原值协议回复继续保留，新 route 前清除旧状态。
- 增加无硬件依赖的 host 测试，覆盖 Consumer release、1–4 个 Usage、
  golden vectors、slot 状态 golden vectors、profile/slot 全组合、校验损坏
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
direnv exec . just annepro2-ble213
direnv exec . just annepro2-c15
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
- `annepro2` 与 `annepro2-ble213` 均通过，当前两个普通构建均为
  43,996 字节；二者只改变无有效 EEPROM 记录时的默认 profile。
- C15 default 继续链接原 BLE 驱动并构建通过，大小为 37,504 字节。
- C18 的 `ap2_led.*`、`protocol.*`、`rgb_driver.*` 与 QMK 分支基线逐字节
  无差异。
- 新增/修改的 C18、parser、profile、state 和 host test 文件通过
  `clang-format --dry-run --Werror`；两个仓库通过 `git diff --check`。
- debug 固件启动时输出 `QMK_GIT_HASH` 与可用的
  `QMK_USERSPACE_VERSION`，后续实机日志可以绑定到实际构建来源；普通固件不
  包含这些字符串，大小与哈希不受影响。
- `qmk lint` 仍报告上游已有的 license header 和带连字符 keymap 名称问题；
  报告涉及的文件/名称不在本分支差异中。

完整命令、构建哈希与逐需求证据见
[C18 KEY 双 BLE 首版验证矩阵](ble213-validation-matrix.md)。

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

这些结果证明 BLE 2.13 已在目标板上完成启动、连接和普通键盘输入。IAP
协议没有可用的 flash readback，所以 status-zero 传输不能解释成逐字节写回
验证；媒体键、完整四槽和外部 LED MCU 回归也仍需单独验收。`7c344d4e7d`
debug 固件后来已重新刷入，并记录到 BLE 2.13 的 `20/07 00`、HID-ready 和
ACTIVE route；但新加入 Caps 状态桥接的 `e3dfb6829d` 尚未刷入，不能把旧记录
写成当前精确二进制的锁定灯验证。

核心 backport 的实机使用官方 BLE 2.13 原始镜像。另行提供的
`HEXCORE AnnePro 2C` 固定宽度名称变体只用于兼容模式显示，不参与上述结论，
也不覆盖官方镜像。

## 下一步门禁

1. 依次验证 Consumer、清除配对、四个 slot 的
   广播/连接/超时/迟到事件。
2. 用当前精确日志固件重新验证普通键盘，并换回 BLE 2.05 重复全部用例。
3. 刷入 `e3dfb6829d` 日志固件，验证 `20/07 00/01`、QMK Caps bit 与 C18
   外置 LED MCU 的实体灯位一致；不移植 AP2D 直驱 LED/RGB。
4. 若要把刷写从“status-zero 传输”提升到完整验证，仍需 CC254x 调试接口保存
   256 KiB flash 与 Information Page，并完成写后 readback。
5. Vendor Report ID 2 的方向适配保持关闭，直到业务 UART opcode 被确认。
6. 验证通过后再考虑把 BLE 2.13 profile 纳入上游 PR；在此之前它保持实验性。
