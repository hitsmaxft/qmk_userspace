# Anne Pro 2 BLE 固件与键盘侧 UART 协议

本文记录对官方 Anne Pro 2 C18 固件和 QMK AnnePro2 驱动的静态分析结果，用于清洁室（clean-room）实现 BLE 模块。结论分为三类：**已确认（固件）**、**已确认（QMK）**、**推断**。未标为已确认的字段不能作为互操作性保证。

> 本文不复制官方固件代码，也不建议把官方 BLE 映像或 TI BLE 协议栈反编译结果直接并入新的实现。

QMK 键盘侧可靠性补丁、验证边界与上游 PR 模板见 [BLE 可靠性修复说明](ble-reliability-pr.md)。

## 固件样本与映像布局

| 映像 | 路径 | 大小 | SHA-256 | 静态结论 |
| --- | --- | ---: | --- | --- |
| 键盘主控 | [`key-c18-2.36.3.bin`](../../assets/ap2_fw/key-c18-2.36.3.bin) | 46,779 B (`0xb6bb`) | `b9bfa750e8c7ccdbed0c1f6de8aeb4eb1d569dc5cc075afdb2a93fe5e20730de` | 裸 Cortex-M0 映像；初始 SP `0x20001db0`，复位向量 `0x0000f479`。 |
| BLE 旧版 | [`ble-c18-1.00.bin`](../../assets/ap2_fw/ble-c18-1.00.bin) | 155,648 B (`0x26000`) | `48389584acaecc90b7ae012760dc6048ee9eb72b254c4619461d2eaf672ea123` | 8051 系列原始 flash 映像，尾部以 `0xff` 填充。 |
| BLE alpha | [`ble-c18-2.00-alpha.bin`](../../assets/ap2_fw/ble-c18-2.00-alpha.bin) | 155,648 B (`0x26000`) | `d7547d8cfd7b05685539b6d6eef2d2a0ae88d8fba9a775b740766e32b9f86ba6` | 同为 8051 系列原始 flash 映像，尾部以 `0xff` 填充。 |

### BLE SoC

**已确认（固件）**：alpha 映像从 `0x0000` 起以 8051 `LJMP` 向量表开始，复位跳转目标为 `0x2529`；代码中还可见跳转至 `0x3ad2`、`0x3f25` 等中断入口。alpha 映像在 `0x1b219` 写入 SFR `0x86`，在 `0x1f84` 写入 SFR `0xc1`。

