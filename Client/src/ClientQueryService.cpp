

#include <thallium.hpp>
#include <thallium/serialization/stl/vector.hpp>
#include <cereal/archives/binary.hpp>

#include "chronolog_errcode.h"
#include "chrono_monitor.h"
#include "StoryChunk.h"
#include "ClientQueryService.h"
#include "PlaybackQueryRpcClient.h"

namespace tl = thallium;
namespace chl = chronolog;



chl::ClientQueryService::ClientQueryService(thallium::engine & tl_engine, chl::ServiceId const& client_service_id)
        : tl::provider <ClientQueryService>(tl_engine, client_service_id.provider_id)
        , queryServiceEngine(tl_engine)
        , queryServiceId(client_service_id)
{

    LOG_DEBUG("[ClientQueryService] created  service {}", chl::to_string(queryServiceId));

         define("receive_story_chunk", &ClientQueryService::receive_story_chunk, tl::ignore_return_value());
         //set up callback for the case when the engine is being finalized while this provider is still alive
         get_engine().push_finalize_callback(this, [p = this]()
         { delete p; });
}


chl::ClientQueryService::~ClientQueryService()
{
    LOG_DEBUG("[ClientQueryService] Destructor called. Cleaning up...");
    get_engine().pop_finalize_callback(this);
}


    // send the playback request to the ChronoPlayer PlaybackService and return queryId
int chl::ClientQueryService::send_playback_query_request(chl::ServiceId const& playback_service_id,
                chl::ChronicleName const&, chl::StoryName const&, chl::chrono_time const&, chl::chrono_time const&)
{

    return chl::CL_ERR_UNKNOWN;
}

// build transfer of the Response StoryChunks
void chl::ClientQueryService::receive_story_chunk(tl::request  const& request, tl::bulk &b)
{
    try
    {
        tl::endpoint ep = request.get_endpoint();
        LOG_DEBUG("[ClientQueryService] receive_story_chunk :Endpoint obtained, ThreadID={}", tl::thread::self_id());
        
        std::vector <char> mem_vec(b.size());
        std::vector <std::pair <void*, std::size_t>> segments(1);
        segments[0].first = (void*)(&mem_vec[0]);
        segments[0].second = mem_vec.size();
        LOG_DEBUG("[ClientQueryService] Bulk memory prepared, size: {}, ThreadID={}", mem_vec.size(), tl::thread::self_id());
        tl::engine local_engine = get_engine();
        
        tl::bulk local = local_engine.expose(segments, tl::bulk_mode::write_only);
        LOG_DEBUG("[ClientQueryService] Bulk memory exposed, ThreadID={}", tl::thread::self_id());
        b.on(ep) >> local;
        LOG_DEBUG("[ClientQueryService] Received {} bytes of StoryChunk data, ThreadID={}", b.size(), tl::thread::self_id());
  
        StoryChunk*story_chunk = new StoryChunk();
        int ret = deserializedWithCereal(&mem_vec[0], b.size()
                                               , *story_chunk);
        if(ret != CL_SUCCESS)
        {
            LOG_ERROR("[ClientQueryService] Failed to deserialize a story chunk, ThreadID={}"
                            , tl::thread::self_id());
            delete story_chunk;
            ret = 10000000 + tl::thread::self_id(); // arbitrary error code encoded with thread id
            LOG_ERROR("[ClientQueryService] Discarding the story chunk, responding {} to Keeper", ret);
            request.respond(ret);
            return;
        }
        LOG_DEBUG("[ClientQueryService] StoryChunk received: StoryId {} StartTime {} eventCount {} ThreadID={}"
                        , story_chunk->getStoryId(), story_chunk->getStartTime(), story_chunk->getEventCount()
                        , tl::thread::self_id());
  
        request.respond(b.size());
        LOG_DEBUG("[ClientQueryService] StoryChunk recording RPC responded {}, ThreadID={}", b.size()
                        , tl::thread::self_id());
 
        // add StoryChunk to the QueryResponse Object 
        }
        catch(std::bad_alloc const &ex)
        {
            LOG_ERROR("[ClientQueryService] Failed to allocate memory for StoryChunk data, ThreadID={}" , tl::thread::self_id());
            request.respond(20000000 + tl::thread::self_id());
        }
}

int chl::ClientQueryService::deserializedWithCereal(char *buffer, size_t size, chl::StoryChunk &story_chunk)
     {
         std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
         try
         {
             ss.write(buffer, size);
             cereal::BinaryInputArchive iarchive(ss);
             iarchive(story_chunk);
             return chl::CL_SUCCESS;
         }
         catch(cereal::Exception const &ex)
        {
            LOG_ERROR("[ClientQueryService] Failed to deserialize a story chunk, size={}, ThreadID={}. "
                       "Cereal exception: {}", ss.str().size(), tl::thread::self_id(), ex.what());
         }
         catch(std::exception const &ex)
         {
          LOG_ERROR("[ClientQueryService] Failed to deserialize a story chunk, size={}, ThreadID={}. "
                       "std::exception : {}", ss.str().size(), tl::thread::self_id(), ex.what());
         }
         catch(...)
         {
             LOG_ERROR("[ClientQueryService] Failed to deserialize a story chunk, ThreadID={}. Unknown exception "
                       "encountered.", tl::thread::self_id());
         }
        return chl::CL_ERR_UNKNOWN;
     }

