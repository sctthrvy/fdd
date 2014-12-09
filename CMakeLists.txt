cmake_minimum_required(VERSION 2.8.4)
project(fdd)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -DDEBUG -DCOLOR")

set(FDD_SOURCE_FILES
    src/fdd.c)

set(FD_LIB_SOURCE_FILES
    lib/fd.c)

include_directories(${fdd_SOURCE_DIR}/include)

# Create a shared library: libfd
add_library(fd SHARED ${FD_LIB_SOURCE_FILES})

# The fdd[aemon] exe
add_executable(fdd ${FDD_SOURCE_FILES})
