# Anne Pro 2 reverse-engineering helpers

`DecompileAt.java` is a small Ghidra headless post-script used to make the
official keyboard-MCU analysis reproducible. Import
`assets/ap2_fw/annepro2/c18/firmware/2.36.3/key-c18-2.36.3.bin` as a raw
little-endian Cortex-M image with base address `0x4000`, then pass one or more
function entry addresses to the script. It disassembles up to 80 instructions
and prints Ghidra's decompiler output for each address.

Useful entries for the BLE UART path:

| Address | Role |
| ---: | --- |
| `0x58a4` | byte-oriented UART frame parser |
| `0x58b2` | complete-frame group dispatch |
| `0x6410` | group `0x20` RX handler |
| `0x7eac` | group `0x20` response builder |
| `0x85c0` | common frame-header builder |
| `0x8606` | append group/opcode/payload |
| `0xacac` | protocol transmit path |

The firmware images are analysis inputs provided by the `assets/ap2_fw`
submodule; the helper does not modify them or export proprietary code.

`replay_uart.py` replays raw hexadecimal bytes or `qmk console` lines through
the same `8 + payload length` framing rules used by the QMK driver. It prints
complete frames and the keyboard-MCU response expected for the confirmed
`20/07` and `20/0c` requests:

```sh
direnv exec . python tools/reverse/annepro2/replay_uart.py --self-test
direnv exec . python tools/reverse/annepro2/replay_uart.py ap2-console.log
```

The self-test covers garbage-prefix resynchronization, variable frame lengths,
the value-preserving `20/07` response, and the fixed `20/0c` response. This is
a host-side protocol replay, not an RF or physical UART test.

`recover_ap2d_data.py` executes only AP2D KEY 3.08's position-independent
Thumb decompressor at `0x13700`. It restores the initialized RAM image and
prints the protocol group dispatch table at `0x20000414`:

```sh
direnv exec . just annepro2-recover-ap2d-data
```

The helper validates the expected eleven group IDs before writing the
recovered data to `/tmp/ap2d-key-3.08.data.bin`. Unicorn emulates only the
firmware decompressor; the script does not emulate the board, UART, BLE radio,
or RTOS and therefore provides static binary evidence rather than hardware
validation.

## BLE cross-flash input validation

`validate_ble_crossflash.py` validates the exact official C18 BLE 2.05 and
AP2D BLE 2.13 update images without accessing hardware:

```sh
direnv exec . python3 tools/reverse/annepro2/validate_ble_crossflash.py
```

It also accepts debugger-created 256 KiB CC2541 flash and 2 KiB Information
Page backups. These backups are device-private and must not be committed:

```sh
direnv exec . python3 tools/reverse/annepro2/validate_ble_crossflash.py \
  --full-flash-backup /private/path/full-flash.bin \
  --information-page-backup /private/path/information-page.bin \
  --require-hardware-backups
```

This is a preflight checker, not a flasher. It performs no USB or CC2541 I/O.
`patch_ble213_name.py` 从已知官方 BLE 2.13 镜像生成固定宽度的 C18 兼容名称
变体。它只把两个 18 字节名称改为 `HEXCORE AnnePro 2C`，并验证输入/输出
哈希及完整 diff 范围：

```sh
direnv exec . just annepro2-ble213-name-image
```

The generated image is written to the userspace root by default. It is an
ignored build artifact; the archived, checksummed derivative remains in the
firmware submodule.

`plan_ble_iap.py` 为已知官方 BLE 2.05/2.13 镜像生成不可执行的 IAP 请求计划。
它固定 BLE route、C18 实测的 `0x4000` transport base 和 32 字节分块，输出
首末帧与整体摘要。计划同时记录已验证工具要求回复匹配
target/command/key 且 status 为零，但脚本本身没有 HID transport：

```sh
direnv exec . just annepro2-ble-crossflash-plan \
  /tmp/annepro2-ble213-iap-plan.json
```
