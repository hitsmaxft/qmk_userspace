# C18 KEY 双 BLE 首版验证矩阵

本页是 `codex/annepro2-ble213-backport` 当前状态的验收入口，记录时的 QMK
提交为 `e3dfb6829d`。结论严格区分源码/host 测试、固件构建、旧版本实机记录
和当前精确二进制实机验证。

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
| 普通键盘报告 | 部分通过 | 8 字节键盘报告路径保持不变；此前官方 BLE 2.13 + C18 实机已连接并正常输入 | 当前分支精确二进制尚未重新刷写验证；BLE 2.05 尚未重测 |
| Consumer/媒体键 | 软件通过 | BLE 2.05 八个 bitmap bit 与组合 `0xFF`；BLE 2.13 四个有序 16 位 Usage golden vector 均通过 | 两种 BLE 固件上的媒体键实机输入 |
| 配对 | 部分通过 | 四槽 broadcast/tap-hold/ACK/HID-ready 状态路径 host 测试通过；此前 BLE 2.13 slot 1 新配对成功 | slot 2–4 以及 BLE 2.05 实机配对 |
| 四主机切换 | 软件通过 | 四个 slot 的 connect、broadcast、ACK、HID-ready、持久化循环测试；latest-intent 和迟到 ACK 隔离测试 | 四台真实 host 的切换、超时和压力测试 |
| 上电恢复 | 软件通过 | 保存 slot、被动握手、500 ms fallback、一次有界恢复和停止条件测试 | 两种 BLE 固件的多轮断电重连 |
| 锁定灯/外部 LED MCU | 软件通过 | C18/AP2D 静态证据确认 `20/07 00/01` 为 Caps；严格 decoder host 测试；QMK host LED bit 1 桥接；`macvim` 使用 C18 既有 sticky-key API；LED driver 文件与基线逐字节相同 | 当前精确固件的 `20/07 01/00` 日志、Caps 实体灯亮灭与重连/切槽清理；Num/Scroll 尚无共享 UART 帧证据 |
| UART RX 健壮性 | 软件通过 | 完整 24 位长度检查、32 字节上限、delimiter、20 ms 半帧超时、噪声重同步和 timer wrap host 测试 | 当前固件上的 UART 噪声/断帧压力测试 |
| 日志与源码绑定 | 软件通过 | debug 启动日志包含 `QMK_GIT_HASH` 和可用的 `QMK_USERSPACE_VERSION`；审计脚本可要求两个 revision 并在缺失时失败 | 刷写后保存 revision 行 |
| C15 非回归 | 构建通过 | C15 继续链接原 BLE 驱动，default 固件构建为 37,504 字节 | C15 实机回归 |
| BLE 2.13 二进制不变 | 通过 | 官方输入按精确大小与 SHA-256 校验；QMK 构建不修改或嵌入 BLE 镜像 | IAP 仍没有已验证的写后 readback |

“软件通过”不表示射频连接、bond 数据、macOS 收包或 LED 硬件已通过。

## 2026-07-26 可复现软件验证

全部命令从 userspace 根目录运行：

```sh
direnv exec . just annepro2-test
direnv exec . just annepro2
direnv exec . just annepro2-ble213
direnv exec . just annepro2-c15
```

也可用 `direnv exec . just annepro2-validate` 顺序执行上述门禁。

| 构建 | 大小 | SHA-256 |
|---|---:|---|
| C18，BLE 2.05 默认 profile | 43,996 B | `3db6c726118643697f8d831b96d07f5a1913d07d2c566d5f6e2ee24e1e13fe41` |
| C18，BLE 2.13 默认 profile | 43,996 B | `5ee4d1f46022c63fac4a5a29d3ecf631371da6094bc046d4bc8557da8c94d7f7` |
| C15 default | 37,504 B | `4712f841f5c4b61ed583dfe862d822caa557c13f0a2485a50a6602ea33b0d306` |

这些哈希只用于定位本次本机构建；QMK 构建元数据或工具链变化可能产生不同哈希。

host 测试使用 `-std=c11 -Wall -Wextra -Werror`，覆盖：

- BLE 2.05/2.13 Consumer 编码和 release-all；
- `20/07 00/01` Caps Lock 严格解码，以及错误长度、routing、opcode 和
  value 不改变状态；
- profile/slot EEPROM 编解码、损坏和边界输入；
- tap/hold、四槽、命令重试、ACK/HID-ready 分离、快速切槽、断开/解绑、
  有界恢复和 32 位计时器回绕；
- UART 完整帧、payload 内 `0x7B`、无效长度高字节、超长、错误 delimiter、
  半帧超时、重同步和计时器回绕；
- C18 外部 LED 驱动文件相对 QMK 分支基线无变化。

新增与修改的 C18/parser/profile/state/test 文件通过
`clang-format --dry-run --Werror`，两个 Git 仓库通过 `git diff --check`。
`qmk lint -kb annepro2/c18` 仍会报告上游已有的 `ap2_led.h`/默认 keymap
license header 和带连字符 keymap 名称问题；这些文件与命名均不在本分支差异中。

## 2026-07-26 最新实机观察

操作者确认一次 IAP 刷写已经完成，随后蓝牙可正常使用。当前 macOS
`system_profiler` 条目为 `HEXCORE AnnePro 2D`、`BLE-1.5.2`、Keyboard，
USB 侧枚举为 `AC20:8009 Anne Pro 2 C18 (QMK)`。这与官方 BLE 2.13 正常启动
一致，但 macOS 条目可能来自配对缓存，且 USB 产品名不包含 KEY revision。

初次 `qmk console` 能列出设备但无法打开 console HID 接口。进程审计随后
发现一个从先前会话遗留的 `.qmk-wrapped console` 正独占接口；终止该精确
进程后，当前 KEY 的 console 立即连接成功。因此故障不表示 KEY 缺少日志
接口。监听开始时启动 revision 已经错过，本轮也尚未采集新的按键事件，所以
仍不能确认硬件上运行的是当前 `e3dfb6829d` KEY，也不能把它算作 2C 名称
变体验证。保持当前可用 BLE 不再重复刷写；后续测试先启动受控 console，再
触发按键或重启，并在结束时显式关闭监听器。

旧的 `/tmp/annepro2-console-fast-switch.log` 经结构化审计只得到：

- 无 build revision，无法归属当前固件；
- 仅覆盖 slot 0/1，包含 5 次 broadcast、3 次 connect、4 次 HID-ready、
  2 次 BLE route 和 3 次 handshake timeout；
- 只见原始 `20/07 00`，没有 Caps on/off 解码、keyboard debug report 或
  consumer report。

因此旧日志保留为协议演进证据，不关闭普通键、媒体键、Caps 或四槽的当前实机
门禁。

为下一次受控验证准备的 BLE 2.13 debug KEY 由已推送 userspace
`6c6bf8ba2c` 和 QMK `e3dfb6829d` 构建，大小 43,080 B，SHA-256 为
`82ab7fdb8a3b5fca620c074e8ad379a80c5ace09974bfe70ab1afa66f80291bd`；
镜像内版本为 `latest-154-g6c6bf8` 与 `e3dfb6829d*`。构建时仅对 Git 状态
忽略固件归档子模块中无关的工作树改动，未改动源文件。该镜像是忽略的本地
产物，目前尚未刷入，不能作为硬件通过证据。

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
