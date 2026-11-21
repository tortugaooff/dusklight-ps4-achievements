#!/usr/bin/env bash
# Run from the isolated Linux build root prepared according to r25 BUILDING.md.
set -euo pipefail

root="$(cd "${1:?Informe a raiz isolada do build Linux do r25}" && pwd)"
if [[ "$(uname -s)" != Linux || "$(uname -m)" != x86_64 ]]; then
    echo "O build integral do r25 requer Linux x86_64." >&2
    exit 1
fi

source_tree="$root/src/dusklight"
if [[ ! -f "$source_tree/src/dusk/AchievementCore.hpp" ]] ||
   ! grep -Fq -- '--user-dir=/data/dusklight-achievements-poc/user' "$source_tree/ps4/main.cpp" ||
   ! grep -Fq '/data/dusklight-achievements-poc/cache' "$source_tree/src/m_Do/m_Do_main.cpp"; then
    echo "A variante com os caminhos de dados isolados precisa estar em src/dusklight." >&2
    exit 1
fi

# The r25 dependency recipes and all build outputs stay under this root.
cd "$root"
export FLAVOR=dev
unset PGO
tools/build-mesa.sh
tools/build-dawn.sh
tools/build-sdl3.sh
tools/build-game.sh
find "$root/build/game" -type f -name eboot.bin -print
