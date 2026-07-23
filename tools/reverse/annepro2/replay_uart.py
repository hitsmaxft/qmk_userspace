#!/usr/bin/env python3
"""Replay Anne Pro 2 BLE UART RX bytes and show keyboard-MCU responses."""

from __future__ import annotations

import argparse
import re
import sys
import unittest
from collections.abc import Iterable, Iterator

HEADER_SIZE = 8
MAX_PAYLOAD = 32
HEX_BYTE = re.compile(r"(?i)\b[0-9a-f]{2}\b")


class FrameParser:
    """Mirror the byte-oriented framing rules used by annepro2_ble.c."""

    def __init__(self) -> None:
        self.frame = bytearray()
        self.expected_size = 0

    def reset(self, byte: int | None = None) -> None:
        self.frame.clear()
        self.expected_size = 0
        if byte == 0x7B:
            self.frame.append(byte)

    def feed(self, byte: int) -> bytes | None:
        if not self.frame:
            if byte == 0x7B:
                self.frame.append(byte)
            return None

        self.frame.append(byte)
        if len(self.frame) == 5:
            payload_size = self.frame[4]
            if payload_size > MAX_PAYLOAD:
                self.reset(byte)
                return None
            self.expected_size = HEADER_SIZE + payload_size

        if len(self.frame) == HEADER_SIZE and self.frame[7] != 0x7D:
            self.reset(byte)
            return None

        if self.expected_size and len(self.frame) == self.expected_size:
            complete = bytes(self.frame)
            self.reset()
            return complete

        return None


def parse_hex_lines(lines: Iterable[str]) -> Iterator[int]:
    for line in lines:
        # QMK console prefixes include timestamps. Only replay bytes after rxN.
        match = re.search(r"\brx\d+\b", line, flags=re.IGNORECASE)
        if match:
            line = line[match.end() :]
        for token in HEX_BYTE.findall(line):
            yield int(token, 16)


def response_for(frame: bytes) -> bytes | None:
    if len(frame) < 11 or frame[1:3] != b"\x12\x35":
        return None
    if frame[4:6] != b"\x03\x00" or frame[8] != 0x20:
        return None
    if frame[9] == 0x07:
        return bytes.fromhex("7b 12 43 00 03 00 00 7d 20 07") + frame[10:11]
    if frame[9] == 0x0C:
        return bytes.fromhex("7b 12 43 00 04 00 00 7d 20 0c 00 00")
    return None


def format_hex(data: bytes) -> str:
    return " ".join(f"{byte:02X}" for byte in data)


def replay(values: Iterable[int]) -> int:
    parser = FrameParser()
    count = 0
    for value in values:
        frame = parser.feed(value)
        if frame is None:
            continue
        count += 1
        group = frame[8] if len(frame) > 8 else 0
        opcode = frame[9] if len(frame) > 9 else 0
        print(f"rx len={len(frame)} group={group:02X} opcode={opcode:02X}: {format_hex(frame)}")
        response = response_for(frame)
        if response is not None:
            print(f"tx len={len(response)}: {format_hex(response)}")
    return count


class ReplayTests(unittest.TestCase):
    def test_state_sync_response_preserves_value(self) -> None:
        frame = bytes.fromhex("7b 12 35 00 03 00 00 7d 20 07 a5")
        self.assertEqual(
            response_for(frame),
            bytes.fromhex("7b 12 43 00 03 00 00 7d 20 07 a5"),
        )

    def test_hid_handshake_response(self) -> None:
        frame = bytes.fromhex("7b 12 35 00 03 00 00 7d 20 0c 00")
        self.assertEqual(
            response_for(frame),
            bytes.fromhex("7b 12 43 00 04 00 00 7d 20 0c 00 00"),
        )

    def test_stream_resynchronizes_and_parses_variable_lengths(self) -> None:
        stream = bytes.fromhex(
            "00 ff "
            "7b 12 35 00 ff "
            "7b 12 35 00 03 00 00 7d 40 04 00 "
            "7b 12 35 00 04 00 00 7d 20 0c 00 01"
        )
        parser = FrameParser()
        frames = [frame for byte in stream if (frame := parser.feed(byte)) is not None]
        self.assertEqual([len(frame) for frame in frames], [11, 12])
        self.assertEqual(frames[0][8:10], b"\x40\x04")
        self.assertEqual(frames[1][8:10], b"\x20\x0c")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("file", nargs="?", help="hex bytes or QMK console log; stdin if omitted")
    parser.add_argument("--self-test", action="store_true", help="run deterministic parser/reply tests")
    args = parser.parse_args()

    if args.self_test:
        suite = unittest.defaultTestLoader.loadTestsFromTestCase(ReplayTests)
        return 0 if unittest.TextTestRunner(verbosity=2).run(suite).wasSuccessful() else 1

    if args.file:
        with open(args.file, encoding="utf-8") as source:
            return 0 if replay(parse_hex_lines(source)) else 1
    return 0 if replay(parse_hex_lines(sys.stdin)) else 1


if __name__ == "__main__":
    raise SystemExit(main())
