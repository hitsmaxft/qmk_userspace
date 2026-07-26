# C18 KEY 源码升级实施计划

> 2026-07-26 范围修正：不移植 AP2D 的 GPIO/RGB、HID LED Output callback
> 和 suspend 灯控。AP2D 已不再使用 C18 的外置 LED MCU，这些硬件实现不适用
> 于 C18。现有 C18 LED MCU 与板级灯控代码保持原样；共享的 `20/07` Caps
> 逻辑状态通过 QMK host LED 接口桥接，并做实机回归。

实施目标：

- 修改 C18 KEY 源码；
- BLE 2.05 和官方 BLE 2.13 样本均不修改；可选 2C 名称镜像独立生成；
- 同一代码库内置两个 profile，并生成两个默认值不同的固件目标；
- 首版覆盖普通键盘、媒体键、Caps 锁定灯、配对、四主机切换、串口容错和
  USB 救援；
- 不移植 AP2D USB suspend 中的 LED/RGB 行为；
- 保持 C18 矩阵、USB、独立 LED MCU、IAP 和板级 HAL。

## 代码模块

```text
c18-key/
  board/
    c18_gpio
    c18_usb
    c18_matrix
    c18_led_uart

  ble/
    ble_profile
    ble_uart_framer
    ble_protocol
    ble_hid_codec
    ble_vendor_bridge
    ble_host_slots
    ble_status

  power/
    usb_power_manager
    wake_sources

  diagnostics/
    protocol_counters
    event_trace
```

模块职责：

| 模块 | 职责 |
|---|---|
| `ble_profile` | 选择 BLE 2.05/2.13，提供 Consumer、Vendor、槽位能力 |
| `ble_uart_framer` | 搜索帧头、收齐头/负载、超时、重新同步 |
| `ble_protocol` | 头字段、长度、序号、命令和槽号校验 |
| `ble_hid_codec` | Keyboard、Consumer 编解码 |
| `ble_vendor_bridge` | 18 B 厂商数据逻辑方向映射 |
| `ble_host_slots` | 四槽配对/切换异步状态机 |
| `ble_status` | 连接、配对、广播和错误状态 |
| `usb_power_manager` | suspend/resume 顺序与全释放 |
| `c18_led_uart` | C18 原有 LED MCU 路径；本项目不修改 |
| `protocol_counters` | 半帧、错长度、错序号、超时、旧事务计数 |

板级模块不得依赖 BLE 版本。BLE 业务模块不得直接访问 GPIO、USB 寄存器或 LED MCU 协议。

## 构建与配置

建议配置：

```c
static const struct ble_profile profiles[] = {
    BLE_PROFILE_C18_205,
    BLE_PROFILE_AP2D_213,
};

selected = nvm_profile_valid()
    ? nvm_profile_get()
    : BUILD_DEFAULT_BLE_PROFILE;
```

输出文件名应包含 profile：

```text
c18-key-ble205-<version>.bin
c18-key-ble213-<version>.bin
```

固件版本页和 USB 诊断状态要显示当前 profile。profile 选择记录采用独立 NVM 记录，包含 magic、结构版本、目标值和 CRC；非法记录回退 BLE 2.05。USB 维护命令和启动组合键可以修改该记录，普通配置写入不能触及它。

两个发布构建都包含双 profile，只改变 `BUILD_DEFAULT_BLE_PROFILE`。这样既能现场切换，也保留清晰的升级入口。

启动救援组合键应在 BLE 初始化前强制：

- 进入 USB 维护模式；
- 选择 BLE 2.05 profile；
- 禁止自动发送配对、切槽和 IAP 命令；
- 允许重新写入 KEY 固件和 profile NVM。

## 初始化顺序

```text
reset
  1. C18 clock/GPIO 初始化
  2. 矩阵和 LED UART 初始化
  3. BLE UART 初始化
  4. 加载编译期 ble_profile
  5. 清空 framer、序号与 transaction 状态
  6. 启动 UART 接收
  7. 初始化 USB 与模式检测
  8. 读取 KEY 持久化的 active_slot 提示
  9. 等待 BLE 返回真实状态
 10. 连接确认后更新活动槽
```

