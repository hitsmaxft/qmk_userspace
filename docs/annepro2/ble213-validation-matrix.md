# C18/C18D 固定 BLE 型号验证矩阵

本页是 `codex/annepro2-ble213-backport` 当前状态的验收入口。结论严格区分
源码/host 测试、固件构建、旧版本实机记录和当前精确二进制实机验证。

## 范围与不变量

- `annepro2/c18` 固定使用 C18 BLE 2.05 协议；
- `annepro2/c18d` 是独立型号，固定使用 AP2D BLE 2.13 协议；
- 两个型号不提供运行时版本切换 API 或按键。EEPROM 只保存带型号 tag 的最后
  成功 slot；旧双配置记录和跨型号记录会失效并重写为无 slot；
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
| C18/C18D 代码隔离 | 软件通过 | C18 只链接 `c18/annepro2_ble_protocol.c`；C18D 只链接 `c18d/annepro2_ble_protocol.c` 和 2.13 slot 前导；静态门禁拒绝旧切换符号 | 两种型号各自的完整硬件回归 |
| EEPROM 型号隔离 | 软件通过 | C18、C18D、C2D 的 tag/slot 全组合 host 测试；跨型号、损坏和旧双配置格式均拒绝 | 实机升级后确认只需重新选择 slot |
| 普通键盘报告 | BLE 2.13 实机通过 | 8 字节键盘报告路径保持不变；操作者刷入本轮正式 KEY、拔除 USB 后通过 BLE 准确输入 `ap2ble-1234567890-qwerty`，无报告漏键或重复键 | BLE 2.05 尚未重测 |
| Consumer/媒体键 | BLE 2.13 实机通过 | BLE 2.05 八个 bitmap bit 与组合 `0xFF`；BLE 2.13 四个有序 16 位 Usage golden vector 均通过；拔除 USB 后，`MO(8)+Z/X/C` 的音量加、音量减、静音均由操作者确认 | BLE 2.05 实机尚未重测 |
| QMK 鼠标键 | 软件与构建通过 | C18 KEY 2.36.3 和 AP2D KEY 3.08 静态恢复出共同的 `0x60/0x04 + 8B` 发送路径；C18 3-button/wheel/pan 与 C18D 16-button/wheel 型号专用 golden vector、全零 release、范围裁剪和 loss flag host 测试通过；两目标均链接 QMK `mousekey.c` 与非空 BLE `send_mouse` | C18/BLE 2.05 与 C18D/BLE 2.13 均需实机验证移动、按下/释放、滚轮和快速连续输入；C18D horizontal pan 未确认，不宣称支持 |
| 配对 | BLE 2.13 实机通过 | BLE 2.13 slot 1 新配对成功；修复前 slot 2–4 能连接但 SMP 失败。AP2D 3.08 静态分析恢复遗漏的 `c0/17`、`40/17`、两阶段 `02/01` 前导后，操作者清空四槽 bond，并分别与四台主机重新配对成功；精确 wire vector、时序、缺失回复和 2.05 负向 gate host 测试通过 | BLE 2.05 实机配对 |
| 四主机切换 | BLE 2.13 实机通过 | 四个 slot 的主状态机、latest-intent 和迟到 ACK 隔离测试；新增 2.13-only 内部槽选择状态机；活动 keymap 已提供 `MO(9)+1…4`；四台真实主机的快速交叉切槽、连接超时和压力测试均由操作者确认成功 | BLE 2.05 四槽实机回归 |
| 上电恢复 | BLE 2.13 实机通过 | 保存 slot、被动握手、500 ms fallback、一次有界恢复和停止条件 host 测试；2026-07-28 操作者将 C18 BLE 2.13 移植整体验收为通过，其中包括断电恢复 | BLE 2.05 多轮断电重连 |
| 锁定灯/外部 LED MCU | BLE 2.13 实机通过 | C18/AP2D 静态证据确认 `20/07 00/01` 为 Caps；严格 decoder host 测试；QMK host LED bit 1 桥接；拔除 USB 后，操作者经 `MO(9)+物理 Caps` 得到准确的 `ABCabc` 开/关结果，C18 外置 LED MCU 的红灯同步亮灭；整体 2.13 验收包括切槽后状态正常 | BLE 2.05 尚未重测 |
| UART RX 健壮性 | 软件通过 | 完整 24 位长度检查、32 字节上限、delimiter、20 ms 半帧超时、噪声重同步和 timer wrap host 测试；C18D slot 回复严格校验全部 header/reserved 字节；每次 matrix scan 最多消费 64 UART 字节 | 当前固件上的 UART 噪声/断帧压力测试 |
| 日志与源码绑定 | 软件通过 | debug 启动日志包含 `QMK_GIT_HASH` 和可用的 `QMK_USERSPACE_VERSION`，并在启动 2 秒后重复一次以跨过 USB 枚举窗口；审计脚本可要求两个 revision 并在缺失时失败 | 刷写日志版后保存 revision 行 |
| 配对/连接状态提示 | 软件通过 | 弱回调报告 advertising、connecting、connected、failed；`macvim` 只使用 C18 现有 LED MCU API，在物理 1–4 上以蓝/黄/红提示，不移植 AP2D 直驱 LED | 实体灯节奏及失败后停止行为 |
| C15 非回归 | 构建通过 | C15 继续链接原 BLE 驱动，default 固件构建为 37,508 字节 | C15 实机回归 |
| BLE 2.13 二进制不变 | 通过 | 官方输入按精确大小与 SHA-256 校验；QMK 构建不修改或嵌入 BLE 镜像 | IAP 仍没有已验证的写后 readback |

“软件通过”不表示射频连接、bond 数据、macOS 收包或 LED 硬件已通过。

## 2026-08-01 可复现软件验证

