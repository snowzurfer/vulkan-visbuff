# Auth: Alberto Taiuti

cmake_minimum_required(VERSION 2.8)

project(songho-timer)

# Export commands for YCM
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(songhotimer ${CMAKE_CURRENT_SOURCE_DIR}/src/Timer.cpp)

set(SONGHO_LIBRARY songhotimer PARENT_SCOPE)
