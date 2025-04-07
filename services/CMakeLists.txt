cmake_minimum_required(VERSION 3.10)

project(ai_requester_service)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(MYLIBRARY_PATH "${CMAKE_SOURCE_DIR}/../libs/")
set(THIRDLIBRARY_PATH "${CMAKE_SOURCE_DIR}/../third_party_libs/")

add_executable(ai_requester_service ai_requester_service/ai_requester_service.cpp)

target_include_directories(ai_requester_service PRIVATE
    ${MYLIBRARY_PATH}/ai_requester_lib/include/
    ${THIRDLIBRARY_PATH}/json/include/
)

target_link_libraries(ai_requester_service PRIVATE
    ${MYLIBRARY_PATH}/ai_requester_lib/build/libai_requester_lib.so
)