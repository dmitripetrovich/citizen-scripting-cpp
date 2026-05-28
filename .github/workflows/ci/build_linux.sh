#!/bin/bash
set -euo pipefail

cd "$(cd "$(dirname "$0")/../../.." && pwd)"

echo "Fetching wasmtime..."
tools/ext/wasmtime

echo "Building runtime..."
premake5 --os=linux gmake
make -C build config=release \
        CC="zig cc -target x86_64-linux-musl" \
        CXX="zig c++ -target x86_64-linux-musl" \
        -j"$(nproc)"

echo "All checks passed."
