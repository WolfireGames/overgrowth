ADD_SUBDIRECTORY(../Libraries/googletest-1.11.0 googletest)

set(SRCDIR "${CMAKE_SOURCE_DIR}/../Source")

set(TEST_SRCS
    ${SRCDIR}/Utility/serialize.h
    ${SRCDIR}/Utility/serialize.cpp
    ${SRCDIR}/Utility/strings.h
    ${SRCDIR}/Utility/strings.cpp)

add_executable(OvergrowthTests 
    string_test.cpp
    ${TEST_SRCS})

target_link_libraries(OvergrowthTests gtest gtest_main OvergrowthLogging)

include(GoogleTest)
gtest_discover_tests(OvergrowthTests)