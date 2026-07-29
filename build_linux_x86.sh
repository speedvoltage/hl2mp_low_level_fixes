#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")" && pwd)
: "${MMSOURCE_ROOT:?Set MMSOURCE_ROOT to the Metamod:Source 2.0 source tree}"
: "${HL2SDK_ROOT:?Set HL2SDK_ROOT to the HL2DM SDK tree or Valve Source SDK src directory}"
cmake -Wno-dev -S "$root" -B "$root/build/linux-x86" -G Ninja \
	-DCMAKE_TOOLCHAIN_FILE="$root/cmake/linux-x86-clang.cmake" \
	-DCMAKE_BUILD_TYPE=Release \
	-DMMSOURCE_ROOT="$MMSOURCE_ROOT" \
	-DHL2SDK_ROOT="$HL2SDK_ROOT"
cmake --build "$root/build/linux-x86"
rm -rf "$root/dist/linux-x86"
cmake --install "$root/build/linux-x86" --prefix "$root/dist/linux-x86"
binary="$root/dist/linux-x86/addons/hl2mp_lowlevel_fixes/bin/hl2mp_lowlevel_fixes.so"
mkdir -p "$root/dist/linux-x86/symbols/linux-x86"
objcopy --only-keep-debug "$binary" "$root/dist/linux-x86/symbols/linux-x86/hl2mp_lowlevel_fixes.so.debug"
strip --strip-unneeded "$binary"
objcopy --add-gnu-debuglink="$root/dist/linux-x86/symbols/linux-x86/hl2mp_lowlevel_fixes.so.debug" "$binary"
