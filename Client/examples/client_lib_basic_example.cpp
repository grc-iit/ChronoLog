#include "ClientConfiguration.h"
#include "cmd_arg_parse.h"
#include <chronolog_client.h>
#include "chrono_monitor.h"
#include <common.h>
#include <cassert>
#include <unistd.h>

int main(int argc, char** argv) {
    // Load configuration
    std::string conf_file_path = parse_conf_path_arg(argc, argv);
    chronolog::ClientConfiguration confManager;
    if (!conf_file_path.empty()) {
        if (!confManager.load_from_file(conf_file_path)) {
            std::cerr << "[ClientExample] Failed to load configuration file '" << conf_file_path << "'. Using default values instead." << std::endl;
        } else {
            std::cout << "[ClientExample] Configuration file loaded successfully from '" << conf_file_path << "'." << std::endl;
        }
    } else {
        std::cout << "[ClientExample] No configuration file provided. Using default values." << std::endl;
    }
    confManager.log_configuration(std::cout);

    // Initialize logging
    int result = chronolog::chrono_monitor::initialize(confManager.LOG_CONF.LOGTYPE,
                                                       confManager.LOG_CONF.LOGFILE,
                                                       confManager.LOG_CONF.LOGLEVEL,
                                                       confManager.LOG_CONF.LOGNAME,
                                                       confManager.LOG_CONF.LOGFILESIZE,
                                                       confManager.LOG_CONF.LOGFILENUM,
                                                       confManager.LOG_CONF.FLUSHLEVEL);
    if (result == 1) {
        return EXIT_FAILURE;
    }

    // Build portal configs
    chronolog::ClientPortalServiceConf portalConf;
    portalConf.PROTO_CONF = confManager.PORTAL_CONF.PROTO_CONF;
    portalConf.IP = confManager.PORTAL_CONF.IP;
    portalConf.PORT = confManager.PORTAL_CONF.PORT;
    portalConf.PROVIDER_ID = confManager.PORTAL_CONF.PROVIDER_ID;

    // Build query configs
    chronolog::ClientQueryServiceConf queryConf;
    queryConf.PROTO_CONF = confManager.QUERY_CONF.PROTO_CONF;
    queryConf.IP = confManager.QUERY_CONF.IP;
    queryConf.PORT = confManager.QUERY_CONF.PORT;
    queryConf.PROVIDER_ID = confManager.QUERY_CONF.PROVIDER_ID;

    LOG_INFO("[ClientExample] Starting ChronoLog Client Example");

    // Create a ChronoLog client
    chronolog::Client client(portalConf, queryConf);

    // Connect to ChronoVisor
    int ret = client.Connect();
    assert(ret == chronolog::CL_SUCCESS);

    // Create a chronicle
    std::string chronicle_name = "MyChronicle";
    std::map<std::string, std::string> chronicle_attrs;
    int flags = 0;
    ret = client.CreateChronicle(chronicle_name, chronicle_attrs, flags);
    assert(ret == chronolog::CL_SUCCESS);

    // Acquire a story
    std::string story_name = "MyStory";
    std::map<std::string, std::string> story_attrs;
    auto acquire_result = client.AcquireStory(chronicle_name, story_name, story_attrs, flags);
    assert(acquire_result.first == chronolog::CL_SUCCESS);
    auto story_handle = acquire_result.second;

    // Log a few events to the story
    story_handle->log_event("Event 1");
    story_handle->log_event("Event 2");
    story_handle->log_event("Event 3");

    // Release the story
    ret = client.ReleaseStory(chronicle_name, story_name);
    assert(ret == chronolog::CL_SUCCESS);

    std::vector<chronolog::Event> events;
    uint64_t start_time = 1;
    uint64_t end_time = UINT64_MAX;

    int replay_ret = chronolog::CL_ERR_UNKNOWN;
    int max_seconds = 300;
    int retry_interval_sec = 5;
    int max_attempts = max_seconds / retry_interval_sec;

    bool success = false;

    for (int attempt = 1; attempt <= max_attempts; ++attempt) {
        events.clear(); // Clear previous results
        replay_ret = client.ReplayStory(chronicle_name, story_name, start_time, end_time, events);

        if (replay_ret == chronolog::CL_SUCCESS) {
            if (!events.empty()) {
                std::cout << "[ClientExample] Replay succeeded with " << events.size() << " event(s) after "
                        << attempt * retry_interval_sec << " seconds.\n";
                success = true;
                break;
            } else {
                std::cout << "[ClientExample] Replay returned 0 events (attempt " << attempt << "). Waiting...\n";
            }
        } else {
            std::cout << "[ClientExample] Replay attempt " << attempt << " failed. Retrying...\n";
        }

        sleep(retry_interval_sec);
    }

    if (!success) {
        std::cerr << "[ClientExample] Replay failed after " << max_seconds << " seconds with no events.\n";
        return EXIT_FAILURE;
    }

    // If we reach here, events are available
    for (const auto& ev : events) {
        std::cout << ev.to_string() << "\n";
    }

    // Destroy the story
    ret = client.DestroyStory(chronicle_name, story_name);
    assert(ret == chronolog::CL_SUCCESS);

    // Destroy the chronicle
    ret = client.DestroyChronicle(chronicle_name);
    assert(ret == chronolog::CL_SUCCESS);

    // Disconnect from ChronoVisor
    ret = client.Disconnect();
    assert(ret == chronolog::CL_SUCCESS);

    LOG_INFO("[ClientExample] Finished successfully");
    return 0;
}