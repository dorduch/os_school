cmake_minimum_required(VERSION 3.6)
project(os_school)

set(CMAKE_C_STANDARD)
set(SOURCE_FILES ex4/pcc_client.c ex4/pcc_server.c)
add_executable(pcc_client ex4/pcc_client.c)
add_executable(pcc_server ex4/pcc_server.c)