# Anne Pro 2 BLE 可靠性修复：QMK 上游 PR 说明

本文是 [`keyboards/annepro2`](../../modules/qmk_firmware/keyboards/annepro2/) 的 BLE 可靠性修复说明和未来上游 PR 的提交依据。当前实现位于本仓库固定的 `annepro2-upstream` QMK fork 分支，而非 userspace 补丁。它只描述 QMK 端可由源码确认的问题与改动；闭源 BLE 固件的连接完成回包仍未还原，因此本文不把固定延时称为连接成功确认。

相关协议证据见 [BLE 固件与 UART 协议分析](ble-firmware-and-uart-protocol.md)。

## 问题与影响

Anne Pro 2 的 HT32 主控通过 115200 bit/s UART 控制闭源 BLE 模块。原有移植代码在 `annepro2_ble.c` 中直接切换 QMK 的 host driver，并将一条逻辑 UART 帧拆成多次 `sdPut()` / `sdWrite()` 调用；`annepro2.c` 则在矩阵扫描中以 10 ms 超时读取 11 字节状态记录。

下表区分源码可直接确认的缺陷与合理但尚未由实机抓包证实的影响。

| 项目 | 源码事实 | 可能表现 | 修复 |
| --- | --- | --- | --- |
| 连接时机 | `annepro2_ble_connect()` 写出 connect 命令后立即将 host driver 切至 BLE | BLE 模块尚未完成连接时，后续 HID 报告可能过早送出 | 发送命令后保留原 driver 200 ms，再在扫描循环中切换 |
| TX 帧发送 | broadcast、connect、键盘 HID、consumer HID 都由多次串口调用组成 | 若未来存在并发发送者或底层传输点变化，逻辑帧可能被交错 | 每个逻辑帧合并为一次 `sdWrite()` |
| slot 状态 | 已选 slot 是函数内静态值，USB 切换或解绑不清除它 | 后续同 slot 按键可能误走“再次按下即 connect”的分支 | 将 slot 状态提升为模块状态，并在断开/解绑时清除 |
| BLE RX | 只要 RX 非空，扫描循环就调用 `sdReadTimeout(..., 11, 10)` | 缓冲区仅有部分记录时，单次矩阵扫描可阻塞至 10 ms | 逐字节非阻塞排空，完整 11 字节后一次性发布状态 |

第一项是最接近“连接不可靠”反馈的 QMK 侧证据，但它不能证明 BLE 固件没有其他问题。BLE 模块没有在现有移植代码中被解析的 connect-complete / disconnect UART 事件，因此端到端可靠性不能仅由此补丁保证。

## 当前分支行为

固定的 QMK fork 分支直接包含该实现，涉及四个文件：

- [`annepro2_ble.c`](../../modules/qmk_firmware/keyboards/annepro2/annepro2_ble.c)
- [`annepro2_ble.h`](../../modules/qmk_firmware/keyboards/annepro2/annepro2_ble.h)
- [`annepro2.c`](../../modules/qmk_firmware/keyboards/annepro2/annepro2.c)
- [`annepro2.h`](../../modules/qmk_firmware/keyboards/annepro2/annepro2.h)

### 连接与 host driver

当用户对当前 slot 再次执行连接时，主控先写完整的 12-byte connect 帧，再记录 `timer_read32()`。`matrix_scan_kb()` 调用 `annepro2_ble_task()`；超过 `ANNEPRO2_BLE_CONNECT_GUARD_MS`（默认 200 ms）后才安装 BLE host driver。

这段保护期的目的只是确保 UART 命令已进入 BLE 模块处理路径，且避免刚切换 host driver 时立刻转发 HID 报告。它不是 radio 建链状态机，也不会重试命令或确认链路成功。宏可在键盘配置中覆盖，便于实机测量后调整。

切换至不同 slot 会先执行本地 disconnect：取消尚未完成的 driver 切换、释放按键状态、恢复原 host driver（若当前在 BLE）。解绑同样清除本地状态。这里没有虚构一个 BLE 侧 disconnect 命令，因为现有协议证据中没有已确认的相应帧。

### UART 原子逻辑帧

以下发送路径现在各使用一次 `sdWrite()`：

- broadcast 与 connect：10-byte 命令头 + slot + 固定尾字节；
- boot keyboard：前导 `00` + 10-byte header + `KEYBOARD_REPORT_SIZE` payload；
- consumer：前导 `00` + 10-byte header + 4-byte payload。

字节序列与原代码相同。此改动不依赖或定义 BLE 协议字段的额外语义，只缩小主控侧帧边界被拆开的机会。

### 非阻塞状态接收

