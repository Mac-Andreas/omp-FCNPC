#!/usr/bin/env bash
# Cross-compile FCNPC.dll with the Microsoft C++ ABI on macOS/Linux, without
# Visual Studio. Uses brew LLVM's clang-cl + lld-link against MSVC headers/libs
# fetched by msvc-wine's vsdownload.py. Produces a DLL that open.mp's MSVC-built
# Windows server can actually load (correct vtable ABI).
#
#   brew install llvm msitools
#   git clone https://github.com/mstorsjo/msvc-wine /tmp/msvc-wine
#   python3 /tmp/msvc-wine/vsdownload.py --accept-license \
#       --architecture x86 --host-arch x64 --dest /tmp/msvc
#   MSVC_ROOT=/tmp/msvc ./build-msvc-clang.sh
#
set -euo pipefail
cd "$(dirname "$0")"

# lld-link ships in the separate `lld` formula (/opt/homebrew/bin); clang-cl
# locates it via PATH when -fuse-ld=lld is used.
export PATH="/opt/homebrew/bin:$PATH"

LLVM_BIN="${LLVM_BIN:-/opt/homebrew/opt/llvm/bin}"
CLANG_CL="$LLVM_BIN/clang-cl"
MSVC_ROOT="${MSVC_ROOT:-/tmp/msvc}"

# Locate the installed MSVC toolchain + Windows SDK versions (newest).
MSVC_VER=$(ls "$MSVC_ROOT/VC/Tools/MSVC" | sort -V | tail -1)
SDK_ROOT="$MSVC_ROOT/Windows Kits/10"
SDK_VER=$(ls "$SDK_ROOT/Include" | sort -V | tail -1)

MSVC_INC="$MSVC_ROOT/VC/Tools/MSVC/$MSVC_VER/include"
MSVC_LIB="$MSVC_ROOT/VC/Tools/MSVC/$MSVC_VER/lib/x86"
SDK_INC="$SDK_ROOT/Include/$SDK_VER"
SDK_LIB="$SDK_ROOT/Lib/$SDK_VER"

echo "MSVC $MSVC_VER / SDK $SDK_VER"

OUR_INCLUDES=(
	-Isdk/include
	-Isdk/lib/glm
	-Isdk/lib/span-lite/include
	-Isdk/lib/string-view-lite/include
	-Isdk/lib/robin-hood-hashing/src/include
	-I. -Isrc -Inetwork
	-Ipawn/source -Ipawn/source/amx
)

"$CLANG_CL" \
	--target=i686-pc-windows-msvc \
	-fuse-ld=lld \
	/std:c++17 /EHsc /MT /O2 /w \
	-DHAVE_STDINT_H=1 -DPAWN_CELL_SIZE=32 -DWIN32 -D_WIN32 -DHAVE_ALLOCA_H=0 \
	-DWINDOWS_IGNORE_PACKING_MISMATCH -DNDEBUG -DNOMINMAX \
	/imsvc "$MSVC_INC" \
	/imsvc "$SDK_INC/ucrt" \
	/imsvc "$SDK_INC/shared" \
	/imsvc "$SDK_INC/um" \
	/imsvc "$SDK_INC/winrt" \
	"${OUR_INCLUDES[@]}" \
	/LD src/main.cpp src/FCNPC.cpp src/natives.cpp src/natives_array.cpp src/bridge.cpp \
	/Fe:FCNPC.dll \
	/link \
	/LIBPATH:"$MSVC_LIB" \
	/LIBPATH:"$SDK_LIB/ucrt/x86" \
	/LIBPATH:"$SDK_LIB/um/x86"

echo "---"
file FCNPC.dll