KEY 保存的 `active_slot` 只能作为启动提示。BLE 的实际 bond/连接结果具有更高优先级。

如果 BLE 没有响应：

- 保持矩阵和 USB 可用；
- 停止重复发送槽位命令；
- 使用有界退避重试；
- 记录 `BLE_BOOT_TIMEOUT`；
- 不清除任何 bond 或 KEY 槽数据。

## 四槽状态机

状态定义：

| 状态 | 含义 |
|---|---|
| `IDLE` | 无切换事务，可能已连接 |
| `DISCONNECTING` | 等待旧主机断开 |
| `SELECTING` | 已选择目标槽，尚未发起连接/广播 |
| `CONNECTING` | 连接目标槽已有 bond |
| `PAIRING` | 目标槽处于可配对广播 |
| `CONNECTED` | 连接和安全过程完成 |
| `FAILED` | 事务失败，等待回退/用户操作 |

持久状态：

```c
struct host_slot_state {
    uint8_t active_slot;
    uint8_t last_good_slot;
    uint8_t target_slot;
    uint8_t pairing_slot;
    uint32_t generation;
    enum slot_phase phase;
    bool bond_known[4];
};
```

短按 `FN2+n`：

```text
检查 n 合法
generation++
target_slot = n
发送 Keyboard/Consumer release all
若当前已连接且 active_slot != n:
    请求断开
    phase = DISCONNECTING
否则:
    进入 SELECTING

收到匹配 generation 的断开完成:
    请求切换到 n
    phase = CONNECTING

收到安全连接完成:
    active_slot = n
    last_good_slot = n
    持久化 active_slot
    phase = CONNECTED
```

长按 `FN2+n`：

```text
检查 n 合法
generation++
pairing_slot = n
发送全释放
断开当前连接
清除目标槽 bond
启动目标槽配对广播
phase = PAIRING

收到配对和安全连接完成:
    bond_known[n] = true
    active_slot = n
    last_good_slot = n
    持久化
    phase = CONNECTED
```

失败规则：

- 旧 generation 的事件只增加 `stale_event_count`；
- 断开超时后最多执行一次受控重试；
- 连接/广播超时后不清除其他槽；
- `active_slot` 只在连接完成后修改；
- 若旧连接仍有效，失败时恢复 `last_good_slot`；
- LED 指示由 phase 驱动，禁止用固定延时猜测结果。

## 按键与媒体报告

普通键盘：

- 两 profile 共用 C18 的 6KRO 扫描结果；
- 发送前做去重和键数上限检查；
- 槽位切换、断开、suspend 前发送全释放；
- resume 后再次发送全释放，再接受新扫描结果。

媒体键：

- 业务层产生标准 16 位 Consumer Usage；
- `ble_hid_codec` 根据 profile 转为 C18 位图或 AP2D 4×u16；
- BLE 2.05 遇到无法表达的 Usage 时返回 `UNSUPPORTED_USAGE`；
- BLE 2.13 最多同时发送四个 Usage；
- release 报告优先级高于新的 press，避免媒体键粘连。

## Vendor 通道

业务层调用：

```c
int vendor_send_to_host(const uint8_t payload[18]);
void vendor_receive_from_host(const uint8_t payload[18]);
```

`ble_vendor_bridge` 只做：

- 18 B 长度校验；
- profile 方向映射；
- UART 命令封装/解封装；
- transaction 和超时；
- 未知 opcode 记录。

它不解释 ObinsKit/Hexcore Link 私有 payload。配置业务仍由现有 C18 代码处理。

在 BLE 2.13 UART 抓包确认前，把该模块置于受控实验开关下：

```text
FEATURE_VENDOR_BLE213=0  默认关闭
FEATURE_VENDOR_BLE213=1  抓包验证后启用
```

