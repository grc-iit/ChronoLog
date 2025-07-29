#include <cassert>
#include <common.h>
#include <thread>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <getopt.h>

#include <chrono_monitor.h>
#include <ClientConfiguration.h>
#include <chronolog_client.h>


struct thread_arg
{
    int tid;
    std::string chronicle;
    std::string story;
    uint64_t segment_start;
    uint64_t segment_end;
};

chronolog::Client*client;

namespace chl = chronolog;

void reader_thread( int tid, struct thread_arg * t, std::vector<chronolog::Event> & replay_events)
{
    LOG_INFO("[ClientLibStoryReader] Reader thread tid={} starting",tid);

    // make the reader thread sleep to allow the writer threads create the story and log some events 
    // allow the events to propagate through the keeper/grappher into ChronoLog store
    while(t->segment_end==0)
    { 
        LOG_INFO("[ClientLibStoryReader] Reader thread tid={} is waiting",tid);
        sleep(60);
    }

    int ret = chronolog::CL_ERR_UNKNOWN;
    std::map <std::string, std::string> chronicle_attrs;
    std::map <std::string, std::string> story_attrs;
    int flags = 1;

    // std::vector<chronolog::Event> replay_events;

    // Create the chronicle
    ret= client->CreateChronicle(t->chronicle, chronicle_attrs, flags);
    LOG_INFO("[ClientLibStoryReader] Chronicle created: tid={}, ChronicleName={}, Flags: {}", t->tid , t->chronicle, flags);


    // Acquire the story
    auto acquire_ret = client->AcquireStory(t->chronicle, t->story, story_attrs, flags);
    LOG_INFO("[ClientLibStoryReader] Reader thread tid={} acquired story: {} {}, Ret: {}", tid , t->chronicle, t->story, chl::to_string_client(acquire_ret.first));

    if(acquire_ret.first == chronolog::CL_SUCCESS)
    {
        auto story_handle = acquire_ret.second;

        LOG_INFO("[StoryReaderClient] Reader thread tid={} sending playback_request for story: {} {} segment{}-{}", tid , t->chronicle, t->story, t->segment_start, t->segment_end);

        ret = client->ReplayStory(t->chronicle, t->story,t->segment_start, (t->segment_end), replay_events);

        if(ret == chronolog::CL_SUCCESS)
        {   
        
            LOG_INFO("[StoryReaderClient] Reader thread tid={} completed replay story{}-{}  event_series has {} events",
                         tid , t->chronicle, t->story, replay_events.size());
        
            for( auto event: replay_events)
            {
                LOG_DEBUG("[StoryReaderClient] replay event{}", event.to_string());
            }
        }
        else 
        {
            LOG_INFO("[StoryReaderClient] Reader thread tid={} failed to replay story: {}-{}, Return_code: {}", tid , t->chronicle, t->story, chl::to_string_client(ret));
        }

        
        
        // Release the story
        ret = client->ReleaseStory(t->chronicle, t->story);
        LOG_INFO("[StoryReaderClient] Reader thread tid={} released story: {} {}, Ret: {}", tid , t->chronicle, t->story, chl::to_string_client(ret));

    }

    LOG_INFO("[ClientLibStoryReader] Reader thread tid={} exiting", tid);
}

int parse_command_args(int argc, char**argv, std::string & config_file, std::string & chronicle , std::string & story, int & interval, std::string & output_csv_file)
                  //  , uint64_t start_time, uint64_t end_time)
{
    int opt=0;

    // Define the long options and their corresponding short options
    struct option long_options[] = {{"config", required_argument, 0, 'c'}
                                    , {"chronicle", required_argument, 0, 'r'}
                                    , {"story", required_argument, 0, 's'}
                                    , {"interval", required_argument,0,'i'}
                                    , {"output_file", required_argument, 0, 'f'}
                                    , {0       , 0                , 0, 0} // Terminate the options array
    };


    // Parse the command-line options
    while((opt = getopt_long(argc, argv, "c:r:s:i:f:", long_options, nullptr)) != -1)
    {
        switch(opt)
        {
            case 'c':
                config_file = std::string(optarg);
                break;
            case 'r':
                chronicle = std::string(optarg);
                break;
            case 's':
                story = std::string(optarg); 
                break;
            case 'i':
                interval = std::stol(optarg);
                break;
            case 'f':
                output_csv_file = std::string(optarg);
                break; 
            default:
                // Unknown option
                std::cerr << "[cmd_arg_parse] Encountered an unknown option: " << static_cast<char>(opt) << std::endl;
        }
    }

    return 1;
}

