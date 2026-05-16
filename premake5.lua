newoption {
    trigger = "wasm",
    description = "Build with WebAssembly support only"
}

newoption {
    trigger = "native",
    description = "Build with native support only"
}

local component = dofile("component.lua")

workspace "citizen-scripting-cpp"
    configurations { "Debug", "Release" }
    architecture "x86_64"
    language "C++"
    cppdialect "C++23"
    location "build"

project "citizen-scripting-cpp"
    kind "SharedLib"
    targetname "citizen-scripting-cpp"
    targetprefix "lib"
    files {
        "src/CppScriptRuntime.cpp",
        "src/Component.cpp",
    }
    includedirs { "." }

    if _OPTIONS["wasm"] then
    elseif _OPTIONS["native"] then
        defines { "FXCPP_NATIVE_SUPPORT" }
    else
        defines { "FXCPP_NATIVE_SUPPORT" }
    end

    component()

    filter "configurations:Debug"
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        optimize "On"

project "test-runtime"
    kind "SharedLib"
    targetname "server"
    targetprefix ""
    files {
        "tools/example/server.cpp",
    }
    includedirs { "." }
    defines { "FXCPP_RUNTIME" }

    filter "configurations:Debug"
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        optimize "On"
