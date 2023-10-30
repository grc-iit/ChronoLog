#ifndef INGESTION_QUEUE_H
#define INGESTION_QUEUE_H


#include <iostream>
#include <deque>
#include <unordered_map>
#include <mutex>

#include "chronolog_types.h"
<<<<<<< HEAD
=======
#include "StoryIngestionHandle.h"
>>>>>>> 2398801f427786d5ef9f35c8ae47efa9bad3ea5a

//
// IngestionQueue is a funnel into the MemoryDataStore
// std::deque guarantees O(1) time for addidng elements and resizing 
// (vector of vectors implementation)

namespace chronolog
{

typedef std::deque<LogEvent> EventDeque;
<<<<<<< HEAD

class StoryIngestionHandle
{

public:
	StoryIngestionHandle( std::mutex & a_mutex, EventDeque & event_deque)
       		: handleMutex(a_mutex)
	  	, activeDeque(&event_deque)
			{}
	~StoryIngestionHandle() = default;

	void ingestEvent( LogEvent const& logEvent)
	{   // assume multiple service threads pushing events on ingestionQueue
	    std::lock_guard<std::mutex> lock(handleMutex); 
	    activeDeque->push_back(logEvent); 
	}

	void swapActiveDeque( EventDeque * empty_deque, EventDeque * full_deque)
	{
	    if( activeDeque->empty())
	    {	    return ;       }
//INNA: check if atomic compare_and_swap will work here

	    std::lock_guard<std::mutex> lock(handleMutex); 
	    if(  activeDeque->empty() )
	    {	    return ;       }

	    full_deque = activeDeque;
	    activeDeque = empty_deque;

	}
	     
private:

	std::mutex & handleMutex;   
	EventDeque * activeDeque;
};


=======

>>>>>>> 2398801f427786d5ef9f35c8ae47efa9bad3ea5a
class IngestionQueue
{
public:
	IngestionQueue()
	{ }
	~IngestionQueue()
	{ shutDown();}

void addStoryIngestionHandle( StoryId const& story_id, StoryIngestionHandle * ingestion_handle)
{
	std::lock_guard<std::mutex> lock(ingestionQueueMutex);
	storyIngestionHandles.emplace(story_id,ingestion_handle);
}

void removeIngestionHandle(StoryId const & story_id)
{
	std::lock_guard<std::mutex> lock(ingestionQueueMutex);
	storyIngestionHandles.erase(story_id);
}

void ingestLogEvent(LogEvent const & event)
{
<<<<<<< HEAD
	auto ingestionHandle_iter = storyIngestionHandles.find(event.storyId);
	if( ingestionHandle_iter == storyIngestionHandles.end())
	{
		std::cout <<" orphan event"<<std::endl;
=======
	std::cout <<"IngestionQueue: ingestLogEvent : storyIngestionHandles {"<< &storyIngestionHandles<<"} .size="<<storyIngestionHandles.size()<<std::endl;
	std::cout<<"IngestionQueue: received event {"<<event<< "}"<<std::endl;

	auto ingestionHandle_iter = storyIngestionHandles.find(event.storyId);
	if( ingestionHandle_iter == storyIngestionHandles.end())
	{
		std::cout <<" orphan event {"<<event<<"}"<<std::endl;
>>>>>>> 2398801f427786d5ef9f35c8ae47efa9bad3ea5a
		std::lock_guard<std::mutex> lock(ingestionQueueMutex);
		orphanEventQueue.push_back(event);
	}
	else
	{       //individual StoryIngestionHandle has its own mutex
		(*ingestionHandle_iter).second->ingestEvent(event);
	}	
}

void drainOrphanEvents()
{
	if ( orphanEventQueue.empty())
	{	return; }

	std::lock_guard<std::mutex> lock(ingestionQueueMutex);
    for( EventDeque::iterator iter = orphanEventQueue.begin(); iter != orphanEventQueue.end(); )
	{
	   auto ingestionHandle_iter = storyIngestionHandles.find((*iter).storyId);
	   if( ingestionHandle_iter != storyIngestionHandles.end())
        {	//individual StoryIngestionHandle has its own mutex
		    (*ingestionHandle_iter).second->ingestEvent(*iter);
           	//remove the event from the orphan deque and get the iterator to the next element prior to removal
		    iter = orphanEventQueue.erase(iter);	
	   }
	   else
	   {    ++iter; }
	}
}

bool is_empty() const
{
    return (orphanEventQueue.empty() && storyIngestionHandles.empty());
}

void shutDown()
{
<<<<<<< HEAD
=======
	std::cout <<"IngestionQueue: shutdown : storyIngestionHandles {"<<storyIngestionHandles.size()
               << "} orphanEventQueue {"<<orphanEventQueue.size()<<"}"<<std::endl;
>>>>>>> 2398801f427786d5ef9f35c8ae47efa9bad3ea5a
	// last attempt to drain orphanEventQueue into known ingestionHandles
 	drainOrphanEvents();
	// disengage all handles
	std::lock_guard<std::mutex> lock(ingestionQueueMutex);
	storyIngestionHandles.clear();
}

private:

	IngestionQueue(IngestionQueue const &) = delete;
	IngestionQueue & operator=(IngestionQueue const &) = delete;

	std::mutex ingestionQueueMutex;
	std::unordered_map<StoryId, StoryIngestionHandle*> storyIngestionHandles;

	// events for unknown stories or late events for closed stories will end up
	// in orphanEventQueue that we'll periodically try to drain into the DataStore
	std::deque<LogEvent> orphanEventQueue;

	//Timer to triger periodic attempt to drain orphanEventQueue and collect/log statistics
};

}

#endif

