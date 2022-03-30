CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)
PROJECT(gfx)

IF(WIN32)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4018 /wd4244")
ENDIF()

SET(SRCDIR "${CMAKE_SOURCE_DIR}/../Libraries/qslim-2.1/libgfx/")
SET(LIBDIR "${CMAKE_SOURCE_DIR}/../Libraries")


INCLUDE_DIRECTORIES(
    ${SRCDIR}/src
    ${SRCDIR}/include
)

FILE(GLOB LIBGFX_SRCS 
    ${SRCDIR}/src/arcball.cxx
    ${SRCDIR}/src/baseball.cxx
    ${SRCDIR}/src/geom3d.cxx
    ${SRCDIR}/src/geom4d.cxx
    ${SRCDIR}/src/gltools.cxx
    ${SRCDIR}/src/mat2.cxx
    ${SRCDIR}/src/mat3.cxx
    ${SRCDIR}/src/mat4.cxx
    ${SRCDIR}/src/quat.cxx
    ${SRCDIR}/src/raster.cxx
    ${SRCDIR}/src/raster-jpeg.cxx
    ${SRCDIR}/src/raster-png.cxx
    ${SRCDIR}/src/raster-pnm.cxx
    ${SRCDIR}/src/raster-tiff.cxx
    ${SRCDIR}/src/script.cxx
    ${SRCDIR}/src/symmat3.cxx
    ${SRCDIR}/src/symmat4.cxx
    ${SRCDIR}/src/time.cxx
    ${SRCDIR}/src/trackball.cxx
)

ADD_LIBRARY(gfx STATIC ${LIBGFX_SRCS})

ADD_DEFINITIONS(-DHAVE_BOOL=1)
