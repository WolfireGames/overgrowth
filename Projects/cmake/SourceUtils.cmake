macro(target_add_grouped_source target folder)
    file(GLOB GROUPED_SRCS RELATIVE ${CMAKE_SOURCE_DIR}
        ${SRCDIR}/${folder}/*.cpp
        ${SRCDIR}/${folder}/*.h
        ${SRCDIR}/${folder}/*.hpp
    )
    target_sources(${target} PRIVATE ${GROUPED_SRCS})
    string(REGEX REPLACE "/" "\\\\" folder_backslash ${folder})
    source_group(${folder_backslash} FILES ${GROUPED_SRCS})
endmacro()

macro(target_add_grouped_script target folder)
    if(EXISTS "${DATADIR}/")
        file(GLOB GROUPED_SRCS RELATIVE ${CMAKE_SOURCE_DIR}
            ${DATADIR}/${folder}/*.as
        )
        target_sources(${target} PRIVATE ${GROUPED_SRCS})
        string(REGEX REPLACE "/" "\\\\" folder_backslash ${folder})
        source_group(${folder_backslash} FILES ${GROUPED_SRCS})
    endif()
endmacro()

macro(target_add_grouped_shaders target folder)
    if(EXISTS "${DATADIR}/")
        file(GLOB GROUPED_SRCS RELATIVE ${CMAKE_SOURCE_DIR}
            ${DATADIR}/${folder}/*.glsl
            ${DATADIR}/${folder}/*.vert
            ${DATADIR}/${folder}/*.frag
        )
        target_sources(${target} PRIVATE ${GROUPED_SRCS})
        string(REGEX REPLACE "/" "\\\\" folder_backslash ${folder})
        source_group(${folder_backslash} FILES ${GROUPED_SRCS})
    endif()
endmacro()