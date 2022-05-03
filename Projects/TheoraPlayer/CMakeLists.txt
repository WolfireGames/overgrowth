SET(LIBDIR "${CMAKE_SOURCE_DIR}/../Libraries")
SET(SRCDIR "${CMAKE_SOURCE_DIR}/../Libraries/TheoraPlayer/Source")

INCLUDE_DIRECTORIES(
    ${SRCDIR}
    ${LIBDIR}/Linux/include
    ${LIBDIR}/theora/include
    ${LIBDIR}/ogg/include
    ${LIBDIR}/vorbis/include
)

FILE(GLOB THEORAPLAYER_SRCS
    ${SRCDIR}/TheoraPlayer/*.cpp
)

ADD_LIBRARY(TheoraPlayer STATIC ${THEORAPLAYER_SRCS})

