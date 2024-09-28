#include <chronolog_client.h>
#include <cassert>
#include <common.h>
#include <thread>
#include <chrono>
#include <cmd_arg_parse.h>
#include "chrono_monitor.h"
#include <vector>
#include "thread_interdependency.h"
#include <bitset>

#define STORY_NAME_LEN 5

struct thread_arg
{
    int tid;
    std::string client_id;
};

chronolog::Client*client;
std::vector <int> shared_state;

// Enum for thread states represented as bitmasks
enum ThreadState
{
    UNKNOWN = 1 << 0,              // 00000001
    THREAD_INITIALIZED = 1 << 1,   // 00000010
    CHRONICLE_CREATED = 1 << 2,    // 00000100
    STORY_ACQUIRED = 1 << 3,       // 00001000
    DATA_ACCESS_FINISHED = 1 << 4, // 00010000
    STORY_RELEASED = 1 << 5,       // 00100000
    STORY_DESTROYED = 1 << 6,      // 01000000
    CHRONICLE_DESTROYED = 1 << 7,  // 10000000
    THREAD_FINALIZED = 1 << 8      // 10000001
};

// Helper function to get the state name from the bitmask
std::string get_state_name(int state_bitmask)
{
    switch(state_bitmask)
    {
        case UNKNOWN:
            return "UNKNOWN";
        case THREAD_INITIALIZED:
            return "THREAD_INITIALIZED";
        case CHRONICLE_CREATED:
            return "CHRONICLE_CREATED";
        case STORY_ACQUIRED:
            return "STORY_ACQUIRED";
        case DATA_ACCESS_FINISHED:
            return "DATA_ACCESS_FINISHED";
        case STORY_RELEASED:
            return "STORY_RELEASED";
        case STORY_DESTROYED:
            return "STORY_DESTROYED";
        case CHRONICLE_DESTROYED:
            return "CHRONICLE_DESTROYED";
        case THREAD_FINALIZED:
            return "THREAD_FINALIZED";
        default:
            return "UNKNOWN_STATE";
    }
}

void validate_chronicle_created(int tid, int ret)
{
    // Check if the chronicle is already created
    bool chronicle_already_created = false;
    for(size_t i = 0; i < shared_state.size(); ++i)
    {
        if(i == tid) continue;
        if(i % 2 == tid % 2)
        {
            if(shared_state[i]&CHRONICLE_CREATED)
            {
                chronicle_already_created = true;
                break;
            }
        }
    }

    // Validate the return value based on the chronicle creation status
    if(ret == chronolog::CL_SUCCESS)
    {
        // Assert that the chronicle was not already created by another thread
        assert(!chronicle_already_created && "Chronicle was already created, but return value is CL_SUCCESS!");
        std::cout << "Chronicle successfully created by thread " << tid << "." << std::endl;
    }
    else if(ret == chronolog::CL_ERR_CHRONICLE_EXISTS)
    {
        // Assert that the chronicle was already created by another thread
        assert(chronicle_already_created && "Chronicle was not created, but return value is CL_ERR_CHRONICLE_EXISTS!");
        std::cout << "Chronicle was already created by another thread, as expected." << std::endl;
    }
    else
    {
        assert(false && "Chronicle creation failed!");
    }
}


void validate_story_acquired(int tid, int ret)
{
    // Check if the chronicle has already been destroyed
    bool chronicle_destroyed = false;

    for(size_t i = 0; i < shared_state.size(); ++i)
    {
        if(i == tid) continue; // Skip the current thread
        if(i % 2 == tid % 2) // Check if the thread operates on the same chronicle
        {
            if(shared_state[i]&CHRONICLE_DESTROYED)
            {
                chronicle_destroyed = true;
                break; // Exit loop once chronicle destruction is confirmed
            }
        }
    }

    // Validate the return value based on the chronicle state
    if(ret == chronolog::CL_SUCCESS)
    {
        // Assert that the chronicle was not destroyed
        assert(!chronicle_destroyed && "Chronicle was destroyed, but return value is CL_SUCCESS!");
        std::cout << "Thread " << tid << " successfully acquired the story." << std::endl;
    }
    else if(ret == chronolog::CL_ERR_NOT_EXIST)
    {
        // Assert that the chronicle was destroyed or not created
        assert(chronicle_destroyed && "Chronicle was not destroyed, but return value is CL_ERR_NOT_EXIST!");
        std::cerr << "Thread " << tid << " attempted to acquire a story, but the chronicle does not exist."
                  << std::endl;
    }
    else
    {
        std::cerr << "Thread " << tid << " encountered an unexpected error while acquiring the story." << std::endl;
        assert(false && "Unexpected error during story acquisition.");
    }
}


