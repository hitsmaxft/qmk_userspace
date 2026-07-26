# 验证、风险与回滚计划

协议适配具备明确实现路径。官方 BLE 2.13 已在 C18 副控板完成 IAP
status-zero 传输、启动、广播、macOS 连接和普通输入；IAP 没有已验证的
readback，因此本页仍保留刷写前门禁和恢复边界，不能把运行结果扩大成 flash
逐字节或所有硬件批次的证明。

发布分层：

- 首版兼容固件：双 profile、普通键盘、媒体、Caps 锁定灯、四槽、UART
  容错、USB 维护/救援。
- AP2D GPIO/RGB、LED Output callback 和 suspend 灯控明确排除；共享
  `20/07` Caps 状态桥接到保持不变的 C18 LED MCU 路径。

## 烧录前门禁

| 门禁 | 要求 | 失败处理 |
|---|---|---|
| C18 KEY 恢复 | 能通过已验证工具回刷 KEY 2.36 | 停止实验 |
| C18 BLE 恢复 | 备份 BLE 2.05 可恢复区域并验证回刷 | 停止 BLE 2.13 烧录 |
| Bootloader/IAP | 确认应用区起止、升级包覆盖范围和校验方式 | 不使用 AP2D 升级流程猜测 |
| SNV | 记录/备份 bond 与应用 SNV；接受升级后清 bond | 明确回滚后重新配对 |
| 信息页 | 保护工厂 IEEE 地址和 RF/校准数据 | 不执行全片擦除 |
| UART | 逻辑分析仪能观察 KEY↔BLE TX/RX | 无法诊断时不进入四槽联调 |
| 功耗 | 建立 C18 BLE 2.05 基线 | 无法判断 suspend/连接退化 |

BLE 镜像大小相同不代表可安全写入相同地址。需要确认：

- 155,648 B 文件对应的应用容器布局；
- bootloader 是否位于镜像覆盖范围外；
- C18 与 AP2D 的 IAP 跳转地址和向量布局；
- RF 参数、MAC/IEEE 地址、校准页是否保存在独立信息区；
- BLE UART、复位、唤醒、电源控制引脚在两块板上是否一致；
- 升级程序是否会执行整片擦除。

建议先在可恢复样机上操作，保留第二块原版 C18 作为对照。

## 抓包与诊断

需要三类观测：

| 工具 | 观测内容 |
|---|---|
| KEY↔BLE UART 逻辑分析 | 外层帧、命令、槽号、Vendor 方向、超时和重试 |
| BLE 空口抓包 | 广播、连接参数、断开原因、配对与重连 |
| USB/主机日志 | HID Report、suspend/resume、枚举与 Output report |

UART 抓包最小场景：

1. 冷启动，无 bond；
2. 槽 1 长按配对；
3. 槽 1 已绑定后短按；
4. 槽 1→槽 2 切换；
5. 槽 2 配对超时；
6. 主机发送 CapsLock；
7. Mute、Volume、Next Track 的 press/release；
8. 主机读取和写入一项 Vendor 配置；
9. BLE 断电/复位；
10. 半帧、错长度和随机前缀注入。

每条日志建议记录：

```text
timestamp
direction
ble_profile
frame_type
command
declared_length
actual_length
slot
generation
result
power_state
```

日志中不要保存 IRK、SRK、LTK 或完整配对密钥。

## 离线单元测试

| 模块 | 测试 |
|---|---|
| Keyboard encoder | modifier、6 键、空报告、release all |
| Consumer 2.05 | 八个 bit、组合、padding、非法 Usage |
| Consumer 2.13 | 1–4 个 Usage、小端、超出四个、release all |
| LED decoder | 1 B、`[0x01,bits]`、错误 ID、0 B、超长 |
| Vendor bridge | 18 B、17/19 B 拒绝、205/213 方向 |
| Framer | 噪声、分段头、分段负载、超时、连续帧、超长 |
| Sequence | `0x10` 复位、`0x12` 连续、错序号 |
| Slot manager | 正常切换、迟到事件、超时、连续快速切换 |
| Suspend | 重入、ACK 超时、resume 重复、IAP 互斥 |

建议为每个 decoder/encoder 保存黄金向量。Report Map 变化属于固定二进制契约，单元测试可以在无硬件条件下完成。

## 实机验收矩阵

| 测试组 | 场景 | 通过标准 |
|---|---|---|
| 普通键盘 | BLE 2.05/2.13，各 OS 连续输入、组合键、6KRO | 无丢键、重复、粘键 |
| Consumer | Mute、音量、播放、上下曲、亮度 | 两 profile 均正确；release 无粘连 |
| Vendor | 读取/写入可回滚配置 | 方向和 18 B 长度正确；配置工具无退化 |
| CapsLock | BLE 发出 `20/07 01/00`；重连、切槽、睡眠和输入法切换 | 严格帧日志、QMK Caps bit 与 C18 实体灯一致；旧 host 状态不跨槽残留 |
| 双键盘同主机 | 两块 BLE 2.13 键盘依次绑定 Windows/macOS/Linux | 两块均保留并可重连；地址/身份不冲突 |
| 四槽配对 | 四台不同主机长按配对 | 每槽 bond 独立；超时不影响其他槽 |
| 四槽切换 | 短按、快速连续切换、连接中再切换；至少 100 次跨槽压力 | 旧事件不污染新槽；状态灯真实；无永久失联 |
| 断电重连 | KEY/BLE/整机分别复位 | last good 槽可恢复；无假连接 |
| 串口容错 | 注入半帧、错长度、错序号、噪声 | 错帧被拒绝，合法帧随后恢复 |
| USB suspend | 第二阶段：Windows 10/11 关机、睡眠、Modern Standby | 灯灭、电流下降、恢复无粘键 |
| 跨模式 | USB↔BLE、suspend 中拔线、切槽中接 USB | 单一活动输入源，无重复报告 |
| IAP | 正常升级、断电中断、回滚 | bootloader 可恢复，校准/地址保留 |

