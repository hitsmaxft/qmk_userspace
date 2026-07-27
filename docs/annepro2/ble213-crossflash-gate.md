# C18 BLE 2.13 交叉刷写门禁

本页记录 AP2D BLE 2.13 写入 C18 BLE 副控的门禁、工具修复和实机结果。
2026-07-26 已使用修复后的 `annepro2-tools` 完成官方镜像传输并启动 BLE；
原始镜像仍保持不变，传输日志只能证明每条 IAP 请求收到成功状态，不能代替
独立 flash readback。

## 已确认的镜像边界

首版使用的两个官方文件：

| 镜像 | 大小 | SHA-256 | 最后一个非 `FF` 字节 |
|---|---:|---|---:|
| C18 BLE 2.05 | `0x26000` | `52dc5c6542ad9b30915ea07f042b46ef19cd8acf4f2dce286a0dbdf11ce7cb92` | `0x1F480` |
| AP2D BLE 2.13 | `0x26000` | `1b904ae9cd8bf6835c0b77c72618256b701a2c3b74dc04e9dddb8a388bdfc73d` | `0x1E24C` |

两者均满足：

- 文件从 CC254x 代码地址 `0` 开始，首指令是 8051 `LJMP`；
- 文件长度能被 BLE IAP 的 32 字节写入块整除；
- `0x20000..0x25FFF` 全部为 `0xFF`；
- 所有非擦除数据均位于 `0x00000..0x1FFFF`。

仓库内其余 C18/AP2D 官方 BLE 包也全部是 `0x26000` 字节。这证明
`0x26000` 是官方更新包的一致包络，但不能单独证明 bootloader 的位置或 IAP
擦除范围。

静态检查：

```sh
direnv exec . just annepro2-ble-crossflash-check
```

该命令只读取文件，不访问键盘。它会拒绝未知哈希、长度变化、非 32 字节对齐、
错误 reset vector 或 `0x20000` 以上出现非 `FF` 数据。

只读 IAP 计划：

```sh
direnv exec . just annepro2-ble-crossflash-plan \
  /tmp/annepro2-ble213-iap-plan.json
```

计划器没有 HID 依赖和硬件 transport，只生成当前已知协议的请求字节摘要：

- BLE route 固定为 `0x51`；
- C18 IAP 描述符报告的 BLE transport base 为 `0x4000`；
- erase payload 固定为 `02 43 00 40 00 00`；
- `0x26000` 镜像固定拆为 4,864 个 32 字节写块；
- 首块 transport 地址为 `0x04000`，末块为 `0x29FE0`，结束地址为
  `0x2A000`；
- JSON 明确标记 `executable=false`、`hardware_access=false`；
- 回复策略要求 target/command/key 匹配且 status 为零。

官方文件仍从 image offset 0 开始；`0x4000` 是 KEY IAP 报告并使用的 transport
地址，不能直接解释为 CC254x 物理 flash 中需要保留的 `0x0000..0x3FFF`。
该计划只证明请求字节和边界可重复生成，不提供 HID 传输或 readback。

## CC2541 必须保留的数据

TI 的 CC2541 产品资料确认该器件有 128 KiB 与 256 KiB 两种 flash 版本。本项目
的 `0x26000` 更新包已经超过 128 KiB，因此目标必须按 256 KiB 器件审计。

TI CC254x 用户指南确认：

- Information Page 是独立的 2 KiB 只读区域；
- CC2540/CC2541 的 48 位工厂 IEEE 地址位于 Information Page 映射后的
  XDATA `0x780E`，低字节在前；
- 普通 flash 由 2 KiB 页面组成。

TI BLE Software Developer's Guide 还说明 BLE-Stack 默认把最后两个 2 KiB
flash 页面用于 SNV，保存 bond 和加密材料。Anne Pro 固件可能通过链接配置
改变默认位置，因此不能只按 TI 默认地址备份两个页面，必须读取完整 256 KiB
flash。

原始资料：

