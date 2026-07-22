# Repository guidance

This is an external QMK userspace. `modules/qmk_firmware` is pinned to an
official QMK revision. Use the repository's Nix/direnv environment for all
QMK commands.

## Environment initialization

From the repository root:

```sh
git submodule update --init --recursive modules/qmk_firmware
direnv allow .
direnv exec . just annepro2
```

- `scripts/qmk-worktree.sh` creates a disposable worktree from the pinned
  official submodule, reuses its initialized nested QMK dependencies, overlays
  local custom keyboards into its standard `keyboards/` directory, and applies
  `patches/qmk/*.patch` in lexical order.
- Do not reintroduce `EXTRA_KEYBOARD_FOLDER_PATH`; it was a private fork
  extension. Add custom keyboard source under this userspace instead.
- Keep QMK core changes as focused patches under `patches/qmk/`. They must
  pass `git apply --check` against the pinned official revision.
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

Before handing off changes, run `git diff --check`, verify every QMK patch
with `git -C modules/qmk_firmware apply --check "$(pwd)/patches/qmk/<patch>"`,
and run the relevant QMK build. Generated firmware files are build artifacts
and should not be committed.
