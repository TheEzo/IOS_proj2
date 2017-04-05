cmake_minimum_required(VERSION 3.7)
project(IOS_proj2)

set(CMAKE_CXX_STANDARD -std=gnu99 -Wall -Wextra -Werror -pedantic)

set(SOURCE_FILES main.c)
add_executable(IOS_proj2 ${SOURCE_FILES})