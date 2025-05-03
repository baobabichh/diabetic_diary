cmake_minimum_required(VERSION 3.10)
project(mysharedlib)

find_package(CURL REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MYLIBRARY_PATH "${CMAKE_SOURCE_DIR}")
set(THIRDLIBRARY_PATH "${CMAKE_SOURCE_DIR}/../third_party_libs/")

set(SOURCES
    functions.cpp
    functions.hpp
    gemini.cpp
    gemini.hpp
    openai.cpp
    openai.hpp
)

add_library(mysharedlib SHARED ${SOURCES})

target_include_directories(mysharedlib 
    PUBLIC 
        include
        ${CURL_INCLUDE_DIRS}
        $(MYLIBRARY_PATH)
        ${THIRDLIBRARY_PATH}/json/include/
)

target_link_libraries(mysharedlib PUBLIC CURL::libcurl)

set_target_properties(mysharedlib PROPERTIES OUTPUT_NAME "mysharedlib")