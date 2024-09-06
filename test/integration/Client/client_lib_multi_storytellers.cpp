#include <chronolog_client.h>
#include <cassert>
#include <common.h>
#include <thread>
#include <chrono>
#include <cmd_arg_parse.h>
#include "chrono_monitor.h"
#include <mutex>
#include <vector>
#include <condition_variable>

#define STORY_NAME_LEN 5

struct thread_arg
{
    int tid;
    std::string client_id;
};

chronolog::Client*client;

std::vector <int> shared_state;
std::mutex state_mutex;
std::condition_variable state_cv;

// Define bitmask for thread states
enum ThreadState
{
    THREAD_INITIALIZED = 1 << 0, // 00000001 (binary), 1 (decimal)
    CHRONICLE_CREATED = 1 << 1, // 00000010 (binary), 2 (decimal)
    STORY_ACQUIRED = 1 << 2, // 00000100 (binary), 4 (decimal)
    FINISHED_LOGGING = 1 << 3, // 00001000 (binary), 8 (decimal)
    STORY_RELEASED = 1 << 4, // 00010000 (binary), 16 (decimal)
    STORY_DESTROYED = 1 << 5, // 00100000 (binary), 32 (decimal)
    CHRONICLE_DESTROYED = 1 << 6  // 01000000 (binary), 64 (decimal)
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

// Function to handle ChronoLog API call, log results, and assert conditions
bool handle_chronolog_api_call(const std::string &log_message, int ret, const std::vector <int> &expected_errors)
{
    LOG_DEBUG(log_message + ", Ret: {}", ret);
    if(std::find(expected_errors.begin(), expected_errors.end(), ret) == expected_errors.end())
    {
        LOG_ERROR(log_message + ", Unexpected ErrorCode: {}", ret);
        return false;
    }
    return true;
}

void thread_body(struct thread_arg*t)
{
    int flags = 0;
    std::string chronicle_name = (t->tid % 2 == 0) ? "CHRONICLE_2" : "CHRONICLE_1";

    update_shared_state_and_wait(t->tid, THREAD_INITIALIZED); // State: Threads initialized

    // Create the chronicle
    std::map <std::string, std::string> chronicle_attrs;
    chronicle_attrs.emplace("Priority", "High");
    flags = 1;
    int ret = client->CreateChronicle(chronicle_name, chronicle_attrs, flags);

    if(handle_chronolog_api_call(
            "[ClientLibMultiStorytellers] Chronicle creation attempted: tid=" + std::to_string(t->tid), ret, {
                    chronolog::CL_SUCCESS, chronolog::CL_ERR_CHRONICLE_EXISTS}))
    {
        update_shared_state_and_wait(t->tid, CHRONICLE_CREATED); // State: Chronicle created
    }
    else
    {
        assert(false && "Chronicle creation failed with unexpected error code!");
    }

    // Acquire the story
    std::string story_name = gen_random(STORY_NAME_LEN);
    std::map <std::string, std::string> story_attrs;
    flags = 2;
    auto acquire_ret = client->AcquireStory(chronicle_name, story_name, story_attrs, flags);

    if(handle_chronolog_api_call("[ClientLibMultiStorytellers] Story acquired: tid=" + std::to_string(t->tid)
                                 , acquire_ret.first, {chronolog::CL_SUCCESS, chronolog::CL_ERR_NOT_EXIST
                                                       , chronolog::CL_ERR_NO_KEEPERS}))
    {
        update_shared_state_and_wait(t->tid, STORY_ACQUIRED); // State: Story acquired

        if(acquire_ret.first == chronolog::CL_SUCCESS)
        {
            auto story_handle = acquire_ret.second;
            for(int i = 0; i < 100; ++i)
            {
                story_handle->log_event("line " + std::to_string(i));
                std::this_thread::sleep_for(std::chrono::milliseconds(i % 10));
            }
            update_shared_state_and_wait(t->tid, FINISHED_LOGGING); // State: Finished logging data

            ret = client->ReleaseStory(chronicle_name, story_name);
            if(handle_chronolog_api_call("[ClientLibMultiStorytellers] Story released: tid=" + std::to_string(t->tid)
                                         , ret, {chronolog::CL_SUCCESS, chronolog::CL_ERR_NO_CONNECTION}))
            {
                update_shared_state_and_wait(t->tid, STORY_RELEASED); // State: Story released
            }
        }

        // Destroy the story
        ret = client->DestroyStory(chronicle_name, story_name);
        if(handle_chronolog_api_call("[ClientLibMultiStorytellers] Story destroyed: tid=" + std::to_string(t->tid), ret
                                     , {chronolog::CL_SUCCESS, chronolog::CL_ERR_NOT_EXIST, chronolog::CL_ERR_ACQUIRED
                                        , chronolog::CL_ERR_NO_CONNECTION}))
        {
            update_shared_state_and_wait(t->tid, STORY_DESTROYED); // State: Story destroyed
        }

        // Destroy the chronicle
        ret = client->DestroyChronicle(chronicle_name);
        if(handle_chronolog_api_call("[ClientLibMultiStorytellers] Chronicle destroyed: tid=" + std::to_string(t->tid)
                                     , ret, {chronolog::CL_SUCCESS, chronolog::CL_ERR_NOT_EXIST
                                             , chronolog::CL_ERR_ACQUIRED, chronolog::CL_ERR_NO_CONNECTION}))
        {
            update_shared_state_and_wait(t->tid, CHRONICLE_DESTROYED); // State: Chronicle destroyed
        }
    }
}


int main(int argc, char**argv)
{
    std::string conf_file_path;
    conf_file_path = parse_conf_path_arg(argc, argv);
    if(conf_file_path.empty())
    {
        std::exit(EXIT_FAILURE);
    }

    int provided;
    std::string client_id = gen_random(8);

    int num_threads = 4;

    std::vector <struct thread_arg> t_args(num_threads);
    std::vector <std::thread> workers(num_threads);

    // Initialize shared state array
    shared_state.resize(num_threads, 0); // Initialize with zeros

    ChronoLogRPCImplementation protocol = CHRONOLOG_THALLIUM_SOCKETS;
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
    LOG_INFO("[ClientLibMultiStorytellers] Running test.");


    std::string server_ip = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.IP;
    int base_port = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.BASE_PORT;
    client = new chronolog::Client(confManager);//protocol, server_ip, base_port);

    std::string server_uri = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.PROTO_CONF;
    server_uri += "://" + server_ip + ":" + std::to_string(base_port);

    int flags = 0;
    uint64_t offset;
    int ret = client->Connect();

    if(chronolog::CL_SUCCESS != ret)
    {
        LOG_ERROR("[ClientLibMultiStorytellers] Failed to connect to ChronoVisor");
        delete client;
        return -1;
    }

    for(int i = 0; i < num_threads; i++)
    {
        t_args[i].tid = i;
        t_args[i].client_id = client_id;
        std::thread t{thread_body, &t_args[i]};
        workers[i] = std::move(t);
    }

    for(int i = 0; i < num_threads; i++)
        workers[i].join();

    ret = client->Disconnect();
    delete client;

    return 0;
}
