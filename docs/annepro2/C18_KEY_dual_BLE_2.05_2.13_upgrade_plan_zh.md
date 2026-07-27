# Anne Pro 2 C18 KEY 兼容 BLE 2.05 与 AP2D BLE 2.13 升级方案

日期：2026-07-25  
改造对象：Anne Pro 2 C18 KEY 源码  
目标 BLE 固件：C18 BLE 2.05、未经修改的 AP2D BLE 2.13  
证据范围：Hexcore/Obins 官方 KEY 与 BLE 二进制，不采用 QMK 或社区协议定义

## 1. 目标、边界与完成标准

最终形态是一套 C18 KEY 代码，包含公共 UART/帧处理层和两个 BLE profile：

```text
C18 矩阵、USB、LED MCU、板级驱动
                 │
          BLE 统一接口
          ┌──────┴──────┐
   C18 BLE 2.05     AP2D BLE 2.13
```

首版必须完成：

- 普通键盘、修饰键和 6KRO；
- Consumer 媒体键按下与释放；
- `20/07` Caps Lock 状态与 C18 外置 LED MCU 灯位；
- 四个主机槽的配对、切换、断电重连和重新配对；
- BLE 2.05 与 BLE 2.13 profile 的手动选择及持久化；
- USB 下的恢复、重新刷写和降级能力。

首版不纳入：

- AP2D KEY 3.08 的 USB suspend 重构；
- AP2D 的 RGB、矩阵和板级 HAL；
- AP2D 的锁定灯、HID LED Output 和 KEY MCU 直驱 RGB 实现；
- BLE 2.13 内部身份密钥、连接参数和协议栈逻辑的 KEY 侧复刻；
- 未确认的 BLE 2.13 在线升级命令；
- 无可靠版本查询命令时的自动识别。

硬边界：

1. BLE 2.13 二进制保持原样。
2. C18 KEY 的矩阵 GPIO、USB、`PB0/PB1` LED MCU UART、IAP 和板级时钟保持原实现。
   本次 backport 不修改、替换或扩展这条 C18 LED 路径。
3. AP2D BLE 2.13 能否在 C18 BLE 副控硬件上可靠启动、射频工作并保留工厂 IEEE/RF 信息页，必须先通过实机门槛测试。KEY 代码无法修复 BLE 副控自身的硬件或 Bootloader 不兼容。
4. BLE 2.13 的广播名称、GATT Report Map 和主机侧设备身份由 BLE 固件决定。KEY 兼容层无法让主机把它完整识别为原版 C18。

完成标准：

| 项目 | 验收要求 |
|---|---|
| BLE 2.05 回归 | 改造后的 KEY 在 2.05 profile 下与原 C18 功能一致 |
| BLE 2.13 基础输入 | 普通键、组合键、长按和全键释放无错报、无 stuck key |
| 媒体键 | 每个支持的 Consumer Usage 都能按下和释放，断线前主动发送全零报告 |
| Caps 锁定灯 | `20/07 01/00` 严格解码，QMK 状态与 C18 实体灯同步，切槽不残留 |
| 四主机 | 四槽可分别配对，短按切换、长按重配，重启后可恢复活动槽 |
| 连续切换 | 至少 100 次跨槽切换无槽位串写、旧事件覆盖和永久失联 |
| 恢复 | 无论 BLE profile 状态如何，USB 均可进入维护模式并重新刷写 KEY |

## 2. 已确认的兼容关系

| 协议项 | C18 BLE 2.05 | AP2D BLE 2.13 | KEY 处理 |
|---|---|---|---|
| 物理 UART | `PA4/PA5`、AF6、115200、8N1 | 相同 | 共用驱动 |
| 基础帧族 | `0x7B` 起始、`0x10/0x12` 类型、8 字节头、`0x7D` 头部标记 | 相同 | 共用收发和重组层 |
| Keyboard Report ID 1 | 标准 6KRO | 描述符逐字节相同 | 完全共用 |
| Vendor Report ID 2 | Usage 2 为 OUT，Usage 3 为 IN，18 字节 | Usage 2 为 IN，Usage 3 为 OUT，18 字节 | profile 决定收发 Usage |
| Consumer Report ID 3 | C18 现有媒体键位图 | 四个小端 `u16 Usage`，共 8 字节 | profile 决定编码器 |
| 主机槽 | `0x40/01` 广播、`0x40/04` 连接，`20/0B` 状态 | 同一主命令，广播前 `20/0B slot,1`、连接前 `20/24 slot,2` | profile 编码一次性状态通知；ACK 与 HID-ready 分离 |
| BLE 安全身份 | C18 2.05 内部管理 | 2.13 使用工厂 IEEE 地址校验并重建 SNV 安全材料 | KEY 不生成密钥；升级后清除槽状态并重绑 |

