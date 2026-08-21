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

add_executable(PlanConversionTest PlanConversionTest.cpp $<TARGET_OBJECTS:MainTest>)
add_test(NAME PlanConversionTest COMMAND PlanConversionTest)

add_executable(EngineTest EngineTest.cpp $<TARGET_OBJECTS:MainTest>)
add_test(NAME EngineTest COMMAND EngineTest)

add_executable(CompatibilityTest CompatibilityTest.cpp $<TARGET_OBJECTS:MainTest>)
add_test(NAME CompatibilityTest COMMAND CompatibilityTest)

add_executable(SchemaVersionTest SchemaVersionTest.cpp $<TARGET_OBJECTS:MainTest>)
add_test(NAME SchemaVersionTest COMMAND SchemaVersionTest)


# Fixtures are read straight from the source tree via an absolute path, rather
# than copied into the build. file(COPY), which is what this did before, runs at
# configure time only: editing a fixture and rebuilding silently tested the
# previous copy, and it duplicated 12MB on every configure.
#
# Passing the path in rather than setting WORKING_DIRECTORY to the source root
# is deliberate. Both fix the staleness, but the working-directory version also
# sends anything a test writes to a relative path into the source tree. Tests
# still run in the binary dir, so stray output lands in build/ where it belongs.
set(TORS_TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
foreach(_test TrackTest VehicleTest RulesTest PlanConversionTest EngineTest
              CompatibilityTest SchemaVersionTest)
    target_compile_definitions(${_test} PRIVATE TORS_DATA_DIR="${TORS_TEST_DATA_DIR}")
endforeach()

# CompatibilityTest's HIP fixture is the same Kleine Binckhorst data as the
# repo's top-level example (examples_kleine_binckhorst/) - reused directly
# rather than duplicated under fixtures/, so there is exactly one copy.
set(TORS_EXAMPLE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../examples_kleine_binckhorst")
target_compile_definitions(CompatibilityTest PRIVATE TORS_EXAMPLE_DIR="${TORS_EXAMPLE_DIR}")

