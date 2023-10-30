set(client_examples client_lib_connect_rpc_test client_lib_metadata_rpc_test client_lib_multi_argobots_test client_lib_multi_pthread_test client_lib_multi_storytellers)

add_custom_target(copyserverlist)

add_custom_command(
        TARGET copyserverlist
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/../server_list.in
        ${CMAKE_CURRENT_BINARY_DIR}/server_list)


foreach(client ${client_examples})
    add_executable(${client} ${client}.cpp)
    add_dependencies(${client} copyserverlist)
    target_include_directories(${client} PRIVATE ../Client/include)
    target_link_libraries(${client} ChronoLogClient_lib -lpthread -lrt)
    #add_test(NAME ${client} COMMAND ${client})
endforeach()

set(client_openmp client_lib_multi_openmp_test)

foreach(client ${client_openmp})
<<<<<<< HEAD
   add_executable(${client} ${client}.cpp)
   add_dependencies(${client} copyserverlist)
   target_include_directories(${client} PRIVATE ../Client/include)
   target_link_libraries(${client} ChronoLogClient_lib OpenMP::OpenMP_CXX)
endforeach()   
=======
    add_executable(${client} ${client}.cpp)
    add_dependencies(${client} copyserverlist)
    target_include_directories(${client} PRIVATE ../Client/include)
    target_link_libraries(${client} chronolog_client OpenMP::OpenMP_CXX)
endforeach()
>>>>>>> 2398801f427786d5ef9f35c8ae47efa9bad3ea5a

set(client_mpi client_lib_hybrid_argobots_test)

find_package(MPI)
foreach(client ${client_mpi})
<<<<<<< HEAD
	add_executable(${client} ${client}.cpp)
	add_dependencies(${client} copyserverlist)
	target_include_directories(${client} PRIVATE ../Client/include ${MPI_CXX_INCLUDE_DIRS})
	target_link_libraries(${client} ChronoLogClient_lib ${MPI_CXX_LIBRARIES} -lpthread -lrt)
=======
    add_executable(${client} ${client}.cpp)
    add_dependencies(${client} copyserverlist)
    target_include_directories(${client} PRIVATE ../Client/include ${MPI_CXX_INCLUDE_DIRS})
    target_link_libraries(${client} chronolog_client ${MPI_CXX_LIBRARIES} -lpthread -lrt)
>>>>>>> 2398801f427786d5ef9f35c8ae47efa9bad3ea5a
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../default_conf.json.in
            ${CMAKE_CURRENT_BINARY_DIR}/default_conf.json COPYONLY)
endforeach()

# install binaries
install(
        TARGETS ${client_examples} DESTINATION bin
)

# install configuration file
install(
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/../default_conf.json.in DESTINATION conf RENAME default_conf.json
)