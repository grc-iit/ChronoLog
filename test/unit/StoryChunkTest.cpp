#include "StoryChunk.h"
#include "chrono_monitor.h"
#include <gtest/gtest.h>
#include <map>
#include <spdlog/spdlog.h>

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
    int storyId(1);
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
    int storyId(1);
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
    int storyId(1);
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
    int storyId(1);
    int startTime = 100, endTime = 200;
    chl::StoryChunk chunk("ChronicleName", "StoryName", storyId, startTime, endTime, 10);

    chl::LogEvent event(storyId, startTime, 1, 1, "test");
    chunk.insertEvent(event);

    EXPECT_EQ(chunk.getEventCount(), 1);
}

// Insert events out of order at different timestanps
// They should be still stored chronologically by their timestamp
TEST(StoryChunk_TestInsertEvent, testOutOfOrderInsert)
{
    int storyId(1);
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
    for(int i = 0; i < (int)expected.size(); ++i) EXPECT_EQ(series[i].time(), expected[i]);
}

// Insert events with a duplicate timestamp
// All shpuld be stored, no rejections cause of duplicate ts
TEST(StoryChunk_TestInsertEvent, testDuplicateTimestamp)
{
    int storyId(1);
    int startTime = 100, endTime = 200;
    chl::StoryChunk chunk("ChronicleName", "StoryName", storyId, startTime, endTime, 10);

    std::vector<int> timestamps = {180, 120, 150, 150, 180};
    for(int t: timestamps)
    {
        chl::LogEvent ev(storyId, t, 1, 1, "test");
        EXPECT_EQ(chunk.insertEvent(ev), 1);
    }

    EXPECT_EQ(chunk.getEventCount(), timestamps.size());

    std::vector<chl::Event> series;
    chunk.extractEventSeries(series);
    ASSERT_EQ(series.size(), timestamps.size());

    std::vector<int> expected = {120, 150, 150, 180, 180};
    for(int i = 0; i < (int)expected.size(); ++i) EXPECT_EQ(series[i].time(), expected[i]);
}

// Stress test insertEvent(): insert many events at once
// expect no failures and correct ordering/insert count
TEST(StoryChunk_TestInsertEvent, testStressInsert)
{
    int storyId(1);
    int startTime = 0;
    const int eventCount = 100000;
    // make endTime large enough to cover all timestamps
    int endTime = startTime + eventCount;
    // set capacity to eventCount so we can insert all
    chl::StoryChunk chunk("ChronicleName", "StoryName", storyId, startTime, endTime, eventCount);

    // Insert eventCount events at sequential timestamps
    for(int i = 0; i < eventCount; ++i)
    {
        chl::LogEvent ev(storyId,
                         startTime + i,// timestamp
                         /*client=*/i % 5,
                         /*index=*/i, "stress");
        EXPECT_EQ(chunk.insertEvent(ev), 1) << "Failed to insert event #" << i;
    }

    // All inserts should succeed
    EXPECT_EQ(chunk.getEventCount(), eventCount);

    // Extract and verify ordering & count
    std::vector<chl::Event> series;
    chunk.extractEventSeries(series);
    ASSERT_EQ(series.size(), eventCount);
    for(int i = 0; i < eventCount; ++i)
    {
        EXPECT_EQ(series[i].time(), startTime + i) << "series[" << i << "].time() mismatch";
    }

    // After extraction, chunk should be empty again
    EXPECT_TRUE(chunk.empty());
}

// Insert an event with a different story id as compared to the storychunk's storyid
// It should be rejected ideally
TEST(StoryChunk_TestInsertEvent, testIncorrectEventId)
{
    int storyId(1);
    int startTime = 100, endTime = 200;
    chl::StoryChunk chunk("ChronicleName", "StoryName", storyId, startTime, endTime, 10);

    chl::LogEvent event(10, startTime + 5, 1, 1, "test");
    chunk.insertEvent(event);

    EXPECT_EQ(chunk.getEventCount(), 0);
}


