cmake_minimum_required(VERSION 2.8.4)
project(fdd)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -DDEBUG -DCOLOR")

set(FDDAEMON_SOURCE_FILES
    src/fddaemon.c)

set(FDD_LIB_SOURCE_FILES
    lib/fdd.c)


include_directories(${fdd_SOURCE_DIR}/include)

# Create a shared library: libfddl
add_library(fddl SHARED ${FDD_LIB_SOURCE_FILES})

# The fdd daemon exe
add_executable(fdd ${FDDAEMON_SOURCE_FILES})

# fdd exe might link with the shared library libfddl
#target_link_libraries(fdd fddl)