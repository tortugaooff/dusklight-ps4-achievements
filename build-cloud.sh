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
git -C "src/$llvm" apply "$root/patches/libcxx-ps4-atomic-wait.patch"
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
bash "$bundle/build-ps4.sh" "$root"

eboot=build/game/dusklight_eboot/eboot.bin
elf=build/game/dusklight
test -s "$eboot"
test -s "$elf"
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