**已确认（芯片文档）**：TI CC254x 的 UART0 寄存器图中 `U0CSR=0x86`、`U0DBUF=0xc1`；CC2541 是带两个 UART 的 8051 BLE SoC，提供 128/256 KB flash。见 [CC2541 产品页](https://www.ti.com/product/CC2541)、[CC2541 datasheet](https://www.ti.com/lit/ds/symlink/cc2541.pdf) 与 [CC253x/CC254x 用户指南](https://www.ti.com/lit/ug/swru191f/swru191f.pdf)。

**结论（推断，置信度高）**：BLE 板使用 CC254x 兼容 SoC，且很可能是 CC2541F256 一类器件。由于 CC2540/CC2541 等器件具有相近的 8051 与寄存器特征，尚不能仅凭映像确定精确型号；实现和量产前应读取 PCB 丝印或调试接口芯片 ID。

两个 BLE 映像只有约 28,377/155,648 字节（18.2%）在同一偏移相同，说明两次构建发生了大量重排或重链接；不能把原始二进制 diff 直接解释为功能变更。可见字符串包括 `AnnePro2 P1` 至 `P4`、`HID Keyboard`、`Hardware Revision`、`Software Revision`、`Manufacturer Name` 等，强烈指示四个 profile 与标准 HID/DIS GATT 服务；字符串本身不足以还原 GATT handle、具体行为或版本差异。

## 键盘主控与 BLE 模块的物理链路

**已确认（QMK）**：C18 主控使用 UART 与 BLE 模块连接：

| 方向 | 主控引脚 | 宏名 |
| --- | --- | --- |
| 键盘主控 TX → BLE RX | PA4 | `LINE_BT_UART_TX` |
| BLE TX → 键盘主控 RX | PA5 | `LINE_BT_UART_RX` |

串口速率为 **115200 bit/s**。QMK 启动时初始化串口，发送 wakeup 命令，等待 100 ms 后清空接收缓冲区。协议由主控 UART 直接控制 BLE 行为，不是标准 HCI 传输。

```mermaid
sequenceDiagram
    participant K as HT32 keyboard MCU
    participant B as CC254x-class BLE MCU
    K->>B: wakeup command
    K->>K: wait 100 ms; discard pending RX
    K->>B: pair/connect/profile or HID report frames
    B->>Host: BLE HID over GATT
    B-->>K: fixed 11-byte status record (Caps Lock at byte 10)
```

源码依据：[引脚定义](../../modules/qmk_firmware/keyboards/annepro2/c18/config.h)、[初始化与接收](../../modules/qmk_firmware/keyboards/annepro2/annepro2.c)、[BLE 发送路径](../../modules/qmk_firmware/keyboards/annepro2/annepro2_ble.c)。

## 主控 → BLE：帧格式

除 bootloader 命令外，QMK 构造的命令都以如下 10 字节头开始：

```text
7b 12 53 00 LL 00 FF 7d GG OO [payload...]
```

- `LL 00`：**推断**为小端长度。键盘报告的 `LL=0x0a` 恰好等于 `GG+OO` 两字节加 8-byte boot keyboard report；consumer 的 `0x06` 同理等于两字节加 4-byte payload。
- `FF`：在 wakeup 为 `0x01`，普通命令为 `0x00`；语义未知。
- `0x7d`：所有已知模板固定出现；语义未知，不能当作校验和或转义字节处理。
- `GG` / `OO`：**推断**为命令组和操作码。下表的分组名称仅为实现时的便捷命名。

### 已知命令

表中每个字节均为 QMK 实际发送模板。`S` 是 slot，取 `0..3`。

| 用途 | 帧（十六进制） | 证据与说明 |
| --- | --- | --- |
| 唤醒 BLE | `7b 12 53 00 03 00 01 7d 02 01 02` | 系统组（推断）`0x02/0x01`，值 `0x02`。 |
| 请求 BLE IAP/bootloader | `7b 10 51 10 03 00 00 7d 02 01 01` | 特殊前缀/版本字段；不要与普通帧合并解释。尚未在实机捕获验证。 |
| 广播/选择 slot | `7b 12 53 00 03 00 00 7d 40 01 S 00` | profile/pairing 组（推断）`0x40/0x01`。 |
| 连接 slot | `7b 12 53 00 03 00 00 7d 40 04 S 00` | profile/pairing 组（推断）`0x40/0x04`。 |
| 删除配对 | `7b 12 53 00 02 00 00 7d 40 05` | `0x40/0x05`，无额外 payload。 |
| 键盘 HID 报告 | `00 7b 12 53 00 0a 00 00 7d 10 04 R0..R7` | 先送一个额外 `00`，再送 `0x10/0x04` 和 8-byte boot keyboard report。 |
| Consumer 报告 | `00 7b 12 53 00 06 00 00 7d 10 08 C0 00 00 00` | 先送一个额外 `00`，再送 `0x10/0x08` 和 4-byte payload。 |

### Slot 命令的特殊尾字节

**已确认（QMK 历史）**：QMK 的多配对修复将 broadcast/connect 模板改为 10 字节头，并在 slot 后显式再发送 `0x00`。因此即使 `LL=0x03` 看起来只覆盖组、操作和 slot，实际线上帧仍必须含该尾字节。它是兼容性必需字节，语义未确认，不能假定为 checksum。

历史依据：QMK commit [`37c271460a`](https://github.com/qmk/qmk_firmware/commit/37c271460a)（“fix bluetooth multi-pairing issue”）。

### Consumer 位掩码

QMK 的 4-byte consumer payload 只使用 `C0`，其余三字节始终为 0：

| 功能 | `C0` |
| --- | ---: |
| 静音 | `0x01` |
| 音量增加 | `0x02` |
| 音量降低 | `0x04` |
| 播放/暂停 | `0x08` |
| 下一曲 | `0x10` |
| 上一曲 | `0x20` |

鼠标报告在现有 QMK BLE host driver 中为空实现。选择 “USB” 主机模式时，QMK 只在本地切换 host driver，没有发送一个已知的 BLE disconnect UART 命令。

## BLE → 主控：状态回传

**已确认（QMK）**：主控从 UART RX 流中按固定大小读取 `ble_capslock_t`：

```c
struct __attribute__((packed)) {
    uint8_t opaque[10];
    bool caps_lock;  // byte 10
};
```

因此目前可以保证的兼容接口只有：BLE 在 Caps Lock 状态改变时（或按其既有节奏）发送 **11-byte 记录**，第 10 个字节为 0/非 0 的 Caps Lock 状态。前 10 字节没有被 QMK 解释；它们的帧头、长度、校验和、发送时机均未还原。新的 BLE 实现应先保持 11-byte 格式，再通过逻辑分析仪确定是否存在其他键盘固件依赖。

## 清洁室 BLE 实现契约

一个兼容替代实现的最小边界如下：

1. 先确认实际 BLE SoC 型号、供电、复位、SWD/debug 与 UART0 引脚；不得仅因本分析而直接刷写 CC2541 目标。
2. UART0 以 115200 8N1 工作，采用可恢复的环形缓冲/parser；完整匹配上表的字节序列，尤其是 slot 命令和 HID 命令前的额外 `0x00`。
3. 维护四个持久化 bond/profile slot，对应设备名中的 `P1..P4`；slot 行为应通过实机抓包定义（广播、连接、覆盖、删除）。
4. 提供 BLE HID keyboard 和 consumer-control 报告路径；至少覆盖表中的 boot keyboard 8-byte 和 consumer `C0` 位掩码。
5. 向主控发送 11-byte Caps Lock 状态记录，最后一字节为状态值。
6. IAP/bootloader 命令在完成抓包、失败恢复与映像格式确认前不实现或默认拒绝，避免把未验证的 `0x7b 10 51 10...` 帧暴露为刷写入口。
7. 只使用有授权的 8051/BLE SDK、协议栈和自行编写的代码；官方二进制只作行为和接口参考。

## 建议的实机验证顺序

用逻辑分析仪同时抓 PA4/PA5（115200、8N1），并保存原始样本与解码 CSV：

1. 上电、QMK wakeup、USB/BLE 模式切换。
2. 首次按各 slot、重复按当前 slot、已配对连接、未配对广播。
3. 键盘、NKRO（如启用）、consumer、Caps Lock LED 同步。
4. 删除配对与重新绑定四个 slot。
5. 官方 BLE 更新/IAP 的完整会话；仅在有可恢复刷写流程后测试。

将每一类样本做成可回放测试：输入 UART 字节流，断言 BLE 状态机、GATT 通知和 11-byte Caps Lock 回传。这比基于静态字符串猜测协议字段更可靠。

## 证据索引

- [`annepro2_ble.c`](../../modules/qmk_firmware/keyboards/annepro2/annepro2_ble.c)：所有主控→BLE 命令模板与 HID 映射。
- [`annepro2.c`](../../modules/qmk_firmware/keyboards/annepro2/annepro2.c)：115200 UART 初始化、wakeup 时序、RX 读取。
- [`annepro2.h`](../../modules/qmk_firmware/keyboards/annepro2/annepro2.h)：11-byte Caps Lock 记录定义。
- [`c18/config.h`](../../modules/qmk_firmware/keyboards/annepro2/c18/config.h)：C18 UART 引脚。
- [TI CC2541 product page](https://www.ti.com/product/CC2541)、[datasheet](https://www.ti.com/lit/ds/symlink/cc2541.pdf)、[CC253x/CC254x user guide](https://www.ti.com/lit/ug/swru191f/swru191f.pdf)：8051/UART/flash 寄存器与器件能力交叉验证。
