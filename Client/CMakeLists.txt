project(ChronoLogClient)

set(CMAKE_CXX_STANDARD 17)

#set(CMAKE_FIND_DEBUG_MODE TRUE)

find_package(mercury REQUIRED)
find_package(thallium REQUIRED)

<<<<<<< HEAD
#set(CMAKE_FIND_DEBUG_MODE FALSE)

add_library(ChronoLogClient_lib
        src/client.cpp
        include/client.h
        include/RPCClient.h
        ../ChronoAPI/ChronoLog/include/macro.h
        ../ChronoAPI/ChronoLog/include/rpc.h
        ../ChronoAPI/ChronoLog/include/RPCFactory.h
        ../ChronoAPI/ChronoLog/include/singleton.h
        ../ChronoAPI/ChronoLog/src/city.cpp
        ../ChronoAPI/ChronoLog/include/city.h)
target_include_directories(ChronoLogClient_lib PUBLIC include
        ../ChronoAPI/ChronoLog/include
        ../Client/include)
target_link_libraries(ChronoLogClient_lib thallium)
=======
add_library(chronolog_client
        include/chronolog_client.h
        src/ChronologClientImpl.h
        src/KeeperRecordingClient.h
        src/StorytellerClient.h
        src/StorytellerClient.cpp
        src/ChronologClient.cpp
        src/ChronologClientImpl.cpp
)

#include/client.h
#include/RPCClient.h
#../ChronoAPI/ChronoLog/include/macro.h
#../ChronoAPI/ChronoLog/include/rpc.h
#../ChronoAPI/ChronoLog/include/RPCFactory.h
#../ChronoAPI/ChronoLog/include/singleton.h
#../ChronoAPI/ChronoLog/src/city.cpp
#../ChronoAPI/ChronoLog/include/city.h)

target_include_directories(chronolog_client PUBLIC include
        ../ChronoAPI/ChronoLog/include
        ../chrono_common/
        ../Client/include)

target_link_libraries(chronolog_client thallium)

add_subdirectory(storyteller_test)

# install library
install(
        TARGETS chronolog_client DESTINATION lib
)
>>>>>>> 2398801f427786d5ef9f35c8ae47efa9bad3ea5a
