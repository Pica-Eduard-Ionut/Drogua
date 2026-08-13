cmake_minimum_required(VERSION 3.5)
project(drogua_test CXX)

add_executable(${PROJECT_NAME}
    test_main.cc
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

ParseAndAddDrogonTests(${PROJECT_NAME})