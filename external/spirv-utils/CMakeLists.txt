#  MIT License

#  Copyright (c) 2017 Alberto Taiuti

#  Permission is hereby granted, free of charge, to any person obtaining a copy
#  of this software and associated documentation files (the "Software"), to deal
#  in the Software without restriction, including without limitation the rights
#  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#  copies of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions:

#  The above copyright notice and this permission notice shall be included in all
#  copies or substantial portions of the Software.

#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#  SOFTWARE.

cmake_minimum_required(VERSION 2.8)

project(spirv-utils)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Build mode: debug")
else()
  message(STATUS "Build mode: release")
endif (CMAKE_BUILD_TYPE STREQUAL "Debug")

# Export commands for YCM
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# Set custom cmake modules path
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

# Set default cmake build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo\
MinSizeRel. Default is Debug."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if(MSVC)
  # Force to always compile with W4
  if(CMAKE_CXX_FLAGS_DEBUG MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS_DEBUG
           "${CMAKE_CXX_FLAGS_DEBUG}")
  else()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W4 /EHsc")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
   CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # Update if necessary
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra")
endif()

if(UNIX)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Color error output
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11 -stdlib=libc++")
  endif()
endif()

message(STATUS "Compiler custom flags are: ${CMAKE_CXX_FLAGS}")
message(STATUS "Compiler debug flags are: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "Compiler release flags are: ${CMAKE_CXX_FLAGS_RELEASE}")

# Setup handy variables
set(SUT_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}")
set(SPIRV-HEADERS_SOURCE_DIR 
    "${CMAKE_CURRENT_SOURCE_DIR}/external/SPIRV-Headers")
set(CATCH_SOURCE_DIR 
    "${CMAKE_CURRENT_SOURCE_DIR}/external/catch")

# Add external projects
add_subdirectory(${SPIRV-HEADERS_SOURCE_DIR})

# Set headers and sources
set(SUT_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/include/spv_utils.h)

set(SUT_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/source/spv_utils.cpp)

# Create library
add_library(sut
  ${SUT_HEADERS}
  ${SUT_SOURCES})

# Set include directories for targets
target_include_directories(sut PUBLIC
  ${SUT_SOURCE_DIR}/include
  ${SPIRV-HEADERS_SOURCE_DIR}/include)

option(SUT_BUILD_TESTS "Build the tests" OFF)

if(SUT_BUILD_TESTS)
  add_subdirectory(unit_tests)
endif(SUT_BUILD_TESTS)
