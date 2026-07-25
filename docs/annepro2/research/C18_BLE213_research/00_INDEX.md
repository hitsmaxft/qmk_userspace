# Anne Pro 2 C18 适配 BLE 2.13 技术调研

分析日期：2026-07-25  
研究对象：Anne Pro 2 C18 KEY 2.36 / BLE 2.05 与 Anne Pro 2D KEY 3.08 / BLE 2.13  
证据范围：Hexcore/Obins 官方固件、官方版本清单、官方产品与使用文档、TI CC254x 文档  
目标硬件：C18 主控、C18 板级外设、AP2D BLE 2.13 副控固件  
修改范围：只修改 C18 KEY 源码，BLE 2.13 二进制保持原样

## 调研结论

1. C18 KEY 适配 AP2D BLE 2.13 在协议层具备可行性。两代 KEY 使用相同的 BLE 串口 GPIO、复用功能、UART 外设、115200 bit/s 波特率和 `0x7B…0x7D` 基础帧族。
2. BLE 2.13 无法依靠“改产品 ID”完成适配。标准键盘 Report ID 1 保持一致，厂商 Report ID 2 的 IN/OUT 方向发生交换，Consumer Report ID 3 从 C18 的 8 位媒体键位图改为四个 16 位 Usage 数组。
3. 最稳妥的首版交付形态是同一份 C18 KEY 源码内置两个 profile，并生成两个默认值不同的发布固件：
   - `C18_BLE205`：首次启动默认 BLE 2.05。
   - `C18_BLE213`：首次启动默认 BLE 2.13。
   - 用户可通过 USB 维护命令或启动组合键显式选择 profile，选择结果带版本和校验后持久化。
4. 当前二进制分析没有确认一条安全、无副作用的 BLE 版本查询命令。首版不采用串口探测包猜测版本。单固件自动识别可以在取得 KEY↔BLE UART 抓包并确认版本查询协议后追加。
5. “同一主机绑定两台键盘”的主要修复已经包含在 BLE 2.13 内部。它使用 CC254x 工厂 IEEE 地址校验并重建本地安全材料。C18 KEY 侧需要提供正确的清除旧配对、进入指定槽配对和重新绑定流程。
6. macOS CapsLock 修复位于 KEY 侧。C18 应同时接受单字节 `LED bits` 与双字节 `[Report ID, LED bits]`，并把 Num/Caps/Scroll 状态转交给 C18 独立 LED MCU。
7. USB suspend 修复位于 KEY 侧。AP2D 3.08 的挂起钩子不能逐指令移植到 C18；C18 需要围绕 USB、矩阵、BLE UART 和独立 LED MCU 实现语义等价的停用、休眠和恢复顺序。
8. 四主机首版需要 KEY 侧完整状态机。BLE 2.13 已提供 AP2D 的四槽能力，KEY 必须正确发送槽位命令、等待断开/连接结果、隔离迟到事件，并在成功连接后更新活动槽。
9. AP2D KEY 3.08 的完整帧投递、实际长度检查和 `0x10/0x12` 序号隔离可以移植到 C18 KEY，且不会要求修改 BLE 2.05 或 2.13。
10. AP2D BLE 2.13 镜像在 C18 BLE 板上的烧录安全性仍需硬件门禁验证。镜像大小和 CC254x 架构一致只能证明软件家族接近，无法证明 bootloader、IAP、RF 校准区和板级引脚契约完全相同。
11. BLE 2.13 保持原二进制时，广播/GAP 名称和无线侧 GATT 契约仍属于 AP2D。C18 KEY 无法单独把无线设备名恢复成 `AnnePro2`；主机将看到 `HEXCORE AnnePro 2D` 一类名称，蓝牙配置通道也必须遵循 2.13 的 Vendor 方向。

## 报告目录

