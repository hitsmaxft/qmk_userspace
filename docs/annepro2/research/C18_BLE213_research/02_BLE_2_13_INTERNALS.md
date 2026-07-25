# AP2D BLE 2.13 升级内容与内部实现

BLE 2.13 是重新链接并修改过应用逻辑的 CC254x 固件。它与 BLE 2.10 大小相同，代码地址、广告数据、安全记录初始化和 HID Report Map 都发生变化。产品名称和协议栈标记也已更新，能够排除“仅修改版本字符串”的可能。

## BLE-Stack 1.5.0 升级到 1.5.2

C18 BLE 2.05 与 AP2D BLE 2.10 均包含 `BLE-1.5.0` 标记，BLE 2.13 包含 `BLE-1.5.2`。TI 将 BLE-Stack 1.5.2 定义为面向 CC2540/CC2541 的维护版本，并列出 pairing confirmation 校验修复：发起方会拒绝与本地 confirm value 相同的远端值。

TI 同时说明：

- BLE-Stack 1.5.x 通过 Bluetooth 5.0 qualification；
- CC254x 实现的核心功能仍与旧版 BLE 4.0 功能集合一致；
- PHY 仍为 1 Mbps GFSK；
- 不能由“BLE 5.0”推导出 LE 2M、LE Coded 或扩展广播。

因此，2.13 的可见收益来自维护版配对修复与 Hexcore 应用层改动，未发现新 PHY 能力。

