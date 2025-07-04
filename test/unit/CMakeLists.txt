cmake_minimum_required(VERSION 3.25)

add_executable(story_chunk_test StoryChunkTest.cpp)

target_link_libraries(story_chunk_test
  PRIVATE
    GTest::gtest_main
    chronolog_client
)

include(GoogleTest)
gtest_discover_tests(story_chunk_test)