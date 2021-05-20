cmake_minimum_required(VERSION 3.4)

project(SOL_PROJECT VERSION 0.1 LANGUAGES C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
   -Wall \
   -Wpedantic \
   -Wextra \
   -Wshadow \
   -Wcomment \
   -Wno-missing-braces \
   -Wno-missing-field-initializers \
   -Wswitch-default \
   -Wcast-align \
   -Wpointer-arith \
   -Wundef \
   -Wuninitialized \
   -Wredundant-decls \
   -Wold-style-definition \
   -Wunreachable-code \
   -Wunused-macros")

set(default_build_type "Debug")

set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

# Server
file(GLOB_RECURSE SRC_FILES "${SRC_DIR}/server/*.c" "${SRC_DIR}/server/*.h" "${LIB_DIR}/tomlc99/toml.c")
add_executable(server "${SRC_FILES}")
target_include_directories(server PUBLIC "${LIB_DIR}")
target_link_libraries(server pthread)
