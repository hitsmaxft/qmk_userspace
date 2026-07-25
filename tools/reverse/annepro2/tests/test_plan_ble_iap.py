import tempfile
import unittest
from pathlib import Path

import sys

SCRIPT_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPT_DIR))

import plan_ble_iap as planner
import validate_ble_crossflash as validator


class BleIapPlanTest(unittest.TestCase):
    def test_ble_route_and_erase_vector(self):
        payload = planner.build_erase_payload(0x4000)
        report = planner.build_hid_report(payload)

        self.assertEqual(planner.AP2_BLE_ROUTE, 0x51)
        self.assertEqual(payload, bytes.fromhex("024300400000"))
        self.assertEqual(len(report), 65)
        self.assertEqual(
            report[:15],
            bytes.fromhex("007b1051100600007d024300400000"),
        )

    def test_write_vector(self):
        chunk = bytes(range(32))
        payload = planner.build_write_payload(0x29FE0, chunk)
        report = planner.build_hid_report(payload)

        self.assertEqual(
            payload[:6],
            bytes.fromhex("0231e09f0200"),
        )
        self.assertEqual(payload[6:], chunk)
        self.assertEqual(
            report[:15],
            bytes.fromhex("007b1051102600007d0231e09f0200"),
        )

    def test_known_ap2d_plan_boundaries(self):
        plan = planner.build_plan(validator.DEFAULT_AP2D_BLE213)

        self.assertFalse(plan["executable"])
        self.assertFalse(plan["hardware_access"])
        self.assertIsNone(plan["transport"])
        self.assertEqual(plan["routing"]["route_byte"], 0x51)
        self.assertEqual(
            plan["erase"]["reply_policy"],
            "matching-target-command-key-and-status-zero",
        )
        self.assertEqual(plan["writes"]["chunk_size"], 32)
        self.assertEqual(plan["writes"]["count"], 4864)
        self.assertEqual(plan["writes"]["first_address"], 0x4000)
        self.assertEqual(plan["writes"]["last_address"], 0x29FE0)
        self.assertEqual(plan["writes"]["end_exclusive"], 0x2A000)
        self.assertEqual(
            plan["image"]["sha256"],
            validator.KNOWN_IMAGES["ap2d_ble213"]["sha256"],
        )
        self.assertGreaterEqual(len(plan["observed_transfer"]), 3)

    def test_non_reported_base_is_rejected(self):
        with self.assertRaisesRegex(planner.PlanError, "transport base"):
            planner.build_plan(validator.DEFAULT_AP2D_BLE213, 0)

    def test_unaligned_write_is_rejected(self):
        with self.assertRaisesRegex(planner.PlanError, "aligned"):
            planner.build_write_payload(0x4001, bytes(32))

    def test_short_write_is_rejected(self):
        with self.assertRaisesRegex(planner.PlanError, "exactly 32"):
            planner.build_write_payload(0x4000, bytes(31))

    def test_unknown_image_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "unknown.bin"
            path.write_bytes(bytes([0xFF]) * validator.OFFICIAL_UPDATE_IMAGE_SIZE)
            with self.assertRaisesRegex(planner.PlanError, "unknown BLE image"):
                planner.build_plan(path)


if __name__ == "__main__":
    unittest.main()