Vendor 方向按 HID 主机视角定义：

| profile | KEY/BLE → 主机 | 主机 → KEY/BLE |
|---|---:|---:|
| C18 BLE 2.05 | Usage 3，Input | Usage 2，Output |
| AP2D BLE 2.13 | Usage 2，Input | Usage 3，Output |

仍需实机确认：

- AP2D `0x21/0x22` 厂商对象业务的精确语义（不把它们当作 slot 主命令）；
- 配对开始、断开完成、切换成功、连接失败等 BLE→KEY 回包；
- 启动握手、状态查询和清除 bond 命令是否与 C18 相同；
- KEY 串口消息是否显式携带 Vendor Usage selector；
- BLE 2.13 IAP 命令与 C18 updater 是否兼容。

在完成抓包前，禁止把 `0x21=切换、0x22=配对` 或相反关系写入正式代码。静态分析只确认两个命令、四个槽和 9 字节槽数据参与了 AP2D 的状态机。

## 3. KEY 软件结构

公共层保留 C18 已有实现：

- `ble_uart`：`PA4/PA5`、115200、DMA/中断收发；
- `ble_frame`：`0x7B…0x7D` 帧族、8 字节头、`0x10/0x12`；
- `keyboard_report`：Report ID 1；
- C18 矩阵、USB、LED MCU、配置存储和 IAP。

新增 profile 接口：

```c
struct ble_profile_ops {
    void (*send_consumer)(const uint16_t *usage, size_t count);
    uint8_t vendor_tx_usage;
    uint8_t vendor_rx_usage;
    int (*pair_slot)(uint8_t slot);
    int (*select_slot)(uint8_t slot);
};
```

两份实现：

```text
ble_profile_c18_205
    复用原 C18 Consumer 编码、Vendor 方向和槽位命令

ble_profile_ap2d_213
    8 字节 Consumer Usage 数组
    交换 Vendor Usage 2/3
    使用抓包确认后的 AP2D 四槽命令与事件
```

键盘输入路径不分支：

```c
send_report(REPORT_ID_KEYBOARD, keyboard_6kro, 8);
```

BLE 2.13 Consumer 编码：

```c
uint8_t report[8] = {0};
for (size_t i = 0; i < count && i < 4; ++i) {
    report[i * 2]     = usage[i] & 0xff;
    report[i * 2 + 1] = usage[i] >> 8;
}
send_report(REPORT_ID_CONSUMER, report, sizeof(report));
```

释放媒体键时必须发送同格式的全零报告。BLE 2.05 继续调用原 C18 位图编码器。

LED 不属于 profile 适配接口。AP2D 的 KEY MCU 直驱 RGB 与 HID Output
处理建立在 AP2D 板级实现上，不能回移到仍使用独立 LED MCU 的 C18。本项目
保持 C18 原有 LED MCU 路径，不根据 AP2D 固件新增 LED UART 解释或测试要求。

profile 选择首版使用显式配置，不依赖自动探测：

```text
NVM.ble_profile = C18_205 或 AP2D_213
启动按键或 USB 配置命令可修改
无效配置默认回退 C18_205
```

推荐同时生成两个发布构建：

- `c18-key-ble205.bin`：默认 2.05；
- `c18-key-ble213.bin`：默认 2.13。

代码内部仍保留双 profile。两个构建只改变首次启动默认值，便于升级和救援。

## 4. 四主机协议取证与实现

使用 AP2D KEY 3.08 与 BLE 2.13 实机，在 `PA4/PA5` 上双向抓取 UART。每个动作都从断电冷启动开始，记录时间戳和方向。

抓包矩阵：

