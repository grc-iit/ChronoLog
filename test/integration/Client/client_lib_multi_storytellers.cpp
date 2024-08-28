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


/*
 * Elements to check:
 * 1. All threads initialized
 * 2. Chronicle created
 * 3. Story Acquired.
 * 4. Finished logging data
 * 5. Release Story
 * 6. Destroy Story
 * 7. Destroy Chronicle
 */

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

void thread_body(struct thread_arg*t)
{
    // Local variable declarations
    int flags = 0;
    uint64_t offset;
    int ret;
    std::string chronicle_name;

    // Lock the mutex to safely update shared state
    {
        std::lock_guard <std::mutex> lock(state_mutex);
        shared_state[t->tid] = 1; // Update shared state
        LOG_DEBUG("[ClientLibMultiStorytellers] Created, State updated: tid={}, State={}", t->tid
                  , shared_state[t->tid]);

        // Notify other threads that might be waiting for state changes
        state_cv.notify_all(); // Notify all threads that the state has changed
    }

    // Wait until all threads have their state set to 1
    {
        std::unique_lock <std::mutex> lock(state_mutex); // Lock mutex again
        state_cv.wait(lock, []
        {
            return std::all_of(shared_state.begin(), shared_state.end(), [](int state)
            { return state == 1; });
        });
        LOG_DEBUG("[ClientLibMultiStorytellers] All threads have state 1, proceeding: tid={}", t->tid);
    }

    if(t->tid % 2 == 0)
        chronicle_name = "CHRONICLE_2";
    else
        chronicle_name = "CHRONICLE_1";

    LOG_DEBUG("[ClientLibMultiStorytellers] Thread={} using chronicle name={}", t->tid, chronicle_name);


    // Create attributes for the chronicle
    std::map <std::string, std::string> chronicle_attrs;
    chronicle_attrs.emplace("Priority", "High");
    flags = 1;

    // Create the chronicle
    ret = client->CreateChronicle(chronicle_name, chronicle_attrs, flags);
    LOG_DEBUG("[ClientLibMultiStorytellers] Chronicle creation attempted: tid={}, ChronicleName={}, Flags: {}, Ret: {}"
              , t->tid, chronicle_name, flags, ret);

    // Check if chronicle creation was successful or if chronicle already exists
    if(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_CHRONICLE_EXISTS)
    {
        // Lock the mutex to safely update shared state to 2
        {
            std::lock_guard <std::mutex> lock(state_mutex);
            shared_state[t->tid] = 2; // Update shared state to 2
            LOG_DEBUG("[ClientLibMultiStorytellers] Chronicle handled, State updated to 2: tid={}, State={}", t->tid
                      , shared_state[t->tid]);
            state_cv.notify_all(); // Notify all threads that the state has changed
        }

        // Wait until all threads have their state set to 2
        {
            std::unique_lock <std::mutex> lock(state_mutex);
            state_cv.wait(lock, []
            {
                return std::all_of(shared_state.begin(), shared_state.end(), [](int state)
                { return state == 2; });
            });
            LOG_DEBUG("[ClientLibMultiStorytellers] All threads have state 2, proceeding: tid={}", t->tid);
        }
    }
    else
    {
        LOG_ERROR(
                "[ClientLibMultiStorytellers] Failed to create chronicle: tid={}, ChronicleName={}, Flags: {}, ErrorCode: {}"
                , t->tid, chronicle_name, flags, ret);
        assert(false && "Chronicle creation failed with unexpected error code!");
    }

    // Create attributes for the story
    std::string story_name = gen_random(STORY_NAME_LEN);
    std::map <std::string, std::string> story_attrs;
    flags = 2;

    // Acquire the story
    auto acquire_ret = client->AcquireStory(chronicle_name, story_name, story_attrs, flags);
    LOG_DEBUG("[ClientLibMultiStorytellers] Story acquired: tid={}, ChronicleName={}, StoryName={}, Ret: {}", t->tid
              , chronicle_name, story_name, acquire_ret.first);

    // Assertion for successful story acquisition or expected errors
    assert(acquire_ret.first == chronolog::CL_SUCCESS || acquire_ret.first == chronolog::CL_ERR_NOT_EXIST ||
           acquire_ret.first == chronolog::CL_ERR_NO_KEEPERS);

    // If story acquisition is successful, log events to the story
    if(chronolog::CL_SUCCESS == acquire_ret.first)
    {
        // Lock the mutex to safely update shared state to 3
        {
            std::lock_guard <std::mutex> lock(state_mutex);
            shared_state[t->tid] = 3; // Update shared state to 3
            LOG_DEBUG("[ClientLibMultiStorytellers] Story acquired, State updated to 3: tid={}, State={}", t->tid
                      , shared_state[t->tid]);
            state_cv.notify_all(); // Notify all threads that the state has changed
        }

        // Wait until all threads have their state set to 3
        {
            std::unique_lock <std::mutex> lock(state_mutex);
            state_cv.wait(lock, []
            {
                return std::all_of(shared_state.begin(), shared_state.end(), [](int state)
                { return state == 3; });
            });
            LOG_DEBUG("[ClientLibMultiStorytellers] All threads have state 3, proceeding: tid={}", t->tid);
        }

        auto story_handle = acquire_ret.second;
        for(int i = 0; i < 100; ++i)
        {
            // Log an event to the story
            story_handle->log_event("line " + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(i % 10));
        }
        // Lock the mutex to safely update shared state to 3
        {
            std::lock_guard <std::mutex> lock(state_mutex);
            shared_state[t->tid] = 4; // Update shared state to 3
            LOG_DEBUG("[ClientLibMultiStorytellers] Data logged, State updated to 4: tid={}, State={}", t->tid
                      , shared_state[t->tid]);
            state_cv.notify_all(); // Notify all threads that the state has changed
        }

        // Wait until all threads have their state set to 3
        {
            std::unique_lock <std::mutex> lock(state_mutex);
            state_cv.wait(lock, []
            {
                return std::all_of(shared_state.begin(), shared_state.end(), [](int state)
                { return state == 4; });
            });
            LOG_DEBUG("[ClientLibMultiStorytellers] All threads have state 4, proceeding: tid={}", t->tid);
        }

        // Release the story
        ret = client->ReleaseStory(chronicle_name, story_name);
        LOG_DEBUG("[ClientLibMultiStorytellers] Story released: tid={}, ChronicleName={}, StoryName={}, Ret: {}", t->tid
                  , chronicle_name, story_name, ret);

        // Assertion for successful story release or expected errors
        assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NO_CONNECTION);

        if(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NO_CONNECTION)
        {
            // Lock the mutex to safely update shared state to 2
            {
                std::lock_guard <std::mutex> lock(state_mutex);
                shared_state[t->tid] = 5; // Update shared state to 2
                LOG_DEBUG("[ClientLibMultiStorytellers] Story released, State updated to 5: tid={}, State={}", t->tid
                          , shared_state[t->tid]);
                state_cv.notify_all(); // Notify all threads that the state has changed
            }

            // Wait until all threads have their state set to 2
            {
                std::unique_lock <std::mutex> lock(state_mutex);
                state_cv.wait(lock, []
                {
                    return std::all_of(shared_state.begin(), shared_state.end(), [](int state)
                    { return state == 5; });
                });
                LOG_DEBUG("[ClientLibMultiStorytellers] All threads have state 5, proceeding: tid={}", t->tid);
            }
        }
        else
        {
            LOG_ERROR(
                    "[ClientLibMultiStorytellers] Failed to release the story: tid={}, ChronicleName={}, StoryName={}, Ret: {}"
                    , t->tid, chronicle_name, story_name, ret);
        }
    }

    // Destroy the story
    ret = client->DestroyStory(chronicle_name, story_name);
    LOG_DEBUG("[ClientLibMultiStorytellers] Story destroyed: tid={}, ChronicleName={}, StoryName={}, Ret: {}", t->tid
              , chronicle_name, story_name, ret);

    // Assertion for successful story destruction or expected errors
    assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NOT_EXIST || ret == chronolog::CL_ERR_ACQUIRED ||
           ret == chronolog::CL_ERR_NO_CONNECTION);

    if(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NOT_EXIST || ret == chronolog::CL_ERR_ACQUIRED ||
       ret == chronolog::CL_ERR_NO_CONNECTION)
    {
        // Lock the mutex to safely update shared state to 2
        {
            std::lock_guard <std::mutex> lock(state_mutex);
            shared_state[t->tid] = 6; // Update shared state to 2
            LOG_DEBUG("[ClientLibMultiStorytellers] Story destroyed, State updated to 6: tid={}, State={}", t->tid
                      , shared_state[t->tid]);
            state_cv.notify_all(); // Notify all threads that the state has changed
        }

        // Wait until all threads have their state set to 2
        {
            std::unique_lock <std::mutex> lock(state_mutex);
            state_cv.wait(lock, []
            {
                return std::all_of(shared_state.begin(), shared_state.end(), [](int state)
                { return state == 6; });
            });
            LOG_DEBUG("[ClientLibMultiStorytellers] All threads have state 6, proceeding: tid={}", t->tid);
        }
    }
    else
    {
        LOG_ERROR(
                "[ClientLibMultiStorytellers] Failed to destroy the story: tid={}, ChronicleName={}, StoryName={}, Ret:"
                " {}", t->tid, chronicle_name, story_name, ret);
    }

    // Destroy the chronicle
    ret = client->DestroyChronicle(chronicle_name);
    LOG_DEBUG("[ClientLibMultiStorytellers] Chronicle destroyed: tid={}, ChronicleName={}", t->tid, chronicle_name);

    // Assertion for successful chronicle destruction or expected errors
    assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NOT_EXIST || ret == chronolog::CL_ERR_ACQUIRED ||
           ret == chronolog::CL_ERR_NO_CONNECTION);

    if(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NOT_EXIST || ret == chronolog::CL_ERR_ACQUIRED ||
       ret == chronolog::CL_ERR_NO_CONNECTION)
    {
        // Lock the mutex to safely update shared state to 2
        {
            std::lock_guard <std::mutex> lock(state_mutex);
            shared_state[t->tid] = 7; // Update shared state to 2
            LOG_DEBUG("[ClientLibMultiStorytellers] Chronicle destroyed, State updated to 7: tid={}, State={}", t->tid
                      , shared_state[t->tid]);
            state_cv.notify_all(); // Notify all threads that the state has changed
        }

        // Wait until all threads have their state set to 2
        {
            std::unique_lock <std::mutex> lock(state_mutex);
            state_cv.wait(lock, []
            {
                return std::all_of(shared_state.begin(), shared_state.end(), [](int state)
                { return state == 7; });
            });
            LOG_DEBUG("[ClientLibMultiStorytellers] All threads have state 7, proceeding: tid={}", t->tid);
        }
    }
    else
    {
        LOG_ERROR(
                "[ClientLibMultiStorytellers] Failed to destroy the chronicle: tid={}, ChronicleName={}, StoryName={}, "
                "Ret:"
                " {}", t->tid, chronicle_name, story_name, ret);
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
