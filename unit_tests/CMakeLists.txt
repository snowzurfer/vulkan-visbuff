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

enable_testing()

# Set headers and sources
set(CATCH_MAIN_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/catch_main.cpp)

#
# add_catch_test(<target> <sources>...)
#
function(add_catch_test TARGET)
  add_executable(${TARGET} ${CATCH_MAIN_SOURCE} ${ARGN})
  target_include_directories(${TARGET} PUBLIC
    ${CATCH_SOURCE_DIR}/single_include)
    
  add_test(${TARGET} ${TARGET})

  add_custom_command(TARGET ${TARGET}
    POST_BUILD
    COMMAND ${TARGET}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running ${TARGET}" VERBATIM)
endfunction()

set(SPV_ASSETS_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/sample_spv_modules)

add_catch_test(test_0 test_0.cpp)
target_include_directories(test_0 PUBLIC
  ${SUT_SOURCE_DIR}/include
  ${SPIRV-HEADERS_SOURCE_DIR}/include)
target_link_libraries(test_0
  sut)
target_compile_definitions(test_0
  PUBLIC SPV_ASSETS_FOLDER=${SPV_ASSETS_FOLDER})