int main(int argc, char**argv)
{
    // Load configuration
    std::string conf_file_path("./default_client_conf.json");
    std::string chronicle_name("CHRONICLE");
    std::string story_name("STORY");
    int interval_in_secs = 300;
    std::string output_csv_file_path("/tmp/chronolog_reader_client.csv");

    parse_command_args(argc, argv, conf_file_path, chronicle_name, story_name, interval_in_secs, output_csv_file_path);

    std::cout<<"Using configuration parameters : conf_file_path {"<< conf_file_path
            <<"} chronicle {"<<chronicle_name<<"} story {"<<story_name
            <<"} output_csv_file_path {"<< output_csv_file_path 
            <<"} interval {" << interval_in_secs<<" secs}"<<std::endl;


    chronolog::ClientConfiguration confManager;
    if (!conf_file_path.empty()) {
        if (!confManager.load_from_file(conf_file_path)) {
            std::cerr << "[StoryReaderClient] Failed to load configuration file '" << conf_file_path << "'. Using default values instead." << std::endl;
        } else {
            std::cout << "[StoryReaderClient] Configuration file loaded successfully from '" << conf_file_path << "'." << std::endl;
        }
    } else {
        std::cout << "[StoryReaderClient] No configuration file provided. Using default values." << std::endl;
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

    // Build portal config
    chronolog::ClientPortalServiceConf portalConf;
    portalConf.PROTO_CONF = confManager.PORTAL_CONF.PROTO_CONF;
    portalConf.IP = confManager.PORTAL_CONF.IP;
    portalConf.PORT = confManager.PORTAL_CONF.PORT;
    portalConf.PROVIDER_ID = confManager.PORTAL_CONF.PROVIDER_ID;

    // Build query config
    chronolog::ClientQueryServiceConf clientQueryConf;
    clientQueryConf.PROTO_CONF = confManager.QUERY_CONF.PROTO_CONF;
    clientQueryConf.IP = confManager.QUERY_CONF.IP;
    clientQueryConf.PORT = confManager.QUERY_CONF.PORT;
    clientQueryConf.PROVIDER_ID = confManager.QUERY_CONF.PROVIDER_ID;

    LOG_INFO("[StoryReaderClient] Running...");

    client = new chronolog::Client(portalConf, clientQueryConf);

    int ret = client->Connect();

    if(chronolog::CL_SUCCESS != ret)
    {
        LOG_ERROR("[StoryReaderClient] Failed to connect to ChronoVisor");
        delete client;
        return -1;
    }

    auto end_point = std::chrono::high_resolution_clock::now();
    auto start_point = end_point - std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(interval_in_secs));

    uint64_t start_time = start_point.time_since_epoch().count();
    uint64_t end_time= end_point.time_since_epoch().count();

    auto start_time_t = std::chrono::system_clock::to_time_t(start_point);
    auto end_time_t = std::chrono::system_clock::to_time_t(end_point);
    

    std::cout <<"Replay time range "<<start_time <<" " << std::put_time(std::localtime(&start_time_t), "%Y-%m-%d %H:%M:%S.")<<start_time%1000000000 
            <<" - "<< end_time << " " << std::put_time(std::localtime(&end_time_t), "%Y-%m-%d %H:%M:%S.") << end_time % 1000000000
            << std::endl;

    LOG_INFO("[StoryReaderClient] ReplayStory Request for story {}-{} time range {}-{}", chronicle_name,story_name ,start_time,end_time);

    std::vector<chronolog::Event> replay_event_series;
    
    struct thread_arg segment_arg{0, chronicle_name,story_name,start_time,end_time};

    reader_thread(0, & segment_arg, replay_event_series); 

    LOG_INFO("[StoryReaderClient] Finished reader test for story: {}-{}", chronicle_name, story_name);
    LOG_INFO("[StoryReaderClient] Replay event series has {} events", replay_event_series.size());
    
    std::ofstream output_csv_fstream;

    output_csv_fstream.open(output_csv_file_path, std::ofstream::out|std::ofstream::app);
    for(const auto& event: replay_event_series)
    {
        time_t event_time_t = event.time()/1000000000;
        output_csv_fstream << std::put_time(std::localtime(&event_time_t), "%Y-%m-%d %H:%M:%S.")<<event.time()%1000000000 <<','
            << event.client_id() << ',' << event.index() << ',' << event.log_record() << std::endl;
    }
    output_csv_fstream.close();


    ret = client->Disconnect();
    delete client;

    return 0;
}
