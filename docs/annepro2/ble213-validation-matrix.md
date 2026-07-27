# C18 KEY 双 BLE 首版验证矩阵

本页是 `codex/annepro2-ble213-backport` 当前状态的验收入口，记录时的 QMK
提交为 `11c08ff0dc`。userspace 对应本页所在提交；结论严格区分源码/host
测试、固件构建、旧版本实机记录和当前精确二进制实机验证。

## 范围与不变量

- 改造主体是 QMK 的 Anne Pro 2 C18 KEY 源码；
- C18 同一套 KEY 固件可在 EEPROM 中选择 `C18_BLE205` 或
  `AP2D_BLE213` profile；
- 官方 BLE 2.13 镜像保持原样，SHA-256 为
  `1b904ae9cd8bf6835c0b77c72618256b701a2c3b74dc04e9dddb8a388bdfc73d`；
- `HEXCORE AnnePro 2C` 是另行生成的可选兼容名称镜像，不是本轮 backport
  构建或实机结论所使用的官方镜像；
- AP2D 已取消 C18 的外部 LED MCU。本轮不移植 AP2D 直驱 LED/RGB 代码，
  保持 C18 原灯控 driver 不变；共享的 `20/07` Caps 逻辑状态由 QMK 标准
  host LED 接口桥接到现有 C18 LED MCU keymap 行为。

## 需求覆盖

| 项目 | 当前结论 | 证据 | 尚缺 |
|---|---|---|---|
| C18 KEY 为主体 | 通过 | C18 专用实现位于 `keyboards/annepro2/c18/annepro2_ble.c`；profile/parser/state 只由 `c18/rules.mk` 链接 | 无 |
| BLE 2.05 与 2.13 双 profile | 软件通过 | profile/slot EEPROM 全组合 host 测试；两种默认 profile 均完成 C18 构建 | 两种 BLE 模块各自的完整硬件回归 |
| 普通键盘报告 | BLE 2.13 实机通过 | 8 字节键盘报告路径保持不变；操作者刷入本轮正式 KEY、拔除 USB 后通过 BLE 准确输入 `ap2ble-1234567890-qwerty`，无报告漏键或重复键 | BLE 2.05 尚未重测 |
| Consumer/媒体键 | BLE 2.13 实机通过 | BLE 2.05 八个 bitmap bit 与组合 `0xFF`；BLE 2.13 四个有序 16 位 Usage golden vector 均通过；拔除 USB 后，`MO(8)+Z/X/C` 的音量加、音量减、静音均由操作者确认 | BLE 2.05 实机尚未重测 |
| 配对 | 修复待实测 | BLE 2.13 slot 1 新配对成功；slot 2–4 实测能连接但 SMP 失败。AP2D 3.08 静态分析恢复了遗漏的 `c0/17`、`40/17`、两阶段 `02/01` 前导；精确 wire vector、时序、缺失回复和 2.05 负向 gate host 测试通过 | 刷入新 KEY 后重测 slot 2–4；BLE 2.05 实机配对 |
| 四主机切换 | 修复待实测 | 四个 slot 的主状态机、latest-intent 和迟到 ACK 隔离测试；新增 2.13-only 内部槽选择状态机；活动 keymap 已提供 `MO(9)+1…4` | 四台真实 host 的配对、切换、超时和压力测试 |
| 上电恢复 | 软件通过 | 保存 slot、被动握手、500 ms fallback、一次有界恢复和停止条件测试 | 两种 BLE 固件的多轮断电重连 |
| 锁定灯/外部 LED MCU | BLE 2.13 实机通过 | C18/AP2D 静态证据确认 `20/07 00/01` 为 Caps；严格 decoder host 测试；QMK host LED bit 1 桥接；拔除 USB 后，操作者经 `MO(9)+物理 Caps` 得到准确的 `ABCabc` 开/关结果，C18 外置 LED MCU 的红灯同步亮灭 | 当前 debug 固件的 `20/07 01/00` 与切槽清理仍待日志验证；BLE 2.05 尚未重测 |
| UART RX 健壮性 | 软件通过 | 完整 24 位长度检查、32 字节上限、delimiter、20 ms 半帧超时、噪声重同步和 timer wrap host 测试 | 当前固件上的 UART 噪声/断帧压力测试 |
| 日志与源码绑定 | 软件通过 | debug 启动日志包含 `QMK_GIT_HASH` 和可用的 `QMK_USERSPACE_VERSION`，并在启动 2 秒后重复一次以跨过 USB 枚举窗口；审计脚本可要求两个 revision 并在缺失时失败 | 刷写日志版后保存 revision 行 |
| 配对/连接状态提示 | 软件通过 | 弱回调报告 advertising、connecting、connected、failed；`macvim` 只使用 C18 现有 LED MCU API，在物理 1–4 上以蓝/黄/红提示，不移植 AP2D 直驱 LED | 实体灯节奏及失败后停止行为 |
| C15 非回归 | 构建通过 | C15 继续链接原 BLE 驱动，default 固件构建为 37,504 字节 | C15 实机回归 |
| BLE 2.13 二进制不变 | 通过 | 官方输入按精确大小与 SHA-256 校验；QMK 构建不修改或嵌入 BLE 镜像 | IAP 仍没有已验证的写后 readback |

“软件通过”不表示射频连接、bond 数据、macOS 收包或 LED 硬件已通过。

## 2026-07-27 可复现软件验证

全部命令从 userspace 根目录运行：

```sh
direnv exec . just annepro2-test
direnv exec . just annepro2
direnv exec . just annepro2-ble213
direnv exec . just annepro2-c15
direnv exec . just annepro2-ble213-log
```

