cmake_minimum_required(VERSION 3.4)

project(SOL_PROJECT VERSION 0.1 LANGUAGES C)

set(
  CMAKE_C_FLAGS
  "${CMAKE_C_FLAGS} \
  -std=c99 \
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
  -Wunused-macros"
)

set(default_build_type "Debug")

set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Suppress warnings for dependencies.
set_source_files_properties(
  "${LIB_DIR}/tomlc99/toml.c"
  "${LIB_DIR}/xxHash/xxhash.c"
  PROPERTIES
  COMPILE_FLAGS "-w"
)

# API library
add_library(server_api "${SRC_DIR}/serverapi.c")
target_link_libraries(server_api utils)
target_include_directories(server_api PUBLIC "${INCLUDE_DIR}")

# Server
file(
  GLOB
  SERVER_FILES
  "${SRC_DIR}/*.c"
  "${SRC_DIR}/*.h"
  "${SRC_DIR}/server/*.c"
  "${SRC_DIR}/server/*.h"
  "${LIB_DIR}/logc/src/log.c"
  "${LIB_DIR}/tomlc99/toml.c"
  "${LIB_DIR}/xxHash/xxhash.c"
)
add_executable(server "${SERVER_FILES}")
target_include_directories(server PUBLIC "${LIB_DIR}" "${INCLUDE_DIR}")
target_link_libraries(server pthread)

# Client
file(
  GLOB
  CLIENT_FILES
  "${SRC_DIR}/*.c"
  "${SRC_DIR}/*.h"
  "${SRC_DIR}/client/*.c"
  "${SRC_DIR}/client/*.h"
  "${LIB_DIR}/logc/src/log.c"
)
add_executable(client "${CLIENT_FILES}")
target_include_directories(client PUBLIC "${LIB_DIR}" "${INCLUDE_DIR}")
