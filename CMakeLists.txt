cmake_minimum_required(VERSION 4.0)
project(anvil)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Werror -Wextra)

include_directories("include")

file(GLOB_RECURSE SOURCES "src/**.c")
add_executable(anvil src/main.c "${SOURCES}")
