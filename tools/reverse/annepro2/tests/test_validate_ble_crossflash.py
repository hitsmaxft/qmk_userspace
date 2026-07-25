import hashlib
import tempfile
import unittest
from pathlib import Path

import sys

SCRIPT_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPT_DIR))

import validate_ble_crossflash as validator


class BleCrossflashValidationTest(unittest.TestCase):
    def test_known_official_images(self):
        c18 = validator.validate_update_image(
            validator.DEFAULT_C18_BLE205,
            "c18_ble205",
            validator.KNOWN_IMAGES["c18_ble205"]["sha256"],
            validator.KNOWN_IMAGES["c18_ble205"]["last_non_ff"],
        )
        ap2d = validator.validate_update_image(
            validator.DEFAULT_AP2D_BLE213,
            "ap2d_ble213",
            validator.KNOWN_IMAGES["ap2d_ble213"]["sha256"],
            validator.KNOWN_IMAGES["ap2d_ble213"]["last_non_ff"],
        )
        self.assertEqual(c18.size, validator.OFFICIAL_UPDATE_IMAGE_SIZE)
        self.assertEqual(ap2d.size, validator.OFFICIAL_UPDATE_IMAGE_SIZE)
        self.assertTrue(c18.code_tail_is_erased)
        self.assertTrue(ap2d.code_tail_is_erased)

    def test_modified_official_image_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            modified = bytearray(validator.DEFAULT_AP2D_BLE213.read_bytes())
            modified[0x100] ^= 0x01
            path = Path(directory) / "modified.bin"
            path.write_bytes(modified)
            with self.assertRaisesRegex(validator.ValidationError, "SHA-256"):
                validator.validate_update_image(
                    path,
                    "modified",
                    validator.KNOWN_IMAGES["ap2d_ble213"]["sha256"],
                    validator.KNOWN_IMAGES["ap2d_ble213"]["last_non_ff"],
                )

    def test_non_erased_package_tail_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            image = bytearray([0xFF] * validator.OFFICIAL_UPDATE_IMAGE_SIZE)
            image[0:3] = b"\x02\x12\x34"
            image[validator.OFFICIAL_CODE_CEILING] = 0x00
            path = Path(directory) / "tail.bin"
            path.write_bytes(image)
            digest = hashlib.sha256(image).hexdigest()
            with self.assertRaisesRegex(
                validator.ValidationError, "protected tail"
            ):
                validator.validate_update_image(
                    path,
                    "tail",
                    digest,
                    validator.OFFICIAL_CODE_CEILING,
                )

    def test_debugger_backups_and_factory_ieee(self):
        with tempfile.TemporaryDirectory() as directory:
            directory_path = Path(directory)
            flash = bytearray([0xFF] * validator.CC2541_F256_FLASH_SIZE)
            flash[0:3] = b"\x02\x12\x34"
            information_page = bytearray(
                [0xFF] * validator.CC2541_INFORMATION_PAGE_SIZE
            )
            ieee_offset = validator.CC2541_INFORMATION_PAGE_IEEE_OFFSET
            information_page[ieee_offset : ieee_offset + 6] = bytes.fromhex(
                "010203040506"
            )

            flash_path = directory_path / "flash.bin"
            information_page_path = directory_path / "information-page.bin"
            flash_path.write_bytes(flash)
            information_page_path.write_bytes(information_page)

            report = validator.validate_hardware_backups(
                flash_path, information_page_path
            )
            self.assertTrue(report.factory_ieee_valid)
            self.assertEqual(
                report.factory_ieee_sha256,
                hashlib.sha256(bytes.fromhex("010203040506")).hexdigest(),
            )

    def test_blank_full_flash_backup_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            directory_path = Path(directory)
            flash_path = directory_path / "flash.bin"
            information_page_path = directory_path / "information-page.bin"
            flash_path.write_bytes(
                bytes([0xFF] * validator.CC2541_F256_FLASH_SIZE)
            )
            information_page = bytearray(
                [0xFF] * validator.CC2541_INFORMATION_PAGE_SIZE
            )
            ieee_offset = validator.CC2541_INFORMATION_PAGE_IEEE_OFFSET
            information_page[ieee_offset : ieee_offset + 6] = bytes.fromhex(
                "010203040506"
            )
            information_page_path.write_bytes(information_page)

            with self.assertRaisesRegex(validator.ValidationError, "blank"):
                validator.validate_hardware_backups(
                    flash_path, information_page_path
                )


if __name__ == "__main__":
    unittest.main()
