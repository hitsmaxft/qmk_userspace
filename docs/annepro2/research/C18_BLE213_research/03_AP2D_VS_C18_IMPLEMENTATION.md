# Anne Pro 2D 相对 C18 的改进与实现细节

AP2D 的改进分为三类：硬件改版、KEY 3.0 软件架构重构、BLE 2.13 应用/协议栈升级。C18 回移只采用与 C18 板级结构兼容的行为，不复制 AP2D 的 GPIO 和 RGB 驱动。

## 硬件与 GPIO 差异

两者都是 14 列 × 5 行矩阵。19 个矩阵 GPIO 中仅列 1 `PC4`、列 2 `PC5`、列 8 `PC13` 保持在相同逻辑位置。

| 逻辑列 | C18 KEY 2.36 | AP2D KEY 3.08 |
|---:|---|---|
| 1 | PC4 | PC4 |
| 2 | PC5 | PC5 |
| 3 | PD0 | PC8 |
| 4 | PB15 | PC0 |
| 5 | PC11 | PA10 |
| 6 | PA15 | PB1 |
| 7 | PC12 | PA8 |
| 8 | PC13 | PC13 |
| 9 | PA8 | PC12 |
| 10 | PA10 | PA15 |
| 11 | PA11 | PA14 |
| 12 | PA14 | PA11 |
| 13 | PD2 | PD1 |
| 14 | PD3 | PD2 |

| 逻辑行 | C18 KEY 2.36 | AP2D KEY 3.08 |
|---:|---|---|
| 1 | PB5 | PC3 |
| 2 | PB4 | PC14 |
| 3 | PB3 | PC15 |
| 4 | PB2 | PC1 |
| 5 | PD1 | PC2 |

C18 的扫描器位于 `0xAE54–0xAFF6`，端口表解压到 RAM：

- 行表：`0x200001B0`；
- 列表：`0x200001D8`。

AP2D 的矩阵初始化位于 `0xA26C–0xA302`，`PinName` 描述表位于文件偏移 `0x1BC0`。

对 C18 KEY 源码的约束：

- 保持 C18 矩阵引脚和扫描顺序；
- 不移植 AP2D GPIO 表；
- suspend/resume 时使用 C18 行列 GPIO 重新配置；
- 任何 AP2D 板级函数都只能提取调用顺序和状态语义。

## RGB 与线程资源

| 项目 | C18 | AP2D |
|---|---|---|
| RGB 控制器 | 独立 LED MCU | KEY MCU 直接驱动 |
| KEY→LED 通道 | `PB0/PB1`、AF6、115200 UART | 无独立灯控 UART |
| BLE 通道 | `PA4/PA5`、AF6、115200 UART | 相同 |
| AP2D RGB 引脚 | 不适用 | `PB3/PB4/PB5` AF5，`PB2/PD3` GPIO |
| 官方固件包 | KEY + LED + BLE | KEY + BLE |

AP2D 把 C18 的 `PB1` 灯控 UART 引脚改为矩阵列 6，RGB 初始化位于 `0x134F4`，发送函数 `0x1228E` 采用 SPI0 风格的三字节传输。

AP2D 去掉独立 LED MCU 后，BLE UART 不再与 LED UART 共用 C18 的 `Uart process` 搬运线程。该架构降低两条串口通道的共同排队和唤醒，但 C18 无法移除 LED MCU。C18 的等价优化是：

- BLE 与 LED 使用独立软件队列；
- BLE 接收优先执行完整帧组包；
- LED 动画大流量不能占满 BLE 消息队列；
- 两个 UART 中断只负责搬运，业务在线程/任务中执行；
- suspend 时分别执行 BLE 和 LED 有界 flush。

## KEY↔BLE 串行协议改进

两代基础帧构造函数保持同族：

- C18 `0x85C0–0x8606`；
- AP2D `0x9D5C–0x9DA2`；
- 两段共 70 B，逐字节相同。

共同特征：

- `0x7B` 起始；
- 类型包含 `0x10` 和 `0x12`；
- 固定 8 B 头；
- `0x7D` 作为头部结束标记；
- BLE UART 为 `PA4/PA5 + AF6 + 115200`。

接收路径差异：

| 项目 | C18 KEY 2.36 | AP2D KEY 3.08 |
|---|---|---|
| UART 搬运 | `Uart process @ 0xAD2A`，BLE/LED 任意长度块 | `Thread Uart @ 0xCE7C`，BLE 专用整帧组包 |
| 协议入口 | 调用 `0x86AE` 的 AT/状态机 | `Thread Protocl @ 0xC0B8` |
| 帧头校验 | `0x7B`、`0x10/0x12` | 再校验第三字节两个半字节范围 |
| 负载完整性 | `0x8748` 请求 `len-2`，忽略实际读回数 | `0x9F20` 比较实际读回数和 `len-2` |
| 序号 | `0x8778` 共用状态 | `0x9F52` 在 `0x10` 复位，仅 `0x12` 参与序号匹配 |

AP2D `Thread Uart` 的处理顺序：

1. 搜索 `0x7B`；
2. 收齐余下 7 B 头部；
3. 从头字段取得负载长度；
4. 收齐 `8 + payload_len`；
5. 把完整消息投递给协议线程；
6. 协议线程执行头字段、长度、序号、分片与命令检查。

该设计在 UART 丢字节、半帧、错误长度和队列分块时更容易重新同步。C18 可以移植这套逻辑，同时保留两个硬件 UART。

## macOS CapsLock 修复

