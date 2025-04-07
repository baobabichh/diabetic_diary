cmake_minimum_required(VERSION 3.10)

project(ai_requester_lib VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(MYLIBRARY_PATH "${CMAKE_SOURCE_DIR}/../")
set(THIRDLIBRARY_PATH "${CMAKE_SOURCE_DIR}/../../third_party_libs/")

add_library(ai_requester_lib SHARED
    src/AIRequester.cpp
    include/AIRequester.hpp
)

target_include_directories(ai_requester_lib PUBLIC
    ${MYLIBRARY_PATH}/ai_requester_lib/include/
    ${THIRDLIBRARY_PATH}/json/include/
)
