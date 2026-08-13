cmake_minimum_required(VERSION 3.5)
project(drogua_test CXX)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(${PROJECT_NAME}
    ${TEST_SOURCES}
)

# ##############################################################################
# If you include the drogon source code locally in your project, use this method
# to add drogon 
# target_link_libraries(${PROJECT_NAME} PRIVATE drogon)
#
# and comment out the following lines

#==
#target_include_directories(${PROJECT_NAME} PRIVATE
#    ${CMAKE_CURRENT_SOURCE_DIR}/..
#    ${luabridge3_SOURCE_DIR}/Source
#)

#target_link_libraries(${PROJECT_NAME} PRIVATE 
#    Drogon::Drogon
#    PkgConfig::LUA
#)
#==

target_link_libraries(${PROJECT_NAME} PRIVATE
    drogua_lua
)

# ##############################################################################
# Test runtime files
#
# Copy the files required by the tests into the build/test directory so that
# tests can be executed independently of the source directory.

file(COPY
    "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
)

file(COPY
    "${CMAKE_CURRENT_SOURCE_DIR}/lua"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
)

# ##############################################################################
# CTest
#
# Discover all DROGON_TEST(TestName) declarations and register each test
# individually with CTest.

set(DROGUA_TEST_NAMES)

foreach(TEST_SOURCE ${TEST_SOURCES})
    file(READ "${TEST_SOURCE}" TEST_SOURCE_CONTENT)

    string(REGEX MATCHALL
        "DROGON_TEST\\([A-Za-z_][A-Za-z0-9_]*"
        FOUND_TESTS
        "${TEST_SOURCE_CONTENT}"
    )

    foreach(FOUND_TEST ${FOUND_TESTS})
        string(REGEX REPLACE
            "DROGON_TEST\\("
            ""
            TEST_NAME
            "${FOUND_TEST}"
        )

        list(APPEND DROGUA_TEST_NAMES "${TEST_NAME}")
    endforeach()
endforeach()

# Remove duplicate test names.
list(REMOVE_DUPLICATES DROGUA_TEST_NAMES)

# Register every Drogon test as an individual CTest test.
foreach(TEST_NAME ${DROGUA_TEST_NAMES})
    add_test(
        NAME "${TEST_NAME}"
        COMMAND ${PROJECT_NAME} -r "${TEST_NAME}"
    )

    set_tests_properties("${TEST_NAME}" PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
endforeach()

# Do not use ParseAndAddDrogonTests() here because the tests above are
# registered individually with CTest.