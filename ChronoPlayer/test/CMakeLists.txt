cmake_minimum_required(VERSION 3.25)

find_package(Thallium REQUIRED)
find_package(spdlog REQUIRED)
find_package(HDF5 REQUIRED COMPONENTS C CXX)

message("Building CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
message("build target : hdf5_archive_reader_test")

add_executable(hdf5_archive_reader_test
    test/HDF5ArchiveReadingAgentTest.cpp
    src/HDF5ArchiveReadingAgent.cpp
    ${CMAKE_SOURCE_DIR}/chrono_common/StoryChunk.cpp)

target_include_directories(hdf5_archive_reader_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/chrono_common/include
    ${CMAKE_SOURCE_DIR}/chrono_common/src
    ${CMAKE_SOURCE_DIR}/ChronoAPI/ChronoLog/include)

target_link_directories(hdf5_archive_reader_test PRIVATE ${HDF5_LIBRARY_DIRS})

target_link_libraries(hdf5_archive_reader_test chronolog_client thallium)
target_link_libraries(hdf5_archive_reader_test ${HDF5_LIBRARIES})
