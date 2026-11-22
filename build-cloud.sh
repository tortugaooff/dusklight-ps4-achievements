#!/usr/bin/env bash
set -euo pipefail
bundle="$(cd "$(dirname "$0")" && pwd)"
root="$(realpath -m "${1:?Informe uma raiz vazia de build}")"
[[ "$(uname -sm)" == "Linux x86_64" ]]
mkdir -p "$root"
[[ -z "$(find "$root" -mindepth 1 -maxdepth 1 -print -quit)" ]]
cd "$root"
mkdir -p refs src downloads/symgen toolchain

checkout() {
    git init -q "$3"
    git -C "$3" remote add origin "$1"
    git -C "$3" fetch --depth=1 origin "$2"
    git -C "$3" checkout -q --detach FETCH_HEAD
}

checkout https://github.com/BgbrhrWatchinOVO/dusklight-ps4.git fd483cf5d4f04b0f4d887ff412d0e8691925de08 refs/r25
checkout https://github.com/iHaiDeeZ/DolphinPS4.git b1879cc0267a2a4f0949d7b80478fcf2abca24f6 refs/DolphinPS4
git -c advice.detachedHead=false clone --branch ps5-v0.1.0 https://github.com/mshivam019/dusklight-ps5.git refs/dusklight-ps5
cp -a refs/r25/tools refs/r25/patches .
chmod +x tools/*.sh
git -C refs/DolphinPS4 apply "$root/patches/dolphinps4-ps4-runtime.patch"
cp tools/ac_ps4_drm.c refs/DolphinPS4/port/mesa/ac_ps4_drm.c
tools/fetch-openorbis.sh
tools/fetch-sources.sh
test "$(git -C src/dusklight rev-parse HEAD)" = 5811958531cb0a9eafeb293fafcb36071883bfdf
git -C src/dusklight apply --check "$bundle/dusklight-r25-conquistas-cloud.patch"
git -C src/dusklight apply "$bundle/dusklight-r25-conquistas-cloud.patch"
cp -a "$bundle/visual-source/." src/dusklight/
git -C src/dusklight fetch --no-tags https://github.com/TwilitRealm/dusklight.git refs/tags/v2.0.3:refs/tags/v2.0.3
test "$(git -C src/dusklight rev-parse 'v2.0.3^{commit}')" = 40457c6adb381928e4b5fef6ed459ed291edd5e2
git -C src/dusklight merge-base --is-ancestor v2.0.3 HEAD
test "$(git -C src/dusklight rev-list --count v2.0.3..HEAD)" = 2
test "$(git -C src/dusklight describe --tags --long --match 'v*')" = v2.0.3-2-g581195853
g++ -std=c++20 -Wall -Wextra -Werror -iquote src/dusklight/src/dusk "$bundle/catalog-tests.cpp" -o "$root/catalog-tests"
mkdir "$root/catalog-test-data"
"$root/catalog-tests" "$root/catalog-test-data"
echo 'Carapaca descriptor, legacy migration and catalog tests passed'
for name in aurora borealis; do
    git -C "src/dusklight/extern/$name" apply "$root/patches/$name-ps4.patch"
done
for name in dawn sdl3 abseil; do
    git -C "src/$name" apply "$root/patches/$name-ps4.patch"
done
checkout https://gitlab.freedesktop.org/mesa/mesa.git refs/tags/mesa-26.0.8 src/mesa
git -C src/mesa apply --check "$root/patches/mesa-26.0.8-ps4.patch"
git -C src/mesa apply "$root/patches/mesa-26.0.8-ps4.patch"
checkout https://github.com/OTPR26/twilight-hd-hud.git 9fd501de8ce05381bd188d88a7bda600586bbc23 src/twilight-hd-hud
git -C src/twilight-hd-hud apply "$root/patches/twilight-hd-ps4.patch"

llvm=llvm-project-21.1.6.src
tar -xJf "downloads/$llvm.tar.xz" -C src "$llvm/"{runtimes,libcxx,libcxxabi,libunwind,libc,cmake,llvm/cmake,llvm/utils/lit,llvm/utils/llvm-lit,third-party}
git -C "src/$llvm/libcxx/src" apply "$root/patches/libcxx-ps4-atomic-wait.patch"
curl --fail --silent --show-error --location --retry 3 \
    https://github.com/encounter/symgen/releases/download/v1.3.6/symgen-linux-x86_64 \
    -o downloads/symgen/symgen-linux-x86_64
echo '236bc5cd4bda03a8dd53a4531efff1fec3bcd19b09890893b9b6f8f5ae77f041  downloads/symgen/symgen-linux-x86_64' | sha256sum -c
chmod +x downloads/symgen/symgen-linux-x86_64
source tools/env.sh
bash refs/DolphinPS4/toolchain/build-libcxx.sh
bash refs/DolphinPS4/toolchain/build-runtime.sh
# Dawn and SDL3 expect a local CMake toolchain path missing from the release tree.
printf 'include("%s/refs/DolphinPS4/toolchain/ps4-love-modern.cmake")\n' "$root" > toolchain/ps4.cmake
# Both Dawn configurations must use the r25-patched Abseil checkout.
for script in tools/build-dawn.sh tools/build-game.sh; do
    sed -i '/-DCMAKE_BUILD_TYPE=Release/i\        "-DDAWN_ABSEIL_DIR=$PS4_PORT_ROOT/src/abseil" \\' "$script"
done
# PacBrew identifies the target as Generic. SDL needs its Unix source-selection
# branch for pthreads and timers, while the PS4 linker and sysroot stay in use.
printf 'include("%s/toolchain/ps4.cmake")\nset(CMAKE_SYSTEM_NAME FreeBSD)\n' "$root" > toolchain/sdl3-ps4.cmake
sed -i 's@/toolchain/ps4.cmake@/toolchain/sdl3-ps4.cmake@' tools/build-sdl3.sh
sed -i '/-DCMAKE_BUILD_TYPE=Release/i\        -DSDL_PTHREADS=ON \\' tools/build-sdl3.sh
bash "$bundle/build-ps4.sh" "$root"

eboot=build/game/dusklight_eboot/eboot.bin
elf=build/game/dusklight
test -s "$eboot"
test -s "$elf"
LC_ALL=C grep -aFq 2.0.3.2 "$elf"
mkdir -p cloud-output
llvm-readelf-21 -h -lW "$elf" > cloud-output/elf-layout.txt
first_load="$(awk '$1 == "LOAD" && !seen { print $3, $7, $8; seen = 1 }' cloud-output/elf-layout.txt)"
[[ "$first_load" == "0x0000000000000000 R E" ]]
cp "$eboot" cloud-output/
cp src/dusklight/ps4/default-config.json cloud-output/
cp "$bundle/LICENSE" "$bundle/THIRD_PARTY_LICENSES.txt" cloud-output/
{
    printf 'Build source: https://github.com/tortugaooff/dusklight-ps4-achievements/commit/%s\n' "$(git -C "$bundle" rev-parse HEAD)"
    for tree in refs/r25 refs/DolphinPS4 src/dusklight src/dawn src/sdl3 src/abseil src/mesa src/libdrm src/twilight-hd-hud; do
        printf '%s %s\n' "$tree" "$(git -C "$tree" rev-parse HEAD)"
    done
    git -C src/dusklight submodule status --recursive
} > cloud-output/source-revisions.txt
(cd cloud-output && sha256sum eboot.bin default-config.json elf-layout.txt source-revisions.txt LICENSE THIRD_PARTY_LICENSES.txt > SHA256SUMS.txt)
