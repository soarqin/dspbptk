cmake_minimum_required(VERSION 3.15)

project(dspbptk C)

set(CMAKE_C_STANDARD 11)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(CustomCompilerOptions)
fix_compile_flags()
fix_release_flags()
add_static_runtime_option()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(lib)
add_subdirectory(app)
