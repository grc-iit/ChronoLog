#include <chronolog_client.h>
#include <cassert>
#include <common.h>
#include <thread>
#include <chrono>
#include <cmd_arg_parse.h>
#include "chrono_monitor.h"
#include <vector>

#define STORY_NAME_LEN 5

struct thread_arg
{
    int tid;
    std::string client_id;
};

chronolog::Client*client;

void thread_body(struct thread_arg*t)
{
    // Chronicle creation
    std::string chronicle_name = (t->tid % 2 == 0) ? "CHRONICLE_2" : "CHRONICLE_1";
    std::map <std::string, std::string> chronicle_attrs;
    chronicle_attrs.emplace("Priority", "High");
    int flags = 1;
    int ret = client->CreateChronicle(chronicle_name, chronicle_attrs, flags);
    LOG_DEBUG("[ClientLibMultiStorytellers] Chronicle creation: tid= {}, Ret: {}", std::to_string(t->tid), ret);


    // Acquire story
    std::string story_name = gen_random(STORY_NAME_LEN);
    std::map <std::string, std::string> story_attrs;
    flags = 2;
    auto acquire_ret = client->AcquireStory(chronicle_name, story_name, story_attrs, flags);
    LOG_DEBUG("[ClientLibMultiStorytellers] Story acquired: tid={}, Ret: {}", std::to_string(t->tid)
              , acquire_ret.first);
    assert(acquire_ret.first == chronolog::CL_SUCCESS || acquire_ret.first == chronolog::CL_ERR_NOT_EXIST ||
           acquire_ret.first == chronolog::CL_ERR_NO_KEEPERS);


    if(chronolog::CL_SUCCESS == acquire_ret.first)
    {
        // Log Data
        auto story_handle = acquire_ret.second;
        for(int i = 0; i < 100; ++i)
        {
            story_handle->log_event("line " + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(i % 10)); // Simulate work
        }


        // Release the story
        ret = client->ReleaseStory(chronicle_name, story_name);
        LOG_DEBUG("[ClientLibMultiStorytellers] Story released: tid={}, Ret: {}", std::to_string(t->tid), ret);
        if(ret != chronolog::CL_SUCCESS && ret != chronolog::CL_ERR_NOT_ACQUIRED)
        {
            assert(false && "Story couldn't get released!");
        }
        assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NO_CONNECTION);
    }

    // Destroy the story
    ret = client->DestroyStory(chronicle_name, story_name);
    LOG_DEBUG("[ClientLibMultiStorytellers] Story destroyed: tid={}, Ret: {}", std::to_string(t->tid), ret);
    assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NOT_EXIST || ret == chronolog::CL_ERR_ACQUIRED ||
           ret == chronolog::CL_ERR_NO_CONNECTION);

    // Destroy the chronicle
    ret = client->DestroyChronicle(chronicle_name);
    LOG_DEBUG("[ClientLibMultiStorytellers] Chronicle destroyed: tid={}, Ret: {}", std::to_string(t->tid), ret);
    assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NOT_EXIST || ret == chronolog::CL_ERR_ACQUIRED ||
           ret == chronolog::CL_ERR_NO_CONNECTION);
}


int main(int argc, char**argv)
{
    // Configuration
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
        std::exit(EXIT_FAILURE);
    }

    // Set up client & Connect
    client = new chronolog::Client(confManager);
    int ret = client->Connect();
    if(chronolog::CL_SUCCESS != ret)
    {
        LOG_ERROR("[ClientLibMultiStorytellers] Failed to connect to ChronoVisor");
        delete client;
        return -1;
    }

    // Initiate test
    LOG_INFO("[ClientLibMultiStorytellers] Running test.");
    int num_threads = 4;
    std::vector <struct thread_arg> t_args(num_threads);
    std::vector <std::thread> workers(num_threads);
    std::string client_id = gen_random(8);

    // Create and start the worker threads
    for(int i = 0; i < num_threads; i++)
    {
        t_args[i].tid = i;  // Assign thread ID
        t_args[i].client_id = client_id;  // Assign client ID
        std::thread t{thread_body, &t_args[i]};  // Start the thread
        workers[i] = std::move(t);  // Move thread to workers vector
    }

    // Join all worker threads to wait for their completion
    for(int i = 0; i < num_threads; i++)
    {
        workers[i].join();
    }

    // Disconnect the client and clean up
    ret = client->Disconnect();
    delete client;

    // Return success
    return 0;
}
