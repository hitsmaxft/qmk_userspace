#!/usr/bin/env python3
"""Generate a non-executable Anne Pro 2 BLE IAP transfer plan.

This module deliberately has no HID dependency and no transport abstraction.
It records the request bytes used by the current AnnePro2-Tools implementation
at the C18 IAP layout address observed during the BLE 2.13 transfer.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path
from typing import Iterable

import validate_ble_crossflash as validator


AP2_TARGET_BLE = 0x05
AP2_SOURCE_USB_HOST = 0x01
AP2_BLE_ROUTE = (AP2_TARGET_BLE << 4) | AP2_SOURCE_USB_HOST
AP2_LAYER2_FIRMWARE = 0x02
AP2_IAP_WRITE_MEMORY = 0x31
AP2_IAP_ERASE_MEMORY = 0x43
AP2_OUTER_FRAME_SIZE = 64
AP2_HID_REPORT_ID = 0x00
# The official update file starts at image offset zero. The C18 USB IAP
# descriptor reports 0x4000 as the transport address for the BLE application,
# so request addresses are image offsets plus 0x4000.
BLE_TRANSPORT_BASE = 0x4000


class PlanError(ValueError):
    """Raised when a requested dry-run plan violates a fixed invariant."""


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def build_outer_frame(payload: bytes) -> bytes:
    """Build the 64-byte AP2 USB-host-to-BLE outer frame."""
    if len(payload) > AP2_OUTER_FRAME_SIZE - 8:
        raise PlanError(f"payload is too large for AP2 outer frame: {len(payload)}")

    frame = bytearray(AP2_OUTER_FRAME_SIZE)
    frame[:8] = bytes(
        [
            0x7B,
            0x10,
            AP2_BLE_ROUTE,
            0x10,
            len(payload),
            0x00,
            0x00,
            0x7D,
        ]
    )
    frame[8 : 8 + len(payload)] = payload
    return bytes(frame)


def build_hid_report(payload: bytes) -> bytes:
    """Prepend the report ID used by the current HID transport."""
    return bytes([AP2_HID_REPORT_ID]) + build_outer_frame(payload)


def build_erase_payload(base: int) -> bytes:
    if base != BLE_TRANSPORT_BASE:
        raise PlanError(
            f"BLE IAP transport base must be 0x{BLE_TRANSPORT_BASE:X}, "
            f"found 0x{base:X}"
        )
    return bytes([AP2_LAYER2_FIRMWARE, AP2_IAP_ERASE_MEMORY]) + base.to_bytes(
        4, "little"
    )


def build_write_payload(address: int, chunk: bytes) -> bytes:
    if address < BLE_TRANSPORT_BASE:
        raise PlanError(
            f"BLE IAP address precedes transport base 0x{BLE_TRANSPORT_BASE:X}"
        )
    if address % validator.BLE_IAP_CHUNK_SIZE != 0:
        raise PlanError(f"BLE IAP address is not 32-byte aligned: 0x{address:X}")
    if len(chunk) != validator.BLE_IAP_CHUNK_SIZE:
        raise PlanError(
            "BLE IAP writes must contain exactly "
            f"{validator.BLE_IAP_CHUNK_SIZE} bytes"
        )
    return (
        bytes([AP2_LAYER2_FIRMWARE, AP2_IAP_WRITE_MEMORY])
        + address.to_bytes(4, "little")
        + chunk
    )


def identify_and_validate_image(path: Path) -> tuple[str, bytes]:
    data = path.read_bytes()
    digest = sha256(data)

    for label, known in validator.KNOWN_IMAGES.items():
        if digest != known["sha256"]:
            continue
        validator.validate_update_image(
            path,
            label,
            known["sha256"],
            known["last_non_ff"],
        )
        return label, data

    raise PlanError(f"unknown BLE image SHA-256: {digest}")


def build_plan(path: Path, base: int = BLE_TRANSPORT_BASE) -> dict[str, object]:
    if base != BLE_TRANSPORT_BASE:
        raise PlanError(
            f"BLE IAP transport base must be 0x{BLE_TRANSPORT_BASE:X}, "
            f"found 0x{base:X}"
        )

    label, image = identify_and_validate_image(path)
    if len(image) % validator.BLE_IAP_CHUNK_SIZE != 0:
        raise PlanError("BLE image does not end on a complete 32-byte block")

    erase_payload = build_erase_payload(base)
    erase_report = build_hid_report(erase_payload)
    write_reports: list[bytes] = []
    first_write: dict[str, object] | None = None
    last_write: dict[str, object] | None = None

    for offset in range(0, len(image), validator.BLE_IAP_CHUNK_SIZE):
        address = base + offset
        chunk = image[offset : offset + validator.BLE_IAP_CHUNK_SIZE]
        payload = build_write_payload(address, chunk)
        report = build_hid_report(payload)
        write_reports.append(report)
        summary = {
            "address": address,
            "address_hex": f"0x{address:05X}",
            "chunk_sha256": sha256(chunk),
            "payload_hex": payload.hex(),
            "hid_report_hex": report.hex(),
        }
        if first_write is None:
            first_write = summary
        last_write = summary

    assert first_write is not None
    assert last_write is not None
    wire_plan = erase_report + b"".join(write_reports)

    return {
        "schema": "annepro2-ble-iap-dry-run-v1",
        "executable": False,
        "hardware_access": False,
        "transport": None,
        "image": {
            "label": label,
            "size": len(image),
            "sha256": sha256(image),
        },
        "routing": {
            "source": "usb_host",
            "target": "ble",
            "route_byte": AP2_BLE_ROUTE,
            "route_byte_hex": f"0x{AP2_BLE_ROUTE:02X}",
            "hid_report_id": AP2_HID_REPORT_ID,
        },
        "erase": {
            "base": base,
            "base_kind": "device-reported-transport-address",
            "payload_hex": erase_payload.hex(),
            "hid_report_hex": erase_report.hex(),
            "reply_policy": "matching-target-command-key-and-status-zero",
        },
        "writes": {
            "chunk_size": validator.BLE_IAP_CHUNK_SIZE,
            "count": len(write_reports),
            "first_address": base,
            "last_address": base + len(image) - validator.BLE_IAP_CHUNK_SIZE,
            "end_exclusive": base + len(image),
            "first": first_write,
            "last": last_write,
            "wire_plan_sha256": sha256(wire_plan),
            "reply_policy": "matching-target-command-key-and-status-zero",
        },
        "observed_transfer": [
            "C18 IAP reported BLE transport base 0x00004000.",
            "Erase and all 4864 writes returned matching status-zero replies.",
            "The official BLE 2.13 image booted and established a macOS BLE link.",
        ],
        "limitations": [
            "This output is a byte plan, not a flasher.",
            "Generating this plan never opens HID and never reads or writes hardware.",
            "The IAP protocol exposes no validated flash readback command.",
            "Status-zero transfer completion is not byte-for-byte flash verification.",
            "The physical CC254x erase mapping behind transport base 0x4000 is inferred.",
        ],
    }


def parse_int(value: str) -> int:
    try:
        return int(value, 0)
    except ValueError as error:
        raise argparse.ArgumentTypeError(str(error)) from error


def parse_args(argv: Iterable[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Generate a non-executable BLE IAP request plan for a known "
            "official Anne Pro image. This command has no hardware transport."
        )
    )
    parser.add_argument(
        "image",
        nargs="?",
        type=Path,
        default=validator.DEFAULT_AP2D_BLE213,
    )
    parser.add_argument("--base", type=parse_int, default=BLE_TRANSPORT_BASE)
    parser.add_argument("--output", type=Path)
    return parser.parse_args(argv)


def main(argv: Iterable[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        plan = build_plan(args.image, args.base)
        rendered = json.dumps(plan, indent=2, sort_keys=True) + "\n"
        if args.output:
            args.output.write_text(rendered, encoding="utf-8")
            print(f"dry-run plan written: {args.output}")
        else:
            print(rendered, end="")
    except (OSError, PlanError, validator.ValidationError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