- [CC2541 product page](https://www.ti.com/product/CC2541)
- [CC253x/4x User's Guide, SWRU191F](https://www.ti.com/lit/ug/swru191f/swru191f.pdf)
- [CC2540/CC2541 BLE Software Developer's Guide, SWRU271I](https://www.ti.com/lit/ug/swru271i/swru271i.pdf)

## `annepro2-tools` 修复

旧版 `OpenAnnePro/AnnePro2-Tools@aa84bd1d` 的 BLE 路径存在会使失败被误报为
完成的问题：固定默认 base、不校验回复来源和命令、非零状态只打印后继续、超时
无界、完成后仍可能写 main MCU AP flag。

修复版分别保存在：

- `hitsmaxft/AnnePro2-Tools@158aa04a0755090506d1318b96836645586b1686`；
- `hitsmaxft/nix-annepro2-tools@3a1602d652180c72930d1fb82b56069f43004d30`。

当前实现：

1. `--probe` 只读获取 main/LED/BLE IAP layout 和 mode；
2. 默认采用设备报告的 target base，显式 `--base` 不一致时拒绝执行；
3. 只接受 destination/source、command 和 key 与请求匹配的回复；
4. erase/write 回复 body 的第一个字节必须为零，非零立即中止；
5. 普通请求 5 秒、erase 30 秒超时；
6. 每 4 KiB 报告进度，任意错误使进程以非零状态退出；
7. `--boot` 只向 main target 发送 mode 2，匹配 ObinsKit 启动流程。

工具仍没有经过验证的 BLE flash readback 命令，因此“传输完成”严格表示
erase 和 4,864 次 write 都收到匹配的 status-zero 回复，不表示已逐字节读回。

## 可复现的一次 IAP 刷写流程

先在正常模式构建并记录将要刷入的 KEY artifact，避免设备进入 IAP 后才决定
profile：

```sh
direnv exec . just annepro2-ble213-log
stat -f '%N %z bytes' annepro2_c18_macvim_ble213_log.bin
shasum -a 256 annepro2_c18_macvim_ble213_log.bin
```

进入 IAP 后先只读探测。必须看到设备报告的三个分区和 BLE mode，才进入写入：

```sh
direnv exec . just annepro2-iap-probe
```

核心 backport 使用未经修改的官方 2.13：

```sh
direnv exec . just annepro2-flash-ble213-official
```

需要验证另行管理的 `HEXCORE AnnePro 2C` 名称变体时，使用：

```sh
direnv exec . just annepro2-flash-ble213-2c
```

2C recipe 每次都从精确官方输入重新生成 `/tmp` 镜像，并校验输入/输出哈希、
大小、两个固定偏移和仅四字节差异。两个 BLE recipe 都固定
`--target ble`，且故意不传 `--boot`，使 main MCU 留在 IAP。随后在同一会话
写入刚才构建的 KEY artifact，并由这一步重启：

```sh
direnv exec . just annepro2-flash-key annepro2_c18_macvim_ble213_log.bin
```

QMK 总是先生成通用文件 `annepro2_c18_macvim.bin`，而 2.05、2.13、正式版和
debug 版都会覆盖它。userspace 的各 C18 recipe 会立即复制出带 profile 的稳定
文件名；进入 IAP 后只使用该稳定文件，避免验证期间的后续构建悄悄改变待刷
镜像。

正常流程不传 `--base`；flasher 使用设备报告值，并在 target mode 为 2、回复
不匹配、status 非零或超时时于 erase/write 边界停止。

## 硬件备份门禁

在任何 BLE erase/write 前，使用 CC254x 调试接口完成两次独立读取，并确认两次
结果哈希一致：

1. 普通 flash 全量 `0x40000` 字节；
2. Information Page 全量 `0x800` 字节；
3. 记录 Information Page 偏移 `0x0E..0x13` 的 6 字节工厂 IEEE 地址；
4. 保留当前官方 C18 BLE 2.05 更新包作为第二条恢复路径；
5. 确认可在不依赖运行中 KEY/BLE IAP 的情况下，通过调试接口恢复全 flash。

备份不得提交到公开 Git。它可能包含唯一设备地址、bond 和安全材料。验证时使用
本机私有路径：

```sh
direnv exec . just annepro2-ble-crossflash-backup-check \
  /private/path/c18-ble-full-flash.bin \
  /private/path/c18-ble-information-page.bin
```

检查器要求：

- full flash 恰好为 `0x40000` 字节且不是全 `00`/全 `FF`；
- Information Page 恰好为 `0x800` 字节；
- 工厂 IEEE 既不是全 `00` 也不是全 `FF`；
- 输出只包含 IEEE 的 SHA-256 摘要，不打印设备唯一地址原文；
- 两个备份必须同时提供。

文件长度和非空检查只能发现明显错误，不能证明调试器读取过程正确；两次读取哈希
一致和实际恢复演练仍是人工门禁。

## 已执行的写入与回滚边界

实机 `--probe` 报告 main、LED、BLE 的 transport base 均为 `0x4000`，BLE
target mode 为 1。修复版工具随后：

- 在 `0x4000` 发出 erase；
- 从 `0x4000` 写到 `0x29FE0`，共 4,864 个 32 字节块；
- 每个 erase/write 都收到匹配的 status-zero 回复；
- 传输结束后由 main IAP 启动键盘。

官方输入镜像 SHA-256 为
`1b904ae9cd8bf6835c0b77c72618256b701a2c3b74dc04e9dddb8a388bdfc73d`。
刷写日志 SHA-256 为
`1702b86607f4b641fbd16004952f07654f4c8a72c55c1b476fbac5725485b1ba`；
日志是本机验证产物，不提交仓库。

任一步失败时的恢复原则仍是：

1. 不继续启动或配对；
2. 通过 CC254x 调试接口恢复写前完整 flash；
3. 再读完整 flash 与 Information Page，比对原备份哈希；
4. 恢复 C18 BLE 2.05 后清理 KEY 侧 profile/slot，并重新配对。

## 当前结论

静态镜像门禁：**PASS**。

修复版 IAP status-zero 传输：**PASS**。

官方 BLE 2.13 启动、macOS 连接、普通按键、音量加减/静音与 Caps：
**PASS**。USB console 曾收到 `0x20/0x0C` HID-ready 后切到 BLE route；
2026-07-27 正式 KEY 又在拔除 USB 后完成普通输入、三种媒体键和 Caps
`ABCabc`，C18 外置 LED MCU 红灯同步亮灭。

硬件全量备份与逐字节 readback：**未完成**。工具协议仍无法证明物理擦除边界
或写后字节完全一致。

清空四槽 bond 后分别与四台主机重新配对，以及快速交叉切槽、连接超时和压力
测试：**PASS（2026-07-28 操作者实机确认，未保存启动 revision 行）**。

操作者于 2026-07-28 将 C18 KEY 对官方 BLE 2.13 的完整功能回归判定为
**PASS**，其中包括切槽后的状态、断电恢复及上述四主机压力项目。

BLE 2.05 全量回归：**待验收**。

因此 C18 KEY 对官方 BLE 2.13 的功能 backport 已完成实机验收。BLE 2.05
非回归仍是双 profile 首版的剩余门禁；readback 是 IAP 工具链证据边界，不
否定已经通过的 2.13 功能结论。
