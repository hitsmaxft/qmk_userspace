# 样本、版本链与证据边界

本报告集仅使用官方固件和芯片/协议官方文档建立结论。社区固件、QMK 板级定义和非官方引脚表没有进入最终证据链。

## 证据等级

| 等级 | 定义 | 用途 |
|---|---|---|
| A | 官方二进制中的直接指令、常量、描述符、字符串或逐字节差分 | 可直接形成实现要求 |
| B | 官方发布说明与二进制变化互相印证 | 可进入方案，仍需实机确认最终行为 |
| C | 依据静态结构作出的工程推断 | 只用于设计验证项，不作为已证实功能 |

涉及烧录安全、RF 校准、IAP 边界、连接参数最终协商值和功耗的结论必须经过实机验证。静态二进制无法替代这些测试。

## 样本完整性

| 产品/阶段 | 文件 | 大小 | SHA-256 |
|---|---|---:|---|
| C18 KEY 2.36 | `key-c18-2.36.3.bin` | 46,779 B | `b9bfa750e8c7ccdbed0c1f6de8aeb4eb1d569dc5cc075afdb2a93fe5e20730de` |
| C18 BLE 2.05 | `ap2_c18_0205_ble.bin` | 155,648 B | `52dc5c6542ad9b30915ea07f042b46ef19cd8acf4f2dce286a0dbdf11ce7cb92` |
| AP2D KEY 3.05 | `annepro2_discovery_KEY_APP.bin` | 63,408 B | `e3b5b40ec370c4147dcba8817d67b3d057ed6f33ee845f756741970ca5ad81b2` |
| AP2D BLE 2.09 | `annepro2_discovery_ble.bin` | 155,648 B | `36322742dc061ee217d500be0392741e2e4b63a2ae27453df3f153a4b9dc0ef3` |
| AP2D KEY 3.06 | `annepro2_discovery_KEY_APP.bin` | 63,822 B | `8914176a3d34214668cccd700d10ee81c060763c73fbb0224e23b48bdb054b01` |
| AP2D BLE 2.10 | `annepro2_discovery_ble.bin` | 155,648 B | `3494ffedf34708e3b0ddcb3fa2d26d7ba7bbad32e2b2b9e9d330dbbd5e0c8eea` |
| AP2D KEY 3.08 | `annepro2_discovery_KEY_APP.bin` | 64,116 B | `ab7a91fe4150b18face32df40cb122b8fa95e4821ebeac63927b60ca6f69f7c1` |
| AP2D BLE 2.13 | `annepro2_discovery_ble.bin` | 155,648 B | `1b904ae9cd8bf6835c0b77c72618256b701a2c3b74dc04e9dddb8a388bdfc73d` |

以上文件均与对应官方清单中的 SHA-256 一致。BLE 2.13 由用户提供后完成强校验，已经排除同名错误文件或修改版镜像。

KEY 架构：

- C18 KEY 与 AP2D KEY 均为 ARM Thumb 应用，链接基址 `0x4000`。
- C18 初始 SP 为 `0x20001DB0`，AP2D 3.08 初始 SP 为 `0x20003418`。
- AP2D KEY 3.08 比 C18 KEY 2.36 大 17,337 B，体积约为其 1.371 倍。

BLE 架构：

- C18 BLE 2.05、AP2D BLE 2.09/2.10/2.13 都具有 CC254x 风格 8051 指令和向量布局。
- 四份 BLE 镜像大小均为 155,648 B。
- C18 BLE 2.05 内含 `experimentalBLE-1.5.0`。
- AP2D BLE 2.10 内含 `experimental BLE-1.5.0`。
- AP2D BLE 2.13 内含 `experimental BLE-1.5.2`。

相同大小和相同芯片家族不能单独证明镜像可以安全交叉烧录。还需验证 bootloader、IAP、SNV、RF 配置、信息页和板级串口引脚。

## 官方版本链

C18 稳定包 `2.36.3` 发布于 2021-11-04：

- KEY 2.36；
- LED 2.33；
- BLE 2.05；
- 主要新增 CapsLock 指示灯颜色自定义。

C18 的 `2.34.6` 已经使用 BLE 2.05，并要求升级后删除、重新绑定。该版本增加蓝牙模式下的 ObinsKit 控制接口，并修复广播结束后指示灯概率不熄灭。

AP2D 正式版本链：