普通键盘、媒体和四槽首版不应依赖 Vendor 配置通道。

## LED/RGB 范围

AP2D 的 LED Output callback、KEY MCU 直驱 RGB 和 suspend 灯控只作为差异
证据保留，不生成硬件实现任务。C18 继续使用现有外置 LED MCU、UART 与 QMK
驱动。BLE driver 只严格解释两代固件共同确认的
`7B 12 35 00 03 00 00 7D 20 07 00/01`，把它映射为标准 Caps bit；其他
LED group/opcode 不猜测，Num/Scroll 也不在没有证据时虚构。

## UART 容错

推荐资源上限：

| 项目 | 要求 |
|---|---|
| RX 环形缓冲 | 能覆盖最大合法帧和一次 UART burst |
| 完整帧队列 | 独立于 LED UART 队列 |
| payload 上限 | 以协议已知最大值定义，拒绝更大长度 |
| 头部超时 | 短于业务响应超时 |
| payload 超时 | 按最大帧与 115200 bit/s 计算并留调度余量 |
| 业务超时 | 命令级单独计时 |

异常计数器：

```text
rx_noise_bytes
rx_bad_header
rx_oversize
rx_payload_timeout
rx_short_payload
rx_bad_sequence
rx_unknown_command
rx_stale_transaction
tx_queue_full
```

计数器可通过 USB 诊断接口读取，避免为了调试修改 BLE 厂商通道。

## USB suspend 与恢复

suspend 入口：

```text
1. 标记 power_state = SUSPENDING
2. 禁止新的 USB HID 入队
3. 发送 Keyboard release all
4. 发送 Consumer release all
5. BLE UART 有界 flush
6. 配置 C18 矩阵唤醒源
7. 停止 USB endpoint/PHY 时钟
8. 进入低功耗
```

resume：

```text
1. 恢复系统时钟
2. 恢复 BLE UART；C18 LED UART 由原实现负责
3. 恢复矩阵 GPIO 与消抖状态
4. 恢复 USB endpoint/PHY
5. 清空旧 framer/分片/输入队列
6. 发送 Keyboard/Consumer release all
7. 查询或等待接口状态
8. power_state = ACTIVE
9. 允许新 HID 入队
```

关键约束：

- suspend 和槽位切换互斥；
- IAP 期间禁止进入会关闭升级通道的深度 suspend；
- resume 事件只能执行一次；
- USB 关机、睡眠、Modern Standby 和拔插需要分别测试。

## 开发阶段

| 阶段 | 交付 | 退出条件 |
|---:|---|---|
| 0 | C18 stock 基线、完整备份、UART 测试点 | 可稳定回刷原 KEY/BLE |
| 1 | `ble_profile` 与两构建目标 | 二进制明确标识 205/213 |
| 2 | Keyboard/Consumer/LED 编解码 | 离线单元测试覆盖全部 Usage |
| 3 | Vendor 方向适配框架 | 不影响普通 HID；等待抓包启用 |
| 4 | 完整帧接收与协议校验 | 错帧注入后自动恢复 |
| 5 | BLE 2.13 四槽状态机 | 四主机配对/切换通过 |
| 6 | BLE 2.05 回归 | 原功能和 ObinsKit 无退化 |
| 7 | BLE 2.13 首版综合验收 | HID、四槽、重绑、USB 救援通过 |
| 8 | 第二阶段 C18 suspend/resume | Windows/macOS/Linux 与功耗测试通过 |

## 代码审查门禁

每个补丁必须满足：

- 板级 GPIO 只来自 C18 HAL；
- BLE profile 分支集中，业务层没有散落的 `if (ble_version)`；
- 所有 UART 长度来自已验证的常量或边界检查；
- 槽号始终检查 0–3；
- 所有异步事件带 generation/transaction；
- 所有等待均有超时；
- press 路径都存在可达的 release 路径；
- suspend/切换失败不会擦除其他槽 bond；
- IAP 与普通业务命令明确隔离。
