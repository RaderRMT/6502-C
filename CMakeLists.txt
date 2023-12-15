cmake_minimum_required(VERSION 3.27)
project(curses6502 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "-lncurses")

add_executable(curses6502 src/main.c
        src/arguments.c
        src/arguments.h
        src/cpu.c
        src/cpu.h
)
