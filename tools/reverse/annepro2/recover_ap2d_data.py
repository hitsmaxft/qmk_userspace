#!/usr/bin/env python3
"""Recover the AP2D KEY initialized-RAM image with its own decompressor.

The 3.08 image uses the Thumb routine at 0x13700 to expand the compressed
payload described by the constructor record at 0x138c0 into 0x20000000.
Executing only that position-independent routine avoids emulating the board,
RTOS, or peripherals.
"""

from __future__ import annotations

import argparse
import struct
from pathlib import Path

from unicorn import UC_ARCH_ARM, UC_MODE_THUMB, Uc
from unicorn.arm_const import UC_ARM_REG_LR, UC_ARM_REG_PC, UC_ARM_REG_R0, UC_ARM_REG_SP

FLASH_BASE = 0x4000
FLASH_MAP_BASE = 0x0000
FLASH_MAP_SIZE = 0x20000
RAM_BASE = 0x20000000
RAM_MAP_SIZE = 0x10000
DECOMPRESS_ENTRY = 0x13700
DECOMPRESS_RECORD = 0x138C0
RETURN_SENTINEL = 0x18000
INITIALIZED_DATA_SIZE = 0x474
PROTOCOL_TABLE_OFFSET = 0x414
EXPECTED_PROTOCOL_GROUPS = (0x01, 0x02, 0x03, 0x10, 0x11, 0x12, 0x20, 0x30, 0x40, 0x50, 0x60)


def recover(image: bytes) -> bytes:
    if len(image) <= DECOMPRESS_ENTRY - FLASH_BASE:
        raise ValueError("image is too small to contain the AP2D decompressor")

    emulator = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
    emulator.mem_map(FLASH_MAP_BASE, FLASH_MAP_SIZE)
    emulator.mem_write(FLASH_BASE, image)
    emulator.mem_map(RAM_BASE, RAM_MAP_SIZE)
    emulator.mem_write(RETURN_SENTINEL, b"\x00\xbe")  # bkpt 0

    emulator.reg_write(UC_ARM_REG_R0, DECOMPRESS_RECORD)
    emulator.reg_write(UC_ARM_REG_SP, RAM_BASE + 0x8000)
    emulator.reg_write(UC_ARM_REG_LR, RETURN_SENTINEL | 1)
    emulator.emu_start(DECOMPRESS_ENTRY | 1, RETURN_SENTINEL, count=1_000_000)

    if emulator.reg_read(UC_ARM_REG_PC) != RETURN_SENTINEL:
        raise RuntimeError("AP2D decompressor did not reach its return sentinel")

    data = bytes(emulator.mem_read(RAM_BASE, INITIALIZED_DATA_SIZE))
    groups = tuple(data[PROTOCOL_TABLE_OFFSET + index * 8] for index in range(len(EXPECTED_PROTOCOL_GROUPS)))
    if groups != EXPECTED_PROTOCOL_GROUPS:
        raise ValueError("recovered RAM does not contain the AP2D 3.08 protocol table")
    return data


def print_protocol_handlers(data: bytes) -> None:
    print("protocol handlers at 0x20000414:")
    for index in range(len(EXPECTED_PROTOCOL_GROUPS)):
        offset = PROTOCOL_TABLE_OFFSET + index * 8
        group = data[offset]
        handler = struct.unpack_from("<I", data, offset + 4)[0]
        print(f"  [{index:2}] group=0x{group:02x} handler=0x{handler:08x}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    data = recover(args.image.read_bytes())
    if args.output:
        args.output.write_bytes(data)
    print_protocol_handlers(data)


if __name__ == "__main__":
    main()
