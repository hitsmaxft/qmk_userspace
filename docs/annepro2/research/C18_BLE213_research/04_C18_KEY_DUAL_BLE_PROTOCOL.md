# C18 KEY 兼容 BLE 2.05 与 BLE 2.13 的协议规格

本文件把二进制差异转换为 C18 KEY 源码接口。首版在同一二进制源码中编入两个 profile，通过显式配置选择；两个发布构建只改变首次启动默认值。业务层只操作逻辑事件，BLE profile 负责版本差异。

## 兼容性矩阵

| 层级 | BLE 2.05 | BLE 2.13 | C18 KEY 策略 |
|---|---|---|---|
| UART GPIO | PA4/PA5，AF6 | 相同 | 共用 |
| UART 波特率 | 115200 | 相同 | 共用 |
| 基础帧 | `0x7B`、`0x10/0x12`、8 B 头、`0x7D` | 同族 | 共用 framer/parser |
| Keyboard ID 1 | 6KRO，8 B payload | 相同 | 共用 encoder |
| LED Output | 5 个标准 LED bit | 相同 | 接受 1 B 和 `[ID,bits]` |
| Vendor ID 2 | Usage 2 OUT，Usage 3 IN | Usage 2 IN，Usage 3 OUT | 按逻辑方向映射 |
| Consumer ID 3 | 8 bit + 3 B padding | 4 × 16-bit Usage | 两个 encoder |
| 四主机 | C18 旧业务 | AP2D 四槽业务 | 两个 command profile |
| 身份材料 | BLE 2.05 原实现 | 设备唯一地址校验 | KEY 不处理密钥 |
| USB suspend | KEY 负责 | KEY 负责 | C18 共用实现 |

## Profile 数据模型

建议把版本差异集中在只读配置中：

```c
enum ble_target {
    BLE_TARGET_205,
    BLE_TARGET_213,
};

enum consumer_format {
    CONSUMER_C18_BITMAP4,
    CONSUMER_AP2D_USAGE16X4,
};

struct ble_profile {
    enum ble_target target;
    enum consumer_format consumer;
    uint8_t vendor_host_to_key_usage;
    uint8_t vendor_key_to_host_usage;
    uint8_t host_slot_count;
    enum slot_protocol slots;
    bool led_output_may_include_report_id;
};
```

配置值：

| 字段 | `C18_BLE205` | `C18_BLE213` |
|---|---:|---:|
| `consumer` | `CONSUMER_C18_BITMAP4` | `CONSUMER_AP2D_USAGE16X4` |
| `vendor_host_to_key_usage` | 2 | 3 |
| `vendor_key_to_host_usage` | 3 | 2 |
| `host_slot_count` | 4 | 4 |
| `slots` | C18 旧命令 | AP2D `0x21/0x22` 命令族 |
| `led_output_may_include_report_id` | true | true |

虽然 2.05 的旧 KEY 常见单字节 LED 数据，统一接受 1/2 B 可以提高主机兼容性，且不改变合法 1 B 行为。

两个 profile 始终参与构建，选择项保存在带版本和 CRC 的 KEY NVM 中。无效或缺失配置回退 BLE 2.05：

```c
enum ble_target selected =
    nvm_profile_valid() ? nvm_profile_get() : BUILD_DEFAULT_BLE_PROFILE;

const struct ble_profile *profile = ble_profile_get(selected);
```

发布目标：

```text
make TARGET=c18 DEFAULT_BLE_PROFILE=205
make TARGET=c18 DEFAULT_BLE_PROFILE=213
```

USB 维护命令或启动组合键负责显式切换并持久化。建议把当前 profile 编入诊断日志和 USB 状态页，避免用户混淆。

运行时自动识别暂缓。当前没有从官方二进制中确认无副作用的版本查询命令；向未知 BLE 固件发送试探性业务包可能改变配对、槽位或 IAP 状态。取得 UART 抓包并确认查询/响应后，可以增加 `BLE_TARGET_AUTO`。

## Keyboard Report ID 1

三份 Report Map 的 ID 1 相同。KEY 输入 payload 固定 8 B：

| 字节 | 内容 |
|---:|---|
| 0 | modifier bits |
| 1 | reserved，填 0 |
| 2–7 | 最多 6 个 HID keycode |

Report ID 是否由外层 UART 命令携带，沿用现有 C18 KEY→BLE 封装。核心键盘 payload 不做版本分支。

锁定灯位序：

| bit | 状态 |
|---:|---|
| 0 | Num Lock |
| 1 | Caps Lock |
| 2 | Scroll Lock |
| 3 | Compose |
| 4 | Kana |