| 编号 | 初始状态 | 用户动作 | 目的 |
|---:|---|---|---|
| 1 | 无 bond | 上电等待 | 恢复启动握手和初始状态查询 |
| 2–5 | 无 bond | 分别长按 `FN2+1…4` | 定位四槽配对命令、槽号字段 |
| 6–9 | 各槽已绑定 | 分别短按 `FN2+1…4` | 定位切换命令和成功回包 |
| 10 | 槽 1 已连接 | 快速切换 1→2→3→4 | 识别事务、迟到事件和断开顺序 |
| 11 | 目标主机关机 | 切换到该槽并等待超时 | 定位失败、超时和回退事件 |
| 12 | 已绑定槽 | 长按同槽重新配对 | 定位清 bond 与重配流程 |
| 13 | 四槽已绑定 | 断电重启 | 定位活动槽持久化和自动重连 |

输出一份协议表，至少包含：

```text
direction
frame type 0x10/0x12
command
payload length
slot offset and range
operation field
9-byte record layout
request/response correspondence
success/failure status
sequence or transaction behavior
```

四槽状态机：

```text
IDLE
  ├─ 短按槽位 → RELEASE_ALL → DISCONNECT → SELECT → WAIT_CONNECT
  └─ 长按槽位 → RELEASE_ALL → DISCONNECT → CLEAR/PAIR → WAIT_PAIR
```

必须分别保存：

- `active_slot`：已经成功连接的槽；
- `target_slot`：当前切换目标；
- `pairing_slot`：当前配对目标；
- `transaction_id`：本轮异步操作编号。

实现规则：

1. 槽号只接受 0–3。
2. 切换前发送普通键盘与 Consumer 全释放。
3. 收到断开完成或达到有界超时后，再发目标槽命令。
4. 只有连接成功后更新并持久化 `active_slot`。
5. 回包只允许修改匹配当前 `transaction_id` 的状态；迟到事件直接丢弃。
6. 配对失败保留其他槽的 bond，不全局清空。
7. profile 切换或首次升级到 2.13 时，清除 KEY 侧“已绑定”缓存，并提示主机删除旧配对。
8. BLE 2.13 内部 SNV 和密钥交由副控管理，KEY 只维护槽位 UI 状态。

## 5. 实施阶段与门槛

| 阶段 | 工作内容 | 通过门槛 | 失败处理 |
|---|---|---|---|
| P0 备份与恢复 | 保存 C18 KEY 2.36、BLE 2.05；验证 USB IAP/SWD 恢复 | KEY 和 BLE 均可恢复原版 | 未建立恢复链前停止刷写 |
| P1 BLE 2.13 硬件启动 | 只写 BLE 应用区，保留 Bootloader、IEEE、RF 校准和信息页 | 副控启动、UART 有稳定响应、可广播并连接 | 恢复 BLE 2.05；记录不兼容点 |
| P2 公共层与 profile | 抽离公共帧层，加入 profile 配置；先运行 2.05 | BLE 2.05 全量回归通过 | 修复回归后再进入 2.13 |
| P3 BLE 2.13 基础 HID | 接入 Report ID 1、Consumer 8 字节、Vendor 方向 | 键盘、媒体和 Vendor 透传逐项通过 | 按功能隔离，保留 USB 调试 |
| P4 四槽取证 | 完成上述 UART 抓包矩阵和协议表 | `0x21/0x22` 请求、回包、槽号、9 字节结构可重放 | 缺少字段时继续抓包，不猜协议 |
| P5 四槽实现 | 完成 pair/select 状态机、超时、事务隔离和 NVM | 四主机配对、切换、重启恢复及 100 次压力测试通过 | 保留单槽模式用于诊断 |
| P6 升降级流程 | KEY 双 profile、首次迁移、重绑提示、降级到 2.05 | 不重刷 KEY 即可切换 profile；USB 始终可救援 | 清 NVM profile 并默认 2.05 |
| P7 发布 | 固件、配置说明、协议表、抓包样本、测试报告 | 所有阻断项关闭 | 未确认 IAP 时继续禁用 BLE 在线升级 |

推荐升级顺序：