void validate_story_released(int tid, int ret)
{
    if(ret == chronolog::CL_SUCCESS)
    {
        std::cout << "Thread " << tid << " successfully released the story." << std::endl;
    }
    else if(ret == chronolog::CL_ERR_NOT_EXIST)
    {
        // Assert that the chronicle does not exist and the story can't be acquired
        std::cerr << "Thread " << tid << " attempted to acquire a story, but the chronicle does not exist."
                  << std::endl;
        assert(false && "Chronicle has not been created. Cannot acquire the story.");
    }
    else
    {
        std::cerr << "Thread " << tid << " attempted to acquire a story that was already acquired." << std::endl;
        assert(false && "Story was already acquired by another thread.");
    }
}


void validate_story_destroyed(int tid, int ret)
{
    // Determine the current thread's chronicle and story
    std::string chronicle_name = (tid % 2 == 0) ? "CHRONICLE_2" : "CHRONICLE_1";
    std::string story_name = (tid % 4 < 2) ? "STORY_1" : "STORY_2";

    // Check if the chronicle or story has already been destroyed or if the story is still acquired
    bool chronicle_destroyed = false;
    bool story_acquired = false;
    bool story_destroyed = false;

    for(size_t i = 0; i < shared_state.size(); ++i)
    {
        if(i == tid) continue; // Skip the current thread

        // Check if the thread operates on the same chronicle
        if(i % 2 == tid % 2)
        {
            if(shared_state[i]&CHRONICLE_DESTROYED)
            {
                chronicle_destroyed = true;
                break; // No need to check further if chronicle is destroyed
            }

            // Now, check if the thread also operates on the same story
            if(i % 4 < 2 == tid % 4 < 2)
            {
                if(shared_state[i]&STORY_DESTROYED)
                {
                    story_destroyed = true;
                }
                if(shared_state[i]&STORY_ACQUIRED)
                {
                    story_acquired = true;
                }
            }
        }
    }

    // Validate the return value based on the story and chronicle state
    if(ret == chronolog::CL_SUCCESS)
    {
        // Assert that no other threads have acquired or destroyed the story and the chronicle is not destroyed
        assert(!story_acquired && !chronicle_destroyed &&
               "Story is still acquired or chronicle was destroyed, but return value is CL_SUCCESS!");
        std::cout << "Thread " << tid << " successfully destroyed the story." << std::endl;
    }
    else if(ret == chronolog::CL_ERR_NOT_EXIST)
    {
        // Assert that the story or chronicle was destroyed
        assert((chronicle_destroyed || story_destroyed) &&
               "Chronicle or story was not destroyed, but return value is CL_ERR_NOT_EXIST!");
        std::cerr << "Thread " << tid
                  << " attempted to destroy a story, but the chronicle or story was already destroyed." << std::endl;
    }
    else if(ret == chronolog::CL_ERR_ACQUIRED)
    {
        // Assert that the story is still acquired by other threads
        assert(story_acquired && "Story was not acquired by other threads, but return value is CL_ERR_ACQUIRED!");
        std::cerr << "Thread " << tid << " attempted to destroy a story that is still acquired by other threads."
                  << std::endl;
    }
    else
    {
        std::cerr << "Thread " << tid << " encountered an unexpected error while destroying the story." << std::endl;
        assert(false && "Unexpected error during story destruction.");
    }
}

void validate_chronicle_destroyed(int tid, int ret)
{
    // Determine the current thread's chronicle
    std::string chronicle_name = (tid % 2 == 0) ? "CHRONICLE_2" : "CHRONICLE_1";

    // Variables to track states for chronicle destruction
    bool chronicle_destroyed = false;
    bool story_acquired = false;

    // Iterate over all threads to check their states based on the same chronicle
    for(size_t i = 0; i < shared_state.size(); ++i)
    {
        if(i == tid) continue; // Skip the current thread

        // Check if the thread operates on the same chronicle
        if(i % 2 == tid % 2)
        {
            if(shared_state[i]&CHRONICLE_DESTROYED)
            {
                chronicle_destroyed = true;
                break; // If the chronicle is already destroyed, no need to check further
            }
            if(shared_state[i]&STORY_ACQUIRED)
            {
                story_acquired = true; // Track if any thread still has an acquired story
            }
        }
    }

    // Validate the return value based on the chronicle and story state
    if(ret == chronolog::CL_SUCCESS)
    {
        // Assert that neither the chronicle is destroyed nor stories are still acquired
        assert(!chronicle_destroyed && !story_acquired &&
               "Chronicle is destroyed or stories are still acquired, but return value is CL_SUCCESS!");
        std::cout << "Chronicle successfully destroyed by thread " << tid << "." << std::endl;
    }
    else if(ret == chronolog::CL_ERR_NOT_EXIST)
    {
        // Assert that the chronicle was destroyed
        assert(chronicle_destroyed && "Chronicle was not destroyed, but return value is CL_ERR_NOT_EXIST!");
        std::cerr << "Thread " << tid << " attempted to destroy the chronicle, but it was already destroyed."
                  << std::endl;
    }
    else if(ret == chronolog::CL_ERR_ACQUIRED)
    {
        // Assert that stories are still acquired by other threads
        assert(story_acquired && "No stories are acquired by other threads, but return value is CL_ERR_ACQUIRED!");
        std::cerr << "Thread " << tid
                  << " attempted to destroy the chronicle, but stories are still acquired by other threads."
                  << std::endl;
    }
    else
    {
        std::cerr << "Thread " << tid << " encountered an unexpected error while destroying the chronicle."
                  << std::endl;
        assert(false && "Unexpected error during chronicle destruction.");
    }
}


