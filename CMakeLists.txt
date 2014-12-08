cmake_minimum_required(VERSION 2.8.4)
project(fdd)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")

set(SOURCE_FILES
    src/api.c
    src/fdd.c)

include_directories(include)

add_executable(fdd ${SOURCE_FILES})