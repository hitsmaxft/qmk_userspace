#!/usr/bin/env bash
# Resolve a QMK checkout with initialized nested submodules for this userspace.
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
local_qmk="$repo_root/modules/qmk_firmware"
qmk_revision=$(git -C "$local_qmk" rev-parse HEAD)

qmk_submodules_ready() {
    local qmk_repo=$1
    local submodule_path

    while IFS= read -r submodule_path; do
        # Nested submodules are reachable through their initialized parent.
        if [[ "${submodule_path#*/}" == */* ]]; then
            continue
        fi
        if [[ ! -e "$qmk_repo/$submodule_path/.git" ]] || ! git -C "$qmk_repo/$submodule_path" rev-parse --verify HEAD >/dev/null 2>&1; then
            return 1
        fi
    done < <(git -C "$qmk_repo" config --file .gitmodules --get-regexp path | awk '{print $2}')
}

if qmk_submodules_ready "$local_qmk"; then
    printf '%s\n' "$local_qmk"
    exit 0
fi

# Linked userspace worktrees intentionally do not clone QMK's large nested
# submodules again. Reuse the checkout owned by the primary main worktree, but
# only when it is pinned to the exact same QMK revision.
main_worktree=$(
    git -C "$repo_root" worktree list --porcelain |
        awk '$1 == "worktree" { path = $2 } $1 == "branch" && $2 == "refs/heads/main" { print path; exit }'
)
shared_qmk="$main_worktree/modules/qmk_firmware"

if [[ -z "$main_worktree" || ! -d "$shared_qmk" ]]; then
    echo "no initialized QMK checkout found in the primary main worktree" >&2
    exit 1
fi
if [[ $(git -C "$shared_qmk" rev-parse HEAD) != "$qmk_revision" ]]; then
    echo "primary QMK checkout does not match userspace gitlink $qmk_revision" >&2
    exit 1
fi
if ! qmk_submodules_ready "$shared_qmk"; then
    echo "primary QMK checkout has uninitialized nested submodules: $shared_qmk" >&2
    exit 1
fi

printf '%s\n' "$shared_qmk"
