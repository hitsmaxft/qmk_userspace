#!/usr/bin/env python3
"""Summarize evidence in an Anne Pro 2 QMK BLE USB console log."""

from __future__ import annotations

import argparse
import json
import re
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Any

LOG_LINE = re.compile(r"\bAP2 BLE ([0-9A-Fa-f]{8}) (.*)$")
BUILD = re.compile(r"build qmk=(\S+)(?: userspace=(\S+))?")
WAKE = re.compile(r"wake (-?\d+) profile=(\d+)")
SLOT_EVENT = re.compile(r"slot ([0-3]) (press|release|tap|hold)")
TX_COMMAND = re.compile(r"tx (broadcast|connect) slot=([0-3]) attempt=(\d+)")
COMMAND_ACK = re.compile(
    r"rx command ack=(01|04) value=([0-9A-Fa-f]{2})(?: state=\d+)?"
)
KEYBOARD_REPORT = re.compile(r"tx keyboard report=(\d+)")
CONSUMER_REPORT = re.compile(
    r"tx consumer usage=([0-9A-Fa-f]{4}) bytes=(\d+) profile=(\d+)"
)
CAPS_LOCK = re.compile(r"rx caps lock=([01]) leds=([0-9A-Fa-f]{2})")
CAPS_RAW = re.compile(
    r"rx11 7B 12 35 00 03 00 00 7D 20 07 ([0-9A-Fa-f]{2})",
    re.IGNORECASE,
)


def elapsed_ms(start: int, end: int) -> int:
    """Return unsigned 32-bit timer elapsed time."""

    return (end - start) & 0xFFFFFFFF


def revision_matches(actual: str | None, expected: str) -> bool:
    """Accept plain hashes and git-describe strings containing the hash."""

    return actual is not None and expected in actual


@dataclass
class Command:
    kind: str
    slot: int
    started_at_ms: int
    attempts: int = 1
    ack_at_ms: int | None = None
    route_at_ms: int | None = None

    def to_dict(self) -> dict[str, Any]:
        value = asdict(self)
        value["ack_latency_ms"] = (
            None
            if self.ack_at_ms is None
            else elapsed_ms(self.started_at_ms, self.ack_at_ms)
        )
        value["route_latency_ms"] = (
            None
            if self.route_at_ms is None
            else elapsed_ms(self.started_at_ms, self.route_at_ms)
        )
        return value


