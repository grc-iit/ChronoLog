#include <gtest/gtest.h>
#include "StoryChunk.h"   

using chronolog::StoryChunk;
TEST(StoryChunkTest, DefaultCtor_IsEmpty) {
  StoryChunk chunk;   
  EXPECT_TRUE(chunk.empty());
}