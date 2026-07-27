# Repository guidance

This is an external QMK userspace. `modules/qmk_firmware` is pinned to the
temporary `codex/annepro2-ble213-backport` branch in the user's QMK fork. It is
based on official QMK `upstream/master` and adds the evidence-based AnnePro2 BLE UART
handshake fix, C18 BLE 2.05/AP2D BLE 2.13 profiles, bounded UART parser, host
tests, C2D GPIO/BLE support, and optional USB console instrumentation. C2D
direct-drive RGB is intentionally deferred. Use the repository's Nix/direnv
environment for all QMK commands.

## Environment initialization

From the repository root:

```sh
git submodule update --init --recursive
direnv allow .
direnv exec . just annepro2
```

- `assets/ap2_fw` is the pinned
  `https://github.com/hitsmaxft/annepro-2-firmware.git` archive. Official,
  historical, and derived images use the archive's own directory structure;
  do not copy firmware binaries back into userspace. Locally generated
  firmware remains an ignored build artifact in the userspace root.
- `scripts/qmk-worktree.sh` creates a disposable worktree from the pinned QMK
  submodule, reuses its initialized nested QMK dependencies, overlays
  local custom keyboards into its standard `keyboards/` directory, and applies
  any optional `patches/qmk/*.patch` in lexical order. It owns the spawned QMK
  process: `INT`, `TERM`, and `HUP` terminate and reap the child before the
  temporary worktree is removed. After interactive `qmk console` sessions,
  still verify that no older `.qmk-wrapped console` listener owns the HID
  interface.
- Do not reintroduce `EXTRA_KEYBOARD_FOLDER_PATH`; it was a private fork
  extension. Add custom keyboard source under this userspace instead.
- The AnnePro2 BLE fix and its optional USB instrumentation are part of the
  pinned fork branch, not userspace patches. Build the console variant with
  `direnv exec . just annepro2-log`. Build the validated AP2D BLE 2.13
  profile with `direnv exec . just annepro2-ble213` or
  `direnv exec . just annepro2-ble213-log`. Run all hardware-independent
  profile/state/parser and LED-source gates with
  `direnv exec . just annepro2-test`; use
  `direnv exec . just annepro2-validate` to include both C18 profile builds,
  the C15 non-regression build, and the GPIO/BLE-only C2D build. Build C2D
  alone with `direnv exec . just annepro2-c2d`. This is not a substitute for
  AP2D hardware validation. After hardware validation, keep only the
  upstreamable fix when preparing the QMK PR. Future temporary core patches
  belong under `patches/qmk/` and must pass `git apply --check` against the
  pinned revision.
- The worktree is temporary. Firmware files copied to the userspace root are
  generated artifacts and should not be committed.
- The fork's `origin/master` can lag official QMK. Review the upstreamable
  branch with `git -C modules/qmk_firmware log upstream/master..HEAD` and
  `git -C modules/qmk_firmware diff upstream/master...HEAD`; do not count
  upstream commits as AnnePro2 changes.

## Build and validation

Build AnnePro2 C18 with:

```sh
direnv exec . just annepro2
```

Build another target through the same wrapper, for example:

```sh
direnv exec . just qmk compile -kb bhekb/dk6064 -km 6064 -j20
```

Before handing off changes, run `git diff --check` and the relevant QMK build.
When `patches/qmk/` contains a patch, also verify it with
`git -C modules/qmk_firmware apply --check "$(pwd)/patches/qmk/<patch>"`.
Generated firmware files are build artifacts and should not be committed.

## Anne Pro 2 IAP validation

Build the intended KEY image before entering IAP, then record its size and
SHA-256. For the BLE 2.13 debug profile:

```sh
direnv exec . just annepro2-ble213-log
shasum -a 256 annepro2_c18_macvim_ble213_log.bin
```

Each C18 recipe also preserves a profile-specific copy:
`annepro2_c18_macvim_ble205.bin`, `annepro2_c18_macvim_ble205_log.bin`,
`annepro2_c18_macvim_ble213.bin`, or
`annepro2_c18_macvim_ble213_log.bin`. Use that stable name for IAP; the generic
`annepro2_c18_macvim.bin` is overwritten by every subsequent C18 build.

After the keyboard is in IAP, always probe before writing:

```sh
direnv exec . just annepro2-iap-probe
```

The official and optional 2C BLE recipes validate or reproducibly generate the
exact image, flash only the BLE target, and intentionally leave the keyboard in
IAP:

```sh
direnv exec . just annepro2-flash-ble213-official
# or, for the separately managed compatibility-name variant:
direnv exec . just annepro2-flash-ble213-2c
```

Restore the exact official C18 BLE 2.05 image through the same guarded path:

```sh
direnv exec . just annepro2-flash-ble205-official
```

Finish the same IAP session by flashing the already-built KEY artifact; this is
the step that restarts the keyboard:

```sh
direnv exec . just annepro2-flash-key annepro2_c18_macvim_ble213_log.bin
```

For a BLE 2.05 regression, use
`annepro2_c18_macvim_ble205_log.bin` instead. A valid profile saved in EEPROM
overrides the build default; after booting on USB, explicitly select
`KC_AP2_BLE205` before starting the Bluetooth test.

Do not add `--base` during normal flashing. The hardened tool reads the target
base from the device, rejects a non-IAP target, matches every response to its
request, and stops at the first timeout or non-zero status. A successful
transfer is not flash readback.