1. 备份并验证原 KEY/BLE 恢复方式。
2. 刷入双 profile KEY，默认保持 `C18_205`。
3. 使用原 BLE 2.05 完成一次回归，确认改造未破坏 C18。
4. 通过已验证的 BLE 恢复/编程路径刷入 2.13 应用区，保留芯片信息页。
5. 通过启动按键或 USB 配置把 profile 切到 `AP2D_213`。
6. 清除 KEY 槽位缓存，主机删除旧 Anne Pro 2 配对项。
7. 依次重新绑定四个主机槽。
8. 完成基础 HID、四槽压力和断电重连测试后再设为日常固件。

降级顺序：

1. 通过 USB 把 KEY profile 切回 `C18_205`。
2. 恢复 BLE 2.05 应用镜像。
3. 清除 2.13 产生的配对关系并重新绑定。
4. 若 NVM 配置损坏，启动救援组合键应强制进入 USB 维护模式并采用 2.05 profile。

## 6. 风险、测试与交付物

| 风险 | 影响 | 控制措施 |
|---|---|---|
| BLE 2.13 与 C18 BLE 硬件/Bootloader 不兼容 | 副控不启动、无 RF 或无法恢复 | P0 建立恢复链；P1 单独验证；保留 IEEE/RF 信息页 |
| 四槽 payload 尚未确认 | 误清 bond、槽位串写 | 完成 AP2D 实机双向 UART 抓包后编码 |
| Vendor 方向变化 | 配置通道失效 | profile 映射 Usage；抓包确认 UART 是否携带 selector |
| AP2D 主机身份不同 | 原版 C18 配置软件拒绝设备 | 将软件兼容单独验收；KEY 侧无法改变 BLE GATT 身份 |
| 2.13 重建安全材料 | 旧 bond 全部失效 | 首次迁移清 KEY 缓存并强制重新绑定 |
| BLE 在线升级契约未知 | 错写副控 Flash | 首版禁用 2.13 在线升级入口，仅保留已验证恢复方式 |
| profile 误选 | 媒体键、Vendor、四槽异常 | NVM 校验、启动选择、USB 配置和默认 2.05 回退 |
| 断线期间残留按键 | 主机出现 stuck key | 任何切换、断开和 profile 变更前发送全释放 |

测试矩阵：

- 主机：Windows 10/11、macOS、Linux、iOS/Android；
- profile：BLE 2.05、BLE 2.13；
- 输入：字母、修饰键、组合键、重复按键、Consumer press/release；
- 槽位：四槽首次配对、短按切换、长按重配、目标主机离线、快速连按；
- 生命周期：冷启动、热重启、USB/BLE 切换、BLE 断线重连、低电量；
- 容错：UART 半帧、错长度、未知命令、非法槽号、迟到回包；
- 恢复：KEY profile 损坏、BLE 2.13 启动失败、完整降级到 BLE 2.05。

最终交付物：

1. `ble_profile_c18_205` 与 `ble_profile_ap2d_213`；
2. 公共 `ble_uart`、`ble_frame` 与 Report ID 1 路径；
3. BLE 2.13 Consumer 与 Vendor 适配器；
4. `0x40/01`、`0x40/04`、`20/0B`、`20/24` 四槽协议表和原始 UART 日志；
5. 四槽状态机与 NVM 迁移逻辑；
6. `c18-key-ble205.bin`、`c18-key-ble213.bin`；
7. 升级、重新绑定、降级和救援说明；
8. 跨主机测试报告及已知限制清单。

本方案依据以下既有分析结果重新整理：

- `AnnePro2D_3.08_features_BLE_compatibility_C18_backport_analysis_zh.md`
- `AnnePro2_C18_vs_AnnePro2D_official_KEY_GPIO_BLE_analysis_zh.md`

当前 P0/P1 已完成到“IAP status-zero 传输、BLE 2.13 启动、广播、macOS
连接和普通输入”层级，P2/P3/P5 已形成双 profile、Consumer、严格 parser、
Caps 桥接和四槽状态机代码。BLE 2.13 的媒体键、Caps 实体灯、清空四槽 bond
后的四台主机重新配对，以及快速交叉切槽、连接超时和压力测试已经获得实机
证据；操作者于 2026-07-28 将包含断电恢复在内的 C18 BLE 2.13 功能 backport
整体验收为通过。尚未完成的是 IAP readback、BLE 2.05 实机回归和当前精确
debug revision 的日志归档；这些结果集中写入
[`ble213-validation-matrix.md`](ble213-validation-matrix.md)，不能由静态分析
或构建结果替代。
