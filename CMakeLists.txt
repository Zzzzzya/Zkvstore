cmake_minimum_required(VERSION 3.20)

project(ZKVSTORE)

SET(CMAKE_CXX_STANDARD 20)

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB SRC_LIST 
${PROJECT_SOURCE_DIR}/src/*.cpp
${PROJECT_SOURCE_DIR}/src/*.cc)

add_executable(app ${SRC_LIST})