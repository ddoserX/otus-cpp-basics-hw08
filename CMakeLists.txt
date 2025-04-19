cmake_minimum_required(VERSION 3.31)
project(crc32_crack CXX)

add_executable(crc32_crack main.cpp
        CRC32.cpp
        CRC32.hpp
        IO.cpp
        IO.hpp)

target_compile_features(crc32_crack PUBLIC cxx_std_17)

if (MSVC)
    target_compile_options(crc32_crack PUBLIC /W4)
else ()
    target_compile_options(crc32_crack PUBLIC
        -Wall
        -Werror
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wsign-conversion
        -Wold-style-cast
        -Weffc++
        -O0
    )
endif ()