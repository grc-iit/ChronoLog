#include <chrono>
#include "chronolog_client.h"
#include "common.h"
#include <cassert>
#include <cmd_arg_parse.h>
#include "chrono_monitor.h"
#include <mutex>
#include <condition_variable>
#include <vector>
#include <iostream>

#define NUM_CHRONICLE (10)
#define NUM_STORY (10)
#define CHRONICLE_NAME_LEN 32
#define STORY_NAME_LEN 32

// Define bitmask for operation states
enum OperationState
{
    CHRONICLE_CREATED = 1 << 0, // 0001
    STORY_ACQUIRED = 1 << 1, // 0010
    STORY_RELEASED = 1 << 2, // 0100
    STORY_DESTROYED = 1 << 3, // 1000
    CHRONICLE_DESTROYED = 1 << 4  // 1 << 4
};

std::vector <int> shared_state; // Global array for operation states
std::mutex state_mutex;
std::condition_variable state_cv;

// Function to update shared state and wait for all operations to reach a specific state
void update_shared_state_and_wait(int idx, int target_state_bitmask)
{
    {
        std::lock_guard <std::mutex> lock(state_mutex);
        shared_state[idx] |= target_state_bitmask; // Update the bitmask state
        std::cout << "Operation " << idx << " state updated: " << shared_state[idx] << std::endl;
        state_cv.notify_all();
    }
    {
        std::unique_lock <std::mutex> lock(state_mutex);
        state_cv.wait(lock, [target_state_bitmask]()
        {
            // Check if all operations have at least the target state bit set
            return std::all_of(shared_state.begin(), shared_state.end(), [target_state_bitmask](int state)
            {
                return (state&target_state_bitmask) == target_state_bitmask;
            });
        });
        std::cout << "All operations reached state " << target_state_bitmask << std::endl;
    }
}