也可用 `direnv exec . just annepro2-validate` 顺序执行上述门禁。

| 构建 | 大小 | SHA-256 |
|---|---:|---|
| C18，BLE 2.05 默认 profile | 44,844 B | `3f714f36b26a72eac7f225a039c4ef44464b456e26683ab5c5d01f814b0d2017` |
| C18，BLE 2.13 默认 profile | 44,844 B | `288ebd293fe40e53de89ad7619c45d5f6f398b717b56dae6b1fe81b127442560` |
| C18，BLE 2.13 debug | 44,420 B | `4fa7a18054dd88167d1b082f431432cdffd4aab8b05fa0a7a5c4a858d8a1d2d7` |
| C15 default | 37,504 B | `4712f841f5c4b61ed583dfe862d822caa557c13f0a2485a50a6602ea33b0d306` |

三个 C18 产物与 C15 产物由 QMK `11c08ff0dc` 和本页所在 userspace 工作树
构建。debug 镜像的 strings 同时包含 `11c08ff0dc` 和四条 2.13 slot 前导
日志格式。哈希只用于定位本机构建；QMK 构建元数据或工具链变化可能产生
不同哈希。

host 测试使用 `-std=c11 -Wall -Wextra -Werror`，覆盖：

- BLE 2.05/2.13 Consumer 编码和 release-all；
- `20/07 00/01` Caps Lock 严格解码，以及错误长度、routing、opcode 和
  value 不改变状态；
- profile/slot EEPROM 编解码、损坏和边界输入；
- tap/hold、四槽、命令重试、ACK/HID-ready 分离、快速切槽、断开/解绑、
  有界恢复和 32 位计时器回绕；
- BLE 2.13 的 query/select/prepare 精确 wire vector、5/20/20 ms 非阻塞
  顺序、同槽快路径、无回复保守路径、取消与 timer wrap；BLE 2.05 profile
  明确不进入该路径；
- UART 完整帧、payload 内 `0x7B`、无效长度高字节、超长、错误 delimiter、
  半帧超时、重同步和计时器回绕；
- C18 外部 LED 驱动文件相对 QMK 分支基线无变化。

新增与修改的 C18/parser/profile/state/test 文件通过
`clang-format --dry-run --Werror`，两个 Git 仓库通过 `git diff --check`。
`qmk lint -kb annepro2/c18` 仍会报告上游已有的 `ap2_led.h`/默认 keymap
license header 和带连字符 keymap 名称问题；这些文件与命名均不在本分支差异中。

## 2026-07-27 实机观察

一次断电重启日志观察到 saved slot 0 在 MCU 时间约 504 ms 发出 cold-start
broadcast，约 10 ms 后收到 ACK，约 5.64 s 后收到 `20/0c` 并进入 BLE
route。随后普通键产生三条 `tx keyboard report`。媒体键测试产生 36 条
Consumer 日志，其中明确包含：

- `00E9`（音量加）及 `0000` release；
- `00EA`（音量减）及 `0000` release；
- `00E2`（静音）及 `0000` release；
- 全部为 `bytes=8 profile=1`，符合 BLE 2.13 profile。

同一日志还包含四次
`7B 12 35 00 03 00 00 7D 20 07 00`，但没有
`rx caps lock=0`。这反而证明当时刷入的 KEY 早于严格 Caps decoder 版本；
再加上日志没有启动 build revision，因此这些观察只确认运行时 UART 路径，
不能归属 `01e6d3f18d`，也不能证明插着 USB 时 macOS 的按键来自 BLE。

活动 keymap 原先缺少 Caps 和 slot 4 的回归入口，现已修正：

- `MO(9)+1/2/3/4` 对应 BLE slot 1–4；
- `MO(9)+物理 Caps` 发送 `KC_CAPS`；
- `MO(9)+5/6` 分别选择 `DF(1)`/`DF(0)`。

操作者随后成功刷入 2026-07-27 正式 BLE 2.13 KEY。USB 正常枚举，但
`qmk console -l` 没有设备，符合正式构建未启用 console 的预期。拔除 USB
后，操作者完成以下纯 BLE 主机行为：

- 准确输入 `ap2ble-1234567890-qwerty`；
- `MO(8)+Z/X/C` 的音量加、音量减和静音均生效；
- 两次 `MO(9)+物理 Caps` 分别打开和关闭 Caps，输入结果为 `ABCabc`，物理
  Caps 键位红灯同步亮灭。

这三项不再依赖插线状态或 UART 推断。需要精确 UART 取证时应改刷上表 debug
镜像；切槽后旧 Caps 状态的清理仍要在四槽回归中检查。

此前发现的遗留 `.qmk-wrapped console` 独占问题已经通过 wrapper 的
子进程回收修复；每次测试结束仍需复查 listener，防止旧进程污染结论。

## 当前实机门禁

下一次刷入当前日志固件后，按以下顺序验收：

1. 官方 BLE 2.13：slot 1–4 分别新配对、短按重连、快速交叉切换和断电恢复；
2. 普通键盘及全部实际使用的媒体键；日志中只能在 `0x20/0x0C` 后出现
   `route ble`；
3. C18 外部 LED MCU 的 Caps 灯和现有灯效：日志应出现
   `rx caps lock=1/0`，物理灯位同步亮灭，切槽时不保留旧主机状态；不引入
   AP2D LED 路径；
4. 换回官方 BLE 2.05，重复普通键盘、媒体、配对、四槽和断电恢复；
5. 记录非零 ACK、迟到 ACK、半帧超时或 parser 重同步样本。

只有上述硬件项完成后，首版的“覆盖普通键盘、媒体键、锁定灯、配对和四主机
切换”才可以整体标记为通过。
