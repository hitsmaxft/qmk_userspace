# BLE 2.13 的 AnnePro 2C 兼容名称变体

为 C18 生成一个专用 BLE 2.13 变体，使主机显示
`HEXCORE AnnePro 2C`，其中 `C` 表示 compatibility mode，而不是 AP2D 的
`HEXCORE AnnePro 2D`。官方 BLE 2.13 镜像仍作为只读输入保留，不被覆盖。

## 固定宽度补丁

新旧名称都是 18 字节，因此不需要移动数据或修改长度、指针和代码：

| 用途 | 文件偏移 | 原始 18 B | 新 18 B |
|---|---:|---|---|
| 广播 Complete Local Name | `0x02E5` | `HXECORE AnnePro 2D` | `HEXCORE AnnePro 2C` |
| GAP Device Name | `0x60FB` | `HEXCORE AnnePro 2D` | `HEXCORE AnnePro 2C` |

广播结构的 length `0x13` 和 type `0x09` 保持不变；GAP 名称后的 NUL 终止符
也保持不变。整个 `0x26000` 镜像只有 `0x002E6`、`0x002E7`、`0x002F6` 和
`0x0610C` 四个字节发生变化。

## 可复现生成

```sh
direnv exec . just annepro2-ble213-name-image
```

输入与输出：

| 文件 | SHA-256 |
|---|---|
| 官方 `annepro2_discovery_ble.bin` | `1b904ae9cd8bf6835c0b77c72618256b701a2c3b74dc04e9dddb8a388bdfc73d` |
| 生成的 `c18-ble-2.13-annepro2c.bin` | `3779983ad762edb42ded93744076b0674c016b014488fa59aa02dfc5ca171daf` |

生成器会拒绝未知输入哈希、错误大小、偏移处内容不符或任何声明外的字节变化。
生成的 `.bin` 属于本地固件产物，不提交 Git；Git 保存生成器、测试、输入来源
和预期哈希。

## 验证边界

同长度静态补丁证明固件布局没有移动，但不能代替运行验证。刷写后仍需确认：

1. 未配对扫描显示 `HEXCORE AnnePro 2C`；
2. GAP 连接详情显示同一名称和 `BLE-1.5.2`；
3. 配对、普通键盘、媒体键和四槽行为与原始 BLE 2.13 一致。

macOS 会缓存已配对设备的旧名称。若名称没有立即更新，应先区分缓存与实际广播，
必要时删除旧配对记录后重新扫描；不能仅凭系统设置页中的旧文字判断补丁失败。
