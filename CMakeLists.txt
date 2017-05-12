cmake_minimum_required(VERSION 3.6)
project(os_school)

set(CMAKE_C_STANDARD 90)
set(SOURCE_FILES dispatcher.c counter.c)
add_executable(os_school ${SOURCE_FILES} dispatcher.c)