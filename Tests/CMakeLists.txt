# GoogleTest settings
set(BUILD_GMOCK OFF)
set(INSTALL_GTEST OFF)
set(gtest_force_shared_crt ON)
ADD_SUBDIRECTORY(../Libraries/googletest-1.11.0 googletest)

set(SRCDIR "${CMAKE_SOURCE_DIR}/../Source")

set(TEST_SRCS
    ${SRCDIR}/Graphics/atlasnodetree.h
    ${SRCDIR}/Graphics/atlasnodetree.cpp
    ${SRCDIR}/Internal/checksum.h
    ${SRCDIR}/Internal/checksum.cpp
    ${SRCDIR}/Internal/common.h
    ${SRCDIR}/Internal/common.cpp
    ${SRCDIR}/Internal/datemodified.h
    ${SRCDIR}/Internal/datemodified.cpp
    ${SRCDIR}/Internal/filesystem.h
    ${SRCDIR}/Internal/filesystem.cpp
    ${SRCDIR}/Internal/locale.h
    ${SRCDIR}/Internal/locale.cpp
    ${SRCDIR}/Internal/modid.h
    ${SRCDIR}/Internal/modid.cpp
    #${SRCDIR}/Internal/modloading.cpp
    ${SRCDIR}/Internal/path.h
    ${SRCDIR}/Internal/path.cpp
    ${SRCDIR}/Memory/allocation.h
    ${SRCDIR}/Memory/allocation.cpp
    ${SRCDIR}/Memory/stack_allocator.h
    ${SRCDIR}/Memory/stack_allocator.cpp
    ${SRCDIR}/Threading/thread_sanity.h
    ${SRCDIR}/Threading/thread_sanity.cpp
    ${SRCDIR}/Utility/serialize.h
    ${SRCDIR}/Utility/serialize.cpp
    ${SRCDIR}/Utility/strings.h
    ${SRCDIR}/Utility/strings.cpp
    ${SRCDIR}/Version/git_version.cpp
    ${SRCDIR}/Version/version.h
    ${SRCDIR}/Version/version.cpp)

add_executable(OvergrowthTests
    globals.cpp
    # Tests
    atlas_test.cpp
    #modloading_test.cpp
    string_test.cpp
    vec2_test.cpp
    vec3_test.cpp
    vec4_test.cpp
    ${TEST_SRCS})

target_link_libraries(OvergrowthTests 
    gtest gtest_main 
    OvergrowthCompat
    OvergrowthLogging

    tinyxml
    trex
    glad)

target_compile_definitions(OvergrowthTests PRIVATE NO_ERR=1 OG_TESTING)

include(GoogleTest)
gtest_discover_tests(OvergrowthTests)