BLE 回传仍按已知的 `ble_capslock_t` ABI 处理：11 个字节，最后一个字节是 Caps Lock 状态；前 10 字节保持不解释。代码用私有缓冲累计 11 字节，再复制到公开的 `ble_capslock`，因此其他代码不会观察到混合的新旧记录。

这不是完整的 UART parser。若逻辑分析仪证明 BLE RX 含有变长消息、帧头或校验，下一版应以真实 framing 替换固定 11-byte 累积规则。

## 保持不变的兼容性边界

- PA4/PA5、115200 8N1、所有已知 UART 字节序列均未改变。
- 仍使用 QMK 的既有 `host_set_driver()` 模式；没有改动 QMK core 或 ChibiOS serial driver。
- 不新增官方 BLE 固件、反编译代码或专有 BLE 协议栈内容。
- 不声称修复 BLE radio、bond 数据、天线、电源或 CC254x 固件中的问题。

## 已完成的本地验证

在本仓库的 Nix/direnv QMK 环境中执行：

```sh
git -C modules/qmk_firmware diff --check
direnv exec . just annepro2
```

结果：两项均通过；`annepro2/c18:macvim` 成功链接并生成固件。链接阶段只有工具链的 LTO 串行化提示，无编译错误。

这只证明 C 代码可构建，不证明键盘可枚举、UART 字节已被 BLE 模块接受，或蓝牙连接得到改善。

## 上游 PR 前的实机验证矩阵

提交到 QMK 前，应在 C18 实机、每个已配对 host 上至少执行下列测试，并保留串口抓包或可复现日志：

| 场景 | 操作 | 预期观察 |
| --- | --- | --- |
| 已配对 slot 连接 | 连按同一 BT slot 两次 | 第二次发出 connect；约 200 ms 后开始经 BLE driver 发送 HID；可稳定输入 |
| 切换 slot | 已在 BLE 输入时选择另一 slot | 旧 driver 的按键先被释放；新 slot 的 broadcast 不携带旧 slot 的 HID 报告 |
| USB 返回 | BLE 模式下触发 `KC_AP2_USB` | 恢复先前 host driver，且下一次选择同 slot 会重新经历 broadcast/连接流程 |
| 解绑 | 在 BLE/USB 两种模式各执行一次 `KC_AP2_BT_UNPAIR` | UART 发出既有 unpair 帧，本地状态复位；重新配对流程正常 |
| 键盘与 consumer | 连续按键、快速修饰键组合、音量/媒体键 | PA4 上的每条逻辑帧字节序列与补丁前相同，且没有帧内交错 |
| Caps Lock 回传 | 多次切换 host Caps Lock | PA5 的每个 11-byte 记录只在完整到达后更新 LED 状态；矩阵扫描无可感知卡顿 |
| 长时稳定性 | 每个 slot 反复连接/断开至少数十次 | 记录连接耗时、失败次数、首次按键丢失和是否需要重试 |

应同步抓取 PA4（主控 TX）和 PA5（主控 RX），以 115200 8N1 解码。若发现真正的连接完成事件，应以事件驱动的 `connected` / `failed` / `timeout` 状态机替换 200 ms 保护期；这是比调大固定延时更适合上游的最终设计。

## 建议的上游提交形态

1. 基于 QMK 上游当前 `master` 单独重放此补丁；不要把本 userspace 的 keymap、Nix 环境或闭源固件分析一起提交。
2. 保持 PR 为一个 Anne Pro 2 专用 bugfix，标题可用：`[Keyboards] Anne Pro 2: avoid blocking BLE UART reads and premature host switching`。
3. PR 描述中明确列出“构建已验证”和“硬件已验证”的范围；若没有 PA4/PA5 抓包，不要声明修复已被量化证明。
4. 附上本文件中已完成的测试矩阵结果、固件版本、主机操作系统和复现频率。
5. 若维护者不接受固定 200 ms，保留无争议的单帧 UART 写入、RX 非阻塞化与状态复位；把 driver 延迟拆为后续、由实际 UART 状态机支撑的 PR。

## PR 描述草案

```markdown
## Summary

Fix Anne Pro 2's keyboard-side BLE UART handling:

- emit each known BLE control/HID message with one serial write;
- avoid the 10 ms blocking BLE RX read from `matrix_scan_kb()`;
- reset local slot/connection state on USB switch and unpair;
- defer the BLE host driver briefly after a connect command so reports are not
  forwarded immediately.

## Testing

- `direnv exec . just annepro2` (`annepro2/c18:macvim`)
- `git diff --check`
- Hardware: `<fill in model, BLE firmware version, host OS, slot test results>`

The delay is explicitly a UART command-settle guard, not a decoded BLE
connection-complete indication. A future revision can replace it with a
state machine once the module's RX event framing is captured.
```
