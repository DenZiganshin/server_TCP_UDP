cmake_minimum_required(VERSION 2.8)

project(test_task)

set ( SOURCES_CLI
    ${PROJECT_SOURCE_DIR}/main_cli.cpp
    ${PROJECT_SOURCE_DIR}/src/client.cpp
)

set ( SOURCES_SER
    ${PROJECT_SOURCE_DIR}/main_ser.cpp
    ${PROJECT_SOURCE_DIR}/src/server.cpp
)

set ( HEADERS_CLI
      ${PROJECT_SOURCE_DIR}/include/client.h
)

set ( HEADERS_SER
      ${PROJECT_SOURCE_DIR}/include/server.h
)

include_directories(${PROJECT_SOURCE_DIR}
                    ${PROJECT_SOURCE_DIR}/include)

SET(CMAKE_CXX_FLAGS  "-Wall -std=c++11")


add_executable("client" ${SOURCES_CLI} ${HEADERS_CLI})
add_executable("server" ${SOURCES_SER} ${HEADERS_SER})
target_link_libraries("server" pthread)
target_link_libraries("client" pthread)
