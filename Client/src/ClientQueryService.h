#ifndef CLIENT_QUERY_SERVICE_H
#define CLIENT_QUERY_SERVICE_H

#include <atomic>
#include <map>
#include <mutex>
#include <thallium.hpp>

#include "chronolog_types.h"
#include "ServiceId.h"


namespace tl = thallium;

namespace chronolog
{

class PlaybackQueryRpcClient;
class StoryChunk;

struct StoryPlaybackQuery
{
    uint32_t queryId;
    ChronicleName chronicleName;
    StoryName   storyName;
    chrono_time startTime;
    chrono_time endTime;
    StoryId     storyId;
    PlaybackQueryRpcClient * playbackRpcClient;
    std::map<uint64_t,StoryChunk*> PlaybackResponse;
};

class ClientQueryService : public tl::provider <ClientQueryService>
{
public:
    // Service should be created on the heap not the stack thus the constructor is private...
    static ClientQueryService *
    CreateClientQueryService(thallium::engine & tl_engine, ServiceId const& client_service_id)
    {
        try 
        {
            return new ClientQueryService(tl_engine, client_service_id);
        }
        catch(thallium::exception &)
        {
            return nullptr;
        }
    }

    ~ClientQueryService();

    // send the playback request to the ChronoPlayer PlaybackService and return queryId
    int send_playback_query_request(ServiceId const& playback_service_id,
                ChronicleName const&, StoryName const&,chrono_time const&, chrono_time const&);    

    void receive_story_chunk(tl::request const&, tl::bulk &);

private:
    ClientQueryService(thallium::engine & tl_engine, ServiceId const&);

    ClientQueryService() = delete;
    ClientQueryService(ClientQueryService const&) = delete;

    int deserializedWithCereal(char *buffer, size_t size, StoryChunk &story_chunk);
    thallium::engine  queryServiceEngine;
    ServiceId       queryServiceId;
    std::mutex queryServiceMutex;    
    std::atomic<int> queryIdIndex;
    std::map<uint32_t, StoryPlaybackQuery> activeQueryMap; // map of active queries by queryId
};


}

#endif