@dataclass
class Audit:
    lines: int = 0
    ap2_lines: int = 0
    builds: list[dict[str, str | None]] = field(default_factory=list)
    wakes: list[dict[str, int]] = field(default_factory=list)
    slot_events: dict[int, set[str]] = field(
        default_factory=lambda: {slot: set() for slot in range(4)}
    )
    commands: list[Command] = field(default_factory=list)
    command_acks: dict[str, int] = field(
        default_factory=lambda: {"broadcast": 0, "connect": 0}
    )
    handshake_ready: int = 0
    route_ble: int = 0
    route_pending: int = 0
    keyboard_reports: int = 0
    consumer_usages: list[int] = field(default_factory=list)
    caps_lock_values: list[int] = field(default_factory=list)
    caps_raw_values: list[int] = field(default_factory=list)
    timeouts: dict[str, int] = field(
        default_factory=lambda: {"handshake": 0, "parser": 0}
    )
    parser_errors: dict[str, int] = field(
        default_factory=lambda: {"invalid_length": 0, "invalid_delimiter": 0}
    )
    stale_acks: int = 0
    active_command: Command | None = field(default=None, repr=False)

    def feed(self, line: str) -> None:
        self.lines += 1
        match = LOG_LINE.search(line)
        if match is None:
            return
        self.ap2_lines += 1
        timestamp = int(match.group(1), 16)
        message = match.group(2)

        if found := BUILD.fullmatch(message):
            self.builds.append(
                {"qmk": found.group(1), "userspace": found.group(2)}
            )
        elif found := WAKE.fullmatch(message):
            self.wakes.append(
                {"slot": int(found.group(1)), "profile": int(found.group(2))}
            )
        elif found := SLOT_EVENT.fullmatch(message):
            self.slot_events[int(found.group(1))].add(found.group(2))
        elif found := TX_COMMAND.fullmatch(message):
            kind = found.group(1)
            slot = int(found.group(2))
            attempt = int(found.group(3))
            if (
                attempt > 1
                and self.active_command is not None
                and self.active_command.kind == kind
                and self.active_command.slot == slot
            ):
                self.active_command.attempts = max(
                    self.active_command.attempts, attempt
                )
            else:
                command = Command(kind, slot, timestamp, attempts=attempt)
                self.commands.append(command)
                self.active_command = command
        elif found := COMMAND_ACK.fullmatch(message):
            kind = "broadcast" if found.group(1) == "01" else "connect"
            self.command_acks[kind] += 1
            if (
                self.active_command is not None
                and self.active_command.kind == kind
                and self.active_command.ack_at_ms is None
            ):
                self.active_command.ack_at_ms = timestamp
        elif message == "rx hid handshake ready":
            self.handshake_ready += 1
        elif message == "route ble":
            self.route_ble += 1
            if (
                self.active_command is not None
                and self.active_command.route_at_ms is None
            ):
                self.active_command.route_at_ms = timestamp
                self.active_command = None
        elif message == "route pending":
            self.route_pending += 1
        elif KEYBOARD_REPORT.fullmatch(message):
            self.keyboard_reports += 1
        elif found := CONSUMER_REPORT.fullmatch(message):
            self.consumer_usages.append(int(found.group(1), 16))
        elif found := CAPS_LOCK.fullmatch(message):
            self.caps_lock_values.append(int(found.group(1)))
        elif found := CAPS_RAW.fullmatch(message):
            self.caps_raw_values.append(int(found.group(1), 16))
        elif message.startswith("handshake timeout"):
            self.timeouts["handshake"] += 1
            self.active_command = None
        elif message == "rx partial timeout":
            self.timeouts["parser"] += 1
        elif message == "rx invalid length":
            self.parser_errors["invalid_length"] += 1
        elif message == "rx invalid delimiter":
            self.parser_errors["invalid_delimiter"] += 1
        elif message.startswith("ignore stale command ack="):
            self.stale_acks += 1

    def result(
        self, expect_qmk: str | None = None, expect_userspace: str | None = None
    ) -> dict[str, Any]:
        slots = {
            str(slot): sorted(events)
            for slot, events in self.slot_events.items()
            if events
        }
        command_slots = sorted({command.slot for command in self.commands})
        nonzero_consumer = sorted(
            {usage for usage in self.consumer_usages if usage != 0}
        )
        build_matches = any(
            (expect_qmk is None or revision_matches(build["qmk"], expect_qmk))
            and (
                expect_userspace is None
                or revision_matches(build["userspace"], expect_userspace)
            )
            for build in self.builds
        )

        evidence = {
            "build_attributed": build_matches,
            "keyboard_uart_path": self.route_ble > 0
            and self.keyboard_reports > 0,
            "consumer_press_and_release": bool(nonzero_consumer)
            and 0 in self.consumer_usages,
            "caps_lock_on_and_off": {0, 1}.issubset(self.caps_lock_values),
            "all_four_slots_exercised": command_slots == [0, 1, 2, 3],
            "pairing_protocol_path": any(
                command.kind == "broadcast" for command in self.commands
            )
            and self.command_acks["broadcast"] > 0
            and self.route_ble > 0,
        }
        return {
            "source": {
                "lines": self.lines,
                "ap2_ble_lines": self.ap2_lines,
                "builds": self.builds,
                "expected_qmk": expect_qmk,
                "expected_userspace": expect_userspace,
            },
            "startup": self.wakes,
            "slot_events": slots,
            "command_slots": command_slots,
            "commands": [command.to_dict() for command in self.commands],
            "command_acks": self.command_acks,
            "hid": {
                "handshake_ready": self.handshake_ready,
                "route_ble": self.route_ble,
                "route_pending": self.route_pending,
                "keyboard_reports": self.keyboard_reports,
                "consumer_usages": [
                    f"0x{usage:04X}" for usage in self.consumer_usages
                ],
                "caps_lock_values": self.caps_lock_values,
                "caps_raw_values": self.caps_raw_values,
            },
            "faults": {
                "timeouts": self.timeouts,
                "parser_errors": self.parser_errors,
                "stale_acks": self.stale_acks,
            },
            "evidence": evidence,
            "scope_note": (
                "Console evidence covers the KEY-to-BLE UART/state-machine path. "
                "It does not by itself prove macOS pairing, RF delivery, or that "
                "observed host input came over BLE rather than USB."
            ),
        }


