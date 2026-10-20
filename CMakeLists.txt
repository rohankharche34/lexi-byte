cmake_minimum_required(VERSION 3.16)

project(lexi-byte LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(lexi-byte src/main.cpp)

target_compile_options(lexi-byte PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

