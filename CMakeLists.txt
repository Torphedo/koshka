cmake_minimum_required(VERSION 3.13)
project(koshka C CXX)
# Emit a compile_commands.json for use with clangd in LSPs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

include_directories(src lib/bobtail lib)
add_subdirectory(lib/bobtail)

add_executable(koshka
    src/main.cxx
    src/elf.c
    src/pool.c
    src/boolset.cxx
    src/regalloc.c
    src/arm_asl.c
    src/imlgen/iml.cxx
    src/imlgen/decode.cxx
    src/imlgen/arm_encoding.c
    src/os/nro.c
    src/os/vmem.c
)

target_link_libraries(koshka PUBLIC bobtail)