int main(int argc, char**argv)
{
    std::string conf_file_path = parse_conf_path_arg(argc, argv);
    if(conf_file_path.empty())
    {
        std::exit(EXIT_FAILURE);
    }

    ChronoLog::ConfigurationManager confManager(conf_file_path);
    int result = chronolog::chrono_monitor::initialize(confManager.CLIENT_CONF.CLIENT_LOG_CONF.LOGTYPE
                                                       , confManager.CLIENT_CONF.CLIENT_LOG_CONF.LOGFILE
                                                       , confManager.CLIENT_CONF.CLIENT_LOG_CONF.LOGLEVEL
                                                       , confManager.CLIENT_CONF.CLIENT_LOG_CONF.LOGNAME
                                                       , confManager.CLIENT_CONF.CLIENT_LOG_CONF.LOGFILESIZE
                                                       , confManager.CLIENT_CONF.CLIENT_LOG_CONF.LOGFILENUM
                                                       , confManager.CLIENT_CONF.CLIENT_LOG_CONF.FLUSHLEVEL);
    if(result == 1)
    {
        exit(EXIT_FAILURE);
    }

    LOG_INFO("[ClientLibMetadataRPCTest] Running test.");

    chronolog::Client client(confManager);
    std::vector <std::string> chronicle_names;
    std::chrono::steady_clock::time_point t1, t2;
    std::chrono::duration <double, std::nano> duration_create_chronicle{}, duration_acquire_story{}, duration_release_story{}, duration_destroy_story{}, duration_destroy_chronicle{};
    int flags;
    int ret;

    std::string server_uri = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.PROTO_CONF + "://" +
                             confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.IP + ":" +
                             std::to_string(
                                     confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.BASE_PORT);

    std::string client_id = gen_random(8);
    LOG_INFO("[ClientLibMetadataRPCTest] Connecting to server with URI: {}", server_uri);
    client.Connect();

    chronicle_names.reserve(NUM_CHRONICLE);
    shared_state.resize(NUM_CHRONICLE, 0); // Initialize shared state array

    LOG_INFO("[ClientLibMetadataRPCTest] Starting creation of {} chronicles.", NUM_CHRONICLE);
    for(int i = 0; i < NUM_CHRONICLE; i++)
    {
        std::string chronicle_name(gen_random(CHRONICLE_NAME_LEN));
        chronicle_names.emplace_back(chronicle_name);
        LOG_INFO("[ClientLibMetadataRPCTest] Created chronicle: {}", chronicle_names[i]);

        std::map <std::string, std::string> chronicle_attrs;
        chronicle_attrs.emplace("Priority", "High");
        chronicle_attrs.emplace("Date", "2023-01-15");
        chronicle_attrs.emplace("IndexGranularity", "Millisecond");
        chronicle_attrs.emplace("TieringPolicy", "Hot");

        t1 = std::chrono::steady_clock::now();
        ret = client.CreateChronicle(chronicle_name, chronicle_attrs, flags);
        t2 = std::chrono::steady_clock::now();
        assert(ret == chronolog::CL_SUCCESS);
        duration_create_chronicle += (t2 - t1);

        update_shared_state_and_wait(i, CHRONICLE_CREATED); // State: CHRONICLE_CREATED

        std::vector <std::string> story_names;
        story_names.reserve(NUM_STORY);
        for(int j = 0; j < NUM_STORY; j++)
        {
            flags = 2;
            std::string story_name(gen_random(STORY_NAME_LEN));
            story_names.emplace_back(story_name);
            std::map <std::string, std::string> story_attrs;
            story_attrs.emplace("Priority", "High");
            story_attrs.emplace("IndexGranularity", "Millisecond");
            story_attrs.emplace("TieringPolicy", "Hot");

            t1 = std::chrono::steady_clock::now();
            ret = client.AcquireStory(chronicle_name, story_name, story_attrs, flags).first;
            t2 = std::chrono::steady_clock::now();
            assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NO_KEEPERS);
            duration_acquire_story += (t2 - t1);
        }

        update_shared_state_and_wait(i, STORY_ACQUIRED); // State: STORY_ACQUIRED

        for(int j = 0; j < NUM_STORY; j++)
        {
            flags = 4;
            t1 = std::chrono::steady_clock::now();
            ret = client.ReleaseStory(chronicle_name, story_names[j]);
            t2 = std::chrono::steady_clock::now();
            assert(ret == chronolog::CL_SUCCESS);
            duration_release_story += (t2 - t1);
        }

        update_shared_state_and_wait(i, STORY_RELEASED); // State: STORY_RELEASED

        for(int j = 0; j < NUM_STORY; j++)
        {
            flags = 8;
            t1 = std::chrono::steady_clock::now();
            ret = client.DestroyStory(chronicle_name, story_names[j]);
            t2 = std::chrono::steady_clock::now();
            assert(ret == chronolog::CL_SUCCESS);
            duration_destroy_story += (t2 - t1);
        }

        update_shared_state_and_wait(i, STORY_DESTROYED); // State: STORY_DESTROYED
    }

    for(int i = 0; i < NUM_CHRONICLE; i++)
    {
        flags = 32;
        t1 = std::chrono::steady_clock::now();
        ret = client.DestroyChronicle(chronicle_names[i]);
        t2 = std::chrono::steady_clock::now();
        assert(ret == chronolog::CL_SUCCESS);
        duration_destroy_chronicle += (t2 - t1);

        update_shared_state_and_wait(i, CHRONICLE_DESTROYED); // State: CHRONICLE_DESTROYED
    }

    LOG_INFO("[ClientLibMetadataRPCTest] Disconnecting from the server.");
    client.Disconnect();

    LOG_INFO("[ClientLibMetadataRPCTest] CreateChronicle takes {} ns",
            duration_create_chronicle.count() / NUM_CHRONICLE);
    LOG_INFO("[ClientLibMetadataRPCTest] AcquireStory takes {} ns",
            duration_acquire_story.count() / (NUM_CHRONICLE * NUM_STORY));
    LOG_INFO("[ClientLibMetadataRPCTest] ReleaseStory takes {} ns",
            duration_release_story.count() / (NUM_CHRONICLE * NUM_STORY));
    LOG_INFO("[ClientLibMetadataRPCTest] DestroyStory takes {} ns",
            duration_destroy_story.count() / (NUM_CHRONICLE * NUM_STORY));
    LOG_INFO("[ClientLibMetadataRPCTest] DestroyChronicle takes {} ns",
            duration_destroy_chronicle.count() / NUM_CHRONICLE);

    return 0;
}
