# Anne Pro 2 BLE USB Console 验证

调试构建启用 `CONSOLE_ENABLE` 和 `ANNEPRO2_BLE_DEBUG`，通过 USB HID console
输出 BLE UART 的完整 RX 帧、route 请求和首批键盘报告。日志用于验证主控
parser 与状态转换；它不能单独证明 RF 链路质量。

## 构建、刷写与监听

```sh
direnv exec . just annepro2-log
direnv exec . just flash-annepro2-log
direnv exec . just qmk console -d AC20:8009 -t
```

若 PID 或接口不同，先执行：

```sh
direnv exec . just qmk console -l
```

然后省略 `-d` 或使用列出的设备。监听器晚于键盘上电时可能看不到 wakeup 行。

## 日志格式

每行包含 MCU 毫秒时间戳。完整 RX 帧会同时输出原始字节和解码摘要：

```text
AP2 BLE 00001234 rx11 7B 12 35 00 03 00 00 7D 40 04 00
AP2 BLE 00001234 rx decoded group=40 command=04 value=00 state=2
```

状态编号：`0=USB`、`1=WAIT_BROADCAST_ACK`、`2=WAIT_CONNECT_ACK`、
`3=WAIT_HANDSHAKE`、`4=ACTIVE`、`5=STARTUP_PASSIVE`。

关键日志：

| 日志 | 含义 |
| --- | --- |
| `state N -> M` | 显式状态转换 |
| `wake ...` | 从 keyboard EEPROM 读取的上次成功 slot；`-1` 表示不自动连接 |
| `auto ... after passive window` | 500 ms 内没有被动握手，启动冷启动 broadcast |
| `tx broadcast slot=... attempt=...` | 发出或重试 `40/01` |
| `tx connect slot=... attempt=...` | 发出或重试 `40/04` |
| `tx slot state=0/1` | 一次 slot 动作的 `20/0b` 边沿通知；重试中不得重复 |
| `queue ...`、`dispatch ...` | 切槽期间只保留最后一次意图，并在 1 秒后启动 |
| `rx command ack=... value=... state=...` | 与当前 pending 命令关联的 ACK；value 语义未知 |
| `command timeout ...` | 两次重试仍无 ACK；继续等可能迟到的握手，不切 driver |
| `rx decoded group=20 command=07 ...` | 收到状态同步请求 |
| `tx state sync response value=...` | 原值回复官方主控的 `20/07` 响应 |
| `rx decoded group=20 command=0C ...` | 收到 BLE 发起的建链后握手请求 |
| `tx hid handshake response` | 回发官方主控使用的 `20/0c 00 00` 响应 |
| `rx hid handshake ready`、`route ble` | request 仍有效，因此切到 BLE driver |
| `tx keyboard report=1..3` | 切换后的前三个 BLE 键盘报告 |
| `route pending` | 选择 slot 前退出旧 BLE route |
| `route usb ...` | USB 键或 unpair 取消 request 并恢复原 driver |
| `rx invalid ...` | 长度或 delimiter 无效，parser 已重新同步 |

## 预期操作顺序

### 首次广播和连接

1. 按住一个未配对 BT slot 至少 500 ms。
2. 确认出现一次 `tx broadcast` 和一次 `tx slot state=1`；松开后不得再发
   connect。
3. `40/01` ACK 出现时不应有 `route ble`。
   广播应转换到 state 3，不发送 `40/04`。
4. 在 macOS 点击连接并等待系统完成 HID 配置。
5. `20/0c` 到达后应先出现 `tx hid handshake response`，再出现
   `rx hid handshake ready` 和 `route ble`。
6. 按普通键，确认出现最多三条 `tx keyboard report`，并能在 host 输入。

### 已配对 slot 重连

1. 短按已配对 slot，确认发送一次 `40/04` 和一次 `tx slot state=0`，不得先发
   `40/01`。
2. 命令 ACK 丢失时，只允许重发 `40/04`；`20/0b` 不得随重试再次发送。
3. `40/04` ACK 后不能立即切 driver。
4. 记录从 connect 命令到 `20/0c` 的时间，以及握手请求出现次数；重点确认
   回复后是否停止重复请求。
5. 特别记录：macOS 显示“已连接”的时间、首次可输入时间是否一致。

### 断电后的自动重连

1. 先手动连接一个 slot，确认出现 `20/0c`、`route ble`，并能通过 BLE 输入。
   只有这一步完成后 slot 才写入 EEPROM。
2. 完全断电再上电，不按 `MO(9)` 或 slot。
3. 启动日志先包含 `wake 0` 和 `state 0 -> 5`。若 BLE 模块在 500 ms 内自行
   恢复链接，`20/0c` 应直接完成 route，且不能出现 `auto`。
4. 若 500 ms 内没有握手，应出现 `auto 0 ... after passive window` 和
   `tx broadcast slot=0`（slot 1 的内部编号是 `0`）。冷启动不得发送
   `40/04` 或 `20/0b`；收到后续 `20/0c` 才出现 `route ble`。
