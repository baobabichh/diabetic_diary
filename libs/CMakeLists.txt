cmake_minimum_required(VERSION 3.10)
project(mysharedlib)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    functions.cpp
    functions.hpp
)

add_library(mysharedlib STATIC ${SOURCES})

target_include_directories(mysharedlib PUBLIC include)

set_target_properties(mysharedlib PROPERTIES OUTPUT_NAME "mysharedlib")