cmake_minimum_required(VERSION 3.25)

find_package(Thallium REQUIRED)
find_package(spdlog REQUIRED)

#message("ChronologClient CMakeLists - CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}")
#message("ChronologClient CMakeLists - CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
message("Building CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
message("build target : chronolog_client")


add_library(chronolog_client
    include/chronolog_client.h
    include/ChronologClientImpl.h
    include/KeeperRecordingClient.h
    include/StorytellerClient.h
    src/StorytellerClient.cpp
    src/PlaybackQueryRpcClient.cpp
    src/ClientQueryService.cpp
    src/ChronologClient.cpp
    src/ChronologClientImpl.cpp
    ${CMAKE_SOURCE_DIR}/chrono_common/src/chrono_monitor.cpp
    ${CMAKE_SOURCE_DIR}/chrono_common/src/ConfigurationManager.cpp
    ${CMAKE_SOURCE_DIR}/chrono_common/src/StoryChunk.cpp
)

# Include directories for the library
target_include_directories(chronolog_client PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/chrono_common/include
    ${CMAKE_SOURCE_DIR}/chrono_common/src
    ${CMAKE_SOURCE_DIR}/Client/include
)

target_link_libraries(chronolog_client thallium)

################################

# Define the header files to be copied
set(PUBLIC_HEADER_FILES
    include/chronolog_client.h
    include/ClientConfiguration.h
    ${CMAKE_SOURCE_DIR}/chrono_common/include/ConfigurationManager.h
    ${CMAKE_SOURCE_DIR}/chrono_common/include/chronolog_errcode.h
)

# install library
install(
    TARGETS chronolog_client DESTINATION lib
)

# Install header files
install(
    FILES ${PUBLIC_HEADER_FILES} DESTINATION include
)