| 套件 | 日期 | KEY | BLE | 官方说明重点 |
|---|---|---:|---:|---|
| 3.0.4 | 2022-11-20 | 3.04 | 2.09 | 首个正式版本 |
| 3.05 | 2022-11-22 | 3.05 | 2.09 | 按键释放与虚拟鼠标释放丢失 |
| 3.06 | 2023-01-05 | 3.06 | 2.10 | macOS 媒体键、Tap、蓝牙功耗/连接、多设备切换 |
| 3.08 | 2023-03-20 | 3.08 | 2.13 | 双键盘同主机、macOS CapsLock、USB suspend、多设备切换 |

3.08 的官方提示要求升级后删除并重新绑定键盘。这与 BLE 2.13 修改本地安全记录的二进制证据一致。

## 研究方法

1. 读取 C18 与 AP2D 官方清单，锁定稳定版与相邻版本。
2. 对每个样本计算 SHA-256，并与清单比对。
3. 对 ARM KEY 镜像按 `0x4000` 链接基址反汇编，定位 GPIO、UART、帧构造/解析、LED Output、USB 状态机和线程入口。
4. 对 CC254x BLE 镜像进行 8051 控制流和数据区分析，定位 Report Map、广告数据、栈版本字符串、SNV 记录和工厂地址读取逻辑。
5. 使用 AP2D KEY 3.06→3.08 与 BLE 2.10→2.13 的相邻版本差分，把稳定版四项修复从 3.0 重构中分离。
6. 逐字节比较 C18 BLE 2.05、AP2D BLE 2.10 和 2.13 的 HID Report Map。
7. 通过 TI 文档核对 `GAPROLE_IRK`、`GAPROLE_SRK`、`GAPROLE_SIGNCOUNTER` 的长度和 CC2541 工厂 IEEE 地址性质。

## 官方来源

- [C18 官方固件清单](https://releases.obins.net/annepro2c18/firmware/latest.json)
- [AP2D 官方固件清单](https://s5.hexcore.xyz/releases/firmware/annepro2d/latest.json)
- [Anne Pro 2D 官方手册](https://service.hexcore.xyz/manual/annepro2d/)
- [Anne Pro 2 官方手册](https://service.hexcore.xyz/manual/annepro2/)
- [Anne Pro 2D 产品页](https://www.hexcore.xyz/annepro2d)
- [Anne Pro 2D 发布文章](https://www.hexcore.xyz/news/launch_anne_pro_2d)
- [TI BLE-Stack 1.5.2](https://www.ti.com/tool/download/BLE-STACK-1-X/1.05.02.00)
- [TI CC2540/41 BLE Software Developer's Guide](https://www.ti.com/lit/ug/swru271i/swru271i.pdf)
- [TI CC2541 数据表](https://www.ti.com/lit/gpn/CC2541)
- [USB HID 1.11](https://www.usb.org/sites/default/files/documents/hid1_11.pdf)
- [USB HID Usage Tables](https://www.usb.org/sites/default/files/hut1_6.pdf)

固件服务器目前可能返回错误，因此报告包附带分析时取得的两份清单副本。副本用于复核版本、文件名、日期、说明和哈希，不替代官方来源。

## 已证实与待验证边界

| 项目 | 状态 | 依据 |
|---|---|---|
| PA4/PA5、AF6、115200 | 已证实 | 两份 KEY 二进制 |
| 基础帧构造函数保持同族 | 已证实 | KEY 逐字节比较 |
| 键盘 Report ID 1 相同 | 已证实 | 三份 BLE Report Map |
| Vendor ID 2 方向交换 | 已证实 | BLE 2.05/2.10/2.13 Report Map |
| Consumer ID 3 布局变化 | 已证实 | BLE 2.05/2.10/2.13 Report Map |
| BLE 2.13 使用设备唯一地址校验安全记录 | 已证实 | BLE 2.13 指令与 TI 地址定义 |
| 双键盘同主机修复来自该安全记录变化 | 高可信 | 官方说明与相邻版本差分互证 |
| AP2D 2.13 实际协商连接间隔 | 待实测 | 广告只提供 preferred interval |
| BLE 2.13 在 C18 副控板的 RF/bootloader 安全性 | 待实测 | 静态镜像无法覆盖板级契约 |
| 四槽命令完整 UART 负载格式 | 部分已知 | 已知 `0x21/0x22`、槽 0–3；需抓包确认外层字段 |
| 单固件自动识别 BLE 版本 | 尚未确认 | 未定位无副作用版本查询命令 |