5. 拔掉 USB，确认按键继续通过 BLE 输入。
6. 再连接 USB，按 `KC_AP2_USB` 后完全断电重启；日志应为
   `wake -1`，且不能出现 `auto`。这验证显式 USB 选择会清除
   自动重连偏好。
7. 再次选择 BLE slot 并完成握手后，自动重连应重新启用。

广播、配对或握手失败不能写入新 slot。测试失败路径时先记录原来的成功 slot，
选择另一个未配对 slot 但不完成连接，然后重启；固件应仍自动请求原成功 slot。

### ACK 重试

用逻辑分析仪或临时测试桩丢弃 `40/01`/`40/04` ACK：

1. 同一命令最多出现三次（初次发送加两次重试），间隔约 500 ms。
   整个动作的 `20/0b` 只能出现一次。
2. timeout 不能产生 `route ble`。
3. 重试耗尽后的迟到 `20/0c` 仍应获得握手回复并允许完成 pending route。
4. 已经 active 后的 ACK 应记录为 stale，不重复切换 route。
5. 保存非零 ACK value 样本；在语义确认前状态机只记录，不把它解释为错误。

### 取消 pending

1. 发起连接后，在 ready 前按 `KC_AP2_USB`。
2. 确认 `route usb`。
3. 如果随后收到延迟 `20/0c`，日志应显示 `state=0` 并回发 handshake response，
   但不能出现 `route ble`。

### 从 BLE 切换 slot

1. 已经通过一个 slot 输入时短按另一个 BT slot。
2. `tx connect` 前应出现 `route pending`；等待期间不能继续打印
   `tx keyboard report` 到旧链路。
3. 在上一事务 pending 时连续短按多个 slot，日志可以记录多条 `queue`，但
   1 秒窗口后只能 `dispatch` 最后一个 slot，不能同时发送多个 profile 命令。
4. 新 slot 完成 `20/0c` 握手后才重新出现 `route ble`。由于 `20/0c` 不含
   slot/transaction ID，切槽窗口后的迟到旧握手仍是必须抓日志验证的协议边界。

固件只持久化最后一次成功握手的 slot。启动时不会轮询四个 slot，也不会反复
尝试所有已配对 host；被动窗口失败后只执行该 slot 的有界恢复流程。

### 断链与 unpair

1. 在 BLE 可输入时让 macOS 主动断开、关闭蓝牙或移除设备。
2. 保存断开前后至少 10 秒的所有 `rxN` 行。
3. 查找是否存在稳定的新 group/opcode；不要把沉默自动解释为某个状态。
4. 执行 QMK unpair，确认发出 `40/05` 后本地恢复 USB；再次按同 slot 应从
   broadcast 流程开始。

## 必须记录的边界

- `20/0c` 在旧 QMK 的一次 macOS 建链中出现两次。官方主控汇编会立即回发
  `7B 12 43 00 04 00 00 7D 20 0C 00 00`，因此重复请求更可能是 BLE 侧等待
  回复后的重试。必须用新固件验证次数；`hid handshake` 仍是行为名称，不是
  已确认的 BLE stack 函数名。
- C18 实测的新配对在 broadcast ACK 后约 5.2 秒收到 `20/0c`，回复后进入
  BLE route，并在拔掉 USB 后继续输入；这验证了 event gate 和 HID 报告路径。
  这是手动新配对样本；冷启动自动重连结果见下两项。
- 第一版自动重连实测把 EEPROM slot 装入 `last_slot`，导致冷启动日志为
  `reconnect=1` 并发送 `40/04`。两条命令都有 `00` ACK，但 macOS 始终未连接，
  也没有 `20/0c`；手动的断电后首次 slot 选择则成功。修正版将持久化
  `startup_slot` 与会话内 `last_slot` 分离，冷启动必须显示 `reconnect=0`。
- 修正版 C18 冷启动实测通过：`auto 0` 在 MCU 时间 `0x1f8`（504 ms）出现，
  同时发送 `40/01` 且 `reconnect=0`；`40/01` ACK 在 `0x202`，`20/0c` 在
  `0x214`，`route ble` 在 `0x218`（536 ms）。整个样本没有 `40/04`。
  macOS 同时报告 AnnePro2 已连接，地址 `F8:30:02:4B:8C:84`、固件
  `BLE-1.5.0`。这是引入 `STARTUP_PASSIVE` 前的基线日志；当前日志不再打印
  `reconnect=`，新的 500 ms 被动窗口仍待实机验证。
- 尚未确认断链 UART 帧。若断链时没有 RX，当前 QMK 无法只靠 UART 自动恢复
  USB route；USB 键仍是确定的恢复操作。
- USB console 会增加时序扰动。出现吞吐或 latency 异常时，应改用 PA4/PA5
  逻辑分析仪抓取 115200 8N1 原始数据复核。
