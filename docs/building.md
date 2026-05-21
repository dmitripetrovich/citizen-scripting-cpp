# Building citizen-scripting-cpp

To build the runtime plugin on Linux you need the following dependencies:

- [premake5](https://premake.github.io/) - build system generator
- [Zig](https://ziglang.org/) - musl-targeting C++ compiler (FXServer is a musl binary)
- [Rust toolchain](https://rustup.rs/) - builds wasmtime from source

Then clone with submodules:

```bash
git clone --recursive https://github.com/bd53/citizen-scripting-cpp.git
cd citizen-scripting-cpp
```

### Configure and build runtime

```bash
premake5 gmake2
```

This initializes the wasmtime submodule and builds it if needed.

Then build:

```bash
make -C build -f citizen-scripting-cpp.make config=release \
  CC="zig cc -target x86_64-linux-musl" \
  CXX="zig c++ -target x86_64-linux-musl" \
  -j$(nproc)
```

### Install runtime

Copy `build/bin/Release/libcitizen-scripting-cpp.so` next to your FXServer binary.

### Writing a resource

Just reference your `.cpp` file directly in the manifest:

```lua
server_script 'server.cpp'
```

The server needs the following for compilation:

- [Clang](https://clang.llvm.org/) - with `wasm32-wasip1` target support
- [WASI sysroot](https://github.com/WebAssembly/wasi-sdk) – install `wasi-sdk` or `wasi-sysroot` package

Specifically looks in `/usr/share/wasi-sysroot` and `/opt/wasi-sdk/share/wasi-sysroot`, or set `WASI_SYSROOT`.

The runtime compiles resource to `.wasm` automatically when it's started.

The resource is cached and only recompiled when the `.cpp` or the runtime `.so` changes.

### Known issues

- wasmtime-c-api build fails with a linker error

  Ensure `zig` is in your `PATH` and the Rust target is installed:
  ```bash
  rustup target add x86_64-unknown-linux-musl
  ```

- Resource traps immediately on the first tick

  Hitting the fuel limit (1 billion instructions per call).

  Break up heavy work across ticks or use `fx::createWorker`.

- `error: cannot use 'try' with exceptions disabled`

  Resources require `-fno-exceptions`.

  The runtime passes this flag automatically during on-the-fly compilation.