| 文件 | 内容 |
|---|---|
| [01_EVIDENCE_AND_VERSIONS.md](01_EVIDENCE_AND_VERSIONS.md) | 样本、SHA-256、版本链、证据等级和官方来源 |
| [02_BLE_2_13_INTERNALS.md](02_BLE_2_13_INTERNALS.md) | BLE 2.13 升级内容、身份、安全记录、广播、HID 与 BLE 栈 |
| [03_AP2D_VS_C18_IMPLEMENTATION.md](03_AP2D_VS_C18_IMPLEMENTATION.md) | AP2D 相对 C18 的 GPIO、RGB、任务、协议与四项修复实现 |
| [04_C18_KEY_DUAL_BLE_PROTOCOL.md](04_C18_KEY_DUAL_BLE_PROTOCOL.md) | C18 KEY 同时支持 BLE 2.05/2.13 的协议规格与编码适配 |
| [05_C18_KEY_IMPLEMENTATION_PLAN.md](05_C18_KEY_IMPLEMENTATION_PLAN.md) | 模块划分、四槽状态机、suspend、错误处理和开发阶段 |
| [06_VALIDATION_RISK_AND_ROLLBACK.md](06_VALIDATION_RISK_AND_ROLLBACK.md) | 测试矩阵、烧录门禁、风险表、回滚与验收条件 |
| [07_REVERSE_ENGINEERING_INDEX.md](07_REVERSE_ENGINEERING_INDEX.md) | 反汇编地址、文件偏移、描述符原始字节和复核入口 |
| [08_PRIOR_FINDINGS_AUDIT.md](08_PRIOR_FINDINGS_AUDIT.md) | 前序讨论结论的合并、修正和最终采用状态 |
| [source_manifests/annepro2c18_latest.json](source_manifests/annepro2c18_latest.json) | 分析时取得的 C18 官方固件清单副本 |
| [source_manifests/annepro2d_latest.json](source_manifests/annepro2d_latest.json) | 分析时取得的 AP2D 官方固件清单副本 |

## 最终目标架构

| 层 | C18 BLE 2.05 目标 | AP2D BLE 2.13 目标 | 共同实现 |
|---|---|---|---|
| 板级 HAL | C18 GPIO、USB、LED MCU | 同左 | 完全保持 C18 |
| BLE UART | PA4/PA5、AF6、115200 | 同左 | 同一个 UART 驱动和完整帧接收器 |
| 键盘输入 | Report ID 1，6KRO | 相同 | 同一个 8 字节键盘编码器 |
| 媒体输入 | 8 位媒体键位图 + 3 B padding | 4 × little-endian `uint16_t Usage` | 由 profile 选择编码器 |
| 厂商通道 | Usage 2 OUT、Usage 3 IN | Usage 2 IN、Usage 3 OUT | 由逻辑方向映射到版本方向 |
| 锁定灯 | 接受 BLE 转发结果 | 接受 1 B 或带 ID 的 2 B 形式 | 统一解析并桥接 LED MCU |
| 主机槽 | 四槽旧业务 | 四槽 AP2D 业务 | 统一 KEY 状态机，profile 提供命令适配 |
| BLE 身份 | BLE 2.05 原实现 | BLE 2.13 的 IEEE 派生校验 | KEY 只负责清除/重绑工作流 |

## 实施优先级

| 顺序 | 工作项 | 首版要求 |
|---:|---|---|
| 1 | 引入 `BLE205/BLE213` 显式 profile | 必须 |
| 2 | 键盘、媒体、厂商、锁定灯编解码器 | 必须 |
| 3 | 完整帧接收、长度校验、序号隔离 | 必须 |
| 4 | 四槽配对/切换状态机 | 必须 |
| 5 | C18 USB suspend 与 LED MCU 协调 | 第二阶段；调研和设计已完成 |
| 6 | 单固件运行时自动识别 BLE 版本 | 后续；等待确认版本查询命令 |
| 7 | 量化连接参数、功耗和切换延迟 | 实机验收阶段 |

首版完成定义：同一源码内置两个 profile，可分别以 `C18_BLE205` 与 `C18_BLE213` 为默认值构建；普通键盘、媒体键、锁定灯、配对、四槽、串口容错和 USB 救援通过测试。完整 backport 在第二阶段增加 C18 suspend/resume 验收。BLE 2.13 二进制始终保持原样。
