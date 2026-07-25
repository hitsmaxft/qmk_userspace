# C18 KEY 双 BLE 首版验证矩阵

本页是 `codex/annepro2-ble213-backport` 当前状态的验收入口，记录时的 QMK
提交为 `d674a458db`。结论严格区分源码/host 测试、固件构建、旧版本实机记录
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
  只保证 C18 原灯控路径不被修改并等待实机回归。

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
| 锁定灯/外部 LED MCU | 源码非回归通过 | `ap2_led.*`、`protocol.*`、`rgb_driver.*` 与分支基线逐字节无差异 | Caps/Num 状态和灯效的 C18 实机回归；不能用 AP2D LED 代码代替 |
| UART RX 健壮性 | 软件通过 | 完整 24 位长度检查、32 字节上限、delimiter、20 ms 半帧超时、噪声重同步和 timer wrap host 测试 | 当前固件上的 UART 噪声/断帧压力测试 |
| 日志与源码绑定 | 软件通过 | debug 启动日志包含 `QMK_GIT_HASH` 和可用的 `QMK_USERSPACE_VERSION` | 刷写后保存 revision 行 |
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
| C18，BLE 2.05 默认 profile | 43,844 B | `6c4b180a2f80b5279043b53d2697b6888cec0f9ca5f7bc4e9567e6d9329bd049` |
| C18，BLE 2.13 默认 profile | 43,844 B | `8733df1d4cae3d2a434316378a70b5bdfb1ef4844d346eb8ec12c41a731fa830` |
| C15 default | 37,504 B | `4712f841f5c4b61ed583dfe862d822caa557c13f0a2485a50a6602ea33b0d306` |

这些哈希只用于定位本次本机构建；QMK 构建元数据或工具链变化可能产生不同哈希。

host 测试使用 `-std=c11 -Wall -Wextra -Werror`，覆盖：

- BLE 2.05/2.13 Consumer 编码和 release-all；
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

## 当前实机门禁

下一次刷入当前日志固件后，按以下顺序验收：

1. 官方 BLE 2.13：slot 1–4 分别新配对、短按重连、快速交叉切换和断电恢复；
2. 普通键盘及全部实际使用的媒体键；日志中只能在 `0x20/0x0C` 后出现
   `route ble`；
3. C18 外部 LED MCU 的 Caps/锁定灯和现有灯效，不引入 AP2D LED 路径；
4. 换回官方 BLE 2.05，重复普通键盘、媒体、配对、四槽和断电恢复；
5. 记录非零 ACK、迟到 ACK、半帧超时或 parser 重同步样本。

只有上述硬件项完成后，首版的“覆盖普通键盘、媒体键、锁定灯、配对和四主机
切换”才可以整体标记为通过。
