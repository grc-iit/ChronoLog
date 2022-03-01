add_executable(rocksdb_test_mp rocksDB_test_mp.cpp)
target_include_directories(rocksdb_test_mp PUBLIC ${ROCKDSB_INCLUDE})
target_link_libraries(rocksdb_test_mp ${ROCKSDB_LIB} dl MPI::MPI_CXX)

add_executable(rocksdb_test_mt rocksDB_test_mt.cpp)
target_include_directories(rocksdb_test_mt PRIVATE ${ROCKDSB_INCLUDE})
target_link_libraries(rocksdb_test_mt ${ROCKSDB_LIB} dl pthread)