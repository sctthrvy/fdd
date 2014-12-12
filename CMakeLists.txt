cmake_minimum_required(VERSION 2.8.4)
project(fdd)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -DDEBUG -DCOLOR")

set(FDD_SRCS src/fdd.c)

set(FD_LIB_SRCS lib/fd.c)

set(TEST_SRCS src/test.c)

include_directories(${fdd_SOURCE_DIR}/include)

# Create a shared library: libfd
add_library(fd SHARED ${FD_LIB_SRCS})

# The fdd[aemon] exe
add_executable(fdd ${FDD_SRCS})

add_executable(test ${TEST_SRCS})

target_link_libraries(test fd)
