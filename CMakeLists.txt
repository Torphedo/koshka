cmake_minimum_required(VERSION 3.13)
project(koshka C)
# Emit a compile_commands.json for use with clangd in LSPs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

include_directories(src lib/bobtail)
add_subdirectory(lib/bobtail)

add_executable(koshka
    src/regalloc.c
    src/main.c
    src/os/nro.c
    src/os/vmem.c
    src/irgen/decode.c
    src/irgen/arm_encoding.c
)

target_link_libraries(koshka PUBLIC bobtail)
