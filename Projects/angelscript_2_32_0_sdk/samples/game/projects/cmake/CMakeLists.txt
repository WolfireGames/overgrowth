cmake_minimum_required(VERSION 2.6)
project(game)

add_executable(
    game
    ../../source/gamemgr.cpp
    ../../source/gameobj.cpp
    ../../source/main.cpp
    ../../source/scriptmgr.cpp
    ../../../../add_on/scriptbuilder/scriptbuilder.cpp
    ../../../../add_on/scripthandle/scripthandle.cpp
    ../../../../add_on/scriptstdstring/scriptstdstring.cpp
    ../../../../add_on/weakref/weakref.cpp
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../../angelscript/projects/cmake angelscript)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../../angelscript/include)
target_link_libraries(game ${ANGELSCRIPT_LIBRARY_NAME})

set_target_properties(game PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../../bin)
