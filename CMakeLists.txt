cmake_minimum_required(VERSION 3.20)
project(fx-cpp-sdk LANGUAGES CXX)

add_library(fx-cpp-runtime SHARED
    CppRuntime.cpp
    Component.cpp
)
target_include_directories(fx-cpp-runtime PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(fx-cpp-runtime PUBLIC cxx_std_23)
set_target_properties(fx-cpp-runtime PROPERTIES OUTPUT_NAME "citizen-scripting-cpp")
add_library(example-runtime SHARED example/server.cpp)
target_include_directories(example-runtime PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(example-runtime PUBLIC cxx_std_23)
target_compile_definitions(example-runtime PRIVATE FXCPP_RUNTIME)
set_target_properties(example-runtime PROPERTIES
    OUTPUT_NAME "example"
    PREFIX ""
)
