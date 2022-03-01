project(Serde)

add_library(Serde STATIC include/MessageSerDe.h include/SerDe.h src/SerDe.cpp)
target_include_directories(Serde PUBLIC include ${cereal_SOURCE_DIR}/include)
add_dependencies(Serde cereal)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()