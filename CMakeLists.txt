cmake_minimum_required(VERSION 3.6)
project(os_school)

set(CMAKE_C_STANDARD 90)
set(SOURCE_FILES ex3/counter.c ex3/dispatcher.c)
add_executable(counter ex3/counter.c)
add_executable(dispatcher ex3/dispatcher.c)