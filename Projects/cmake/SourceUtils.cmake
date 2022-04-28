MACRO(AddGroupedSource folder)
    FILE(GLOB GROUPED_SRCS RELATIVE ${CMAKE_SOURCE_DIR}
        ${SRCDIR}/${folder}/*.cpp
        ${SRCDIR}/${folder}/*.h
        ${SRCDIR}/${folder}/*.hpp
    )
    LIST(APPEND OVERGROWTH_SRCS ${GROUPED_SRCS})
    STRING(REGEX REPLACE "/" "\\\\" folder_backslash ${folder})
    SOURCE_GROUP(${folder_backslash} FILES ${GROUPED_SRCS})
ENDMACRO()

MACRO(AddGroupedScript folder)
    IF(EXISTS "${DATADIR}/")
        FILE(GLOB GROUPED_SRCS RELATIVE ${CMAKE_SOURCE_DIR}
            ${DATADIR}/${folder}/*.as
        )
        LIST(APPEND OVERGROWTH_SRCS ${GROUPED_SRCS})
        STRING(REGEX REPLACE "/" "\\\\" folder_backslash ${folder})
        SOURCE_GROUP(${folder_backslash} FILES ${GROUPED_SRCS})
    ENDIF()
ENDMACRO()

MACRO(AddGroupedShaders folder)
    IF(EXISTS "${DATADIR}/")
        FILE(GLOB GROUPED_SRCS RELATIVE ${CMAKE_SOURCE_DIR}
            ${DATADIR}/${folder}/*.glsl
            ${DATADIR}/${folder}/*.vert
            ${DATADIR}/${folder}/*.frag
        )
        LIST(APPEND OVERGROWTH_SRCS ${GROUPED_SRCS})
        STRING(REGEX REPLACE "/" "\\\\" folder_backslash ${folder})
        SOURCE_GROUP(${folder_backslash} FILES ${GROUPED_SRCS})
    ENDIF()
ENDMACRO()