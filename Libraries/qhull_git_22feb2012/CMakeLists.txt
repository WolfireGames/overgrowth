# CMakeLists.txt -- CMake configuration file for qhull, qhull6, and related programs
#
# To build with MSYS/mingw
#   cd build && cmake -G "MSYS Makefiles" .. && cmake ..
#   make
#   make install
#
# To uninstall on unix or MSYS/mingw
#   xargs rm <build/install_manifest.txt
#
# To build Qhull Visual Studio projects, run cmake twice
#   mkdir -p build-cmake && cd build-cmake && cmake -G "Visual Studio 8 2005" .. && cmake ..
#   Double click build-cmake/qhull-all.sln
#   Build INSTALL for C:/Program Files/qhull
#   You may need to copy bin/msvcr80.dll into C:/Program Files/qhull/bin
#   If using library debug targets, please rename with '_d' (e.g., qhullstatic_d.lib)
#
# To uninstall on Windows
#   Delete C:/Program Files/qhull
#
# For qhulltest, use the Qt build (src/qhull-all.pro)
#
# Qhull ships with cmake-derived sln and proj files
#   See eg/make-vcproj.sh -- it created the first versions of these files
#   Change to relative paths
#   Remove ZERO_CHECK, ALL_BUILD, and INSTALL projects
#   Change targets to bin/ and lib/ directories
#   Disable incremental linking and ilk files (LinkIncremental="1")
#   Disable Run-Time Type Info (rtti)
#   Remove src/libqhullcpp from most of the AdditionalIncludeDirectories
#   Remove CMAKE_INTDIR from PreprocessorDefinitions
#   Adjust target names and destinations (e.g., lib/libqhullstatic_d.a)
#  
# $Id: //main/2011/qhull/CMakeLists.txt#23 $$Change: 1494 $
# $DateTime: 2012/02/21 22:41:40 $$Author: bbarber $

project(qhull)
cmake_minimum_required(VERSION 2.6)

# Define qhull_VERSION in CMakeLists.txt, Makefile, qhull-exports.def, qhull_p-exports.def, and qhull-warn.pri
string(REGEX REPLACE ".* ([0-9]+) .*"
    "6.3.1.\\1"  qhull_VERSION "$Change: 1494 $")
# qhull_SOVERSION: 2003 = empty, 2009 = 5 , 2010-2012 = 6
set(qhull_SOVERSION 6)
set(qhull_VERSION2 "2012.1 2012/02/18")

if(INCLUDE_INSTALL_DIR)
else()
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include)
endif()
if(LIB_INSTALL_DIR)
else()
set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib)
endif()
if(BIN_INSTALL_DIR)
else()
set(BIN_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/bin)
endif()
if(MAN_INSTALL_DIR)
else()
    if(WIN32)
        set(MAN_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/man/man1)
    else()
        set(MAN_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/share/man/man1)
    endif()
endif()
if(DOC_INSTALL_DIR)
else()
    if(WIN32)
        set(DOC_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/doc)
    else()
        set(DOC_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/share/doc/qhull)
    endif()
endif()
message(STATUS)
message(STATUS "========== qhull Build Information ==========")
message(STATUS "Build Version: 				   ${qhull_VERSION}")
message(STATUS "Install Prefix (CMAKE_INSTALL_PREFIX):     ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Binary Directory (BIN_INSTALL_DIR):        ${BIN_INSTALL_DIR}")
message(STATUS "Library Directory (LIB_INSTALL_DIR):       ${LIB_INSTALL_DIR}")
message(STATUS "Include Directory (INCLUDE_INSTALL_DIR):   ${INCLUDE_INSTALL_DIR}")
message(STATUS "Documentation Directory (DOC_INSTALL_DIR): ${DOC_INSTALL_DIR}")
message(STATUS "Man Pages Directory (MAN_INSTALL_DIR):     ${MAN_INSTALL_DIR}")
message(STATUS "Build Type (CMAKE_BUILD_TYPE):             ${CMAKE_BUILD_TYPE}")
message(STATUS "To override these options, add -D{OPTION_NAME}=... to the cmake command")
message(STATUS "  Build the debug targets                  -DCMAKE_BUILD_TYPE=Debug")
message(STATUS)
message(STATUS "To build and install qhull, enter \"make\" and \"make install\"")
message(STATUS)

# ---------------------------------------
# Define library source files and variables
#
# Files for individual targets are defined with the target
# ---------------------------------------

