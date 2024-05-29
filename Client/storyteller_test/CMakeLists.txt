cmake_minimum_required(VERSION 3.19)

add_executable(storyteller_test storyteller_test.cpp)

target_link_libraries(storyteller_test chronolog_client thallium)

configure_file(${CMAKE_SOURCE_DIR}/default_conf.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/default_conf.json COPYONLY)

target_link_options(storyteller_test PRIVATE -Wl,-rpath,${CMAKE_INSTALL_PREFIX}/lib)

# install binary
install(
    TARGETS storyteller_test DESTINATION bin
)

# install configuration file
install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../default_conf.json.in DESTINATION conf RENAME default_conf.json
)