来源：[TI BLE-Stack 1.5.2](https://www.ti.com/tool/download/BLE-STACK-1-X/1.05.02.00)、[TI BLE Software Developer's Guide](https://www.ti.com/lit/ug/swru271i/swru271i.pdf)

## 同一主机绑定两台键盘

BLE 2.10 启动初始化 `0xD4D2` 读取三项 SNV 数据：

| SNV ID | 长度 | XDATA 目标 | 对应数据形态 |
|---:|---:|---:|---|
| 2 | 16 B | `0x176F` | `GAPROLE_IRK` |
| 3 | 16 B | `0x177F` | `GAPROLE_SRK` |
| 4 | 4 B | `0x178F` | `GAPROLE_SIGNCOUNTER` |

TI 文档定义 IRK 和 SRK 均为 16 字节，sign counter 为 32 位。BLE 2.10 的 `0xD559` 只把 ID 2 的前四字节与固定标记 `57 35 72 BC` 比较。命中默认标记时生成并保存新记录，没有看到设备工厂地址参与有效性检查。

BLE 2.13 的初始化移动到 `0xE5B9`，仍读取 ID 2/3/4，目标为 `0x176D/0x177D/0x178D`，新增两条设备唯一性校验路径：

- `0xE642`：读取 CC254x information page `0x780E–0x7813` 的 48 位工厂 IEEE 地址，校验 ID 2 中的设备派生字段；不匹配时重建 16 字节记录并写回 SNV ID 2。
- `0xE6E1`：对 ID 3 执行同类校验、重建和写回。
- 设备地址只承担唯一性校验/派生的一部分，其余字节仍由内部生成逻辑填充。

CC2541 数据表确认每颗芯片都有唯一 48 位 IEEE 地址，可用作 Bluetooth 公共设备地址。BLE 2.13 把这个硬件唯一值纳入安全材料校验，消除了两块量产键盘保留相同本地身份材料的条件。

这条变化能够同时解释：

1. 官方 3.08 说明中的“两台 AP2D 无法绑定同一主机”修复；
2. 官方要求升级后删除并重新绑定；
3. BLE 2.13 的 SNV 初始化与 2.10 不同；
4. 单纯修改广播名称无法解决的问题由安全身份层处理。

C18 KEY 适配 BLE 2.13 时不需要生成或改写 IRK/SRK。KEY 侧职责只有：

- 提供清除指定槽、清除全部槽和进入配对的正确命令；
- 升级后把旧槽状态标记为无效；
- 通过灯光或日志明确提示用户在主机端删除旧配对项；
- 等待 BLE 返回真实配对/连接结果后再持久化 `active_slot`。

来源：[TI CC2541 数据表](https://www.ti.com/lit/gpn/CC2541)、[TI GAPRole 参数定义](https://www.ti.com/lit/ug/swru271i/swru271i.pdf)

## HID Report Map 变化

三份 BLE 固件的 Report Map 均位于文件偏移 `0x6000`。

标准键盘 Report ID 1 在 C18 BLE 2.05、AP2D BLE 2.10 和 2.13 中逐字节一致。其结构为：

- 8 个 modifier bit；
- 1 字节保留字段；
- 6 个 8 位键码；
- LED Output：Num、Caps、Scroll、Compose、Kana 共 5 bit；
- CapsLock 固定为 LED bit 1。

这意味着普通键盘输入和锁定灯语义可直接复用。

厂商 Report ID 2 的原始字节：

```text
C18 BLE 2.05 / AP2D BLE 2.10:
06 00 FF 09 01 A1 01 85 02 15 80 26 FF 00
09 02 75 08 95 12 91 02
09 03 75 08 95 12 81 02 C0

AP2D BLE 2.13:
06 00 FF 09 01 A1 01 85 02 15 80 25 FF
09 02 75 08 95 12 81 00
09 03 75 08 95 12 91 00 C0
```

语义变化：

| 项目 | BLE 2.05 / 2.10 | BLE 2.13 |
|---|---|---|
| Usage 2 | OUT，18 B | IN，18 B |
| Usage 3 | IN，18 B | OUT，18 B |
| flags | Data, Variable, Absolute | Data, Array, Absolute |
| Logical Maximum | 16 位编码 `0x00FF` | 8 位编码 `0xFF` |

KEY 源码必须用“主机→KEY”和“KEY→主机”的逻辑方向表达厂商数据，再由 BLE profile 映射到版本对应的 Usage。业务代码直接写 Usage 2/3 会让双版本支持变得脆弱。

Consumer Report ID 3：

```text
C18 BLE 2.05:
05 0C 09 01 A1 01 85 03
15 00 25 01
09 E2 75 01 95 01 81 02
09 E9 09 EA 75 01 95 02 81 02
09 CD 09 B5 09 B6 09 6F 09 70
75 01 95 05 81 02
75 08 95 03 81 03 C0

AP2D BLE 2.10 / 2.13:
05 0C 09 01 A1 01 85 03
15 01 26 02 02
19 01 2A 02 02
75 10 95 04 81 00 C0
```

C18 使用 8 个媒体功能 bit，加 3 字节常量 padding，总负载 4 B。AP2D 使用四个 16 位 Consumer Usage 数组元素，总负载 8 B。2.13 的媒体格式在 2.10 已经采用，3.06 的官方说明也专门列出 macOS Consumer 修复。

## 广播与连接提示

BLE 2.10 在文件偏移 `0x2DC` 附近保存四套 `AnnePro 2D` 广播/扫描响应模板。BLE 2.13 在 `0x2DC` 合并为一套统一模板：

- 广播/扫描响应文本中可见 `HXECORE AnnePro 2D`；
- GAP 名称为 `HEXCORE AnnePro 2D`；
- HID service UUID 保持 `0x1812`；
- Appearance 保持 `0x03C1`；
- preferred connection interval 从 `0x0009/0x0009` 调整为 `0x000C/0x000C`。

按 1.25 ms 单位换算：

| 版本 | 广告中的 preferred interval |
|---|---:|
| BLE 2.10 | 11.25 ms |
| BLE 2.13 | 15 ms |

15 ms 只是一项主机可见的连接参数偏好，实际连接仍由双方协商。它可能减少连接事件频率并降低唤醒次数，功耗和延迟收益需要空口抓包及电流测试。

四套广播模板合并为统一模板的工程意义：

- 活动槽切换不再要求复制/切换四份设备身份文本；
- bond 槽位和外部设备身份分离；
- 降低某一模板保留陈旧名称、状态或长度的风险；
- 同一主机看到两台键盘时，区分依据落在公共地址和安全材料，而非四套名称。

广播文本中的 `HXECORE` 很可能是固件常量中的拼写问题。GAP 名称字符串为正确的 `HEXCORE`，该差异不影响本报告的协议结论。

## 多主机能力与 KEY 边界

AP2D 官方手册定义：

- 长按 `FN2+1…4` 五秒，在对应槽进入配对；
- 短按 `FN2+1…4`，切换到对应已绑定主机；
- 配对广播持续约一分钟。

BLE 2.13 内部承担 bond、安全关系、广播和连接。KEY 3.08 承担按键手势、槽号、命令投递、状态显示和事件调度。两侧需要保持以下一致性：

| 状态 | KEY | BLE |
|---|---|---|
| `target_slot` | 用户刚选择的槽 | 将要加载/建立 bond 的槽 |
| `pairing` | 长按触发、显示红色闪烁 | 清槽或进入可配对广播 |
| `switching` | 短按触发、显示绿色闪烁 | 断开当前主机并选择已有 bond |
| `connected` | 收到确认后持久化活动槽 | 链路已建立且安全过程完成 |
| `timeout/error` | 停止闪烁、保留可恢复状态 | 广播或连接失败 |

KEY 不能在发送命令后立即把槽标记为已连接。BLE 事件是异步的，旧连接的断开回调、超时和新槽连接完成可能交错。

## BLE 2.13 对 C18 KEY 的直接要求

| 变化 | C18 KEY 必须处理 |
|---|---|
| 安全记录基于设备唯一地址 | 升级后执行清槽/重绑工作流；不自行修改密钥 |
| Vendor ID 2 方向交换 | profile 化厂商逻辑方向 |
| Consumer ID 3 为 4 × u16 | 新媒体编码器与 release 报告 |
| 四槽状态机 | 槽号 0–3、配对/切换、异步确认 |
| LED Output 可能带 Report ID | 兼容 1 B 与 2 B |
| 广告身份统一 | KEY 灯态不能依赖设备名或模板编号 |
| BLE-Stack 1.5.2 | 主机端删除旧 bond 后重新配对 |

BLE 2.13 本身不解决 C18 的 USB suspend，也不会控制 C18 独立 LED MCU。这两部分必须在 C18 KEY 中完成。

BLE 2.13 二进制保持原样还意味着：

- 无线广播/GAP 名称继续是 AP2D 名称；
- HID Report Map 继续采用 BLE 2.13 的 Vendor/Consumer 契约；
- 主机蓝牙列表可能把改造后的 C18 显示为 `HEXCORE AnnePro 2D`；
- 主机端需要删除旧 `AnnePro2` bond，再按新身份重新配对；
- KEY-only 改造无法修改广播名称，除非后续确认 BLE 固件提供安全的运行时名称设置命令。
