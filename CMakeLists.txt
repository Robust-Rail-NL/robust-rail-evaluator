cmake_minimum_required(VERSION 3.11)

# set the project name
project(TORS VERSION 2.0.0)
# CMake's project(VERSION ...) is numeric-only and rejects prerelease suffixes
# (e.g. "-alpha.1"); track any such suffix here instead. Empty for stable
# releases. bump-version.sh keeps this and the project() version in sync.
set(TORS_VERSION_SUFFIX "rc.3")

# Verbose.
#set(CMAKE_VERBOSE_MAKEFILE on)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TORS_BuildTests "Build the unit tests when BUILD_TESTING is enabled." ON)
option(TORS_Install "Install CMake targets during install step." ON)


##
## TESTS
## create and configure the unit test target
##
include(CTest) #adds option BUILD_TESTING (default ON)
set(JSON_BuildTests OFF CACHE INTERNAL "")

if(BUILD_TESTING AND TORS_BuildTests)
    enable_testing()
    add_subdirectory(cTORSTest)
endif()

add_subdirectory(cTORS)
add_subdirectory(pyTORS)

add_executable(TORS cTORS/src/main.cpp)
target_link_libraries(TORS PUBLIC cTORS)
