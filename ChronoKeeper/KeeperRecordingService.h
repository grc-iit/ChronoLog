#ifndef KEEPER_RECORDING_SERVICE_H
#define KEEPER_RECORDING_SERVICE_H

#include <iostream>
#include <margo.h>
#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>
<<<<<<< HEAD
=======

#include "errcode.h"
>>>>>>> 2398801f427786d5ef9f35c8ae47efa9bad3ea5a
#include "KeeperIdCard.h"
#include "chronolog_types.h"
#include "IngestionQueue.h"

namespace tl = thallium;


namespace chronolog
{


class KeeperRecordingService : public tl::provider<KeeperRecordingService> 
{
public:
    
// KeeperRecordingService should be created on the heap not the stack thus the constructor is private...

    static KeeperRecordingService * CreateKeeperRecordingService(tl::engine& tl_engine, uint16_t service_provider_id
		    , IngestionQueue & ingestion_queue)
    {
        
          return  new KeeperRecordingService( tl_engine, service_provider_id, ingestion_queue );
    }  

    ~KeeperRecordingService() 
    {
        std::cout<<"KeeperRecordingService::~KeeperRecordingService()"<<std::endl;
        get_engine().pop_finalize_callback(this);
    }

<<<<<<< HEAD
private:

    void record_event(tl::request const& request, 
                       StorytellerId teller_id,  StoryId story_id, 
		       uint64_t timeStamp, const std::string& record) 
    {
        std::cout << "recording {"<< teller_id<<":"<<story_id<<":"<< record <<"}"<< std::endl;
	theIngestionQueue.ingestLogEvent(LogEvent(story_id,teller_id,timeStamp,record));
        request.respond( static_cast<int>(record.size()) );
    }

    void record_event_no_response( StorytellerId teller_id,  StoryId story_id, 
		       uint64_t timeStamp, const std::string& record) 
    {
        std::cout << "recording {"<< teller_id<<":"<<story_id<<":"<< record <<"}"<< std::endl;
	theIngestionQueue.ingestLogEvent(LogEvent(story_id,teller_id,timeStamp,record));
=======
    void record_event(tl::request const& request, LogEvent const & log_event) 
                   //    ClientId teller_id,  StoryId story_id, 
		     //  ChronoTick const& chrono_tick, std::string const& record) 
    {
        std::cout << "KeeperRecordingService : record_event {"<< log_event<<"}"<< std::endl;
	    theIngestionQueue.ingestLogEvent(log_event);
        request.respond(CL_SUCCESS);

>>>>>>> 2398801f427786d5ef9f35c8ae47efa9bad3ea5a
    }

private:

    KeeperRecordingService(tl::engine& tl_engine, uint16_t service_provider_id, IngestionQueue & ingestion_queue)
    	: tl::provider<KeeperRecordingService>(tl_engine, service_provider_id)
	, theIngestionQueue(ingestion_queue)  
    {
<<<<<<< HEAD
        define("record_event", &KeeperRecordingService::record_event);
        define("record_event_no_response", &KeeperRecordingService::record_event_no_response,tl::ignore_return_value() );
	//set up callback for the case when the engine is being finalized while this provider is still alive
	get_engine().push_finalize_callback(this, [p=this](){delete p;} );
=======
        define("record_event", &KeeperRecordingService::record_event, tl::ignore_return_value());
	    //set up callback for the case when the engine is being finalized while this provider is still alive
	    get_engine().push_finalize_callback(this, [p=this](){delete p;} );
>>>>>>> 2398801f427786d5ef9f35c8ae47efa9bad3ea5a
    }

    KeeperRecordingService( KeeperRecordingService const&) = delete;
    KeeperRecordingService & operator= (KeeperRecordingService const&) = delete;

    IngestionQueue & theIngestionQueue;
};

}// namespace chronolog

#endif