// Test with the max chunk size as 0 or negative
TEST(StoryChunk_TestInsertEvent, testZeroChunkSize)
{
    int storyId(1);
    int startTime = 100, endTime = 200;
    chl::StoryChunk chunk("ChronicleName", "StoryName", storyId, startTime, endTime, 0);

    int eventTime = 150, clientId = 2, eventIdx = 9;
    std::string record = "test";
    chl::LogEvent event(storyId, eventTime, clientId, eventIdx, record);
    chunk.insertEvent(event);

    EXPECT_EQ(chunk.getEventCount(), 0);
}

// Leave the event record name as an empty string
TEST(StoryChunk_TestInsertEvent, testEmptyEvent)
{
    int storyId(1);
    int startTime = 100, endTime = 200;
    chl::StoryChunk chunk("ChronicleName", "StoryName", storyId, startTime, endTime, 10);

    int eventTime = 150, clientId = 2, eventIdx = 9;
    std::string record = "test";
    chl::LogEvent event(storyId, eventTime, clientId, eventIdx, "");// nullptr not being caught by code, should be added
    chunk.insertEvent(event);

    EXPECT_EQ(chunk.getEventCount(), 1);
}

// Insert more events than the chunksize capacity nb of events
TEST(StoryChunk_TestInsertEvent, testInsertBeyondCapacity)
{
    int storyId(1);
    int startTime = 100, endTime = 200;
    int chunkCapacity = 5;
    chl::StoryChunk chunk("ChronicleName", "StoryName", storyId, startTime, endTime, chunkCapacity);

    // First insert exact same nb of events as chunk capacity and should be all success
    for(int i = 0; i < chunkCapacity; ++i)
    {
        chl::LogEvent event(storyId, startTime + i, i, i, "test");
        EXPECT_EQ(chunk.insertEvent(event), 1);
    }

    // Now we insert one more event which should get rejected
    chl::LogEvent extraEvent(storyId, startTime + chunkCapacity, 0, 0, "extraevent");
    EXPECT_EQ(chunk.insertEvent(extraEvent), 0);

    // the total event count should be same as the capacity
    EXPECT_EQ(chunk.getEventCount(), chunkCapacity);
}

// Insert a very large event record
TEST(StoryChunk_TestInsertEvent, testInsertLargeEventRecord)
{
    int storyId(2);
    int startTime = 0, endTime = 1000;
    chl::StoryChunk chunk("ChronicleName", "StoryName", storyId, startTime, endTime, 1);

    std::string largePayload(20 * 1024, 'A');
    chl::LogEvent largeEv(storyId, startTime + 1, 42, 7, largePayload);

    EXPECT_EQ(chunk.insertEvent(largeEv), 1);
    EXPECT_EQ(chunk.getEventCount(), 1);

    std::vector<chl::Event> series;
    chunk.extractEventSeries(series);
    ASSERT_EQ(series.size(), 1);

    EXPECT_EQ(series[0].log_record().size(), largePayload.size());
    EXPECT_EQ(series[0].log_record(), largePayload);
    EXPECT_TRUE(chunk.empty());
}


// More TBD test cases for insert events:
// Maybe test with multiple threads inserting at the same time into the same story chunk?
// Maybe test one thread doing insert another doing extract?


/* ----------------------------------
  Tests on mergeEvents()
  ---------------------------------- */

// Need to init chrono logger to avoid abort when we call mergeevents()
static void initLogger()
{
    int ret = chl::chrono_monitor::initialize("console", "", spdlog::level::debug, "unit_test_logger");
    ASSERT_EQ(ret, 0);
}

TEST(StoryChunk_TestMergeEvents, testEmptyInputMap)
{
    initLogger();

    std::map<chl::EventSequence, chl::LogEvent> events;
    auto mergeStart = events.cbegin();
    chl::StoryChunk chunk("ChronicleName", "StoryName", 1, 100, 200, 10);

    // try to merge empty event
    int retMerge = chunk.mergeEvents(events, mergeStart);
    EXPECT_EQ(retMerge, 0);
    EXPECT_TRUE(events.empty());
    EXPECT_EQ(chunk.getEventCount(), 0);
}

