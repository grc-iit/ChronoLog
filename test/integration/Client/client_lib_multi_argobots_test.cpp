//
// Created by kfeng on 7/18/22.
//
#include <chronolog_client.h>
#include <common.h>
#include <thread>
#include <abt.h>
#include <atomic>
#include <cmd_arg_parse.h>
#include "chrono_monitor.h"

#define CHRONICLE_NAME_LEN 32
#define STORY_NAME_LEN 32

chronolog::Client*client;

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

struct thread_arg
{
    int tid;
    std::string client_id;
};

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

void thread_function(void*tt)
{
    struct thread_arg*t = (struct thread_arg*)tt;

    int flags = 0;
    int ret;
    std::string chronicle_name = (t->tid % 2 == 0) ? "Chronicle_1" : "Chronicle_2";

    update_shared_state_and_wait(t->tid, THREAD_INITIALIZED); // State: THREAD_INITIALIZED

    std::map <std::string, std::string> chronicle_attrs;
    chronicle_attrs.emplace("Priority", "High");
    chronicle_attrs.emplace("IndexGranularity", "Millisecond");
    chronicle_attrs.emplace("TieringPolicy", "Hot");
    ret = client->CreateChronicle(chronicle_name, chronicle_attrs, flags);
    LOG_DEBUG("[ClientLibMultiArgobotsTest] Thread (ID: {}) - Created Chronicle: {}. Return Code: {}", t->tid
              , chronicle_name, ret);
    assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_CHRONICLE_EXISTS ||
           ret == chronolog::CL_ERR_NO_KEEPERS);

    update_shared_state_and_wait(t->tid, CHRONICLE_CREATED); // State: CHRONICLE_CREATED

    flags = 2;
    std::string story_name = gen_random(STORY_NAME_LEN);
    std::map <std::string, std::string> story_attrs;
    story_attrs.emplace("Priority", "High");
    story_attrs.emplace("IndexGranularity", "Millisecond");
    story_attrs.emplace("TieringPolicy", "Hot");
    auto acquire_ret = client->AcquireStory(chronicle_name, story_name, story_attrs, flags);
    LOG_DEBUG("[ClientLibMultiArgobotsTest] Thread ID: {} - Acquired Story: {} in Chronicle: {}. Result Code: {}"
              , t->tid, story_name, chronicle_name, acquire_ret.first);
    assert(acquire_ret.first == chronolog::CL_SUCCESS || acquire_ret.first == chronolog::CL_ERR_NOT_EXIST ||
           acquire_ret.first == chronolog::CL_ERR_NO_KEEPERS);

    update_shared_state_and_wait(t->tid, STORY_ACQUIRED); // State: STORY_ACQUIRED

    ret = client->ReleaseStory(chronicle_name, story_name); // Release the story
    LOG_DEBUG("[ClientLibMultiArgobotsTest] Thread ID: {} - Released Story: {} from Chronicle: {}. Result Code: {}"
              , t->tid, story_name, chronicle_name, ret);
    assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NO_KEEPERS || ret == chronolog::CL_ERR_NOT_EXIST);

    update_shared_state_and_wait(t->tid, STORY_RELEASED); // State: STORY_RELEASED

    ret = client->DestroyStory(chronicle_name, story_name);
    LOG_DEBUG("[ClientLibMultiArgobotsTest] Thread ID: {} - Destroyed Story: {} from Chronicle: {}. Result Code: {}"
              , t->tid, story_name, chronicle_name, ret);
    assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NOT_EXIST || ret == chronolog::CL_ERR_ACQUIRED ||
           ret == chronolog::CL_ERR_NO_KEEPERS);

    update_shared_state_and_wait(t->tid, STORY_DESTROYED); // State: STORY_DESTROYED

    ret = client->DestroyChronicle(chronicle_name);
    LOG_DEBUG("[ClientLibMultiArgobotsTest] Thread ID: {} - Destroyed Chronicle: {}. Return Code: {}", t->tid
              , chronicle_name, ret);
    assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NOT_EXIST || ret == chronolog::CL_ERR_ACQUIRED);

    update_shared_state_and_wait(t->tid, CHRONICLE_DESTROYED); // State: CHRONICLE_DESTROYED
}

int main(int argc, char**argv)
{
    std::atomic <long> duration_connect{}, duration_disconnect{};
    std::vector <std::thread> thread_vec;
    uint64_t offset;

    std::string conf_file_path;
    conf_file_path = parse_conf_path_arg(argc, argv);
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
    LOG_INFO("[ClientLibMultiArgobotsTest] Running test.");


    std::string server_ip = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.IP;
    int base_port = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.BASE_PORT;
    client = new chronolog::Client(confManager); //protocol, server_ip, base_port);

    int num_xstreams = 8;
    int num_threads = 8;

    // Initialize shared state array
    shared_state.resize(num_threads, 0); // Initialize with zeros

    ABT_xstream*xstreams = (ABT_xstream*)malloc(sizeof(ABT_xstream) * num_xstreams);
    ABT_pool*pools = (ABT_pool*)malloc(sizeof(ABT_pool) * num_xstreams);
    ABT_thread*threads = (ABT_thread*)malloc(sizeof(ABT_thread) * num_threads);
    std::vector <struct thread_arg> t_args(num_threads);;

    std::string client_id = gen_random(8);;
    std::string server_uri = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.PROTO_CONF;
    server_uri += "://" + server_ip + ":" + std::to_string(base_port);
    int flags = 0;

    int ret = client->Connect();//server_uri, client_id, flags);//, offset);
    if(ret == chronolog::CL_SUCCESS)
    {
        LOG_INFO("[ClientLibMultiArgobotsTest] Connected to the server successfully.");
    }
    else
    {
        LOG_ERROR("[ClientLibMultiArgobotsTest] Failed to connect to the server. Error code: {}", ret);
        exit(1);  // Exit if connection fails
    }
    assert(ret == chronolog::CL_SUCCESS);

    for(int i = 0; i < num_threads; i++)
    {
        t_args[i].tid = i;
        t_args[i].client_id = client_id;
    }

    ABT_init(argc, argv);

    ABT_xstream_self(&xstreams[0]);

    for(int i = 1; i < num_xstreams; i++)
    {
        ABT_xstream_create(ABT_SCHED_NULL, &xstreams[i]);
    }


    for(int i = 0; i < num_xstreams; i++)
    {
        ABT_xstream_get_main_pools(xstreams[i], 1, &pools[i]);
    }


    for(int i = 0; i < num_threads; i++)
    {
        ABT_thread_create(pools[i], thread_function, &t_args[i], ABT_THREAD_ATTR_NULL, &threads[i]);
    }

    for(int i = 0; i < num_threads; i++)
        ABT_thread_free(&threads[i]);

    for(int i = 1; i < num_xstreams; i++)
    {
        ABT_xstream_join(xstreams[i]);
        ABT_xstream_free(&xstreams[i]);
    }

    ABT_finalize();

    free(pools);
    free(xstreams);
    free(threads);

    ret = client->Disconnect(); //client_id, flags);
    LOG_INFO("[ClientLibMultiArgobotsTest] Main disconnect return: {}", ret);
    ret = client->Disconnect();
    if(ret == chronolog::CL_SUCCESS)
    {
        LOG_INFO("[ClientLibMultiArgobotsTest] Disconnected from the server successfully.");
    }
    else
    {
        LOG_ERROR("[ClientLibMultiArgobotsTest] Failed to disconnect from the server. Error code: {}", ret);
        // Handle the failure scenario accordingly
    }
    assert(ret == chronolog::CL_SUCCESS);

    delete client;

    return 0;
}
