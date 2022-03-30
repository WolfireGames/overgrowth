CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)
PROJECT(qslim)

IF(WIN32)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4018 /wd4244")
ENDIF()

SET(SRCDIR "${CMAKE_SOURCE_DIR}/../Libraries/qslim-2.1/tools/qslim/")
SET(LIBDIR "${CMAKE_SOURCE_DIR}/../Libraries")

INCLUDE_DIRECTORIES(
    ${LIBDIR}/qslim-2.1/tools/qslim
    ${LIBDIR}/qslim-2.1/mixkit/src
)

SET(QSLIM_SRCS
    ${LIBDIR}/qslim-2.1/tools/qslim/main.cxx
    ${LIBDIR}/qslim-2.1/tools/qslim/output.cxx
)

ADD_LIBRARY(qslim STATIC ${QSLIM_SRCS})

