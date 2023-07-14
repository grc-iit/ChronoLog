project(Chronovisor)

#add_library(ChronoVisor_lib STATIC
#        include/ChronoVisorServer.h include/ClientRegistryInfo.h
#        include/SerVector.h include/TimeRecord.h
#        src/ChronoVisorServer.cpp src/ClientRegistryRecord.cpp
#        src/TimeRecord.cpp
#        ../ChronoAPI/ChronoLog/src/TimeManager.cpp ../ChronoAPI/ChronoLog/include/TimeManager.h
#        ../ChronoAPI/ChronoLog/src/ClocksourceManager.cpp ../ChronoAPI/ChronoLog/include/ClocksourceManager.h
#        src/ChronicleMetaDirectory.cpp include/ChronicleMetaDirectory.h
#        include/Chronicle.h include/Story.h include/Archive.h include/Event.h
#        ../ChronoAPI/ChronoLog/src/city.cpp ../ChronoAPI/ChronoLog/include/city.h ../ChronoAPI/ChronoLog/include/config.h include/ClientRegistryManager.h src/ClientRegistryManager.cpp)
#target_include_directories(ChronoVisor_lib PUBLIC include
#        ../ChronoAPI/ChronoLog/include
#        ${ROCKSDB_INCLUDE}) #-I/usr/include
#target_link_libraries(ChronoVisor_lib socketpp Serde ${ROCKSDB_LIB} pthread) #_L/usr/bin/mpi.so
#add_dependencies(ChronoVisor_lib socketpp Serde)

#add_executable(ChronoVisor main.cpp include/ClientRegistryManager.h src/ClientInfo.cpp include/ClientInfo.h)
#target_link_libraries(ChronoVisor ChronoVisor_lib socketpp)
#add_dependencies(ChronoVisor ChronoVisor_lib socketpp)

# For new RPC-based ChronoVisor implementation
find_package(mercury REQUIRED)
find_package(thallium REQUIRED)
find_package(Threads REQUIRED)

<<<<<<< HEAD
add_library(ChronoVisor_lib2 STATIC
        include/ChronoVisorServer2.h include/ClientInfo.h
        src/ChronoVisorServer2.cpp src/ClientRegistryRecord.cpp
        ../ChronoAPI/ChronoLog/src/ClocksourceManager.cpp ../ChronoAPI/ChronoLog/include/ClocksourceManager.h
        src/ChronicleMetaDirectory.cpp include/ChronicleMetaDirectory.h
        include/Chronicle.h include/Story.h include/Archive.h include/Event.h
        ../ChronoAPI/ChronoLog/src/city.cpp ../ChronoAPI/ChronoLog/include/city.h ../ChronoAPI/ChronoLog/include/config.h include/ClientRegistryManager.h src/ClientRegistryManager.cpp)
target_include_directories(ChronoVisor_lib2 PUBLIC include
        ../ChronoAPI/ChronoLog/include)
target_link_libraries(ChronoVisor_lib2 thallium)
=======
add_executable(chronovisor_server)

target_sources ( chronovisor_server PRIVATE
	./src/chronovisor_instance.cpp
    ./src/VisorClientPortal.cpp
	#../ChronoAPI/ChronoLog/src/ClocksourceManager.cpp
        ./src/ClientRegistryManager.cpp
        ./src/ClientRegistryRecord.cpp
        ./src/ChronicleMetaDirectory.cpp
	./src/KeeperRegistry.cpp
        ../ChronoAPI/ChronoLog/src/city.cpp)

target_include_directories(chronovisor_server PRIVATE
        ../ChronoAPI/ChronoLog/include
	../chrono_common
        ./include)

target_link_libraries(chronovisor_server chronolog_client thallium)

# prep the stage...
configure_file(${CMAKE_SOURCE_DIR}/default_conf.json.in
        ${CMAKE_CURRENT_BINARY_DIR}/default_conf.json COPYONLY)
add_test(NAME ChronoVisorServerTest COMMAND chronovisor_server)
>>>>>>> 105df6b6cebb07312cf919ade4bb90c617fa51f4

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
