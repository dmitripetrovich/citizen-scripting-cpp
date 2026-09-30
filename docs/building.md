# Building citizen-scripting-cpp

To build the runtime plugin on Linux you need the following dependencies:

- [premake5](https://premake.github.io/) - build system generator
- [Zig](https://ziglang.org/) - compiler (used for both the runtime and resource compilation)

Then clone:

```bash
git clone https://github.com/dmitripetrovich/citizen-scripting-cpp.git
cd citizen-scripting-cpp
```

### Configure and build runtime

```bash
premake5 --os=linux gmake
make -C build -f citizen-scripting-cpp.make config=release \
  CC="zig cc -target x86_64-linux-musl" \
  CXX="zig c++ -target x86_64-linux-musl" \
  -j$(nproc)
```

`premake5` will automatically fetch wasmtime and generate `src/natives/` on first run.

### Install runtime

Copy `build/bin/Release/libcitizen-scripting-cpp.so` next to your FXServer binary.

### Writing a resource

Reference `.cpp` files directly in your manifest, the runtime compiles them automatically:

```lua
server_script 'server.cpp'
```

The compiled `.wasm` is cached next to the source and only rebuilt when the `.cpp` changes.

You can also pre-compile with `tools/ext/build` and use `.wasm` directly if preferred.
