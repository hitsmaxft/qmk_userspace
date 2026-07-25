import unittest
from pathlib import Path

import sys

SCRIPT_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPT_DIR))

import patch_ble213_name as patcher


class Ble213NamePatchTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = patcher.SOURCE.read_bytes()
        cls.variant = patcher.build_variant(cls.source)

    def test_known_hash_and_size(self):
        self.assertEqual(len(self.variant), patcher.IMAGE_SIZE)
        self.assertEqual(patcher.sha256(self.variant), patcher.OUTPUT_SHA256)

    def test_names_are_fixed_width(self):
        self.assertEqual(
            self.variant[
                patcher.ADVERTISEMENT_NAME_OFFSET :
                patcher.ADVERTISEMENT_NAME_OFFSET + 18
            ],
            patcher.COMPATIBILITY_NAME,
        )
        self.assertEqual(
            self.variant[patcher.GAP_NAME_OFFSET : patcher.GAP_NAME_OFFSET + 18],
            patcher.COMPATIBILITY_NAME,
        )
        self.assertEqual(
            self.variant[patcher.ADVERTISEMENT_LENGTH_OFFSET],
            0x13,
        )
        self.assertEqual(self.variant[patcher.ADVERTISEMENT_TYPE_OFFSET], 0x09)
        self.assertEqual(self.variant[patcher.GAP_NAME_OFFSET + 18], 0)

    def test_only_declared_name_bytes_change(self):
        changed = {
            index
            for index, (old, new) in enumerate(zip(self.source, self.variant))
            if old != new
        }
        self.assertEqual(changed, patcher.expected_changed_offsets())
        self.assertEqual(len(changed), 4)

    def test_rejects_unknown_input(self):
        corrupt = bytearray(self.source)
        corrupt[0] ^= 1
        with self.assertRaisesRegex(patcher.PatchError, "SHA-256 mismatch"):
            patcher.build_variant(bytes(corrupt))


if __name__ == "__main__":
    unittest.main()
