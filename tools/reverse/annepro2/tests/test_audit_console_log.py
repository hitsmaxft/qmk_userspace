import unittest
from pathlib import Path

import sys

SCRIPT_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPT_DIR))

import audit_console_log as auditor


def line(timestamp, message):
    return f"Obins:Anne Pro 2 C18 (QMK):1: AP2 BLE {timestamp:08X} {message}"


class ConsoleAuditTest(unittest.TestCase):
    def test_revision_match_accepts_git_describe_and_rejects_other_hash(self):
        self.assertTrue(
            auditor.revision_matches("latest-153-gc2130a", "c2130a")
        )
        self.assertFalse(
            auditor.revision_matches("latest-153-gc2130a", "deadbee")
        )

    def test_expected_revisions_must_appear_on_same_build_line(self):
        result = auditor.audit_lines(
            [
                line(0, "build qmk=e3dfb6829d userspace=old0001"),
                line(1, "build qmk=oldqmk0000 userspace=c2130a"),
            ],
            expect_qmk="e3dfb6829d",
            expect_userspace="c2130a",
        )

        self.assertFalse(result["evidence"]["build_attributed"])

    def test_complete_evidence_and_latency(self):
        lines = [
            line(
                0,
                "build qmk=e3dfb6829d* userspace=latest-153-gc2130a",
            ),
            line(1, "wake 0 profile=1"),
        ]
        for slot in range(4):
            lines.extend(
                [
                    line(100 + slot * 100, f"slot {slot} press"),
                    line(
                        110 + slot * 100,
                        f"tx broadcast slot={slot} attempt=1",
                    ),
                    line(
                        120 + slot * 100,
                        "rx command ack=01 value=00 state=1",
                    ),
                    line(150 + slot * 100, "rx hid handshake ready"),
                    line(151 + slot * 100, "route ble"),
                ]
            )
        lines.extend(
            [
                line(600, "tx keyboard report=1"),
                line(610, "tx consumer usage=00E9 bytes=8 profile=1"),
                line(620, "tx consumer usage=0000 bytes=8 profile=1"),
                line(630, "rx caps lock=1 leds=02"),
                line(640, "rx caps lock=0 leds=00"),
            ]
        )

        result = auditor.audit_lines(
            lines, expect_qmk="e3dfb6829d", expect_userspace="c2130a"
        )

        self.assertTrue(all(result["evidence"].values()))
        self.assertEqual(result["command_slots"], [0, 1, 2, 3])
        self.assertEqual(result["commands"][0]["ack_latency_ms"], 10)
        self.assertEqual(result["commands"][0]["route_latency_ms"], 41)

    def test_missing_build_and_host_evidence_are_not_overclaimed(self):
        lines = [
            line(0xFFFFFFF0, "tx connect slot=0 attempt=1"),
            line(0x00000005, "rx command ack=04 value=00 state=2"),
            line(0x00000010, "route ble"),
            line(0x00000011, "rx11 7B 12 35 00 03 00 00 7D 20 07 00"),
            line(0x00000012, "handshake timeout slot=0 recovery=0"),
            line(0x00000013, "rx invalid delimiter"),
        ]

        result = auditor.audit_lines(lines, expect_qmk="e3df")

        self.assertFalse(result["evidence"]["build_attributed"])
        self.assertFalse(result["evidence"]["keyboard_uart_path"])
        self.assertFalse(result["evidence"]["pairing_protocol_path"])
        self.assertEqual(result["commands"][0]["ack_latency_ms"], 21)
        self.assertEqual(result["commands"][0]["route_latency_ms"], 32)
        self.assertEqual(result["hid"]["caps_raw_values"], [0])
        self.assertEqual(result["faults"]["timeouts"]["handshake"], 1)
        self.assertEqual(result["faults"]["parser_errors"]["invalid_delimiter"], 1)

    def test_retries_remain_one_transaction(self):
        lines = [
            line(100, "tx broadcast slot=2 attempt=1"),
            line(200, "tx broadcast slot=2 attempt=2"),
            line(250, "rx command ack=01 value=00 state=1"),
            line(400, "route ble"),
        ]

        result = auditor.audit_lines(lines)

        self.assertEqual(len(result["commands"]), 1)
        self.assertEqual(result["commands"][0]["attempts"], 2)
        self.assertEqual(result["commands"][0]["ack_latency_ms"], 150)
        self.assertEqual(result["commands"][0]["route_latency_ms"], 300)


if __name__ == "__main__":
    unittest.main()
