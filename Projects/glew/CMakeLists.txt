cmake_minimum_required(VERSION 2.6.0)
project(glew)

set(libdir "${CMAKE_SOURCE_DIR}/../Libraries")

include_directories(${libdir}/glew-2.0.0/include)
add_definitions(-DGLEW_STATIC)

add_library(glew STATIC ${libdir}/glew-2.0.0/src/glew.c)
