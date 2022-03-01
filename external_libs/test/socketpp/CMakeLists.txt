message(STATUS "socketpp: ${socketpp_SOURCE_DIR}")

add_executable(echo_server echo_server.cpp)
#target_include_directories(echo_server PRIVATE ${socketpp_SOURCE_DIR})
target_link_libraries(echo_server socketpp pthread)
add_dependencies(echo_server socketpp)
add_test(NAME echo_server COMMAND echo_server)

add_executable(echo_client echo_client.cpp)
#target_include_directories(echo_client PRIVATE ${socketpp_SOURCE_DIR})
target_link_libraries(echo_client socketpp pthread)
add_dependencies(echo_client socketpp)
add_test(NAME echo_client COMMAND echo_server)