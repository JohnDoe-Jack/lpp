cmake_minimum_required(VERSION 3.10)

# プロジェクト名と使用する言語を設定
project(lpp)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

add_compile_options(-Wall -Wextra -Werror)

# 実行ファイルを生成するためのソースファイルを指定
# add_executable(educational_cpu_board main.c cpuboard.c)
# add_executable(assembler assembler.cpp)
# add_executable(test test.cpp)

# インクルードディレクトリを指定
# target_include_directories(educational_cpu_board PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})