// Merge events should only merge the events within the current story chunks start and end widnow
// So we create a set of events, one of them has a time before the shunks start time
// should get rejected, shouldnt be merged
TEST(StoryChunk_TestMergeEvents, testStartBeforeWindow)
{
    initLogger();

    std::map<chl::EventSequence, chl::LogEvent> events;
    int startTime = 100, endTime = 200;
    int storyId(1);
    events.insert({{50, 0, 0}, chl::LogEvent(storyId, 50, 0, 0, "eventbeforestart")});
    events.insert({{120, 1, 1}, chl::LogEvent(storyId, 120, 1, 1, "eventafterstart")});
    events.insert({{220, 0, 0}, chl::LogEvent(storyId, 50, 0, 0, "eventafterstart")});

    auto mergeStart = events.cbegin();
    chl::StoryChunk chunk("ChronicleName", "StoryName", storyId, startTime, endTime, 10);
    int retMerge = chunk.mergeEvents(events, mergeStart);
    EXPECT_EQ(retMerge, 1);
    EXPECT_EQ(chunk.getEventCount(), 1);
    ASSERT_EQ(events.size(), 2);
    EXPECT_EQ(events.begin()->second.time(), 50);
}

// test merge events on a valid set of events
TEST(StoryChunk_TestMergeEvents, testValidMerge)
{
    initLogger();

    std::map<chl::EventSequence, chl::LogEvent> events;
    int startTime = 100, endTime = 200;
    int storyId(2);
    events.insert({{120, 0, 0}, chl::LogEvent(storyId, 110, 0, 0, "test")});
    events.insert({{150, 1, 1}, chl::LogEvent(storyId, 150, 1, 1, "test")});

    auto mergeStart = events.cbegin();
    chl::StoryChunk chunk("ChronicleName", "StoryName", storyId, startTime, endTime, 10);

    int retMerge = chunk.mergeEvents(events, mergeStart);
    EXPECT_EQ(retMerge, 2);
    EXPECT_TRUE(events.empty());
    EXPECT_EQ(chunk.getEventCount(), 2);
}

// try to merge all invalid events in terms of time window
// all should get rejected
TEST(StoryChunk_TestMergeEvents, testAllInvalidMerges)
{
    initLogger();

    std::map<chl::EventSequence, chl::LogEvent> events;
    int startTime = 100, endTime = 200;
    int storyId(3);
    events.insert({{10, 0, 0}, chl::LogEvent(storyId, 10, 0, 0, "test")});
    events.insert({{50, 1, 1}, chl::LogEvent(storyId, 50, 1, 1, "tes")});
    events.insert({{220, 0, 0}, chl::LogEvent(storyId, 210, 0, 0, "test")});
    events.insert({{250, 1, 1}, chl::LogEvent(storyId, 250, 1, 1, "test")});
    auto mergeStart = events.cbegin();
    chl::StoryChunk chunk("ChronicleName", "StoryName", storyId, startTime, endTime, 10);

    int retMerge = chunk.mergeEvents(events, mergeStart);
    EXPECT_EQ(retMerge, 0);
    EXPECT_EQ(events.size(), 4);
    EXPECT_EQ(chunk.getEventCount(), 0);
}


// when the story chunk capacity has been reached the merge should stop there
// so we try to merge more events that the chunk capacity and it should stop there
// the rejected events should not be deleted from the event map
TEST(StoryChunk_TestMergeEvents, testMergeMoreThanCapacity)
{
    initLogger();

    std::map<chl::EventSequence, chl::LogEvent> events;
    int startTime = 100, endTime = 200, chunkCapacity = 2;
    int storyId(6);

    events.insert({{110, 0, 0}, chl::LogEvent(storyId, 110, 0, 0, "test")});
    events.insert({{120, 1, 1}, chl::LogEvent(storyId, 120, 1, 1, "test")});
    events.insert({{150, 1, 1}, chl::LogEvent(storyId, 150, 1, 1, "test")});
    events.insert({{160, 1, 1}, chl::LogEvent(storyId, 160, 1, 1, "test")});

    auto mergeStart = events.cbegin();
    chl::StoryChunk chunk("ChronicleName", "StoryName", storyId, startTime, endTime, chunkCapacity);

    int retMerge = chunk.mergeEvents(events, mergeStart);

    EXPECT_EQ(retMerge, chunkCapacity);
    EXPECT_EQ(chunk.getEventCount(), chunkCapacity);
    ASSERT_EQ(events.size(), 2);
    EXPECT_EQ(events.begin()->second.time(), 150);
}

