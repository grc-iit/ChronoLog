#ifndef KEEPER_DATA_STORE_H
#define KEEPER_DATA_STORE_H

#include <vector>
#include <list>
#include <map>
#include <mutex>

#include <thallium.hpp>

#include "IngestionQueue.h"
<<<<<<< HEAD
=======
#include "StoryPipeline.h"
#include "StoryChunkExtractionQueue.h"

>>>>>>> 2398801f427786d5ef9f35c8ae47efa9bad3ea5a

namespace chronolog
{


typedef uint64_t StoryId;
typedef uint64_t StorytellerId;
typedef std::string StoryName;
typedef std::string ChronicleName;

// Pool of Sequencing worker threads 
// Pool of Sequencing mutexes 
// Pool of indestion mutexes 
//
// Story Pipeline
// 1. ingestion Queue + hashed ingestion mutex 
// 2. map of 5 secs maps<timestamp,arrival index> + hashed sequencing index
// 
// on acquire story allocate StoryPipeLine object for the story, assign ingestion & sequencing mutex from the mutex pools
//
// Hashmap of story pipelines
//
//
//
//

// StorycChunk contains all the events for the single story
// for the duration [startTime, endTime[
// startTime included, endTime excluded
// startTime/endTime are invariant
class StoryChunk
{
<<<<<<< HEAD
public:
   int mergeEvents(std::deque<LogEvent> const&);
   int mergeEvents(std::vector<LogEvent> const&);

private:
StoryId storyId;          
uint64_t startTime;
uint64_t endTime;   
uint64_t lastRevisionTime;
std::map<uint64_t, std::list<LogEvent>> logEvents;

=======
	UNKNOWN = 0,
	RUNNING	=1,	//  active stories
	SHUTTING_DOWN=2	// Shutting down services
>>>>>>> 2398801f427786d5ef9f35c8ae47efa9bad3ea5a
};

class StoryPipeline
{
public:
<<<<<<< HEAD
	StoryPipeline( StoryId const&, std::mutex &, std::mutex &);
	
	~StoryPipeline();

int mergeEvents(std::vector<LogEvent> const&);
int mergeEvents(StoryChunk const&);

private:

StoryId 	storyId;
ChronicleName	chronicleName;
StoryName	storyName;
uint64_t	revisionTime; //timestamp of the most recent merge 
uint64_t	tailTime; // timestamp of the most recent recorded event
uint64_t	acquisitionTime; //timestamp of the most recent aquisition

// this mutex will be assigned to the story from the DataStore IngestionMutexPool
// based on the StoryId, it is used to protect the IngestionQueue from concurrent access
// by RecordingService threads
std::mutex & ingestionMutex;
// two ingestion queues so that they can take turns playing 
// active/passive ingestion duty
// 
std::deque<LogEvent> eventQueue1;
std::deque<LogEvent> eventQueue2;

StoryIngestionHandle * ingestionQueueHandle;

// this mutex will be assigned to the story from the DataStore SequencingMutexPool
// based  on the StoryId; it is used to protect Story sequencing operations 
// from concurrent access by the DataStore Sequencing threads
std::mutex & sequencingMutex;

// map of storyChunks ordered by inclusive startTime
std::map<uint64_t, StoryChunk > storyTimeline;

};

class KeeperDataStore
{
public:
      KeeperDataStore()
      {}
=======
    KeeperDataStore( IngestionQueue & ingestion_queue, StoryChunkExtractionQueue & extraction_queue)
	      : state(UNKNOWN)
	      , theIngestionQueue(ingestion_queue) 
	      , theExtractionQueue(extraction_queue) 
    {}

    ~KeeperDataStore();
>>>>>>> 2398801f427786d5ef9f35c8ae47efa9bad3ea5a

    bool is_running() const 
    { return (RUNNING == state); }

<<<<<<< HEAD
bool is_shutting_down() const
{
	return (false); // INNA: implement state_management
}

int startStoryRecording(ChronicleName const&, StoryName const&, StoryId const&, uint32_t timeGranularity=30 ); //INNA: 30 seconds?
int stopStoryRecording(StoryId const&);

void shutdownDataCollection();

private:
std::unordered_map<StoryId, StoryPipeline> StoryPipelines;
std::mutex theStoryPipelinesMutex;
=======
    bool is_shutting_down() const 
    { return (SHUTTING_DOWN == state); }

    int startStoryRecording(ChronicleName const&, StoryName const&, StoryId const&
		      , uint64_t start_time, uint32_t time_chunk_ranularity=30, uint32_t access_window = 60 ); 
    int stopStoryRecording(StoryId const&);

    void collectIngestedEvents();
    void extractDecayedStoryChunks();
    void retireDecayedPipelines();
    
    void startDataCollection(int stream_count);
    void shutdownDataCollection();
    void dataCollectionTask();

private:
    KeeperDataStore( KeeperDataStore const&) = delete;
    KeeperDataStore& operator= ( KeeperDataStore const&) = delete;

    DataStoreState	state;
    std::mutex dataStoreStateMutex;
    IngestionQueue &   theIngestionQueue;
    StoryChunkExtractionQueue &   theExtractionQueue;
    std::vector<thallium::managed<thallium::xstream>> dataStoreStreams;
    std::vector<thallium::managed<thallium::thread>> dataStoreThreads;
    
    std::mutex dataStoreMutex;
    std::unordered_map<StoryId, StoryPipeline*> theMapOfStoryPipelines;
    std::unordered_map<StoryId, std::pair<StoryPipeline*, uint64_t>> pipelinesWaitingForExit; 
>>>>>>> 2398801f427786d5ef9f35c8ae47efa9bad3ea5a

std::unordered_map<StoryId, std::mutex> ingestionMutexes;
};

}
#endif
