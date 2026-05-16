local cwd = os.getcwd()

return function()
    filter {}
    if not _OPTIONS["native"] then
        defines { "FXCPP_WASM_SUPPORT" }
        includedirs { cwd .. "/vendor/wasmtime/crates/c-api/include" }
        local genIncludes = os.matchdirs(cwd .. "/vendor/wasmtime/target/x86_64-unknown-linux-musl/release/build/wasmtime-c-api-impl-*/out/include")
        if #genIncludes > 0 then
            includedirs { genIncludes[1] }
        end
        linkoptions {
            cwd .. "/vendor/wasmtime/target/x86_64-unknown-linux-musl/release/libwasmtime.a",
            "-lpthread", "-ldl", "-lm",
        }
    end
end
