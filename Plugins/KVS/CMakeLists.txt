
include_directories("../emu/include")
include_directories("include")

add_executable(KeyValueStore srcs/clientservice.cpp srcs/KeyValueStore.cpp srcs/KeyValueStoreIO.cpp srcs/util.cpp ../emu/srcs/city.cpp)
if(PARALLEL)
target_link_libraries(KeyValueStore ${MPI_CXX_LIBRARIES})
endif()
target_link_libraries(KeyValueStore thallium ${HDF5_C_LIBRARIES} -L${HDF5_VOL_ASYNC}/lib -lasynchdf5 -lh5async ${Boost_LIBRARIES} -lpthread -lrt ${PROJECT_NAME} -lm)
set_target_properties(KeyValueStore PROPERTIES FOLDER KeyValueStore)
