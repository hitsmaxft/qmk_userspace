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
AP2 BLE 00001234 rx decoded group=40 command=04 value=00 requested=1
```

关键日志：

| 日志 | 含义 |
| --- | --- |
| `tx broadcast slot=...` | 发出 `40/01`，并设置 BLE route request |
| `tx connect slot=...` | 发出 `40/04`；此时仍不切 driver |
| `rx decoded group=40 ...` | 命令响应，仅证明 BLE MCU 已处理命令 |
| `rx decoded group=20 command=0C ...` | 收到 BLE 发起的建链后握手请求 |
| `tx hid handshake response` | 回发官方主控使用的 `20/0c 00 00` 响应 |
| `rx hid handshake ready`、`route ble` | request 仍有效，因此切到 BLE driver |
| `tx keyboard report=1..3` | 切换后的前三个 BLE 键盘报告 |
| `route pending` | 选择 slot 前退出旧 BLE route |
| `route usb ...` | USB 键或 unpair 取消 request 并恢复原 driver |
| `rx invalid ...` | 长度或 delimiter 无效，parser 已重新同步 |

## 预期操作顺序

### 首次广播和连接

1. 按一次 BT slot。
2. 确认出现 `tx broadcast`。
3. `40/01` ACK 出现时不应有 `route ble`。
4. 在 macOS 点击连接并等待系统完成 HID 配置。
5. `20/0c` 到达后应先出现 `tx hid handshake response`，再出现
   `rx hid handshake ready` 和 `route ble`。
6. 按普通键，确认出现最多三条 `tx keyboard report`，并能在 host 输入。

### 已配对 slot 重连

1. 对同一 slot 再次操作，使固件发送 `40/04`。
2. `40/04` ACK 后不能立即切 driver。
3. 记录从 connect 命令到 `20/0c` 的时间，以及握手请求出现次数；重点确认
   回复后是否停止重复请求。
4. 特别记录：macOS 显示“已连接”的时间、首次可输入时间是否一致。

### 取消 pending

1. 发起连接后，在 ready 前按 `KC_AP2_USB`。
2. 确认 `route usb`。
3. 如果随后收到延迟 `20/0c`，日志应显示 `requested=0` 并回发 handshake
   response，但不能出现 `route ble`。

### 从 BLE 切换 slot

1. 已经通过一个 slot 输入时按另一个 BT slot。
2. `tx broadcast` 前后应出现 `route pending`；等待期间不能继续打印
   `tx keyboard report` 到旧链路。
3. 新 slot 完成 `20/0c` 握手后才重新出现 `route ble`。

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
- 尚未确认断链 UART 帧。若断链时没有 RX，当前 QMK 无法只靠 UART 自动恢复
  USB route；USB 键仍是确定的恢复操作。
- USB console 会增加时序扰动。出现吞吐或 latency 异常时，应改用 PA4/PA5
  逻辑分析仪抓取 115200 8N1 原始数据复核。
