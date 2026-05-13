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
        "src/Runtime.cpp",
        "src/Component.cpp",
    }
    includedirs { "." }

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
