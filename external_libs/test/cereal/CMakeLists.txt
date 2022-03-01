add_executable(cereal_test cereal_test.cpp)
target_include_directories(cereal_test PRIVATE ${cereal_SOURCE_DIR}/include)
add_dependencies(cereal_test cereal)
add_test(NAME cereal_test COMMAND cereal_test)