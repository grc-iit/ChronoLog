
add_executable(ClientAdmin client_admin.cpp)

target_include_directories(ClientAdmin PRIVATE ../Client/include
        ../ChronoAPI/ChronoLog/include
        ../ChronoVisor/include)
target_link_libraries(ClientAdmin ChronoLogClient_lib)
target_link_libraries(ClientAdmin thallium)

