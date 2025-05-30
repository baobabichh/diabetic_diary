cmake_minimum_required(VERSION 3.10)

project(ai_requester_service)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(MYLIBRARY_PATH "${CMAKE_SOURCE_DIR}/../../libs/")
set(THIRDLIBRARY_PATH "${CMAKE_SOURCE_DIR}/../../third_party_libs/")

message(alarm THIRDLIBRARY_PATH=${THIRDLIBRARY_PATH})

set(MySQLConnectorCpp_INCLUDE_DIR "/usr/include/cppconn")
set(MySQLConnectorCpp_LIBRARIES "/usr/lib/x86_64-linux-gnu/libmysqlcppconn.so")

add_executable(ai_requester_service ai_requester_service.cpp)

target_include_directories(ai_requester_service PRIVATE
    ${MYLIBRARY_PATH}/
    ${THIRDLIBRARY_PATH}/json/include/
    ${THIRDLIBRARY_PATH}/rabbitmq-c/include/
    ${THIRDLIBRARY_PATH}/SimpleAmqpClient/src/
    ${MYSQL_INCLUDE_DIR}
    ${MYSQLCONNECTORCPP_INCLUDE_DIR}
    ${MySQLConnectorCpp_INCLUDE_DIR}
)

target_link_libraries(ai_requester_service PRIVATE
    ${THIRDLIBRARY_PATH}/rabbitmq-c/build/librabbitmq/librabbitmq.so
    ${THIRDLIBRARY_PATH}/SimpleAmqpClient/build/libSimpleAmqpClient.so
    ${MYLIBRARY_PATH}/build/libmysharedlib.so
    ${MySQLConnectorCpp_LIBRARIES}
)