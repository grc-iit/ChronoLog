cmake_minimum_required(VERSION 3.19)

project(ChronoKeeper)

message("Building CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(keeper_registry)
#KeeperRegistryInstance.cpp)
target_include_directories(keeper_registry PRIVATE include)
target_sources( keeper_registry PRIVATE KeeperRegistryInstance.cpp )
target_link_libraries(keeper_registry thallium)
message("buidl target : keeper_registry")

add_executable(chrono_keeper)
target_include_directories(chrono_keeper PRIVATE include ../chrono_common ../ChronoAPI/ChronoLog/include)
target_sources(chrono_keeper PRIVATE ChronoKeeperInstance.cpp StoryPipeline.cpp StoryChunk.cpp KeeperDataStore.cpp StoryChunkExtractor.cpp CSVFileChunkExtractor.cpp)
target_link_libraries(chrono_keeper thallium)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../default_conf.json.in
        ${CMAKE_CURRENT_BINARY_DIR}/default_conf.json COPYONLY)
message("build target : chrono_keeper")

<<<<<<< HEAD
add_subdirectory(storyteller_client)
=======
set_target_properties(chrono_keeper PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

# install binary
install(
        TARGETS chrono_keeper DESTINATION bin
)

# install configuration file
install(
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/../default_conf.json.in DESTINATION conf RENAME default_conf.json
)
>>>>>>> 2398801f427786d5ef9f35c8ae47efa9bad3ea5a
