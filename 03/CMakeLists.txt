cmake_minimum_required(VERSION 3.10)

project(lpp)

if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# add_compile_options(-Wall -Wextra -Werror)
add_executable(cr main.c lpp.h parse.c scan.c util.c hashmap.c)