全部命令从 userspace 根目录运行：

```sh
direnv exec . just annepro2-test
direnv exec . just annepro2
direnv exec . just annepro2-c18d
direnv exec . just annepro2-c15
direnv exec . just annepro2-c2d
```

也可用 `direnv exec . just annepro2-validate` 顺序执行上述门禁。

| 构建 | 大小 | SHA-256 |
|---|---:|---|
| C18，固定 BLE 2.05 | 46,824 B | `8128e3e0d6514f68913bd6a6e6d27025fbcc90c535a0b8475f5cf94b1c7055bf` |
| C18D，固定 BLE 2.13 | 47,360 B | `2879861220e0a4b212d0af86850a8021b34f14afc92214902b5e0fc244614eb9` |
| C15 default | 37,508 B | `1e36e0df1ded60b001ddf60000cb67b5c3eb1d1ed9591bda8005d567237b4cfd` |
| C2D default | 25,448 B | `28457ec22a0bc3cd3bea9d5d7b923f7311602d1f63a3c5649dc737ad834ae1fc` |

以上产物由本页所在 userspace 工作树与主 checkout 已初始化的 QMK 工作树构建。
哈希只用于定位本次本机构建；QMK 元数据或工具链变化可能产生不同哈希。依赖
文件确认 C18 产物没有链接 C18D/2.13 对象，C18D 产物没有链接 C18/2.05
协议对象。

host 测试使用 `-std=c11 -Wall -Wextra -Werror`，覆盖：

- C18/C18D Consumer 编码和 release-all；型号专用 Mouse `0x60/0x04`
  编码、全零 release、按钮能力、滚轮/pan 限制与裁剪；
- C18 2.05 带兼容尾字节、C18D 2.13 不带尾字节的完整
  broadcast/connect 物理 wire vector；
- `20/07 00/01` Caps Lock 严格解码，以及错误长度、routing、opcode 和
  value 不改变状态；
- 型号 tag/slot EEPROM 编解码、旧格式、跨型号、损坏和边界输入；
- tap/hold、四槽、命令重试、ACK/HID-ready 分离、快速切槽、断开/解绑、
  有界恢复和 32 位计时器回绕；主命令实际发送前不启动超时，也不接受迟到
  ACK/handshake；非零 ACK 不推进状态；ACTIVE 状态的重复 handshake 不覆盖
  显式 USB/BLE 输出选择；
- C18D 的 query/select/prepare 精确 wire vector、严格回复 header/reserved
  校验、5/20/20 ms 非阻塞顺序、同槽快路径、无回复保守路径、取消与 timer
  wrap；C18 构建不含该路径；
- UART 完整帧、payload 内 `0x7B`、无效长度高字节、超长、错误 delimiter、
  半帧超时、重同步和计时器回绕；
- C18 外部 LED 驱动文件相对 QMK 分支基线无变化。

新增与修改的 C18/C18D/parser/slot/state/test 文件通过
`clang-format --dry-run --Werror`，两个 Git 仓库通过 `git diff --check`；
`qmk lint` 对 `annepro2/c18`、`annepro2/c18d` 和 `annepro2/c2d` 全部通过。

## 2026-07-27 实机观察

一次断电重启日志观察到 saved slot 0 在 MCU 时间约 504 ms 发出 cold-start
broadcast，约 10 ms 后收到 ACK，约 5.64 s 后收到 `20/0c` 并进入 BLE
route。随后普通键产生三条 `tx keyboard report`。媒体键测试产生 36 条
Consumer 日志，其中明确包含：

- `00E9`（音量加）及 `0000` release；
- `00EA`（音量减）及 `0000` release；
- `00E2`（静音）及 `0000` release；
- 全部为 `bytes=8 profile=1`；这是拆分前 debug 日志字段，符合当时的 BLE
  2.13 路径。

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

加入 AP2D 原厂 slot query/select/two-stage prepare 前导后，操作者确认此前
异常的 slot 切换已经恢复正常。2026-07-28 又清空四个 slot 的 bond，分别与
四台主机重新配对，并完成快速交叉切槽、连接超时和压力测试，全部成功。这是
BLE 2.13 实机行为证据；由于本次反馈没有同时保存启动 revision 行，仍不把
它绑定为上表 SHA-256 镜像的精确二进制验证。

在上述逐项结果基础上，操作者于 2026-07-28 明确认定 C18 KEY 对官方 BLE
2.13 的 backport 完全验证通过。该验收覆盖普通键盘、媒体键、Caps 与 C18
外置锁定灯、清空 bond 后的四槽配对、四主机切换、连接超时、压力测试和断电
恢复。缺少启动 revision 行只影响精确构建可追溯性，不再作为 2.13 功能验收
的阻塞项。

此前发现的遗留 `.qmk-wrapped console` 独占问题已经通过 wrapper 的
子进程回收修复；每次测试结束仍需复查 listener，防止旧进程污染结论。

## 固定型号拆分后的剩余门禁

此前 BLE 2.13 功能验收已经完成，但当时使用的是 C18 双配置实现。拆分后仍需：

1. 在 C18 + 官方 BLE 2.05 上重复普通键盘、媒体、Caps、配对、四槽和断电
   恢复，并确认日志和二进制中都没有 2.13 slot 前导。
2. 在 C18D + 官方 BLE 2.13 上复测上述项目，并保存当前精确日志固件的 build
   revision；此前通过结果不能直接归属新的 C18D 二进制。
3. 若运行时出现非零 ACK、迟到 ACK、半帧超时或 parser 重同步，再保存对应
   样本用于长期回归。

静态隔离、host 测试和四型号构建已通过；C18/C18D 拆分后的实机非回归仍待
执行。IAP readback 仍是工具链证据边界。