def audit_lines(
    lines: list[str],
    expect_qmk: str | None = None,
    expect_userspace: str | None = None,
) -> dict[str, Any]:
    audit = Audit()
    for line in lines:
        audit.feed(line.rstrip("\n"))
    return audit.result(expect_qmk, expect_userspace)


def status(value: bool) -> str:
    return "OBSERVED" if value else "MISSING"


def render_text(result: dict[str, Any]) -> str:
    source = result["source"]
    evidence = result["evidence"]
    faults = result["faults"]
    hid = result["hid"]
    commands = result["commands"]
    completed = [
        command for command in commands if command["route_latency_ms"] is not None
    ]

    lines = [
        "Anne Pro 2 BLE console audit",
        f"  log lines: {source['lines']} ({source['ap2_ble_lines']} AP2 BLE)",
    ]
    if source["builds"]:
        for build in source["builds"]:
            userspace = (
                f" userspace={build['userspace']}"
                if build["userspace"] is not None
                else ""
            )
            lines.append(f"  build: qmk={build['qmk']}{userspace}")
    else:
        lines.append("  build: UNKNOWN (no revision line in log)")

    lines.extend(
        [
            f"  slots commanded: {result['command_slots'] or 'none'}",
            (
                "  commands: "
                f"{sum(command['kind'] == 'broadcast' for command in commands)} broadcast, "
                f"{sum(command['kind'] == 'connect' for command in commands)} connect"
            ),
            (
                "  HID: "
                f"{hid['handshake_ready']} ready, {hid['route_ble']} BLE routes, "
                f"{hid['keyboard_reports']} keyboard reports, "
                f"{len(hid['consumer_usages'])} consumer reports"
            ),
            (
                "  Caps Lock values: "
                f"{hid['caps_lock_values'] or 'none'} "
                f"(raw 20/07: {hid['caps_raw_values'] or 'none'})"
            ),
        ]
    )
    if completed:
        lines.append(
            "  command-to-route latency ms: "
            + ", ".join(str(command["route_latency_ms"]) for command in completed)
        )
    lines.extend(
        [
            (
                "  faults: "
                f"{faults['timeouts']['handshake']} handshake timeout, "
                f"{faults['timeouts']['parser']} parser timeout, "
                f"{sum(faults['parser_errors'].values())} parser errors, "
                f"{faults['stale_acks']} stale ACKs"
            ),
            "  evidence:",
        ]
    )
    for key, value in evidence.items():
        lines.append(f"    {status(value):8} {key}")
    lines.extend(["", f"Scope: {result['scope_note']}"])
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--expect-qmk", help="required QMK revision prefix")
    parser.add_argument(
        "--expect-userspace", help="required userspace revision prefix"
    )
    parser.add_argument("--json", action="store_true", help="emit JSON")
    args = parser.parse_args()

    result = audit_lines(
        args.log.read_text(encoding="utf-8", errors="replace").splitlines(),
        args.expect_qmk,
        args.expect_userspace,
    )
    if args.json:
        print(json.dumps(result, indent=2, sort_keys=True))
    else:
        print(render_text(result))
    revision_required = (
        args.expect_qmk is not None or args.expect_userspace is not None
    )
    return 1 if revision_required and not result["evidence"]["build_attributed"] else 0


if __name__ == "__main__":
    raise SystemExit(main())
