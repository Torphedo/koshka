cmake_minimum_required(VERSION 3.13)
project(koshka C)
# Emit a compile_commands.json for use with clangd in LSPs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

include_directories(src lib/bobtail lib)
add_subdirectory(lib/bobtail)

add_executable(koshka
    src/main.c
    src/elf.c
    src/pool.c
    src/regalloc.c
    src/imlgen/decode.c
    src/imlgen/arm_encoding.c
    src/os/nro.c
    src/os/vmem.c
)

target_link_libraries(koshka PUBLIC bobtail)
