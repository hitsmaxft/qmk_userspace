#!/usr/bin/env bash
# Run the complete AnnePro2 build matrix inside one QMK worktree.
set -euo pipefail

jobs=${QMK_JOBS:-20}

qmk compile -kb annepro2/c18 -km macvim -j"$jobs"
qmk compile -kb annepro2/c18d -km macvim -j"$jobs"
qmk compile -kb annepro2/c15 -km default -j"$jobs"
qmk compile -kb annepro2/c2d -km default -j"$jobs"
