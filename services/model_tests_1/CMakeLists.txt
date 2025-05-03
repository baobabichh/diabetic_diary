cmake_minimum_required(VERSION 3.10)

project(model_tests_1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(MYLIBRARY_PATH "${CMAKE_SOURCE_DIR}/../../libs/")
set(THIRDLIBRARY_PATH "${CMAKE_SOURCE_DIR}/../../third_party_libs/")

add_executable(model_tests_1 model_tests_1.cpp)

target_include_directories(model_tests_1 PRIVATE
    ${MYLIBRARY_PATH}/
    ${THIRDLIBRARY_PATH}/json/include/
)

target_link_libraries(model_tests_1 PRIVATE
    ${MYLIBRARY_PATH}/build/libmysharedlib.so
)

add_executable(model_tests_do_real model_tests_do_real.cpp)

target_include_directories(model_tests_do_real PRIVATE
    ${MYLIBRARY_PATH}/
    ${THIRDLIBRARY_PATH}/json/include/
)

target_link_libraries(model_tests_do_real PRIVATE
    ${MYLIBRARY_PATH}/build/libmysharedlib.so
)

add_executable(model_tests_do_stats model_tests_do_stats.cpp)

target_include_directories(model_tests_do_stats PRIVATE
    ${MYLIBRARY_PATH}/
    ${THIRDLIBRARY_PATH}/json/include/
)

target_link_libraries(model_tests_do_stats PRIVATE
    ${MYLIBRARY_PATH}/build/libmysharedlib.so
)