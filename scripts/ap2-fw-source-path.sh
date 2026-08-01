#!/usr/bin/env bash
# Resolve the pinned firmware archive without reinitializing it per worktree.
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
local_archive="$repo_root/assets/ap2_fw"
archive_revision=$(git -C "$repo_root" ls-files --stage assets/ap2_fw | awk '{print $2}')

archive_ready() {
    local archive=$1

    [[ -e "$archive/.git" ]] &&
        [[ $(git -C "$archive" rev-parse HEAD) == "$archive_revision" ]]
}

if archive_ready "$local_archive"; then
    printf '%s\n' "$local_archive"
    exit 0
fi

main_worktree=$(
    git -C "$repo_root" worktree list --porcelain |
        awk '$1 == "worktree" { path = $2 } $1 == "branch" && $2 == "refs/heads/main" { print path; exit }'
)
shared_archive="$main_worktree/assets/ap2_fw"

if [[ -z "$main_worktree" ]] || ! archive_ready "$shared_archive"; then
    echo "no initialized firmware archive at gitlink $archive_revision" >&2
    exit 1
fi

printf '%s\n' "$shared_archive"