C18 BLE 2.05、AP2D BLE 2.10 和 2.13 的标准键盘 Report ID 1 保持一致，CapsLock 都是 LED Output bit 1。问题位于 KEY 对 BLE 转发数据的长度处理。

AP2D KEY 3.06 `0x8346`：

- 只接受 `length == 1`；
- 读取 `buffer[0]`；
- 检查 CapsLock bit 1；
- 其他长度直接返回。

AP2D KEY 3.08 `0x8426`：

- `length == 1` 时读取 `buffer[0]`；
- `length == 2` 时读取 `buffer[1]`，跳过 Report ID；
- 分别把 Caps bit 1、Num bit 0、Scroll bit 2 传给
  `0xBE1A`、`0xBE5A`、`0xBE60`。

macOS 可能交付 `LED bits`，也可能交付 `[0x01, LED bits]`。3.06 丢弃第二种形式，3.08 兼容两者。

C18 回移不复制这三个 AP2D 直驱 RGB 状态函数。BLE 模块与 KEY 之间已经把
Caps 归一化为共同的 `20/07 00/01`：C18 KEY 2.36.3 的
`0xC45E–0xC4CC` 从 LED Output bit 1 构造该帧，AP2D KEY 3.08 的 group
`0x20`/opcode `0x07` 又把它交给 `0xBE1A` Caps 函数。QMK 因而只严格解码
这个共享 UART ABI，再通过标准 host LED bit 1 和 C18 既有 LED MCU API
显示；Num/Scroll 不在未确认 UART 契约下推断。

## USB suspend 修复

AP2D KEY 3.06 与 3.08 的 USB 主状态机在相同条件下进入 suspend hook：

- 3.06 主线程 `0xBC10`，hook `0xD780`；
- 3.08 主线程 `0xBF78`，hook `0xF404`。

3.06 的 `0xD780` 只有 `bx lr`。3.08 的 `0xF404–0xF448` 执行实际停用、挂起和恢复：

1. 停用/配置 `0x40010000` 外设；
2. 调用 `0xF0AC`、`0xA36E` 停止或切换板级子系统；
3. 通过 `0xF19A`、`0xF18C` 设置低功耗通道；
4. 以参数 1 调用 `0x64CE`、`0x12694`、`0x1262C`；
5. 以参数 0 恢复对应模块，并调用 `0xF0BA`、`0xA3CC`、`0xDA52`。

成对的 1/0 参数和固定调用顺序证明 3.08 用完整 `quiesce → suspend → restore` 序列替换空 hook。官方中文说明“Windows 10 关机后灯光不自动熄灭”和英文说明“USB mode suspend error”对应同一路径。

C18 的语义等价版本必须加入：

- 停止新 USB HID 报告入队；
- 发送键盘与 Consumer 全释放；
- 向 LED MCU 发送 all-off/suspend，等待 ACK 或短超时；
- 关闭 USB endpoint/PHY 时钟；
- 降低矩阵扫描或配置允许的唤醒源；
- 恢复时按时钟、UART、矩阵、USB、灯态、HID 的顺序重建状态；
- 恢复后先发送全释放，防止 stuck key。

## 多主机切换修复

四槽业务在 KEY 3.06 已经存在。3.08 修复了一条事件调度丢失路径。

`0x6A1A` 处理四个 pending flag：

- 3.06 在 `0x6A3E–0x6A46` 发现 `flag[2]` 后只清零；
- 3.08 在相同位置加载 `[global+0x0C]`，调用 `0x8216`；
- `0x8216` 再调用对象 vtable `+0x18` 的回调，随后清零。

3.08 对象回调 `0x82DE` 明确处理：

- 命令 `0x21`：按槽 0–3 选择对象内四块 9 B 槽数据，位置为 `+0x12/+0x2B/+0x4B/+0x64`；
- 命令 `0x22`：按槽 0–3 调用 `0xB640` 和 `0xB664` 取得槽数据指针与长度；
- 槽号超出 0–3 时返回。

因此，3.08 的“优化多设备切换”至少包含一项确定修复：原先被清除的第三类事件重新进入四槽对象状态机。BLE 2.13 还把四套广播模板合并为统一身份，降低切换时的陈旧模板风险。

C18 KEY 需要采用显式异步状态机，避免以下故障：

- 发送切换命令后立即覆盖 `active_slot`；
- 旧槽断开事件晚到并覆盖新槽状态；
- 长按配对和短按切换共用一个 pending flag；
- 配对超时后仍显示连接中；
- 切换失败时清除原槽 bond；
- 槽号未检查导致越界读取。

## AP2D 改进的回移分类

| AP2D 改进 | C18 处理方式 |
|---|---|
| GPIO 重排 | 不移植 |
| KEY 直接驱动 RGB | 不移植；保留 LED MCU |
| BLE 专用 UART 线程 | 移植完整帧思想，保留两路 UART |
| 帧头/长度/序号检查 | 直接移植到 C18 协议层 |
| CapsLock 1/2 B 兼容 | 不复制 AP2D callback；采用其归一化后的共享 `20/07` Caps ABI，并桥接 C18 LED MCU |
| USB suspend hook | 按 C18 外设重写 |
| pending event 修复 | 移植事件投递原则 |
| 四槽命令与状态机 | 适配 BLE 2.13，保留 BLE 2.05 profile |
| BLE 2.13 唯一身份 | 已存在于不修改的 BLE 2.13 |
| BLE 1.5.2 | 已存在于不修改的 BLE 2.13 |
| Vendor/Consumer Report Map | C18 KEY 按 profile 编解码 |
