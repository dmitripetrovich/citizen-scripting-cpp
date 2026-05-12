#!/bin/bash
set -e

SERVER_DIR="${FX_SERVER_DIR:?Set FX_SERVER_DIR to your cfx-server directory}"
RESOURCE_DIR="${FX_RESOURCE_DIR:?Set FX_RESOURCE_DIR to your resource directory}"
BUILD_DIR="$(cd "$(cd "$(dirname "$0")/.." && pwd)" && pwd)/build"

if [ ! -f "$BUILD_DIR/Makefile" ]; then
    mkdir -p "$BUILD_DIR"
    cmake -B "$BUILD_DIR" -S "$(cd "$(dirname "$0")/.." && pwd)" \
        -DCMAKE_C_COMPILER="zig;cc;-target;x86_64-linux-musl" \
        -DCMAKE_CXX_COMPILER="zig;c++;-target;x86_64-linux-musl"
fi

make -C "$BUILD_DIR" -j"$(nproc)"

cp "$BUILD_DIR/libcitizen-scripting-cpp.so" "$SERVER_DIR/"
cp "$BUILD_DIR/server.so" "$RESOURCE_DIR/"
cp "$(cd "$(dirname "$0")/.." && pwd)/tests/fxmanifest.lua" "$RESOURCE_DIR/"
cp "$(cd "$(dirname "$0")/.." && pwd)/tests/server.cpp" "$RESOURCE_DIR/"
cp "$(cd "$(dirname "$0")/.." && pwd)/tests/server.lua" "$RESOURCE_DIR/"

echo "Deployed to $SERVER_DIR and $RESOURCE_DIR"
