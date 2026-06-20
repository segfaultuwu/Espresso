cmake_minimum_required(VERSION 3.16)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CCACHE_ENABLE 1)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(espresso)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/espresso-cli
    COMMAND cc -I${CMAKE_SOURCE_DIR}/include -I${CMAKE_SOURCE_DIR}/host
            ${CMAKE_SOURCE_DIR}/host/espresso-cli.c
            ${CMAKE_SOURCE_DIR}/main/compiler.c
            -o ${CMAKE_BINARY_DIR}/espresso-cli
    DEPENDS ${CMAKE_SOURCE_DIR}/host/espresso-cli.c
            ${CMAKE_SOURCE_DIR}/main/compiler.c
            ${CMAKE_SOURCE_DIR}/include/espresso/compiler.h
            ${CMAKE_SOURCE_DIR}/include/espresso/vm/types.h
    COMMENT "Building host tool: espresso-cli"
)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/espresso-lsp
    COMMAND cc -I${CMAKE_SOURCE_DIR}/include -I${CMAKE_SOURCE_DIR}/host
            ${CMAKE_SOURCE_DIR}/host/espresso-lsp.c
            ${CMAKE_SOURCE_DIR}/main/compiler.c
            -o ${CMAKE_BINARY_DIR}/espresso-lsp
    DEPENDS ${CMAKE_SOURCE_DIR}/host/espresso-lsp.c
            ${CMAKE_SOURCE_DIR}/main/compiler.c
            ${CMAKE_SOURCE_DIR}/include/espresso/compiler.h
            ${CMAKE_SOURCE_DIR}/include/espresso/vm/types.h
    COMMENT "Building host tool: espresso-lsp"
)

add_custom_target(espresso_cli ALL DEPENDS ${CMAKE_BINARY_DIR}/espresso-cli)
add_custom_target(espresso_lsp ALL DEPENDS ${CMAKE_BINARY_DIR}/espresso-lsp)
