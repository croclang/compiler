cmake_minimum_required(VERSION 3.14)

find_program(CCACHE_PROGRAM ccache)

if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 17)

# add_compile_options(-fsanitize=address)
# add_link_options(-fsanitize=address)

set(SOURCES
    src/codegen.c
    src/error.c
    src/environment.c
    src/file_io.c
    src/main.c
    src/parser.c)

project(croc)
add_executable(croc ${SOURCES})

target_include_directories(croc PUBLIC src/)