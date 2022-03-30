CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)
PROJECT(minizip)

SET(SRCDIR "${CMAKE_SOURCE_DIR}/../Libraries/minizip")
SET(LIBDIR "${CMAKE_SOURCE_DIR}/../Libraries")

IF(WIN32)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS /wd4996")
ENDIF()

INCLUDE_DIRECTORIES(
    ${SRCDIR}
    ${LIBDIR}/zlib1.2.7/include
)

SET(MINIZIP_SRCS
    ${SRCDIR}/ioapi.c
    ${SRCDIR}/mztools.c
    ${SRCDIR}/zip.c
    ${SRCDIR}/unzip.c
    ${SRCDIR}/zip_util_internal.c
)

IF(WIN32)
    SET(MINIZIP_SRCS 
        ${MINIZIP_SRCS}
        ${SRCDIR}/iowin32.c
    )
ENDIF()

ADD_LIBRARY(minizip STATIC ${MINIZIP_SRCS})

