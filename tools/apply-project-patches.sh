#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
for entry in snesrecomp:snesrecomp-v0.2.patch recomp-ui:recomp-ui-v0.2.patch recomp-ui:sciv-frontend.patch; do
  target="${entry%%:*}"
  patch="$ROOT/tools/${entry#*:}"
  if git -C "$ROOT/$target" apply --reverse --check "$patch" 2>/dev/null; then
    continue
  fi
  git -C "$ROOT/$target" apply --check "$patch"
  git -C "$ROOT/$target" apply "$patch"
done
