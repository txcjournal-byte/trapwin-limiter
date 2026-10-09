#!/usr/bin/env bash
# Cross-compiles the Windows 64-bit VST3 + CLAP on Linux with llvm-mingw.
# Usage: scripts/build-windows.sh            (downloads the toolchain into ./out/toolchain if needed)
set -euo pipefail
cd "$(dirname "$0")/.."

LLVM_MINGW_VER="${LLVM_MINGW_VER:-20251007}"
TC="out/toolchain/llvm-mingw-${LLVM_MINGW_VER}"
if [ ! -x "$TC/bin/x86_64-w64-mingw32-clang++" ]; then
  mkdir -p "$TC"
  url="https://github.com/mstorsjo/llvm-mingw/releases/download/${LLVM_MINGW_VER}/llvm-mingw-${LLVM_MINGW_VER}-ucrt-ubuntu-22.04-x86_64.tar.xz"
  echo "Downloading $url"
  curl -fsSL "$url" | tar -xJ -C "$TC" --strip-components=1
fi
export PATH="$PWD/$TC/bin:$PATH"

rm -rf bin build
make -C plugin vst3 clap WINDOWS=true \
  CC=x86_64-w64-mingw32-clang CXX=x86_64-w64-mingw32-clang++ AR=x86_64-w64-mingw32-ar -j"$(nproc)"

# sanity: the DLLs must only depend on Windows system libraries
for f in bin/TrapLimit.vst3/Contents/x86_64-win/TrapLimit.vst3 bin/TrapLimit.clap; do
  deps=$(llvm-objdump -p "$f" | awk '/DLL Name/ {print tolower($3)}' | grep -vE '^(api-ms-win-crt-.*|kernel32|user32|gdi32|opengl32|dwmapi|shell32|ole32|advapi32|comdlg32)\.dll$' || true)
  if [ -n "$deps" ]; then echo "Unexpected DLL dependency in $f: $deps"; exit 1; fi
done
echo "Windows build OK"
ls -la bin bin/TrapLimit.vst3/Contents/x86_64-win
