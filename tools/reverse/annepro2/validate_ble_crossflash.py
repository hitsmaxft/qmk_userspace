#!/usr/bin/env python3
"""Validate the static inputs for an Anne Pro 2 C18 BLE 2.13 cross-flash.

This tool deliberately does not talk to the keyboard.  It proves properties of
the official update images and, when supplied, checks debugger-created backup
files.  It cannot prove the semantics of the keyboard's IAP erase command.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import sys
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable


REPOSITORY_ROOT = Path(__file__).resolve().parents[3]
ARCHIVE_ROOT = Path(os.environ.get("AP2_FW_SOURCE", REPOSITORY_ROOT / "assets/ap2_fw"))

DEFAULT_C18_BLE205 = (
    ARCHIVE_ROOT / "annepro2/c18/firmware/2.36.3/ap2_c18_0205_ble.bin"
)
DEFAULT_AP2D_BLE213 = (
    ARCHIVE_ROOT / "annepro2d/firmware/3.08/annepro2_discovery_ble.bin"
)

CC2541_F256_FLASH_SIZE = 0x40000
CC2541_INFORMATION_PAGE_SIZE = 0x800
CC2541_INFORMATION_PAGE_IEEE_OFFSET = 0x0E
OFFICIAL_UPDATE_IMAGE_SIZE = 0x26000
OFFICIAL_CODE_CEILING = 0x20000
BLE_IAP_CHUNK_SIZE = 32

KNOWN_IMAGES = {
    "c18_ble205": {
        "sha256": "52dc5c6542ad9b30915ea07f042b46ef19cd8acf4f2dce286a0dbdf11ce7cb92",
        "last_non_ff": 0x1F480,
    },
    "ap2d_ble213": {
        "sha256": "1b904ae9cd8bf6835c0b77c72618256b701a2c3b74dc04e9dddb8a388bdfc73d",
        "last_non_ff": 0x1E24C,
    },
}


class ValidationError(ValueError):
    """Raised when an input violates a cross-flash safety invariant."""


@dataclass(frozen=True)
class ImageReport:
    label: str
    path: str
    size: int
    sha256: str
    reset_ljmp_target: int
    last_non_ff: int
    trailing_ff: int
    code_tail_is_erased: bool


@dataclass(frozen=True)
class BackupReport:
    full_flash_path: str
    full_flash_sha256: str
    information_page_path: str
    information_page_sha256: str
    factory_ieee_valid: bool
    factory_ieee_sha256: str


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def last_non_ff(data: bytes) -> int:
    for offset in range(len(data) - 1, -1, -1):
        if data[offset] != 0xFF:
            return offset
    return -1


def validate_update_image(
    path: Path, label: str, expected_sha256: str, expected_last_non_ff: int
) -> ImageReport:
    data = path.read_bytes()
    digest = sha256(data)

    if len(data) != OFFICIAL_UPDATE_IMAGE_SIZE:
        raise ValidationError(
            f"{label}: expected 0x{OFFICIAL_UPDATE_IMAGE_SIZE:X} bytes, "
            f"found 0x{len(data):X}"
        )
    if len(data) % BLE_IAP_CHUNK_SIZE != 0:
        raise ValidationError(
            f"{label}: image is not aligned to the BLE IAP 32-byte chunk size"
        )
    if digest != expected_sha256:
        raise ValidationError(
            f"{label}: SHA-256 mismatch: expected {expected_sha256}, found {digest}"
        )
    if len(data) < 3 or data[0] != 0x02:
        raise ValidationError(f"{label}: reset vector is not an 8051 LJMP")

    reset_target = int.from_bytes(data[1:3], "big")
    if reset_target >= OFFICIAL_CODE_CEILING:
        raise ValidationError(
            f"{label}: reset LJMP target 0x{reset_target:X} is outside code envelope"
        )

    final_byte = last_non_ff(data)
    if final_byte != expected_last_non_ff:
        raise ValidationError(
            f"{label}: last non-0xFF byte changed: expected "
            f"0x{expected_last_non_ff:X}, found 0x{final_byte:X}"
        )
    if final_byte >= OFFICIAL_CODE_CEILING:
        raise ValidationError(
            f"{label}: non-erased data reaches protected tail at 0x{final_byte:X}"
        )

    erased_tail = data[OFFICIAL_CODE_CEILING:]
    if any(byte != 0xFF for byte in erased_tail):
        raise ValidationError(
            f"{label}: bytes in 0x{OFFICIAL_CODE_CEILING:X}.."
            f"0x{OFFICIAL_UPDATE_IMAGE_SIZE - 1:X} are not all erased"
        )

    return ImageReport(
        label=label,
        path=str(path),
        size=len(data),
        sha256=digest,
        reset_ljmp_target=reset_target,
        last_non_ff=final_byte,
        trailing_ff=len(data) - final_byte - 1,
        code_tail_is_erased=True,
    )


def validate_hardware_backups(
    full_flash_path: Path, information_page_path: Path
) -> BackupReport:
    full_flash = full_flash_path.read_bytes()
    information_page = information_page_path.read_bytes()

    if len(full_flash) != CC2541_F256_FLASH_SIZE:
        raise ValidationError(
            f"full flash backup: expected 0x{CC2541_F256_FLASH_SIZE:X} bytes, "
            f"found 0x{len(full_flash):X}"
        )
    if all(byte == 0x00 for byte in full_flash) or all(
        byte == 0xFF for byte in full_flash
    ):
        raise ValidationError("full flash backup is uniformly blank")

    if len(information_page) != CC2541_INFORMATION_PAGE_SIZE:
        raise ValidationError(
            "information page backup: expected "
            f"0x{CC2541_INFORMATION_PAGE_SIZE:X} bytes, "
            f"found 0x{len(information_page):X}"
        )
    ieee = information_page[
        CC2541_INFORMATION_PAGE_IEEE_OFFSET : CC2541_INFORMATION_PAGE_IEEE_OFFSET
        + 6
    ]
    if len(ieee) != 6 or ieee in (b"\x00" * 6, b"\xFF" * 6):
        raise ValidationError(
            "information page backup has an invalid factory IEEE address"
        )

    return BackupReport(
        full_flash_path=str(full_flash_path),
        full_flash_sha256=sha256(full_flash),
        information_page_path=str(information_page_path),
        information_page_sha256=sha256(information_page),
        factory_ieee_valid=True,
        factory_ieee_sha256=sha256(ieee),
    )


def parse_args(argv: Iterable[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Validate official C18 BLE 2.05/AP2D BLE 2.13 images and optional "
            "CC2541 debugger backups. This command never writes hardware."
        )
    )
    parser.add_argument("--c18-ble205", type=Path, default=DEFAULT_C18_BLE205)
    parser.add_argument("--ap2d-ble213", type=Path, default=DEFAULT_AP2D_BLE213)
    parser.add_argument("--full-flash-backup", type=Path)
    parser.add_argument("--information-page-backup", type=Path)
    parser.add_argument(
        "--require-hardware-backups",
        action="store_true",
        help="fail unless both debugger-created backup files are supplied",
    )
    parser.add_argument("--json", action="store_true")
    return parser.parse_args(argv)


def main(argv: Iterable[str] | None = None) -> int:
    args = parse_args(argv)

    try:
        images = [
            validate_update_image(
                args.c18_ble205,
                "c18_ble205",
                KNOWN_IMAGES["c18_ble205"]["sha256"],
                KNOWN_IMAGES["c18_ble205"]["last_non_ff"],
            ),
            validate_update_image(
                args.ap2d_ble213,
                "ap2d_ble213",
                KNOWN_IMAGES["ap2d_ble213"]["sha256"],
                KNOWN_IMAGES["ap2d_ble213"]["last_non_ff"],
            ),
        ]

        if bool(args.full_flash_backup) != bool(args.information_page_backup):
            raise ValidationError(
                "supply both --full-flash-backup and "
                "--information-page-backup, or neither"
            )

        backup = None
        if args.full_flash_backup and args.information_page_backup:
            backup = validate_hardware_backups(
                args.full_flash_backup, args.information_page_backup
            )
        elif args.require_hardware_backups:
            raise ValidationError(
                "hardware gate blocked: debugger-created full flash and "
                "information page backups are required"
            )
    except (OSError, ValidationError) as error:
        if args.json:
            print(json.dumps({"status": "error", "error": str(error)}, indent=2))
        else:
            print(f"ERROR: {error}", file=sys.stderr)
        return 2

    hardware_gate = "ready" if backup else "blocked"
    report = {
        "status": "ok",
        "static_image_gate": "pass",
        "hardware_backup_gate": hardware_gate,
        "images": [asdict(image) for image in images],
        "write_envelope": {
            "required_base": 0,
            "package_size": OFFICIAL_UPDATE_IMAGE_SIZE,
            "package_end_exclusive": OFFICIAL_UPDATE_IMAGE_SIZE,
            "code_ceiling": OFFICIAL_CODE_CEILING,
            "iap_chunk_size": BLE_IAP_CHUNK_SIZE,
        },
        "backup": asdict(backup) if backup else None,
        "limitations": [
            "No hardware was read, erased, or written.",
            "The semantics and erase span of the Anne Pro 2 BLE IAP command "
            "remain unverified.",
            "Do not use annepro2_tools for BLE cross-flashing until readback "
            "verification and erase bounds are implemented.",
        ],
    }

    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print("static image gate: PASS")
        for image in images:
            print(
                f"{image.label}: size=0x{image.size:X} "
                f"sha256={image.sha256} reset=0x{image.reset_ljmp_target:04X} "
                f"last_non_ff=0x{image.last_non_ff:X}"
            )
        print("required BLE image write envelope: [0x00000, 0x26000)")
        if backup:
            print("hardware backup gate: READY (factory IEEE is valid)")
        else:
            print(
                "hardware backup gate: BLOCKED "
                "(full flash and information page backups not supplied)"
            )
        print("hardware writes performed: none")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
