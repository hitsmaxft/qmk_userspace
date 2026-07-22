# Anne Pro 2 BLE USB Console 验证

此调试构建仅用于验证 QMK 主控侧状态机。它启用 `CONSOLE_ENABLE` 和
`ANNEPRO2_BLE_DEBUG`，日志通过 USB HID console 输出；不会解码或证明闭源
BLE 固件的连接完成事件。

## 构建与监听

刷写前构建：

```sh
direnv exec . just annepro2-log
```

需要由 QMK 调用现有 `annepro2_tools` 刷写时，使用：

```sh
direnv exec . just flash-annepro2-log
```

刷写后，在另一终端监听：

```sh
direnv exec . just qmk console -d AC20:8009 -t
```

若设备枚举为不同接口或 PID，先执行 `direnv exec . just qmk console -l`，再
省略 `-d` 或改用列出的值。日志只从 console 启用后的固件中产生；若监听器在
上电后才连接，可能看不到启动 wakeup 行。

当前 debug 构建为 48044 bytes，C18 的应用区上限为 49152 bytes，只剩约 1108
bytes。验证期间不要再增加大量格式字符串或额外 console 功能。

## 预期顺序

以下操作中的时间戳为 MCU 启动后的毫秒数。

| 操作 | 预期日志 | 含义 |
| --- | --- | --- |
| 首次按 BT slot `n` | `broadcast slot=n reconnect=0 previous_slot=-1` | 只发 broadcast，不切换 BLE host driver。 |
| 再按同一 slot | `broadcast ... reconnect=1`、`connect slot=n guard_ms=200` | 发出 connect 命令并开始保护期。 |
| 约 200 ms 后 | `connect guard complete elapsed_ms=...`、`driver switched to BLE` | QMK 端现在才将 HID 输出交给 BLE driver。 |
| 随后按键 | 最多三条 `keyboard report #... after driver switch` | 首批 HID 报告没有在保护期内转发。 |
| 按 USB 键 | `disconnect ...` | 取消 pending 状态并恢复先前 USB host driver。 |
| 按解绑键 | `unpair command`，随后 `disconnect ...` | 发出既有 unpair 命令并清理本地 slot/driver 状态。 |

每接收满 11 字节，日志会输出一条 `rx11 ... caps=...`。该记录仅验证主控没有再
用 10 ms 阻塞读取；它不是 BLE 连接成功确认。请保存完整原始日志，并记录 slot、
主机系统、是否实际可输入、首次按键是否丢失及失败/重试次数。

## 需要特别观察的异常

- 没有 `driver switched to BLE`：QMK 侧 timer task 未运行或 connect 流程被打断。
- 在 `connect guard complete` 前已有 `keyboard report`：host driver 切换路径与预期不符。
- 同一 slot 的第一次按键出现 `connect`：本地 `last_broadcast` 状态没有正确复位。
- `rx11` 持续出现但无法连接：应保留 PA4/PA5 抓包；USB log 不能解释 BLE 固件事件语义。
