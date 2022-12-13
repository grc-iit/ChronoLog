add_subdirectory(cereal)
add_subdirectory(socketpp)
add_subdirectory(hcl)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