AP2D KEY 内部对主机 LED Output 的归一化输入：

```c
bool decode_led_output(const uint8_t *buf, size_t len, uint8_t *bits) {
    if (len == 1) {
        *bits = buf[0];
        return true;
    }
    if (len == 2 && buf[0] == 0x01) {
        *bits = buf[1];
        return true;
    }
    return false;
}
```

该 1/2 B decoder 是 AP2D KEY 内部来源证据，不直接复制到 C18。BLE↔KEY
UART 已提供更窄的共同契约：

```text
7B 12 35 00 03 00 00 7D 20 07 00/01
```

C18 backport 只从该精确帧更新 Caps bit，经 QMK `host_driver.keyboard_leds`
通知 C18 既有 LED MCU keymap 行为。新 route 前清零，重复值幂等；Num、
Scroll、Compose 和 Kana 在没有共享 UART opcode 证据时不推断。

## Consumer Report ID 3

C18 BLE 2.05 的 payload 为 4 B：

| bit | Consumer Usage |
|---:|---|
| 0 | `0x00E2` Mute |
| 1 | `0x00E9` Volume Increment |
| 2 | `0x00EA` Volume Decrement |
| 3 | `0x00CD` Play/Pause |
| 4 | `0x00B5` Scan Next Track |
| 5 | `0x00B6` Scan Previous Track |
| 6 | `0x006F` Brightness Increment |
| 7 | `0x0070` Brightness Decrement |

`payload[0]` 是位图，`payload[1..3]` 为常量 padding，发送时置零。

AP2D BLE 2.13 的 payload 为 8 B，由四个 little-endian `uint16_t Usage` 组成。最多同时表达四个 Consumer Usage：

```text
usage[0].lo usage[0].hi
usage[1].lo usage[1].hi
usage[2].lo usage[2].hi
usage[3].lo usage[3].hi
```

示例：

| 逻辑事件 | BLE 2.05 payload | BLE 2.13 payload |
|---|---|---|
| Mute press | `01 00 00 00` | `E2 00 00 00 00 00 00 00` |
| Volume Up press | `02 00 00 00` | `E9 00 00 00 00 00 00 00` |
| Next Track press | `10 00 00 00` | `B5 00 00 00 00 00 00 00` |
| release all | `00 00 00 00` | `00 00 00 00 00 00 00 00` |

编码接口：

```c
size_t encode_consumer(
    const struct ble_profile *profile,
    const uint16_t *usages,
    size_t usage_count,
    uint8_t *out);
```

规则：

1. 2.05 只接受上表八个 Usage，转换为位图；无法表达的 Usage 返回错误。
2. 2.13 最多编码四个非零 Usage，按小端写入 8 B。
3. press 和 release 都必须发送，切换槽、suspend 和断开前强制 release all。
4. 编码失败不能回退为截断数据；记录诊断事件并发送 release all。

## Vendor Report ID 2

Vendor payload 长度在两个版本中都是 18 B，方向发生交换。

业务层定义两个逻辑接口：

```c
send_vendor_to_host(payload18);
on_vendor_from_host(payload18);
```

profile 映射：

| 逻辑方向 | BLE 2.05 | BLE 2.13 |
|---|---|---|
| Host → KEY | Usage 2 OUT | Usage 3 OUT |
| KEY → Host | Usage 3 IN | Usage 2 IN |

业务模块不得直接依赖 Usage 2 或 Usage 3。这样能够保护 ObinsKit/Hexcore Link 桥接、配置读写和厂商命令免受方向交换影响。

Report Map 还把 flags 从 Variable 改为 Array。KEY 侧仍按不透明 18 B payload 搬运，不应按数组索引重新解释厂商协议。主机软件和 BLE GATT 层负责 Report Map 语义，KEY 只保证逻辑方向与长度正确。

当前静态分析能够确定 HID 方向，尚未完整确认 KEY↔BLE UART 业务 opcode 是否也发生变化。开发前需要一组 UART 抓包：

1. 主机向键盘读取配置；
2. 主机写入一项可回滚配置；
3. KEY 主动向主机返回配置或状态；
4. 对 BLE 2.05 与 2.13 分别记录外层 opcode、长度和方向。

在抓包完成前，Vendor 桥接只能实现 profile 接口和严格校验，不能猜测业务 opcode。

