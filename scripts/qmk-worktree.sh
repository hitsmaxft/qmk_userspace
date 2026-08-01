#!/usr/bin/env bash
# Build against a disposable worktree of the pinned official QMK submodule.
set -euo pipefail

if [[ $# -eq 0 ]]; then
    echo "usage: $0 <qmk command and arguments>" >&2
    exit 2
fi

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
local_qmk_repo="$repo_root/modules/qmk_firmware"
qmk_repo=$(bash "$repo_root/scripts/qmk-source-path.sh")
patch_dir="$repo_root/patches/qmk"
cache_root="$repo_root/.cache/qmk-worktrees"

if [[ -n $(git -C "$local_qmk_repo" status --porcelain --untracked-files=normal) ]]; then
    echo "qmk_firmware must be clean; put local changes in patches/qmk first" >&2
    exit 1
fi
if [[ -n $(git -C "$qmk_repo" status --porcelain --untracked-files=normal) ]]; then
    echo "resolved QMK checkout must be clean; commit core changes or put temporary changes in patches/qmk first" >&2
    exit 1
fi

qmk_revision=$(git -C "$local_qmk_repo" rev-parse HEAD)
mkdir -p "$cache_root"
worktree=$(mktemp -d "$cache_root/qmk.XXXXXX")
rmdir "$worktree"
command_pid=

stop_command() {
    if [[ -z "$command_pid" ]] || ! kill -0 "$command_pid" 2>/dev/null; then
        command_pid=
        return
    fi

    kill -TERM "$command_pid" 2>/dev/null || true
    for _ in {1..20}; do
        if ! kill -0 "$command_pid" 2>/dev/null; then
            break
        fi
        sleep 0.1
    done
    if kill -0 "$command_pid" 2>/dev/null; then
        kill -KILL "$command_pid" 2>/dev/null || true
    fi
    wait "$command_pid" 2>/dev/null || true
    command_pid=
}

cleanup() {
    # A long-running command such as `qmk console` must not outlive this
    # wrapper and keep exclusive ownership of the HID console interface.
    trap - INT TERM HUP
    stop_command
    git -C "$qmk_repo" worktree remove --force "$worktree" 2>/dev/null || true
    git -C "$qmk_repo" worktree prune --expire now
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 129' HUP

# A full QMK checkout has more than 20,000 files. Parallel checkout materially
# reduces setup time, and --quiet keeps non-interactive logs focused on builds.
git -C "$qmk_repo" -c checkout.workers=0 worktree add --quiet --detach "$worktree" "$qmk_revision"

# The pinned source checkout owns initialized QMK submodules.  Reuse them as
# read-only links instead of recloning ChibiOS for every disposable worktree.
while IFS= read -r submodule_path; do
    # Nested submodules are already reachable through their top-level parent.
    if [[ "${submodule_path#*/}" == */* ]]; then
        continue
    fi
    source_path="$qmk_repo/$submodule_path"
    destination_path="$worktree/$submodule_path"
    if [[ ! -e "$source_path/.git" ]] || ! git -C "$source_path" rev-parse --verify HEAD >/dev/null 2>&1; then
        echo "missing initialized QMK submodule: $source_path" >&2
        exit 1
    fi
    expected_revision=$(git -C "$worktree" ls-tree HEAD "$submodule_path" | awk '{print $3}')
    actual_revision=$(git -C "$source_path" rev-parse HEAD)
    if [[ "$actual_revision" != "$expected_revision" ]]; then
        echo "QMK submodule revision mismatch: $submodule_path" >&2
        echo "expected $expected_revision, found $actual_revision" >&2
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
    exec env QMK_USERSPACE="$repo_root" "$@"
) &
command_pid=$!
set +e
wait "$command_pid"
exit_code=$?
set -e
command_pid=

if [[ -f "$worktree/compile_commands.json" ]]; then
    cp "$worktree/compile_commands.json" "$repo_root/compile_commands.json"
fi

exit "$exit_code"
