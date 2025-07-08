#include "StoryChunk.h"
#include <gtest/gtest.h>

namespace chl = chronolog;

/* ----------------------------------
  Tests on the constructor
  ---------------------------------- */

TEST(StoryChunk_TestConstructors, testEmptyConstructor)
{
    chl::StoryChunk chunk;
    EXPECT_TRUE(chunk.empty());
}

TEST(StoryChunk_TestConstructors, testNormalCtorInit)
{
    int startTime = 100, endTime = 200;
    chl::StoryChunk chunk("ChronicleName", "StoryName", 1, startTime, endTime, 10);
    EXPECT_EQ(chunk.getStartTime(), startTime);
    EXPECT_EQ(chunk.getEndTime(), endTime);
}

// Test the constructor init when we have
// an endTime time less than the startTime time
// The init should be +5000 as per storychunk code
TEST(StoryChunk_TestConstructors, testBoundaryCtorInit)
{
    int startTime = 100, endTime = 50;
    chl::StoryChunk chunk("ChronicleName", "StoryName", 1, startTime, endTime, 10);
    EXPECT_EQ(chunk.getStartTime(), startTime);
    EXPECT_EQ(chunk.getEndTime(), startTime + 5000);
}


/* ----------------------------------
  Tests on insertevent()
  ---------------------------------- */

// Testing a normal insert event
TEST(StoryChunk_TestInsertEvent, testNormalInsertEvent)
{
    chl::StoryId storyId(1);
    int startTime = 100, endTime = 200;
    chl::StoryChunk chunk("ChronicleName", "StoryName", storyId, startTime, endTime, 10);

    int eventTime = 150, clientId = 2, eventIdx = 9;
    std::string record = "test";
    chl::LogEvent event(storyId, eventTime, clientId, eventIdx, record);
    chunk.insertEvent(event);

    EXPECT_EQ(chunk.getEventCount(), 1);

    std::vector<chl::Event> series;
    chunk.extractEventSeries(series);

    ASSERT_EQ(series.size(), 1);
    EXPECT_EQ(series[0].time(), eventTime);
    EXPECT_EQ(series[0].client_id(), clientId);
    EXPECT_EQ(series[0].index(), eventIdx);
    EXPECT_EQ(series[0].log_record(), record);
}

// Insert an event with the time as endTime time of the chunk
// it should get rejected
TEST(StoryChunk_TestInsertEvent, testInsertAtEndTime)
{
    chl::StoryId storyId(1);
    int startTime = 100, endTime = 200;
    chl::StoryChunk chunk("ChronicleName", "StoryName", storyId, startTime, endTime, 10);

    chl::LogEvent event(storyId, endTime, 1, 1, "test");
    chunk.insertEvent(event);

    EXPECT_EQ(chunk.getEventCount(), 0);
}

// Insert an event with the time before the chunk startTime time
// it should agian get rejected
TEST(StoryChunk_TestInsertEvent, testInsertBeforeStart)
{
    chl::StoryId storyId(1);
    int startTime = 100, endTime = 200;
    chl::StoryChunk chunk("ChronicleName", "StoryName", storyId, startTime, endTime, 10);

    chl::LogEvent event(storyId, startTime - 1, 1, 1, "test");
    chunk.insertEvent(event);

    EXPECT_EQ(chunk.getEventCount(), 0);
}

// try inserting an event at the exact same time as start time
// should be accepted
TEST(StoryChunk_TestInsertEvent, testInsertAtStartTime)
{
    // TBD
}

// Insert events out of order at different timestanps
// They should be still stored chronologically by their timestamp
TEST(StoryChunk_TestInsertEvent, testOutOfOrderInsert)
{
    chl::StoryId storyId(1);
    int startTime = 100, endTime = 200;
    chl::StoryChunk chunk("ChronicleName", "StoryName", storyId, startTime, endTime, 10);

    std::vector<int> timestamps = {180, 120, 150, 160, 199, 101};
    for(int t: timestamps)
    {
        chl::LogEvent ev(storyId, t, 1, 1, "test");
        EXPECT_EQ(chunk.insertEvent(ev), 1);
    }

    EXPECT_EQ(chunk.getEventCount(), timestamps.size());

    std::vector<chl::Event> series;
    chunk.extractEventSeries(series);
    ASSERT_EQ(series.size(), timestamps.size());

    std::vector<int> expected = {101, 120, 150, 160, 180, 199};
    for(int i = 0; i < expected.size(); ++i) EXPECT_EQ(series[i].time(), expected[i]);
}

// Insert events with a duplicate timestamp
// All shpuld be stored, no rejections cause of duplicate ts
TEST(StoryChunk_TestInsertEvent, testDuplicateTimestamp)
{
    // TBD
}

// Stress test insert events by inserting many events at once
// expect no failure and correct inserts
TEST(StoryChunk_TestInsertEvent, testStressInsert)
{
    // TBD
}

// Insert an event with a different story id as compared to the storychunk's storyid
// It should be rejected ideally
TEST(StoryChunk_TestInsertEvent, testIncorrectEventId)
{
    // TBD
}

// More TBD test cases for insert events:
// Insert chunksize+1 nb of events and check the behavior
// Leave the event record name as an empty string
// Insert a very large event record
// Test with the max chunk size as 0 or negative
// Maybe test with multiple threads inserting at the same time into the same story chunk?
// Maybe test one thread doing insert another doing extract?


/* ----------------------------------
  Tests on mergeEvents()
  ---------------------------------- */

/* ----------------------------------
  Tests on eraseEvents()
  ---------------------------------- */

/* ----------------------------------
  Tests on extractEventSeries()
  ---------------------------------- */

/* ----------------------------------
  Tests on serialization?
  ---------------------------------- */