主机覆盖：

- Windows 10、Windows 11；
- macOS，至少覆盖当前版本和一台旧版兼容机；
- Linux BlueZ；
- iOS/iPadOS；
- Android。

AP2D 官方手册确认四槽操作方式：[Anne Pro 2D 官方手册](https://service.hexcore.xyz/manual/annepro2d/)

## 功耗与连接参数

至少测量：

| 状态 | 指标 |
|---|---|
| BLE 未连接广播 | 平均电流、峰值间隔、广播持续时间 |
| BLE 已连接空闲 | 平均电流、连接事件周期 |
| 连续输入 | 平均/峰值电流、报告延迟 |
| 四槽切换 | 从按键到断开、广播、连接完成的时间 |
| USB suspend | 进入时间、稳定电流、灯光状态 |
| resume | 唤醒到首个合法按键报告的时间 |

BLE 2.13 广告中的 preferred connection interval 为 15 ms，BLE 2.10 为 11.25 ms。实际协商值以空口抓包为准。

建议记录 P50/P95/P99 切换时间，避免仅报告平均值。

## 风险表

| 风险 | 影响 | 概率 | 控制 |
|---|---|---:|---|
| AP2D BLE 镜像覆盖 C18 bootloader/校准区 | 失去恢复能力或 RF 异常 | 中 | 先确认应用边界，保护信息页，准备调试器恢复 |
| Vendor UART opcode 与 HID Usage 同时变化 | 配置工具失效 | 中 | 先抓包，方向映射与 opcode 分开验证 |
| Consumer payload 仍按 C18 4 B 发送 | 媒体键错误/丢失 | 高 | profile encoder + 黄金向量 |
| 切槽立即写 `active_slot` | 假连接、槽状态错乱 | 高 | 仅在安全连接完成后持久化 |
| 迟到事件覆盖新事务 | 快速切换失败 | 高 | generation ID |
| suspend 前未全释放 | stuck key | 中 | Keyboard/Consumer release all |
| 自动探测 BLE 版本误触发命令 | 配对/IAP 状态改变 | 中 | 首版采用显式构建 profile |
| 升级后保留旧 bond | 无法重连或双键盘冲突 | 高 | 清槽并要求主机端删除旧项 |
| KEY 与 BLE profile 刷错 | HID/Vendor/媒体异常 | 高 | 文件名、版本字符串、启动诊断标识 |
| BLE 2.13 保留 AP2D 名称/GATT | 主机显示名称变化、蓝牙配置软件识别为 AP2D | 高 | 明确重绑与产品身份变化；KEY 适配 2.13 Vendor 契约 |

## 回滚方案

准备文件：

- 官方 C18 KEY 2.36；
- 官方 C18 BLE 2.05；
- C18 LED 2.33；
- 调试器/升级工具配置；
- 备份的 SNV、信息页和必要校准数据；
- 已验证的恢复步骤记录。

回滚触发条件：

- BLE 无广播或 UART 无响应；
- 公共地址异常；
- 主机反复配对失败；
- Vendor 配置通道写错方向；
- 四槽 bond 相互覆盖；
- suspend 无法恢复；
- 功耗显著高于基线；
- bootloader 无法进入或 IAP 校验失败。

回滚顺序：

1. 停止继续写入和全片擦除；
2. 通过已验证入口恢复 C18 BLE 2.05；
3. 恢复 C18 KEY 2.36；
4. 必要时恢复 LED 2.33；
5. 验证工厂地址和 UART；
6. 清除主机旧配对并重新绑定；
7. 复跑 C18 stock 基线测试。

## 研究完成与工程门禁

已完成：

- 固件版本和 SHA-256；
- BLE 2.13 栈版本、身份记录、广播与 HID 描述符分析；
- AP2D 3.08 四项修复的 KEY/BLE 归属；
- C18/BLE 2.05 与 AP2D/BLE 2.13 协议差异；
- C18 KEY 双 profile 架构；
- 四槽和串口容错设计；
- 测试、风险和回滚计划。

进入编码前仍需取得：

- BLE 2.05 与 2.13 的 KEY↔BLE Vendor 操作抓包；
- AP2D 四槽 `0x21/0x22` 完整负载抓包；
- C18 BLE bootloader/IAP/信息页边界确认。

这些门禁影响实现细节和烧录安全，不改变整体方案：C18 KEY 通过显式 profile 适配两个 BLE 固件，BLE 2.13 保持原二进制。
