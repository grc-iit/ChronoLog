#include <chrono>
#include <unordered_map>
#include "ConfigurationManager.h"
#include "chronolog_client.h"
#include "common.h"
#include <cassert>
#include <cmd_arg_parse.h>
#include "chrono_monitor.h"
#include <mutex>
#include <condition_variable>
#include <vector>
#include <iostream>

#define NUM_CONNECTION (1)

// Define bitmask for connection states
enum ConnectionState
{
    CONNECTED = 1 << 0, // 0001
    DISCONNECTED = 1 << 1  // 0010
};

std::vector <int> shared_state; // Global array for connection states
std::mutex state_mutex;
std::condition_variable state_cv;

// Function to update shared state and wait for all connections to reach a specific state
void update_shared_state_and_wait(int idx, int target_state_bitmask)
{
    {
        std::lock_guard <std::mutex> lock(state_mutex);
        shared_state[idx] |= target_state_bitmask; // Update the bitmask state
        std::cout << "Connection " << idx << " state updated: " << shared_state[idx] << std::endl;
        state_cv.notify_all();
    }
    {
        std::unique_lock <std::mutex> lock(state_mutex);
        state_cv.wait(lock, [target_state_bitmask]()
        {
            // Check if all connections have at least the target state bit set
            return std::all_of(shared_state.begin(), shared_state.end(), [target_state_bitmask](int state)
            {
                return (state&target_state_bitmask) == target_state_bitmask;
            });
        });
        std::cout << "All connections reached state " << target_state_bitmask << std::endl;
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

    LOG_INFO("[ClientLibConnectRPCTest] Running test.");

    std::string server_ip = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.IP;
    int base_port = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.BASE_PORT;
    int num_ports = 1; // Kun: hardcode for now
    chronolog::Client client(confManager);
    std::string server_uri;
    std::vector <std::string> client_ids;
    int flags = 0;
    bool ret = false;
    uint64_t offset;
    std::chrono::steady_clock::time_point t1, t2;
    std::chrono::duration <double, std::nano> duration_connect{}, duration_disconnect{};

    // Initialize shared state array
    shared_state.resize(NUM_CONNECTION, 0); // Initialize with zeros

    client_ids.reserve(NUM_CONNECTION);
    for(int i = 0; i < NUM_CONNECTION; i++)
    {
        client_ids.emplace_back(gen_random(8));
    }

    // Connect clients
    for(int i = 0; i < NUM_CONNECTION; i++)
    {
        switch(confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.RPC_IMPLEMENTATION)
        {
            case CHRONOLOG_THALLIUM_SOCKETS:
            case CHRONOLOG_THALLIUM_TCP:
            case CHRONOLOG_THALLIUM_ROCE:
                server_uri = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.PROTO_CONF;
                break;
            default:
                server_uri = "ofi+sockets";
        }

        server_uri += "://" + server_ip + ":" + std::to_string(base_port + i);

        t1 = std::chrono::steady_clock::now();
        ret = client.Connect(); //server_uri, client_ids[i], flags); //, offset);
        assert(ret == chronolog::CL_SUCCESS);
        t2 = std::chrono::steady_clock::now();
        duration_connect += (t2 - t1);

        update_shared_state_and_wait(i, CONNECTED); // State: CONNECTED
    }

    // Disconnect clients
    for(int i = 0; i < NUM_CONNECTION; i++)
    {
        t1 = std::chrono::steady_clock::now();
        ret = client.Disconnect(); //client_ids[i], flags);
        assert(ret == chronolog::CL_SUCCESS);
        t2 = std::chrono::steady_clock::now();
        duration_disconnect += (t2 - t1);

        update_shared_state_and_wait(i, DISCONNECTED); // State: DISCONNECTED
    }

    LOG_INFO("[ClientLibConnectRPCTest] Average connection time: {} ns", duration_connect.count() / NUM_CONNECTION);
    LOG_INFO("[ClientLibConnectRPCTest] Average disconnection time: {} ns",
            duration_disconnect.count() / NUM_CONNECTION);

    return 0;
}