# Order libqhull object files by frequency of execution.  Small files at end.
set(
    libqhull_HEADERS
        src/libqhull/libqhull.h
        src/libqhull/geom.h
        src/libqhull/io.h
        src/libqhull/mem.h
        src/libqhull/merge.h
        src/libqhull/poly.h
        src/libqhull/qhull_a.h
        src/libqhull/qset.h
        src/libqhull/random.h
        src/libqhull/stat.h
        src/libqhull/user.h
)
set(
    libqhull_SOURCES
        src/libqhull/global.c
        src/libqhull/stat.c
        src/libqhull/geom2.c
        src/libqhull/poly2.c
        src/libqhull/merge.c
        src/libqhull/libqhull.c
        src/libqhull/geom.c
        src/libqhull/poly.c
        src/libqhull/qset.c
        src/libqhull/mem.c
        src/libqhull/random.c
        src/libqhull/usermem.c
        src/libqhull/userprintf.c
        src/libqhull/io.c
        src/libqhull/user.c
        src/libqhull/rboxlib.c
        src/libqhull/userprintf_rbox.c
        ${libqhull_HEADERS}
)

set(
    libqhull_DOC
        src/libqhull/index.htm
        src/libqhull/qh-geom.htm
        src/libqhull/qh-globa.htm
        src/libqhull/qh-io.htm
        src/libqhull/qh-mem.htm
        src/libqhull/qh-merge.htm
        src/libqhull/qh-poly.htm
        src/libqhull/qh-qhull.htm
        src/libqhull/qh-set.htm
        src/libqhull/qh-stat.htm
        src/libqhull/qh-user.htm    
)

set(
    testqset_HEADERS
        src/libqhull/mem.h
        src/libqhull/qset.h
)
set(
    testqset_SOURCES
        src/libqhull/qset.c
        src/libqhull/mem.c
        src/testqset/testqset.c
        ${testqset_HEADERS}
)

set(
    libqhullcpp_HEADERS
        src/libqhullcpp/Coordinates.h
        src/libqhullcpp/functionObjects.h
        src/libqhullcpp/PointCoordinates.h
        src/libqhullcpp/Qhull.h
        src/libqhullcpp/QhullError.h
        src/libqhullcpp/QhullFacet.h
        src/libqhullcpp/QhullFacetList.h
        src/libqhullcpp/QhullFacetSet.h
        src/libqhullcpp/QhullHyperplane.h
        src/libqhullcpp/QhullIterator.h
        src/libqhullcpp/QhullLinkedList.h
        src/libqhullcpp/QhullPoint.h
        src/libqhullcpp/QhullPoints.h
        src/libqhullcpp/QhullPointSet.h
        src/libqhullcpp/QhullQh.h
        src/libqhullcpp/QhullRidge.h
        src/libqhullcpp/QhullSet.h
        src/libqhullcpp/QhullSets.h
        src/libqhullcpp/QhullStat.h
        src/libqhullcpp/QhullVertex.h
        src/libqhullcpp/QhullVertexSet.h
        src/libqhullcpp/RboxPoints.h
        src/libqhullcpp/RoadError.h
        src/libqhullcpp/RoadLogEvent.h
        src/libqhullcpp/UsingLibQhull.h
        src/qhulltest/RoadTest.h
)

set(
    libqhullcpp_SOURCES
        src/libqhullcpp/Coordinates.cpp
        src/libqhullcpp/PointCoordinates.cpp
        src/libqhullcpp/Qhull.cpp
        src/libqhullcpp/QhullFacet.cpp
        src/libqhullcpp/QhullFacetList.cpp
        src/libqhullcpp/QhullFacetSet.cpp
        src/libqhullcpp/QhullHyperplane.cpp
        src/libqhullcpp/QhullPoint.cpp
        src/libqhullcpp/QhullPointSet.cpp
        src/libqhullcpp/QhullPoints.cpp
        src/libqhullcpp/QhullQh.cpp
        src/libqhullcpp/QhullRidge.cpp
        src/libqhullcpp/QhullSet.cpp
        src/libqhullcpp/QhullStat.cpp
        src/libqhullcpp/QhullVertex.cpp
        src/libqhullcpp/QhullVertexSet.cpp
        src/libqhullcpp/RboxPoints.cpp
        src/libqhullcpp/RoadError.cpp
        src/libqhullcpp/RoadLogEvent.cpp
        src/libqhullcpp/UsingLibQhull.cpp
        ${libqhullcpp_HEADERS}
)

set(
    doc_FILES
        README.txt 
        REGISTER.txt 
        Announce.txt 
        COPYING.txt 
        index.htm
)

