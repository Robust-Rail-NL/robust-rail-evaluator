cmake_minimum_required(VERSION 3.11)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# set the project name
project(cTORSTest)

include(FetchContent)

# Verbose.
set(CMAKE_VERBOSE_MAKEFILE on)

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/../bin)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/../bin)

## import the test library
FetchContent_Declare(doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG v2.4.11)

FetchContent_GetProperties(doctest)
if(NOT doctest)
  FetchContent_Populate(doctest)
  add_subdirectory(${doctest_SOURCE_DIR} ${doctest_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

link_libraries(cTORS doctest::doctest)

add_library(MainTest OBJECT MainTest.cpp)

add_executable(TrackTest TrackTest.cpp $<TARGET_OBJECTS:MainTest>)
add_test(NAME TrackTest COMMAND TrackTest)

add_executable(VehicleTest VehicleTest.cpp $<TARGET_OBJECTS:MainTest>)
add_test(NAME VehicleTest COMMAND VehicleTest)

add_executable(RulesTest RulesTest.cpp $<TARGET_OBJECTS:MainTest>)
add_test(NAME RulesTest COMMAND RulesTest)

add_executable(EngineTest EngineTest.cpp $<TARGET_OBJECTS:MainTest>)
add_test(NAME EngineTest COMMAND EngineTest)

add_executable(CompatibilityTest CompatibilityTest.cpp $<TARGET_OBJECTS:MainTest>)
add_test(NAME CompatibilityTest COMMAND CompatibilityTest)

add_executable(SchemaVersionTest SchemaVersionTest.cpp $<TARGET_OBJECTS:MainTest>)
add_test(NAME SchemaVersionTest COMMAND SchemaVersionTest)


file(COPY "../data" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