// try merging events with duplicate timestamp, all should get merged
// no rejects
TEST(StoryChunk_TestMergeEvents, testDuplicateTimestamps)
{
    initLogger();

    std::map<chl::EventSequence, chl::LogEvent> events;
    int startTime = 100, endTime = 200;
    int storyId(7);

    events.insert({{110, 0, 0}, chl::LogEvent(storyId, 110, 0, 0, "test")});
    events.insert({{110, 1, 1}, chl::LogEvent(storyId, 110, 1, 1, "test")});
    events.insert({{120, 0, 0}, chl::LogEvent(storyId, 120, 0, 0, "test")});

    auto mergeStart = events.cbegin();
    chl::StoryChunk chunk("ChronicleName", "StoryName", storyId, startTime, endTime, 10);

    int retMerge = chunk.mergeEvents(events, mergeStart);
    EXPECT_EQ(retMerge, 3);
    EXPECT_EQ(chunk.getEventCount(), 3);
    EXPECT_TRUE(events.empty());
}

// stress test the merge events
// try to merge a big nb of events
TEST(StoryChunk_TestMergeEvents, testHugeMerge)
{
    initLogger();

    std::map<chl::EventSequence, chl::LogEvent> events;
    int nbEvents = 1000;
    int startTime = 0, endTime = nbEvents + 10;
    int storyId(8);

    for(int i = 0; i < nbEvents; ++i) { events.insert({{i, 0, 0}, chl::LogEvent(storyId, i, 0, 0, "test")}); }

    auto mergeStart = events.cbegin();
    chl::StoryChunk chunk("ChronicleName", "StoryName", storyId, startTime, endTime, nbEvents);

    int retMerge = chunk.mergeEvents(events, mergeStart);

    EXPECT_EQ(retMerge, nbEvents);
    EXPECT_EQ(chunk.getEventCount(), nbEvents);
    EXPECT_TRUE(events.empty());
}

// test merging an event at the boundary end time (endtime-1 is okay, endtime should be rejected)
TEST(StoryChunk_TestMergeEvents, BoundaryEndTimeExclusion)
{
    initLogger();

    std::map<chl::EventSequence, chl::LogEvent> events;
    int startTime = 100, endTime = 200;
    int storyId(9);

    events.insert({{100, 0, 0}, chl::LogEvent(storyId, 100, 0, 0, "test")});
    events.insert({{150, 1, 1}, chl::LogEvent(storyId, 150, 1, 1, "test")});
    events.insert({{199, 2, 2}, chl::LogEvent(storyId, 199, 2, 2, "test")});
    events.insert({{200, 3, 3}, chl::LogEvent(storyId, 200, 3, 3, "test")});

    auto mergeStart = events.cbegin();
    chl::StoryChunk chunk("ChronicleName", "StoryName", storyId, startTime, endTime, 10);

    int retMerge = chunk.mergeEvents(events, mergeStart);

    EXPECT_EQ(retMerge, 3);
    EXPECT_EQ(chunk.getEventCount(), 3);
    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events.begin()->second.time(), 200);
}


// test merging the events from a random place in the middle of the event set instead of start
TEST(StoryChunk_TestMergeEvents, testMergeFromMiddle)
{
    initLogger();

    std::map<chl::EventSequence, chl::LogEvent> events;
    int startTime = 100, endTime = 200;
    int storyId(1);

    for(int t: {100, 110, 120, 130}) events.insert({{t, 0, 0}, chl::LogEvent(storyId, t, 0, 0, "test")});

    // update the iterator to start merging from the middle of the event set
    auto mergeStart = std::next(events.cbegin(), 2);

    chl::StoryChunk chunk("ChronicleName", "StoryName", storyId, startTime, endTime, 10);
    int retMerge = chunk.mergeEvents(events, mergeStart);

    EXPECT_EQ(retMerge, 2);
    EXPECT_EQ(chunk.getEventCount(), 2);
    ASSERT_EQ(events.size(), 2);
    EXPECT_EQ(events.begin()->second.time(), 100);
}