由于 BLE 2.13 保持原二进制，蓝牙侧配置软件会看到 AP2D 名称和 2.13 的 Vendor Report Map。C18 的 USB 配置接口仍可保持原 C18 契约；蓝牙配置接口需要完成 AP2D Vendor 方向和业务 opcode 适配。若首版没有完成 Vendor 抓包与命令映射，应明确关闭蓝牙配置功能，普通键盘、媒体、锁定灯和四槽仍可独立验收。

## UART 完整帧接收

接收器使用状态机保存消息边界：

```text
SEARCH_START
  读到 0x7B → READ_HEADER

READ_HEADER
  收齐 8 B → VALIDATE_HEADER
  超时/非法 → SEARCH_START

READ_PAYLOAD
  收齐声明长度 → DELIVER
  超时/越界 → SEARCH_START

DELIVER
  投递完整帧 → SEARCH_START
```

校验要求：

- 起始字节为 `0x7B`；
- 类型仅接受已知 `0x10/0x12`；
- AP2D 语义下第三头字节低半字节为 1–7，高半字节非零；
- 声明长度不超过静态接收缓冲上限；
- 实际 payload 长度必须等于声明值；
- `0x10` 到达时复位分片序号；
- 只有 `0x12` 使用连续/分片序号检查；
- 错帧丢弃后从后续字节继续搜索 `0x7B`；
- 超时、错长度、错序号分别计数。

兼容策略：

- 共享 framer 保留两版本共同的外层边界；
- profile 决定是否启用 AP2D 第三头字节严格检查；
- 负载完整性和缓冲上限检查对两版本都启用；
- 未知业务命令返回受控错误，不进入槽位或 IAP 状态。

## 四主机命令适配

AP2D KEY 3.08 已确认处理业务命令 `0x21/0x22` 和槽号 0–3。命令 `0x21` 选择四块 9 B 槽数据，命令 `0x22` 查询/取得对应槽数据。

静态证据尚未完整恢复外层 UART 帧中所有字段含义，因此源码使用抽象接口：

```c
int ble_pair_slot(uint8_t slot);
int ble_switch_slot(uint8_t slot);
int ble_query_slot(uint8_t slot);
int ble_clear_slot(uint8_t slot);
int ble_clear_all_slots(void);
```

`BLE_TARGET_213` 适配器把这些动作映射到已抓包确认的 `0x21/0x22` 负载；`BLE_TARGET_205` 保持 C18 原命令。

在抓包确认前，不把 `0x21` 或 `0x22` 直接命名为“配对”或“切换”。当前证据只确认它们参与四槽数据处理，完整请求/响应语义仍需实机事务建立。

发送前检查：

- `slot < profile->host_slot_count`；
- 当前没有未完成的 IAP；
- UART 队列有足够空间；
- 切换请求带新的 transaction/generation ID；
- 配对长按与切换短按不能合并。

收到响应后检查：

- 命令、槽号、长度和 transaction 匹配；
- 迟到的旧 transaction 只记录，不更新状态；
- 只有安全连接完成后持久化 `active_slot`；
- 超时保留旧 bond，不把目标槽伪装成已连接。

## 电源与模式切换协议

进入 USB suspend、切换 USB/BLE 模式或 BLE 槽位前统一执行：

1. 停止产生新输入；
2. 发送 Keyboard release all；
3. 发送 Consumer release all；
4. 有界 flush BLE UART；
5. 更新 LED/连接状态；
6. 执行断开、切槽或挂起。

恢复后：

1. 恢复时钟、UART 和矩阵；
2. 清空过期输入与分片状态；
3. 发送 release all；
4. 查询或等待真实 BLE 状态；
5. 同步锁定灯；
6. 恢复新输入。

该顺序同时覆盖 AP2D 3.05 提到的 release 丢失、3.08 的多设备切换和 USB suspend 风险。

## 首版协议完成条件

| 功能 | `C18_BLE205` | `C18_BLE213` |
|---|---|---|
| 普通键盘 | 原行为一致 | 6KRO 正常 |
| Consumer | 4 B 位图 | 8 B Usage 数组 |
| Vendor | 保留旧方向 | 首版关闭；确认业务 opcode 后适配新方向 |
| Caps 锁定灯 | `20/07 00/01` | `20/07 00/01` |
| Num/Scroll | 未确认独立 UART ABI | 未确认独立 UART ABI |
| 四槽 | 原能力无回归 | 配对、切换、超时、重配完整 |
| 串口异常 | 可重新同步 | 可重新同步 |
| suspend | 第二阶段按 C18 语义实现 | 第二阶段使用同一 C18 语义 |
| BLE 二进制 | 2.05 原件 | 2.13 原件，保持不变 |
