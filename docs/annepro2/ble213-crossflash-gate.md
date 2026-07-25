# C18 BLE 2.13 交叉刷写门禁

本页只定义 AP2D BLE 2.13 写入 C18 BLE 副控前必须满足的条件。当前静态镜像
门禁已通过，硬件备份与 IAP 行为门禁尚未通过；因此现在不应使用
`annepro2_tools -t ble` 写入 BLE 芯片。

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

## 当前 `annepro2_tools` 为什么不能用于 BLE 交叉刷写

Nix 环境固定的 `annepro2-tools` 源码版本为
`OpenAnnePro/AnnePro2-Tools@aa84bd1d34c961ada8c812dfea524592f7d2be2c`。
源码审计得到：

1. README 明确写明只有 main MCU 被测试过。
2. CLI 虽接受 `-t ble`，但默认 base 是 main MCU 使用的 `0x4000`；BLE 官方
   镜像从地址 `0` 开始，误用默认值会整体错位。
3. erase 请求只携带一个起始地址，不携带长度；目前没有证据证明 C18 IAP 对
   BLE target 的实际擦除范围。
4. BLE 以 32 字节分块写入，但单块写失败时工具只打印 warning 并继续。
5. 工具读取 IAP 回复后只打印原始字节，不验证 ACK、地址或状态。
6. 写完后没有逐字节 readback，也没有保护区前后哈希比较。
7. 无论中途是否出现 warning，流程仍可能写 main MCU 的 AP flag。

因此，即使使用看似正确的：

```text
annepro2_tools -t ble --base 0 ...
```

也不能满足本项目的恢复、边界和写后校验要求。此命令仅作为协议取证线索，
不是建议执行的刷写命令。

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

## 写入与回滚验收

后续 BLE flasher 至少要先增加：

- 明确拒绝 BLE base 非 `0`、镜像长度非 `0x26000` 或哈希未知；
- 在 erase 前读取并保存目标范围与保护范围；
- 把每个 IAP 回复解析为严格 ACK，任何失败立即停止；
- 明确、验证 erase 的首尾页，不使用含义未知的整区擦除；
- 写完逐字节读取 `0x00000..0x25FFF` 并与 BLE 2.13 镜像比较；
- 再次读取 `0x26000..0x3FFFF` 与 Information Page，确认和写前完全一致；
- 只有全部校验通过后才启动 BLE 应用。

任一步失败：

1. 不继续启动或配对；
2. 通过 CC254x 调试接口恢复写前完整 flash；
3. 再读完整 flash 与 Information Page，比对原备份哈希；
4. 恢复 C18 BLE 2.05 后清理 KEY 侧 profile/slot，并重新配对。

## 当前结论

静态镜像门禁：**PASS**。

硬件全量备份：**BLOCKED**。

IAP erase 边界与 readback：**BLOCKED**。

C18 上 BLE 2.13 的 RF、广播、连接和四主机验收：**BLOCKED**。

在后三项完成前，BLE 2.13 profile 仍是实验功能，构建成功不能被解释为交叉刷写
安全或无线功能可用。
