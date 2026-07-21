# Repository guidance

This is an external QMK userspace. The pinned `modules/qmk_firmware` submodule
is a custom QMK fork, so use the repository's Nix/direnv environment for all
QMK commands.

## Environment initialization

From the repository root:

```sh
git submodule update --init modules/qmk_firmware
direnv allow .
direnv exec . qmk git-submodule --sync
direnv exec . qmk git-submodule --check
direnv exec . qmk doctor
direnv exec . qmk userspace-doctor
```

- Keep both `QMK_HOME` and `EXTRA_KEYBOARD_FOLDER_PATH` from `.envrc`.
  The pinned fork otherwise resolves keyboard paths as `""keyboards/...`,
  skips keyboard-level `rules.mk`, and produces a missing `startup_.mk` error.
- `qmk git-submodule --sync` shallow-clones missing QMK dependencies such as
  ChibiOS and ChibiOS-Contrib. Do not assume the top-level submodule checkout
  includes these nested modules.
- `qmk doctor` may warn that QMK home has no `.git` folder because a Git
  submodule uses a `.git` pointer file. This warning is non-fatal when the
  submodule checks pass.
- Do not update the pinned QMK fork revision unless the task explicitly
  requires it.

## Build and validation

Build AnnePro2 C18 with:

```sh
direnv exec . just annepro2
```

Before handing off changes, run `git diff --check` and the relevant QMK build.
Generated firmware files are build artifacts and should not be committed.
