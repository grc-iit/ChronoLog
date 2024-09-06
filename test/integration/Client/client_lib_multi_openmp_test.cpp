#include <chronolog_client.h>
#include <cassert>
#include <common.h>
#include <thread>
#include <omp.h>
#include <cmd_arg_parse.h>
#include "chrono_monitor.h"
#include <mutex>
#include <condition_variable>
#include <vector>
#include <iostream>

#define STORY_NAME_LEN 32

// Define bitmask for thread states
enum ThreadState
{
    THREAD_INITIALIZED = 1 << 0, // 0001
    CHRONICLE_CREATED = 1 << 1, // 0010
    STORY_ACQUIRED = 1 << 2, // 0100
    STORY_RELEASED = 1 << 3, // 1000
    STORY_DESTROYED = 1 << 4, // 1 << 4
    CHRONICLE_DESTROYED = 1 << 5  // 1 << 5
};

std::vector <int> shared_state; // Global array of thread states
std::mutex state_mutex;
std::condition_variable state_cv;

// Function to update shared state and wait for all threads to reach a specific state
void update_shared_state_and_wait(int tid, int target_state_bitmask)
{
    {
        std::lock_guard <std::mutex> lock(state_mutex);
        shared_state[tid] |= target_state_bitmask; // Update the bitmask state
        std::cout << "Thread " << tid << " state updated: " << shared_state[tid] << std::endl;
        state_cv.notify_all();
    }
    {
        std::unique_lock <std::mutex> lock(state_mutex);
        state_cv.wait(lock, [target_state_bitmask]()
        {
            // Check if all threads have at least the target state bit set
            return std::all_of(shared_state.begin(), shared_state.end(), [target_state_bitmask](int state)
            {
                return (state&target_state_bitmask) == target_state_bitmask;
            });
        });
        std::cout << "All threads reached state " << target_state_bitmask << std::endl;
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

    LOG_INFO("[ClientLibMultiOpenMPTest] Running test.");

    std::string server_ip = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.IP;
    int base_port = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.BASE_PORT;
    chronolog::Client*client = new chronolog::Client(confManager);

    int num_threads = 8;
    omp_set_num_threads(num_threads);

    std::string server_uri = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.PROTO_CONF;
    server_uri += "://" + server_ip + ":" + std::to_string(base_port);

    LOG_INFO("[ClientLibMultiOpenMPTest] Connecting to server at: {}", server_uri);

    int flags = 0;
    std::string client_id = gen_random(8);
    int ret = client->Connect();
    LOG_INFO("[ClientLibMultiOpenMPTest] Successfully connected to the server.");

    // Initialize shared state array
    shared_state.resize(num_threads, 0); // Initialize with zeros

#pragma omp parallel for
    for(int i = 0; i < num_threads; i++)
    {
        update_shared_state_and_wait(i, THREAD_INITIALIZED); // State: THREAD_INITIALIZED

        std::string chronicle_name;
        if(i % 2 == 0)
            chronicle_name = "gscs5er9TcdJ9mOgUDteDVBcI0oQjozK";
        else
            chronicle_name = "6RPkwqX2IOpR41dVCqmWauX9RfXIuTAp";

        std::map <std::string, std::string> chronicle_attrs;
        chronicle_attrs.emplace("Priority", "High");
        chronicle_attrs.emplace("IndexGranularity", "Millisecond");
        chronicle_attrs.emplace("TieringPolicy", "Hot");

        ret = client->CreateChronicle(chronicle_name, chronicle_attrs, flags);
        LOG_INFO("[ClientLibMultiOpenMPTest] Thread {} creating chronicle: {}", i, chronicle_name);

        update_shared_state_and_wait(i, CHRONICLE_CREATED); // State: CHRONICLE_CREATED

        flags = 1;
        std::string story_name = gen_random(STORY_NAME_LEN);
        LOG_INFO("[ClientLibMultiOpenMPTest] Thread {} creating story: {}", i, story_name);

        std::map <std::string, std::string> story_attrs;
        story_attrs.emplace("Priority", "High");
        story_attrs.emplace("IndexGranularity", "Millisecond");
        story_attrs.emplace("TieringPolicy", "Hot");
        flags = 2;
        auto acquire_ret = client->AcquireStory(chronicle_name, story_name, story_attrs, flags);
        assert(acquire_ret.first == chronolog::CL_SUCCESS);

        update_shared_state_and_wait(i, STORY_ACQUIRED); // State: STORY_ACQUIRED

        ret = client->DestroyStory(chronicle_name, story_name);
        LOG_INFO("[ClientLibMultiOpenMPTest] Thread {} destroying story: {}", i, story_name);
        assert(ret == chronolog::CL_ERR_ACQUIRED);

        update_shared_state_and_wait(i, STORY_RELEASED); // State: STORY_RELEASED

        ret = client->ReleaseStory(chronicle_name, story_name);
        assert(ret == chronolog::CL_SUCCESS);

        ret = client->DestroyStory(chronicle_name, story_name);
        assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NOT_EXIST || ret == chronolog::CL_ERR_ACQUIRED);

        update_shared_state_and_wait(i, STORY_DESTROYED); // State: STORY_DESTROYED

        ret = client->DestroyChronicle(chronicle_name);
        assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NOT_EXIST || ret == chronolog::CL_ERR_ACQUIRED);
        LOG_INFO("[ClientLibMultiOpenMPTest] Thread {} destroying chronicle: {}", i, chronicle_name);

        update_shared_state_and_wait(i, CHRONICLE_DESTROYED); // State: CHRONICLE_DESTROYED
    }

    // Disconnecting from the server
    LOG_INFO("[ClientLibMultiOpenMPTest] Disconnecting from the server.");
    ret = client->Disconnect();
    assert(ret == chronolog::CL_SUCCESS);
    LOG_INFO("[ClientLibMultiOpenMPTest] Disconnected successfully.");

    delete client;

    return 0;
}
