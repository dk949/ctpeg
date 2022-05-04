cmake_minimum_required(VERSION 3.15)
project(CTPEG CXX)
set(CMAKE_CXX_STANDARD 20)
include(props.cmake)
option(INCLUDE_TESTS "Compile tests as well as teh example" NO)

add_executable(mathexpr example/example.cpp)
set_props(mathexpr)

if(INCLUDE_TESTS)
    add_subdirectory(testing)
endif()


