# 前序讨论结论审计与最终状态

本文件记录前面分析中的阶段性结论、后续新证据和最终采用状态，避免旧报告中的证据边界与当前方案混用。

## 已撤销或替换的结论

| 阶段性结论 | 后续证据 | 最终状态 |
|---|---|---|
| 初始比较对象包含 Anne Pro 2 C15 | 用户明确要求只分析 C18 与 AP2D | C15 全部排除 |
| GPIO 结论参考了社区板级定义 | 用户要求只分析官方固件；C18 表已从压缩 `.data` 恢复 | 最终报告只使用官方二进制 |
| 尚未取得 AP2D BLE 2.13，无法判断其内部变化 | 用户提供原始镜像，SHA-256 与官方清单一致 | 已完成 BLE 2.13 二进制分析 |
| BLE 优化仅能从 KEY 线程和帧解析推断 | BLE 2.13 样本显示栈、SNV 身份、广告和 Report Map 变化 | 已补齐 BLE 侧证据 |
| 最稳妥路线需要同时修改 C18 BLE 应用和 KEY | 当前任务边界固定为 BLE 2.13 二进制不改，只修改 C18 KEY | 最终采用 KEY 双 profile 方案 |
| AP2D BLE 2.13 不能直接用于 C18 | 物理 UART和基础帧同族，HID ID 1 兼容；ID 2/3 与四槽业务有差异 | 修正为“不能免适配使用”；C18 KEY 可通过 profile 对接，烧录安全仍需门禁 |
| 2.13 可能只改设备描述信息 | 栈版本、SNV 初始化、工厂地址校验、Report Map、广告结构均变化 | 已否定 |
| 改产品 ID 即可兼容 C18 KEY | Vendor 方向和 Consumer 布局发生协议变化 | 已否定 |
| 首版可以忽略多主机 | 用户将四主机配对/切换列为首版要求 | 已纳入核心状态机和验收 |

## 最终保留的核心发现

1. C18 KEY 2.36 与 AP2D KEY 3.08 的 BLE UART 均为 `PA4/PA5 + AF6 + 115200`。
2. 两者基础 `0x7B…0x7D` 帧族延续，AP2D 加强完整帧、长度和序号处理。
3. AP2D BLE 2.13 基于 CC254x/8051 与 BLE-Stack 1.5.2，镜像大小 155,648 B。
4. BLE 2.13 通过工厂 IEEE 地址校验并重建 SNV ID 2/3 的 16 B 安全记录，解决设备身份冲突。
5. Keyboard Report ID 1 在 C18 2.05、AP2D 2.10 和 2.13 中一致。
6. Vendor Report ID 2 在 2.13 中交换 IN/OUT 方向并改变 flags。
7. Consumer Report ID 3 在 AP2D 中为四个 16 位 Usage，C18 使用 8 位媒体键图和 3 B padding。
8. macOS CapsLock 修复来自 KEY 对 1 B/2 B LED Output 的兼容解析。
9. USB suspend 修复来自 KEY 3.08 用完整停用/恢复序列替换空 hook。
10. 多主机切换包含 KEY pending event 恢复投递、四槽 `0x21/0x22` 处理和 BLE 统一广告身份。
11. C18 的独立 LED MCU、矩阵 GPIO和 USB/电源 HAL 必须保留。

## 对关键问题的最终回答

“BLE 2.13 除了描述信息，还有实现改进吗？”

有。已证实的改进包括：

- BLE-Stack 1.5.0→1.5.2；
- 配对确认校验维护修复；
- 工厂 IEEE 地址参与本地身份材料校验；
- SNV ID 2/3 重建逻辑；
- 四套广播模板合并；
- preferred connection interval 提示 11.25 ms→15 ms；
- Vendor Report ID 2 方向/flags 变化；
- 设备名称和产品身份更新。

“改 ID 能让 C18 KEY 直接使用 BLE 2.13 吗？”

不能。还要处理：

- Consumer ID 3 的 4 B→8 B 编码变化；
- Vendor ID 2 的双向 Usage 对调；
- 四槽命令和异步状态；
- LED Output 1/2 B；
- 配对数据迁移和主机端重绑；
- 烧录/IAP/板级安全。

“没有 BLE 源码，怎样兼容 BLE 2.13？”

采用不修改 BLE 二进制的 KEY 适配层：

- 保留共同 UART 和外层帧；
- 使用 `BLE205/BLE213` profile；
- profile 选择 Consumer 编码和 Vendor 方向；
- KEY 实现四槽状态机、CapsLock、suspend 和帧校验；
- BLE 2.13 内部继续负责身份、安全、bond、广播和连接。

“为什么早期方案没有多主机命令？”

早期范围曾被压缩为最小键盘/HID兼容，四槽业务的完整负载也尚未从二进制和抓包中确定。当前任务明确要求四主机进入首版，KEY 3.08 已确认 `0x21/0x22` 和槽 0–3，最终方案已把配对、切换、超时、迟到事件与持久化全部纳入。

“BLE 2.13 能直接刷进 C18 吗？”

软件协议层显示高度接近，C18 KEY 完成 profile 适配后具备通信基础。镜像交叉烧录的硬件安全仍缺少 bootloader、IAP、信息页、RF 校准和 BLE 板级引脚的完整证明。执行前必须通过 [06_VALIDATION_RISK_AND_ROLLBACK.md](06_VALIDATION_RISK_AND_ROLLBACK.md) 的烧录门禁。

“保持 BLE 2.13 原二进制后，设备名称还能显示 AnnePro2 吗？”

当前不能。广播和 GAP 名称位于 BLE 2.13 内，主机将看到 AP2D 名称与 2.13 GATT/Report Map。C18 KEY 可以适配其协议，但无法单独修改无线设备名称；除非后续抓包确认 BLE 2.13 存在可用的运行时名称命令。

## 当前实施基线

最终工程基线：

```text
硬件:
  C18 主控
  C18 矩阵
  C18 独立 LED MCU
  C18 USB/电源
  C18 BLE 副控板

软件:
  修改 C18 KEY 源码
  BLE 2.05 保持原二进制
  BLE 2.13 保持原二进制
  同一源码构建 C18_BLE205 与 C18_BLE213

首版:
  普通键盘
  Consumer 媒体键
  Vendor 桥接框架
  Num/Caps/Scroll
  四槽配对与切换
  UART 完整帧与容错
  USB 维护与救援

第二阶段:
  C18 USB suspend/resume
  LED MCU 挂起协调
  功耗验收
```

该基线取代所有早期的 C15 比较、修改 BLE 源码和省略四主机的方案。
