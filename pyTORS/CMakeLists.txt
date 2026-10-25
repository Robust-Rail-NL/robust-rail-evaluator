cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# set the project name
project(pyTORS)

include(FetchContent)

FetchContent_Declare(pybind
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG v3.1.0)
FetchContent_MakeAvailable(pybind)

pybind11_add_module(pyTORS MODULE module.cpp docstrings.h)
target_link_libraries(pyTORS PRIVATE cTORS)

#Find Python
set(Python_FIND_STRATEGY LOCATION)
set(Python_FIND_VIRTUALENV FIRST)
find_package(Python COMPONENTS Interpreter Development)

# Add target to generate pyi file, 
if(Python_FOUND)
  add_custom_target(pyTORS_pyi ALL
  COMMAND ${Python_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/generate_docs.py" ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
  add_dependencies(pyTORS_pyi pyTORS)
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_link_libraries(pyTORS PRIVATE pybind11::module)
    add_library(restore_default_visibility INTERFACE)
    target_compile_options(restore_default_visibility INTERFACE -fvisibility=default)
    target_link_libraries(pyTORS PRIVATE restore_default_visibility)
endif()

if(Python_FOUND)
  execute_process ( COMMAND ${Python_EXECUTABLE} -c "import sysconfig; print(sysconfig.get_path('purelib'))" OUTPUT_VARIABLE PYTHON_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE)
  message(STATUS "Python found. Intalling to ${PYTHON_SITE_PACKAGES}")
  install(TARGETS pyTORS
    COMPONENT python
    LIBRARY DESTINATION "${PYTHON_SITE_PACKAGES}/pyTORS/")
  file(GLOB stub_files RELATIVE "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/*.pyi")
  list(TRANSFORM stub_files PREPEND "${CMAKE_CURRENT_BINARY_DIR}/")
  install(FILES ${stub_files} COMPONENT python DESTINATION "${PYTHON_SITE_PACKAGES}/pyTORS/")
else()
  message(WARNING "Python not found. Python library not installed.")
endif()


  
