SET(SRCDIR "${CMAKE_SOURCE_DIR}/../Libraries/murmurhash3/")
SET(LIBDIR "${CMAKE_SOURCE_DIR}/../Libraries")

INCLUDE_DIRECTORIES(
    ${LIBDIR}/murmurhash3
)

SET(MURMURHASH3_SRCS
    ${LIBDIR}/murmurhash3/MurmurHash3.cpp
)

ADD_LIBRARY(murmurhash3 STATIC ${MURMURHASH3_SRCS})

