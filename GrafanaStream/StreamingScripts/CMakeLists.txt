cmake_minimum_required(VERSION 3.16)

add_library(grafana_stream STATIC
  InfluxDBSink.cpp
  Transform.cpp
)

set(CHRONOLOG_GEN_INCLUDE_DIR "${CMAKE_BINARY_DIR}/Client/include")
set(CHRONOLOG_SRC_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/Client/include")
target_include_directories(grafana_stream
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}  
    ${CHRONOLOG_GEN_INCLUDE_DIR}   
    ${CHRONOLOG_SRC_INCLUDE_DIR} 
)

find_package(CURL REQUIRED)
target_link_libraries(grafana_stream PUBLIC CURL::libcurl)
target_compile_features(grafana_stream PUBLIC cxx_std_17)
