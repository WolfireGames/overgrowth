SET(SRCDIR "${CMAKE_SOURCE_DIR}/../Libraries/recastnavigation")

INCLUDE_DIRECTORIES(
    ${SRCDIR}/Recast/Include
    ${SRCDIR}/Detour/Include
    ${SRCDIR}/DetourCrowd/Include
    ${SRCDIR}/DetourTileCache/Include
)

FILE(GLOB RECAST_SRCS
    ${SRCDIR}/Recast/Source/*.cpp
)

FILE(GLOB DETOUR_SRCS
    ${SRCDIR}/Detour/Source/*.cpp
)

FILE(GLOB DETOUR_CROWD_SRCS
    ${SRCDIR}/DetourCrowd/Source/*.cpp
)

FILE(GLOB DETOUR_TILE_CACHE_SRCS
    ${SRCDIR}/DetourTileCache/Source/*.cpp
)

ADD_LIBRARY(Recast STATIC ${RECAST_SRCS})
ADD_LIBRARY(Detour STATIC ${DETOUR_SRCS})
ADD_LIBRARY(DetourCrowd STATIC ${DETOUR_CROWD_SRCS})
ADD_LIBRARY(DetourTileCache STATIC ${DETOUR_TILE_CACHE_SRCS})
