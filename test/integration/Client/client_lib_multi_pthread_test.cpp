#include <chronolog_client.h>
#include <cassert>
#include <common.h>
#include <thread>
#include <cmd_arg_parse.h>
#include "chrono_monitor.h"

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

struct thread_arg
{
    int tid;
    std::string client_id;
};

chronolog::Client*client;
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

void thread_body(struct thread_arg*t)
{
    LOG_INFO("[ClientLibMultiPThreadTest] Thread (ID: {}) - Starting execution.", t->tid);
    int flags = 0;
    uint64_t offset;
    int ret;
    std::string chronicle_name;
    if(t->tid % 2 == 0) chronicle_name = "Chronicle_2";
    else chronicle_name = "Chronicle_1";

    update_shared_state_and_wait(t->tid, THREAD_INITIALIZED); // State: THREAD_INITIALIZED

    LOG_INFO("[ClientLibMultiPThreadTest] Thread (ID: {}) - Creating Chronicle: {}", t->tid, chronicle_name);
    std::map <std::string, std::string> chronicle_attrs;
    chronicle_attrs.emplace("Priority", "High");
    chronicle_attrs.emplace("IndexGranularity", "Millisecond");
    chronicle_attrs.emplace("TieringPolicy", "Hot");
    ret = client->CreateChronicle(chronicle_name, chronicle_attrs, flags);
    LOG_INFO("[ClientLibMultiPThreadTest] Thread (ID: {}) - CreateChronicle result for {}: {}", t->tid, chronicle_name
             , ret);
    update_shared_state_and_wait(t->tid, CHRONICLE_CREATED); // State: CHRONICLE_CREATED

    flags = 1;
    std::string story_name = gen_random(STORY_NAME_LEN);
    LOG_INFO("[ClientLibMultiPThreadTest] Thread (ID: {}) - Generating Story: {}", t->tid, story_name);

    std::map <std::string, std::string> story_attrs;
    story_attrs.emplace("Priority", "High");
    story_attrs.emplace("IndexGranularity", "Millisecond");
    story_attrs.emplace("TieringPolicy", "Hot");
    flags = 2;
    auto acquire_ret = client->AcquireStory(chronicle_name, story_name, story_attrs, flags);
    LOG_INFO("[ClientLibMultiPThreadTest] Thread (ID: {}) - AcquireStory result for {}:{} - {}", t->tid, chronicle_name
             , story_name, acquire_ret.first);
    assert(acquire_ret.first == chronolog::CL_SUCCESS || acquire_ret.first == chronolog::CL_ERR_NOT_EXIST);
    update_shared_state_and_wait(t->tid, STORY_ACQUIRED); // State: STORY_ACQUIRED

    ret = client->ReleaseStory(chronicle_name, story_name); // Release the story
    LOG_INFO("[ClientLibMultiPThreadTest] Thread (ID: {}) - ReleaseStory result for {}:{} - {}", t->tid, chronicle_name
             , story_name, ret);
    assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NO_CONNECTION);
    update_shared_state_and_wait(t->tid, STORY_RELEASED); // State: STORY_RELEASED

    ret = client->DestroyStory(chronicle_name, story_name);
    LOG_INFO("[ClientLibMultiPThreadTest] Thread (ID: {}) - DestroyStory result for {}:{} - {}", t->tid, chronicle_name
             , story_name, ret);
    assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NOT_EXIST || ret == chronolog::CL_ERR_ACQUIRED ||
           ret == chronolog::CL_ERR_NO_CONNECTION);
    update_shared_state_and_wait(t->tid, STORY_DESTROYED); // State: STORY_DESTROYED

    ret = client->DestroyChronicle(chronicle_name);
    LOG_INFO("[ClientLibMultiPThreadTest] Thread (ID: {}) - DestroyChronicle result for {}: {}", t->tid, chronicle_name
             , ret);
    assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NOT_EXIST || ret == chronolog::CL_ERR_ACQUIRED ||
           ret == chronolog::CL_ERR_NO_CONNECTION);
    update_shared_state_and_wait(t->tid, CHRONICLE_DESTROYED); // State: CHRONICLE_DESTROYED

    LOG_INFO("[ClientLibMultiPThreadTest] Thread (ID: {}) - Execution completed.", t->tid);
}

int main(int argc, char**argv)
{
    int provided;
    std::string client_id = gen_random(8);

    int num_threads = 4;

    std::vector <struct thread_arg> t_args(num_threads);
    std::vector <std::thread> workers(num_threads);

    // Initialize shared state array
    shared_state.resize(num_threads, 0); // Initialize with zeros

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
    LOG_INFO("[ClientLibMultiPthreadTest] Running test.");

    std::string server_ip = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.IP;
    int base_port = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.BASE_PORT;
    client = new chronolog::Client(confManager);//protocol, server_ip, base_port);

    int ret = client->Connect();

    for(int i = 0; i < num_threads; i++)
    {
        t_args[i].tid = i;
        t_args[i].client_id = client_id;
        std::thread t{thread_body, &t_args[i]};
        workers[i] = std::move(t);
    }

    for(int i = 0; i < num_threads; i++)
        workers[i].join();

    ret = client->Disconnect();//client_id, flags);
    delete client;

    return 0;
}
