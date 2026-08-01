#!/usr/bin/env python3
"""Build the C18 compatibility-name variant of AP2D BLE 2.13.

The official image remains unchanged. This script accepts only that exact
image and replaces its two fixed-width local-name strings. No offsets, length
fields, pointers, or code bytes move.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import sys
from pathlib import Path
from typing import Iterable


REPOSITORY_ROOT = Path(__file__).resolve().parents[3]
ARCHIVE_ROOT = Path(os.environ.get("AP2_FW_SOURCE", REPOSITORY_ROOT / "assets/ap2_fw"))
SOURCE = ARCHIVE_ROOT / "annepro2d/firmware/3.08/annepro2_discovery_ble.bin"
DEFAULT_OUTPUT = Path("c18-ble-2.13-annepro2c.bin")
IMAGE_SIZE = 0x26000
SOURCE_SHA256 = "1b904ae9cd8bf6835c0b77c72618256b701a2c3b74dc04e9dddb8a388bdfc73d"
OUTPUT_SHA256 = "3779983ad762edb42ded93744076b0674c016b014488fa59aa02dfc5ca171daf"

ADVERTISEMENT_NAME_OFFSET = 0x02E5
GAP_NAME_OFFSET = 0x60FB
ADVERTISEMENT_LENGTH_OFFSET = 0x02E3
ADVERTISEMENT_TYPE_OFFSET = 0x02E4
COMPATIBILITY_NAME = b"HEXCORE AnnePro 2C"

PATCHES = (
    (ADVERTISEMENT_NAME_OFFSET, b"HXECORE AnnePro 2D", COMPATIBILITY_NAME),
    (GAP_NAME_OFFSET, b"HEXCORE AnnePro 2D", COMPATIBILITY_NAME),
)


class PatchError(ValueError):
    """Raised when the input or generated image violates an invariant."""


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def expected_changed_offsets() -> set[int]:
    offsets: set[int] = set()
    for offset, old, new in PATCHES:
        offsets.update(
            offset + index
            for index, (old_byte, new_byte) in enumerate(zip(old, new))
            if old_byte != new_byte
        )
    return offsets


def build_variant(source: bytes) -> bytes:
    if len(source) != IMAGE_SIZE:
        raise PatchError(
            f"source size mismatch: expected 0x{IMAGE_SIZE:X}, found 0x{len(source):X}"
        )
    digest = sha256(source)
    if digest != SOURCE_SHA256:
        raise PatchError(
            f"source SHA-256 mismatch: expected {SOURCE_SHA256}, found {digest}"
        )

    result = bytearray(source)
    for offset, old, new in PATCHES:
        if len(old) != len(new):
            raise PatchError("name replacement must not change string length")
        found = bytes(result[offset : offset + len(old)])
        if found != old:
            raise PatchError(
                f"unexpected bytes at 0x{offset:05X}: "
                f"expected {old!r}, found {found!r}"
            )
        result[offset : offset + len(old)] = new

    variant = bytes(result)
    changed = {
        index
        for index, (source_byte, variant_byte) in enumerate(zip(source, variant))
        if source_byte != variant_byte
    }
    expected = expected_changed_offsets()
    if changed != expected:
        raise PatchError(
            f"changed-offset mismatch: expected {sorted(expected)}, found {sorted(changed)}"
        )
    if variant[ADVERTISEMENT_LENGTH_OFFSET] != 0x13:
        raise PatchError("advertising Complete Local Name length changed")
    if variant[ADVERTISEMENT_TYPE_OFFSET] != 0x09:
        raise PatchError("advertising Complete Local Name type changed")
    if variant[GAP_NAME_OFFSET + len(COMPATIBILITY_NAME)] != 0:
        raise PatchError("GAP name terminator changed")
    if variant.count(COMPATIBILITY_NAME) != 2:
        raise PatchError("generated image does not contain exactly two compatibility names")
    if b"AnnePro 2D" in variant:
        raise PatchError("generated image still contains an AP2D local name")

    digest = sha256(variant)
    if digest != OUTPUT_SHA256:
        raise PatchError(
            f"output SHA-256 mismatch: expected {OUTPUT_SHA256}, found {digest}"
        )
    return variant


def parse_args(argv: Iterable[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Generate an AnnePro 2C fixed-width name variant from the exact "
            "official AP2D BLE 2.13 image."
        )
    )
    parser.add_argument("--source", type=Path, default=SOURCE)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    return parser.parse_args(argv)


def main(argv: Iterable[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        variant = build_variant(args.source.read_bytes())
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_bytes(variant)
    except (OSError, PatchError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 2

    print(f"generated: {args.output}")
    print(f"size: {len(variant)} (0x{len(variant):X})")
    print(f"sha256: {sha256(variant)}")
    print(f"changed bytes: {len(expected_changed_offsets())}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