// Test mergevents(chunk, time)

// try to merge an empty chunk to another main chunk
TEST(StoryChunk_TestMergeEvents, testMergeEmptyChunk)
{
    initLogger();

    chl::StoryChunk masterChunk("ChronicleName", "StoryName", 1, 100, 200, 10);
    chl::StoryChunk otherChunk("ChronicleName", "StoryName", 1, 100, 200, 10);

    int retMerge = masterChunk.mergeEvents(otherChunk, 150);
    EXPECT_EQ(retMerge, 0);
    EXPECT_TRUE(masterChunk.empty());
    EXPECT_TRUE(otherChunk.empty());
}


// test merging when the other chunk has events within main chunks time window
// should merge
TEST(StoryChunk_TestMergeEvents, testMergeWithinWindow)
{
    initLogger();

    chl::StoryChunk masterChunk("ChronicleName", "StoryName", 1, 100, 200, 10);
    chl::StoryChunk otherChunk("ChronicleName", "StoryName", 1, 100, 200, 10);

    otherChunk.insertEvent(chl::LogEvent(1, 110, 0, 0, "test"));
    otherChunk.insertEvent(chl::LogEvent(1, 150, 0, 0, "test"));

    int retMerge = masterChunk.mergeEvents(otherChunk, 100);
    EXPECT_EQ(retMerge, 2);
    EXPECT_EQ(masterChunk.getEventCount(), 2);
    EXPECT_TRUE(otherChunk.empty());
}

// test merging when the other chunk has events out of main chunks time window
// other chunk should not get emptied, should just get rejected in merge to main
TEST(StoryChunk_TestMergeEvents, testMergeOutsideWindow)
{
    initLogger();

    chl::StoryChunk masterChunk("ChronicleName", "StoryName", 1, 100, 200, 10);
    chl::StoryChunk otherChunk("ChronicleName", "StoryName", 1, 100, 200, 10);

    otherChunk.insertEvent(chl::LogEvent(1, 50, 0, 0, "test"));
    otherChunk.insertEvent(chl::LogEvent(1, 250, 0, 0, "test"));

    int retMerge = masterChunk.mergeEvents(otherChunk, 0);
    EXPECT_EQ(retMerge, 0);
    EXPECT_TRUE(masterChunk.empty());
    EXPECT_EQ(otherChunk.getEventCount(), 2);
}

// test a mixed merge of other chunk
// the other chunk has a window different from main chunk
// so the main chunk should only merge the events within its accepted time frame
TEST(StoryChunk_TestMergeEvents, testMixedChunkMerge)
{
    initLogger();

    chl::StoryChunk masterChunk("ChronicleName", "StoryName", 1, 100, 200, 10);
    chl::StoryChunk otherChunk("ChronicleName", "StoryName", 1, 50, 250, 10);

    otherChunk.insertEvent(chl::LogEvent(1, 80, 0, 0, "testBefore"));
    otherChunk.insertEvent(chl::LogEvent(1, 110, 0, 0, "testIn"));
    otherChunk.insertEvent(chl::LogEvent(1, 180, 0, 0, "testIn"));
    otherChunk.insertEvent(chl::LogEvent(1, 220, 0, 0, "testAfter"));

    int retMerge = masterChunk.mergeEvents(otherChunk, 50);
    EXPECT_EQ(retMerge, 2);
    EXPECT_EQ(masterChunk.getEventCount(), 2);

    std::vector<chl::Event> leftOtherChunk;
    otherChunk.extractEventSeries(leftOtherChunk);
    ASSERT_EQ(leftOtherChunk.size(), 2);
    EXPECT_EQ(leftOtherChunk[0].time(), 80);
    EXPECT_EQ(leftOtherChunk[1].time(), 220);
}


