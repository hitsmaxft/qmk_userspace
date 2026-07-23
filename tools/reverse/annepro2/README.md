# Anne Pro 2 reverse-engineering helpers

`DecompileAt.java` is a small Ghidra headless post-script used to make the
official keyboard-MCU analysis reproducible. Import
`assets/ap2_fw/key-c18-2.36.3.bin` as a raw little-endian Cortex-M image with
base address `0x4000`, then pass one or more function entry addresses to the
script. It disassembles up to 80 instructions and prints Ghidra's decompiler
output for each address.

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

The firmware images are analysis inputs and remain under `assets/ap2_fw/`;
the helper does not modify them or export proprietary code.

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
