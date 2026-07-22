#!/usr/bin/env bash
# Build against a disposable worktree of the pinned official QMK submodule.
set -euo pipefail

if [[ $# -eq 0 ]]; then
    echo "usage: $0 <qmk command and arguments>" >&2
    exit 2
fi

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
qmk_repo="$repo_root/modules/qmk_firmware"
patch_dir="$repo_root/patches/qmk"
cache_root="$repo_root/.cache/qmk-worktrees"

if ! git -C "$qmk_repo" diff --quiet || ! git -C "$qmk_repo" diff --cached --quiet; then
    echo "qmk_firmware must be clean; put local changes in patches/qmk first" >&2
    exit 1
fi

qmk_revision=$(git -C "$qmk_repo" rev-parse HEAD)
mkdir -p "$cache_root"
worktree=$(mktemp -d "$cache_root/qmk.XXXXXX")
rmdir "$worktree"

cleanup() {
    git -C "$qmk_repo" worktree remove --force "$worktree" 2>/dev/null || true
    git -C "$qmk_repo" worktree prune --expire now
}
trap cleanup EXIT

git -C "$qmk_repo" worktree add --detach "$worktree" "$qmk_revision" >/dev/null

# The pinned source checkout owns initialized QMK submodules.  Reuse them as
# read-only links instead of recloning ChibiOS for every disposable worktree.
while IFS= read -r submodule_path; do
    # Nested submodules are already reachable through their top-level parent.
    if [[ "${submodule_path#*/}" == */* ]]; then
        continue
    fi
    source_path="$qmk_repo/$submodule_path"
    destination_path="$worktree/$submodule_path"
    if [[ ! -d "$source_path" ]]; then
        echo "missing initialized QMK submodule: $source_path" >&2
        exit 1
    fi
    rmdir "$destination_path"
    ln -s "$source_path" "$destination_path"
done < <(git -C "$qmk_repo" config --file .gitmodules --get-regexp path | awk '{print $2}')

link_keyboard() {
    local source=$1
    local destination=$2

    if [[ ! -d "$source" ]]; then
        return
    fi
    if [[ -e "$destination" ]]; then
        echo "keyboard overlay destination already exists: $destination" >&2
        exit 1
    fi
    ln -s "$source" "$destination"
}

# QMK sees standard in-tree keyboard paths; no QMK path-search patch is needed.
link_keyboard "$repo_root/keyboards/bhekb" "$worktree/keyboards/bhekb"
link_keyboard "$repo_root/keyboards/lily58_2040" "$worktree/keyboards/lily58_2040"

shopt -s nullglob
for patch in "$patch_dir"/*.patch; do
    git -C "$worktree" apply --check "$patch"
    git -C "$worktree" apply "$patch"
done

exit_code=0
(
    cd "$worktree"
    QMK_USERSPACE="$repo_root" "$@"
) || exit_code=$?

if [[ -f "$worktree/compile_commands.json" ]]; then
    cp "$worktree/compile_commands.json" "$repo_root/compile_commands.json"
fi

exit "$exit_code"