include_directories(${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/libqhull)

if(CMAKE_BUILD_TYPE MATCHES "[dD]ebug")
    set(qhull_CPP qhullcpp_d)
    set(qhull_SHARED qhull_d)
    set(qhull_SHAREDP qhull_pd)
    set(qhull_STATIC qhullstatic_d)
    set(qhull_STATICP qhullstatic_pd)
else()
    set(qhull_CPP qhullcpp)
    set(qhull_SHARED libqhull)  # Avoid name conflict with qhull executable
    set(qhull_SHAREDP qhull_p)
    set(qhull_STATIC qhullstatic)
    set(qhull_STATICP qhullstatic_p)
endif()

set(
    qhull_TARGETS_INSTALL
        ${qhull_CPP} ${qhull_STATIC} ${qhull_STATICP} ${qhull_SHARED} ${qhull_SHAREDP}
        qhull rbox qconvex qdelaunay qvoronoi qhalf
)
set(
    qhull_TARGETS_TEST
        user_eg user_eg2 user_eg3 testqset
)
set(
    qhull_TARGETS
        ${qhull_TARGETS_INSTALL} ${qhull_TARGETS_TEST}
)


# ---------------------------------------
# Define shared library qhull without qh_QHpointer
# ---------------------------------------

add_library(${qhull_SHARED} SHARED 
        ${libqhull_SOURCES}
        src/libqhull/qhull-exports.def)
        
if(qhull_SHARED MATCHES "libqhull")
   set_target_properties(${qhull_SHARED} PROPERTIES
        OUTPUT_NAME "qhull")
endif()

set_target_properties(${qhull_SHARED} PROPERTIES
    SOVERSION ${qhull_SOVERSION})

if(UNIX)
    target_link_libraries(${qhull_SHARED} m)
    if(APPLE)
        set_target_properties(${qhull_SHARED} PROPERTIES 
            INSTALL_NAME_DIR "${LIB_INSTALL_DIR}")
    else()
        set_target_properties(${qhull_SHARED} PROPERTIES 
            INSTALL_RPATH "${LIB_INSTALL_DIR}"
            INSTALL_RPATH_USE_LINK_PATH TRUE
            BUILD_WITH_INSTALL_RPATH FALSE)
    endif()
endif(UNIX)

# ---------------------------------------
# Define shared library qhull with qh_QHpointer
# ---------------------------------------

add_library(${qhull_SHAREDP} SHARED 
        ${libqhull_SOURCES}
        src/libqhullp/qhull_p-exports.def)
set_target_properties(${qhull_SHAREDP} PROPERTIES
    COMPILE_DEFINITIONS "qh_QHpointer"
    SOVERSION ${qhull_SOVERSION})

if(UNIX)
    target_link_libraries(${qhull_SHAREDP} m)
    if(APPLE)
        set_target_properties(${qhull_SHAREDP} PROPERTIES 
            INSTALL_NAME_DIR "${LIB_INSTALL_DIR}")
    else()
        set_target_properties(${qhull_SHAREDP} PROPERTIES 
            INSTALL_RPATH "${LIB_INSTALL_DIR}"
            INSTALL_RPATH_USE_LINK_PATH TRUE
            BUILD_WITH_INSTALL_RPATH FALSE)
    endif()
endif(UNIX)

# ---------------------------------------
# Define static libraries qhullstatic and qhullstatic_p (qh_QHpointer)
# ---------------------------------------

add_library(${qhull_STATIC} STATIC ${libqhull_SOURCES})
set_target_properties(${qhull_STATIC} PROPERTIES
    VERSION ${qhull_VERSION})

add_library(${qhull_STATICP} STATIC ${libqhull_SOURCES})
set_target_properties(${qhull_STATICP} PROPERTIES
    COMPILE_DEFINITIONS "qh_QHpointer"
    VERSION ${qhull_VERSION})

if(UNIX)
    target_link_libraries(${qhull_STATIC} m)
    target_link_libraries(${qhull_STATICP} m)
endif(UNIX)

# ---------------------------------------
# Define C++ static library qhullcpp (qh_QHpointer)
# ---------------------------------------

add_library(${qhull_CPP} STATIC ${libqhullcpp_SOURCES})
set_target_properties(${qhull_CPP} PROPERTIES
    COMPILE_DEFINITIONS "qh_QHpointer"
    VERSION ${qhull_VERSION})

# ---------------------------------------
# Define qhull executables linked to qhullstatic library
# ---------------------------------------

set(qhull_SOURCES       src/qhull/unix.c)
set(rbox_SOURCES        src/rbox/rbox.c)
set(qconvex_SOURCES     src/qconvex/qconvex.c)
set(qdelaunay_SOURCES   src/qdelaunay/qdelaun.c)
set(qvoronoi_SOURCES    src/qvoronoi/qvoronoi.c)
set(qhalf_SOURCES       src/qhalf/qhalf.c)
set(user_eg2_SOURCES    src/user_eg2/user_eg2.c)
if(MSVC)
    set(user_eg_DEFINES qh_QHpointer qh_QHpointer_dllimport)
    set(user_eg2_DEFINES qh_dllimport)
else()
    set(user_eg_DEFINES qh_QHpointer)
    set(user_eg2_DEFINES )
endif()

add_executable(qhull ${qhull_SOURCES})
target_link_libraries(qhull ${qhull_STATIC})

add_executable(rbox ${rbox_SOURCES})
target_link_libraries(rbox ${qhull_STATIC})

add_executable(qconvex ${qconvex_SOURCES})
target_link_libraries(qconvex ${qhull_STATIC})

add_executable(qdelaunay ${qdelaunay_SOURCES})
target_link_libraries(qdelaunay ${qhull_STATIC})

add_executable(qvoronoi ${qvoronoi_SOURCES})
target_link_libraries(qvoronoi ${qhull_STATIC})

add_executable(qhalf ${qhalf_SOURCES})
target_link_libraries(qhalf ${qhull_STATIC})

# ---------------------------------------
# Define testqset linked to qset.o and mem.o
# ---------------------------------------

add_executable(testqset ${testqset_SOURCES})

# ---------------------------------------
# Define user_eg linked to qhull shared library with qh_QHpointer
# ---------------------------------------

set(user_eg_SOURCES     src/user_eg/user_eg.c)

add_executable(user_eg ${user_eg_SOURCES})
# user_eg may be linked to qhull_STATIC if user_eg_DEFINES is removed
target_link_libraries(user_eg ${qhull_SHAREDP})
set_target_properties(user_eg PROPERTIES
    COMPILE_DEFINITIONS "${user_eg_DEFINES}")

# ---------------------------------------
# Define user_eg2 linked to qhull shared library (e.g., Debian)
# ---------------------------------------

add_executable(user_eg2 ${user_eg2_SOURCES})
target_link_libraries(user_eg2 ${qhull_SHARED})
set_target_properties(user_eg2 PROPERTIES
    COMPILE_DEFINITIONS "${user_eg2_DEFINES}")

# ---------------------------------------
# Define user_eg3 linked to qhullstatic_p and qhullcpp static library
# ---------------------------------------

# libqhullcpp must be before src/libqhull.  The later is not needed.
# Otherwise get error -- user_eg3.cpp:72: error: aggregate 'orgQhull::Qhull qhull' has incomplete type and cannot be defined
include_directories(BEFORE ${CMAKE_SOURCE_DIR}/src/libqhullcpp)

set(user_eg3_SOURCES    src/user_eg3/user_eg3.cpp)

add_executable(user_eg3 ${user_eg3_SOURCES})
# qhull_STATICP must be last, otherwise qh_fprintf,etc. are not loaded from qhull_CPP
target_link_libraries(user_eg3 ${qhull_CPP} ${qhull_STATICP})
set_target_properties(user_eg3 PROPERTIES
    COMPILE_DEFINITIONS "qh_QHpointer")

# ---------------------------------------
# Define install
# ---------------------------------------

install(TARGETS ${qhull_TARGETS_INSTALL}
        RUNTIME DESTINATION ${BIN_INSTALL_DIR}
        LIBRARY DESTINATION ${LIB_INSTALL_DIR}
        ARCHIVE DESTINATION ${LIB_INSTALL_DIR})

install(FILES ${libqhull_HEADERS}    DESTINATION ${INCLUDE_INSTALL_DIR}/libqhull)
install(FILES ${libqhull_DOC}        DESTINATION ${INCLUDE_INSTALL_DIR}/libqhull)
install(FILES ${libqhullcpp_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/libqhullcpp)
install(FILES html/qhull.man         DESTINATION ${MAN_INSTALL_DIR} RENAME qhull.1)
install(FILES html/rbox.man          DESTINATION ${MAN_INSTALL_DIR} RENAME rbox.1)
install(FILES ${doc_FILES}           DESTINATION ${DOC_INSTALL_DIR})
install(DIRECTORY html/              DESTINATION ${DOC_INSTALL_DIR})
