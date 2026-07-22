# Repository guidance

This is an external QMK userspace. `modules/qmk_firmware` is pinned to the
temporary `codex/annepro2-uart-state-v2` branch in the user's QMK fork. It is
based on QMK `origin/master` and adds the evidence-based AnnePro2 BLE UART
handshake fix plus optional USB console instrumentation. Use the repository's
Nix/direnv environment for all QMK commands.

## Environment initialization

From the repository root:

```sh
git submodule update --init --recursive modules/qmk_firmware
direnv allow .
direnv exec . just annepro2
```

- `scripts/qmk-worktree.sh` creates a disposable worktree from the pinned QMK
  submodule, reuses its initialized nested QMK dependencies, overlays
  local custom keyboards into its standard `keyboards/` directory, and applies
  any optional `patches/qmk/*.patch` in lexical order.
- Do not reintroduce `EXTRA_KEYBOARD_FOLDER_PATH`; it was a private fork
  extension. Add custom keyboard source under this userspace instead.
- The AnnePro2 BLE fix and its optional USB instrumentation are part of the
  pinned fork branch, not userspace patches. Build the console variant with
  `direnv exec . just annepro2-log`. After hardware validation, keep only the
  upstreamable fix when preparing the QMK PR. Future temporary core patches
  belong under `patches/qmk/` and must pass `git apply --check` against the
  pinned revision.
- The worktree is temporary. Firmware files copied to the userspace root are
  generated artifacts and should not be committed.

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
