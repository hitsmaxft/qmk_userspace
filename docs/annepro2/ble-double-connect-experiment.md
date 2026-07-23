# Anne Pro 2 BLE 二次 connect 实验记录

本文归档 QMK fork 中一次未完成实机验证的切槽实验。它不是原厂固件行为，
不属于准备提交上游的可靠性修复。

## 实验版本

- QMK branch：`codex/annepro2-uart-state-v2`
- commit：`66230dae4b`（`Retry Anne Pro 2 connect after slot switches`）
- 基线 commit：`0c23243e6f`（`Harden Anne Pro 2 BLE slot handling`）
- debug firmware：42,764 bytes
- debug SHA-256：
  `d63f61e701ab738c38c5df9403fb17ebf9dfbbc033ded4cb9ae8bd895b9d0b3b`
- normal firmware：43,724 bytes

两种固件均通过 QMK 构建，但该版本没有刷入键盘，也没有完成 BLE 实机测试。

## 动机

实机曾出现快速从 slot 2 切回 slot 1 时，第一次 slot 1 操作没有立即连接，
再次操作才成功。实验将其暂时解释为 BLE 模块仍在处理旧 slot 的连接请求。

当另一个 slot 的 connect 仍在进行时，实验版：

1. 立即向新 slot 发送一次 `0x40/0x04 connect`；
2. 设置 200 ms timer；
3. timer 到期后向同一 slot 再发送一次 connect；
4. timer 未到期时收到 `0x20/0x0c`，仍回复握手，但不切换 QMK host driver。

## 已知问题

### 回复后丢弃有效握手

`0x20/0x0c` 不携带 slot 或 transaction ID。第一次 connect 如果在 200 ms
内成功，实验版会向 BLE 模块回发官方握手响应，却因 retry 仍 pending 而不启用
BLE route。BLE 模块可能认为握手已经完成，不再发送第二次请求，导致 QMK 永远
等待。

### 无法区分旧连接和新连接

切槽时 `selected_slot` 已更新，但迟到的旧连接握手与新连接握手具有相同帧格式。
时间窗口不能可靠判断握手归属，可能把旧 host 的成功错误记为新 slot 成功。

### 重复发送原厂的一次性状态通知

实验版的 connect 重发路径会同时重发 `0x20/0x0b slot,0`。原厂主控只在一次
slot 短按动作中发送一次 connect 和一次状态通知，没有静态证据支持 200 ms
后二次发送。

### 可能重启 BLE 内部状态

`0x40/0x04` ACK 只证明 BLE MCU 接收命令，不证明 radio/HID 已建立。无条件
重复命令可能重启或覆盖 BLE 模块正在执行的连接流程，反而扩大随机失败窗口。

## 保留价值

该实验确认了快速切槽需要显式建模，而不能让多个隐式连接事务共享
`selected_slot`。后续实现应采用 single-flight 操作：

- 同一时间只允许一个 BLE profile 操作；
- 新 slot 操作只记录最新意图，不立即与旧操作重叠；
- `0x20/0x0b` 与命令重试解耦；
- 只有能够归属到当前事务的 `0x20/0x0c` 才完成 route；
- 没有协议证据时，不用固定延时推断 BLE 已连接或已断开。

## 回滚与验证

该 commit 使用独立 revert 回滚，以便保留实验历史。回滚后的目标基线是
`0c23243e6f`，其 debug 固件为 42,796 bytes，SHA-256：

```text
be89e91c7354db0fe867c79b63045e2b4e50aa7161f4bf8887e0639a5827dfde
```

这个基线已经刷入 C18，并完成过 BLE slot、冷启动连接和 BLE 输入验证。