// Function to update shared state and notify other threads
void update_shared_state_and_notify(int tid, int target_state_bitmask, int ret)
{
    switch(target_state_bitmask)
    {
        case THREAD_INITIALIZED:
        case DATA_ACCESS_FINISHED:
        case THREAD_FINALIZED:
            break;

        case CHRONICLE_CREATED:
            validate_chronicle_created(tid, ret);
            break;

        case STORY_ACQUIRED:
            validate_story_acquired(tid, ret);
            break;

        case STORY_RELEASED:
            validate_story_released(tid, ret);
            break;

        case STORY_DESTROYED:
            validate_story_destroyed(tid, ret);
            break;

        case CHRONICLE_DESTROYED:
            validate_chronicle_destroyed(tid, ret);
            break;

        default:
            std::cerr << "Error: Unknown or unsupported state transition!" << std::endl;
            break;
    }
    shared_state[tid] |= target_state_bitmask;
    std::cout << "Thread " << tid << " state updated to: " << get_state_name(target_state_bitmask) << std::endl;
}

void thread_body(struct thread_arg*t)
{
    // Initiate thread
    //update_shared_state_and_notify(t->tid, THREAD_INITIALIZED, 0); // State: Thread initialized


    // Chronicle creation
    std::string chronicle_name = (t->tid % 2 == 0) ? "CHRONICLE_2" : "CHRONICLE_1";
    std::map <std::string, std::string> chronicle_attrs;
    chronicle_attrs.emplace("Priority", "High");
    int flags = 1;
    int ret = client->CreateChronicle(chronicle_name, chronicle_attrs, flags);
    std::cout << "[ClientLibMultiStorytellers] Chronicle creation: tid=" << t->tid << ", Ret: " << ret << std::endl;
    //update_shared_state_and_notify(t->tid, CHRONICLE_CREATED, ret);


    // Acquire story
    std::string story_name = (t->tid % 4 < 2) ? "STORY_1" : "STORY_2";
    std::map <std::string, std::string> story_attrs;
    flags = 2;
    auto acquire_ret = client->AcquireStory(chronicle_name, story_name, story_attrs, flags);
    std::cout << "[ClientLibMultiStorytellers] Story acquired: tid=" << t->tid << ", Ret: " << acquire_ret.first
              << std::endl;
    //update_shared_state_and_notify(t->tid, STORY_ACQUIRED, acquire_ret.first);

    std::cout << "Thread: " << t->tid << ", Chronicle: " << chronicle_name << ", Story: " << story_name << std::endl;

    // Log Data
    auto story_handle = acquire_ret.second;
    for(int i = 0; i < 100; ++i)
    {
        story_handle->log_event("line " + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(i % 10)); // Simulate work
    }
    //update_shared_state_and_notify(t->tid, DATA_ACCESS_FINISHED, ret); // State: Data logging finished


    // Release the story
    ret = client->ReleaseStory(chronicle_name, story_name);
    std::cout << "[ClientLibMultiStorytellers] Story released: tid=" << t->tid << ", Ret: " << ret << std::endl;
    //update_shared_state_and_notify(t->tid, STORY_RELEASED, ret);


    // Destroy the story
    ret = client->DestroyStory(chronicle_name, story_name);
    std::cout << "[ClientLibMultiStorytellers] Story destroyed: tid=" << t->tid << ", Ret: " << ret << std::endl;
    //update_shared_state_and_notify(t->tid, STORY_DESTROYED, ret);


    // Destroy the chronicle
    ret = client->DestroyChronicle(chronicle_name);
    std::cout << "[ClientLibMultiStorytellers] Chronicle destroyed: tid=" << t->tid << ", Ret: " << ret << std::endl;
    //update_shared_state_and_notify(t->tid, CHRONICLE_DESTROYED, ret);


    // End thread
    //update_shared_state_and_notify(t->tid, THREAD_FINALIZED, 0);
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
    int num_threads = 8;
    std::vector <struct thread_arg> t_args(num_threads);
    std::vector <std::thread> workers(num_threads);
    std::string client_id = gen_random(8);
    shared_state.resize(num_threads, 0);

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