// try calling merge events with a merge start time as after end time of the chunk
TEST(StoryChunk_TestMergeEvents, testMergeStartTimeAfterEnd)
{
    initLogger();
    chl::StoryChunk masterChunk("ChronicleName", "StoryName", 1, 0, 100, 10);
    chl::StoryChunk otherChunk("ChronicleName", "StoryName", 1, 0, 100, 10);
    otherChunk.insertEvent(chl::LogEvent(1, 10, 0, 0, "test"));

    int retMerge = masterChunk.mergeEvents(otherChunk, 200);
    EXPECT_EQ(retMerge, 0);
    EXPECT_EQ(masterChunk.getEventCount(), 0);
}

// test merging a map of events to the main chunk
// then merge another chunk to the main chunk, it should all be okay
TEST(StoryChunk_TestMergeEvents, testMergeMapThenChunk)
{
    initLogger();

    // first create map of events
    int storyId(1);
    std::map<chl::EventSequence, chl::LogEvent> events;
    events.insert({{110, 0, 0}, chl::LogEvent(storyId, 110, 0, 0, "test")});
    events.insert({{120, 0, 0}, chl::LogEvent(storyId, 120, 0, 0, "test")});
    auto it = events.cbegin();
    // mow merge the map to masterchunk
    chl::StoryChunk masterChunk("ChronicleName", "StoryName", storyId, 100, 200, 10);
    int merged1 = masterChunk.mergeEvents(events, it);
    EXPECT_EQ(merged1, 2);
    EXPECT_EQ(masterChunk.getEventCount(), 2);

    // now create another storychunk with some other events
    chl::StoryChunk otherChunk("ChronicleName", "StoryName", storyId, 100, 200, 10);
    otherChunk.insertEvent(chl::LogEvent(storyId, 130, 0, 0, "test"));
    EXPECT_EQ(otherChunk.getEventCount(), 1);

    // now merge the otherchunk to this master chunk
    int merged2 = masterChunk.mergeEvents(otherChunk, 0);
    EXPECT_EQ(merged2, 1);
    EXPECT_EQ(masterChunk.getEventCount(), 3);

    //  confirm now that other chunk is empty
    EXPECT_TRUE(otherChunk.empty());
}


// try merging an event with a different story id than the chunk storyid
TEST(StoryChunk_TestMergeEvents, testIncorrectStoryId)
{
    initLogger();

    std::map<chl::EventSequence, chl::LogEvent> events;
    events.insert({{110, 0, 0}, chl::LogEvent(99, 110, 0, 0, "wrong")});
    auto it = events.cbegin();
    chl::StoryChunk chunk("ChronicleName", "StoryName", 1, 100, 200, 10);

    int retMerge = chunk.mergeEvents(events, it);
    EXPECT_EQ(retMerge, 0);
    EXPECT_EQ(events.size(), 1);
}

// test merging an empty event and huge payload
// both should merge
TEST(StoryChunk_TestMergeEvents, testEmptyAndHugePayloads)
{
    initLogger();

    std::map<chl::EventSequence, chl::LogEvent> events;
    int storyId(3);
    events.insert({{110, 0, 0}, chl::LogEvent(storyId, 110, 0, 0, "")});
    events.insert({{120, 0, 0}, chl::LogEvent(storyId, 120, 0, 0, std::string(5000, 't'))});

    auto it = events.cbegin();
    chl::StoryChunk chunk("ChronicleName", "StoryName", storyId, 100, 200, 10);
    int retMerge = chunk.mergeEvents(events, it);

    EXPECT_EQ(retMerge, 2);
    EXPECT_EQ(chunk.getEventCount(), 2);
}


// add thread safety tests - TBD

/* ----------------------------------
  Tests on eraseEvents()
  ---------------------------------- */

/* ----------------------------------
  Tests on extractEventSeries()
  ---------------------------------- */

/* ----------------------------------
  Tests on serialization?
  ---------------------------------- */
