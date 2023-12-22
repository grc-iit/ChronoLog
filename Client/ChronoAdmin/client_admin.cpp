/*BSD 2-Clause License

Copyright (c) 2022, Scalable Computing Software Laboratory
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Created by Aparna on 01/12/2023
*/

#include <chronolog_client.h>
#include <cmd_arg_parse.h>
#include <common.h>
#include <cassert>
#include <unistd.h>
#include <pwd.h>
#include <getopt.h>
#include <functional>
#include <chrono>
#include <mpi.h>
#include "log.h"

typedef struct workload_conf_args_
{
    uint64_t chronicle_count = 1;
    uint64_t story_count = 1;
    uint64_t min_event_size = 0;
    uint64_t ave_event_size = 256;
    uint64_t max_event_size = 512;
    uint64_t event_count = 1;
    uint64_t event_interval = 0;
    bool barrier = false;
    std::string event_input_file;
    bool interactive = false;
    bool shared_story = false;
} workload_conf_args;

std::pair <std::string, workload_conf_args> cmd_arg_parse(int argc, char**argv)
{
    int opt;
    char*config_file = nullptr;
    workload_conf_args workload_args;

    // Define the long options and their corresponding short options
    struct option long_options[] = {{  "config"          , required_argument, nullptr, 'c'}
                                    , {"interactive"     , optional_argument, nullptr, 'i'}
                                    , {"chronicle_count" , required_argument, nullptr, 'h'}
                                    , {"story_count"     , required_argument, nullptr, 't'}
                                    , {"min_event_size"  , required_argument, nullptr, 'a'}
                                    , {"ave_event_size"  , required_argument, nullptr, 's'}
                                    , {"max_event_size"  , required_argument, nullptr, 'b'}
                                    , {"event_count"     , required_argument, nullptr, 'n'}
                                    , {"event_interval"  , required_argument, nullptr, 'g'}
                                    , {"barrier"         , optional_argument, nullptr, 'r'}
                                    , {"event_input_file", optional_argument, nullptr, 'f'}
                                    , {"shared_story"    , optional_argument, nullptr, 'o'}
                                    , {nullptr           , 0                , nullptr, 0} // Terminate the options array
    };

    // Parse the command-line options
    while((opt = getopt_long(argc, argv, "c:ih:t:a:s:b:n:g:rf:o", long_options, nullptr)) != -1)
    {
        switch(opt)
        {
            case 'c':
                config_file = optarg;
                break;
            case 'i':
                workload_args.interactive = true;
                break;
            case 'h':
                workload_args.chronicle_count = strtoll(optarg, nullptr, 10);
                break;
            case 't':
                workload_args.story_count = strtoll(optarg, nullptr, 10);
                break;
            case 'a':
                workload_args.min_event_size = strtoll(optarg, nullptr, 10);
                break;
            case 's':
                workload_args.ave_event_size = strtoll(optarg, nullptr, 10);
                break;
            case 'b':
                workload_args.max_event_size = strtoll(optarg, nullptr, 10);
                break;
            case 'n':
                workload_args.event_count = strtoll(optarg, nullptr, 10);
                break;
            case 'g':
                workload_args.event_interval = strtoll(optarg, nullptr, 10);
                break;
            case 'r':
                workload_args.barrier = false;
                break;
            case 'f':
                workload_args.event_input_file = optarg;
                break;
            case 'o':
                workload_args.shared_story = true;
                break;
            case '?':
                // Invalid option or missing argument
                LOGE("\nUsage: %s \n"
                     "-c|--config <config_file>\n"
                     "-i|--interactive\n"
                     "-h|--chronicle_count <chronicle_count>\n"
                     "-t|--story_count <story_count>\n"
                     "-a|--min_event_size <min_event_size>\n"
                     "-s|--ave_event_size <ave_event_size>\n"
                     "-b|--max_event_size <max_event_size>\n"
                     "-n|--event_count <event_count>\n"
                     "-g|--event_interval <event_interval>\n"
                     "-r|--barrier\n"
                     "-f|--input <event_input_file>\n"
                     "-o|--shared_story", argv[0]);
                exit(EXIT_FAILURE);
            default:
                // Unknown option
                LOGE("Unknown option: %c\n", opt);
                exit(EXIT_FAILURE);
        }
    }

    // Check if the config file option is provided
    if(config_file)
    {
        LOGI("Config file specified: %s\n", config_file);
        if(workload_args.interactive)
        {
            LOGI("Interactive mode: on");
        }
        else
        {
            LOGI("Interactive mode: off");
            LOGI("Chronicle count: %lu", workload_args.chronicle_count);
            LOGI("Story count: %lu", workload_args.story_count);
            if(!workload_args.event_input_file.empty())
            {
                LOGI("Event input file specified: %s", workload_args.event_input_file.c_str());
                LOGI("Barrier: %s", workload_args.barrier ? "true" : "false");
                LOGI("Shared story: %s", workload_args.shared_story ? "true" : "false");
            }
            else
            {
                LOGI("No event input file specified, use default/specified separate conf args ...");
                LOGI("Min event size: %lu", workload_args.min_event_size);
                LOGI("Ave event size: %lu", workload_args.ave_event_size);
                LOGI("Max event size: %lu", workload_args.max_event_size);
                LOGI("Event count: %lu", workload_args.event_count);
                LOGI("Event interval: %lu", workload_args.event_interval);
                LOGI("Barrier: %s", workload_args.barrier ? "true" : "false");
                LOGI("Shared story: %s", workload_args.shared_story ? "true" : "false");
            }
        }
        return {std::pair <std::string, workload_conf_args>((config_file), workload_args)};
    }
    else
    {
        LOGI("No config file specified, using default instead:\n");
        LOGI("Interactive mode: %s", workload_args.interactive ? "on" : "off");
        LOGI("Chronicle count: %lu", workload_args.chronicle_count);
        LOGI("Story count: %lu", workload_args.story_count);
        LOGI("Min event size: %lu", workload_args.min_event_size);
        LOGI("Ave event size: %lu", workload_args.ave_event_size);
        LOGI("Max event size: %lu", workload_args.max_event_size);
        LOGI("Event count: %lu", workload_args.event_count);
        LOGI("Event interval: %lu", workload_args.event_interval);
        LOGI("Barrier: %s", workload_args.barrier ? "true" : "false");
        LOGI("Shared story: %s", workload_args.shared_story ? "true" : "false");
        return {};
    }
}

void random_sleep()
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    srand(getpid());
    long usec = random() % 100000;
    std::cout << "Sleeping for " << usec << " us ..." << std::endl;
    usleep(usec * rank);
}

void test_create_chronicle(chronolog::Client &client, const std::string &chronicle_name)
{
    int ret, flags = 0;
    std::unordered_map <std::string, std::string> chronicle_attrs;
    chronicle_attrs.emplace("Priority", "High");
    chronicle_attrs.emplace("IndexGranularity", "Millisecond");
    chronicle_attrs.emplace("TieringPolicy", "Hot");
    ret = client.CreateChronicle(chronicle_name, chronicle_attrs, flags);
    assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_CHRONICLE_EXISTS);
}

chronolog::StoryHandle*
test_acquire_story(chronolog::Client &client, const std::string &chronicle_name, const std::string &story_name)
{
    random_sleep(); // TODO: (Kun) remove this when the hanging bug upon concurrent acquire is fixed
    int flags = 0;
    std::unordered_map <std::string, std::string> story_acquisition_attrs;
    story_acquisition_attrs.emplace("Priority", "High");
    story_acquisition_attrs.emplace("IndexGranularity", "Millisecond");
    story_acquisition_attrs.emplace("TieringPolicy", "Hot");
    std::pair <int, chronolog::StoryHandle*> acq_ret = client.AcquireStory(chronicle_name, story_name
                                                                           , story_acquisition_attrs, flags);
    std::cout << "acq_ret: " << acq_ret.first << std::endl;
    assert(acq_ret.first == chronolog::CL_SUCCESS || acq_ret.first == chronolog::CL_ERR_ACQUIRED);
    return acq_ret.second;
}

void test_write_event(chronolog::StoryHandle*story_handle, const std::string &event_payload)
{
    int ret = story_handle->log_event(event_payload);
    assert(ret == 1);
}

void test_release_story(chronolog::Client &client, const std::string &chronicle_name, const std::string &story_name)
{
    random_sleep(); // TODO: (Kun) remove this when the hanging bug upon concurrent acquire is fixed
    int ret = client.ReleaseStory(chronicle_name, story_name);
    assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NOT_EXIST);
}

void test_destroy_chronicle(chronolog::Client &client, const std::string &chronicle_name)
{
    int ret = client.DestroyChronicle(chronicle_name);
    assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NOT_EXIST || ret == chronolog::CL_ERR_ACQUIRED);
}

void test_destroy_story(chronolog::Client &client, const std::string &chronicle_name, const std::string &story_name)
{
    int ret = client.DestroyStory(chronicle_name, story_name);
    assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NOT_EXIST || ret == chronolog::CL_ERR_ACQUIRED);
}

// function to extract and execute commands from command line input
void command_dispatcher(const std::string &command_line, std::unordered_map <std::string, std::function <void(
        std::vector <std::string> &)>> command_map)
{
    const char*delim = " ";
    std::vector <std::string> command_subs;
    char*s = std::strtok((char*)command_line.c_str(), delim);
    command_subs.emplace_back(s);
    while(s != nullptr)
    {
        s = std::strtok(nullptr, delim);
        if(s != nullptr)
            command_subs.emplace_back(s);
    }
    if(command_map.find(command_subs[0]) != command_map.end())
    {
        command_map[command_subs[0]](command_subs);
    }
    else
    {
        std::cout << "Invalid command. Please try again." << std::endl;
    }
}

uint64_t get_event_timestamp(std::string &event_line)
{
    /*
     * Supported log files: syslog, auth.log, kern.log, ufw.log on Ubuntu
     * Expected format of log record:
     * Nov  5 14:36:49 ares-comp-01 systemd[1]: Started Time & Date Service.
     */
    size_t pos_first_space = event_line.find_first_of("0123456789") - 1;
    size_t pos_second_space = event_line.find_first_of(' ', pos_first_space + 1);
    size_t pos_third_space = event_line.find_first_of(' ', pos_second_space + 1);

    std::string timestamp_str = event_line.substr(0, pos_third_space);
    std::tm timeinfo{};
    auto now = std::chrono::system_clock::now();
    std::time_t current_time = std::chrono::system_clock::to_time_t(now);
    std::tm*time_info_now = std::localtime(&current_time);
    // assume the log file is generated in the same year as the current time
    timeinfo.tm_year = time_info_now->tm_year;
    strptime(timestamp_str.c_str(), "%b %d %H:%M:%S", &timeinfo);
    uint64_t timestamp = timelocal(&timeinfo);
    return timestamp;
}

uint64_t get_bigbang_timestamp(std::ifstream &file)
{
    std::string line;
    std::getline(file, line);
    uint64_t bigbang_timestamp = get_event_timestamp(line);
    file.seekg(0, std::ios::beg);
    return bigbang_timestamp;
}

int main(int argc, char**argv)
{
    MPI_Init(&argc, &argv);

    std::string default_conf_file_path = "./default_conf.json";
    std::pair <std::string, workload_conf_args> cmd_args = cmd_arg_parse(argc, argv);
    std::string conf_file_path = cmd_args.first;
    workload_conf_args workload_args = cmd_args.second;
    if(conf_file_path.empty())
    {
        conf_file_path = default_conf_file_path;
    }
    ChronoLog::ConfigurationManager confManager(conf_file_path);
    chronolog::Client client(confManager);
    chronolog::StoryHandle*story_handle;

    int ret;
    uint64_t total_event_payload_size = 0;

    std::string client_id = gen_random(8);
    std::cout << "Generated client id: " << client_id << std::endl;

    std::string server_ip = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.IP;
    int base_port = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.BASE_PORT;
    std::string server_uri = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.PROTO_CONF;
    server_uri += "://" + server_ip + ":" + std::to_string(base_port);

    std::string username = getpwuid(getuid())->pw_name;
    ret = client.Connect();
    assert(ret == chronolog::CL_SUCCESS);

    Logger::getLogger()->info("connected to server address : {}", server_uri);
    /*std::cout << " connected to server address : " << server_uri << std::endl;*/

    std::chrono::steady_clock::time_point t1, t2;
    t1 = std::chrono::steady_clock::now();
    if(workload_args.interactive)
    {
        std::cout << "Metadata operations: \n" << "\t-c <chronicle_name> , create a Chronicle <chronicle_name>\n"
                  << "\t-a -s <chronicle_name> <story_name>, acquire Story <story_name> in Chronicle <chronicle_name>\n"
                  << "\t-w <event_string>, write Event with <event_string> as payload\n"
                  //                  << "\t-r <event_id>, read Event <event_id>"
                  << "\t-q -s <chronicle_name> <story_name>, release Story <story_name> in Chronicle <chronicle_name>\n"
                  << "\t-d -s <chronicle_name> <story_name>, destroy Story <story_name> in Chronicle <chronicle_name>\n"
                  << "\t-d -c <chronicle_name>, destroy Chronicle <chronicle_name>\n" << "\t-disconnect\n" << std::endl;

        std::unordered_map <std::string, std::function <void(std::vector <std::string> &)>> command_map = {{  "-c", [&](
std::vector <std::string> &command_subs)
        {
            assert(command_subs.size() == 2);
            std::string chronicle_name = command_subs[1];
            test_create_chronicle(client, chronicle_name);
        }}
                                                                                                           , {"-a", [&](
                        std::vector <std::string> &command_subs)
                {
                    if(command_subs[1] == "-s")
                    {
                        assert(command_subs.size() == 4);
                        std::string chronicle_name = command_subs[2];
                        std::string story_name = command_subs[3];
                        story_handle = test_acquire_story(client, chronicle_name, story_name);
                    }
                }}
                                                                                                           , {"-q", [&](
                        std::vector <std::string> &command_subs)
                {
                    if(command_subs[1] == "-s")
                    {
                        assert(command_subs.size() == 4);
                        std::string chronicle_name = command_subs[2];
                        std::string story_name = command_subs[3];
                        test_release_story(client, chronicle_name, story_name);
                    }
                }}
                                                                                                           , {"-w", [&](
                        std::vector <std::string> &command_subs)
                {
                    assert(command_subs.size() == 2);
                    std::string event_payload = command_subs[1];
                    test_write_event(story_handle, event_payload);
                }}
                                                                                                           , {"-d", [&](
                        std::vector <std::string> &command_subs)
                {
                    if(command_subs[1] == "-c")
                    {
                        assert(command_subs.size() == 3);
                        std::string chronicle_name = command_subs[2];
                        test_destroy_chronicle(client, chronicle_name);
                    }
                    else if(command_subs[1] == "-s")
                    {
                        assert(command_subs.size() == 4);
                        std::string chronicle_name = command_subs[2];
                        std::string story_name = command_subs[3];
                        test_destroy_story(client, chronicle_name, story_name);
                    }
                }}};

        std::string command_line;
        while(true)
        {
            command_line.clear();
            std::getline(std::cin, command_line);
            if(command_line == "-disconnect") break;
            command_dispatcher(command_line, command_map);
        }
    }
    else
    {
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);

        std::random_device rand_device;
        std::mt19937 gen(rand_device());
        std::uniform_int_distribution char_dist(0, 255);
        double sigma = (double)(workload_args.max_event_size - workload_args.min_event_size) / 6;
        std::normal_distribution <double> size_dist((double)workload_args.ave_event_size, sigma);
        for(uint64_t i = 0; i < workload_args.chronicle_count; i++)
        {
            // create chronicle test
            std::string chronicle_name;
            if(workload_args.shared_story)
                chronicle_name = "chronicle_" + std::to_string(i);
            else
                chronicle_name = "chronicle_" + std::to_string(rank) + "_" + std::to_string(i);
            test_create_chronicle(client, chronicle_name);
            if(workload_args.barrier)
                MPI_Barrier(MPI_COMM_WORLD);

            uint64_t story_count_per_chronicle = workload_args.story_count / workload_args.chronicle_count;
            for(uint64_t j = 0; j < story_count_per_chronicle; j++)
            {
                // acquire story test
                std::string story_name;
                if(workload_args.shared_story)
                    story_name = "story_" + std::to_string(j);
                else
                    story_name = "story_" + std::to_string(rank) + "_" + std::to_string(j);
                story_handle = test_acquire_story(client, chronicle_name, story_name);
                if(workload_args.barrier)
                    MPI_Barrier(MPI_COMM_WORLD);

                // write event test
                uint64_t event_count_per_story = workload_args.event_count / workload_args.story_count;
                for(uint64_t k = 0; k < event_count_per_story; k++)
                {
                    if(workload_args.event_input_file.empty())
                    {
                        // randomly generate events with size in specified range
                        uint64_t event_size = (unsigned long)std::min(
                                std::max(size_dist(gen), (double)workload_args.min_event_size * 1.0),
                                (double)workload_args.max_event_size * 1.0);
                        total_event_payload_size += event_size;
                        std::string event_payload;
                        event_payload.resize(event_size);
                        for(uint64_t l = 0; l < event_size; l++)
                            event_payload += std::to_string('a' + std::abs(char_dist(gen)) + 1);
                        test_write_event(story_handle, event_payload);
                        if(workload_args.barrier)
                            MPI_Barrier(MPI_COMM_WORLD);

                        if(workload_args.event_interval > 0)
                            usleep(workload_args.event_interval);
                    }
                    else
                    {
                        // read event payload from input file line by line
                        std::ifstream input_file(workload_args.event_input_file);

                        // check if the file opened successfully
                        if(input_file.is_open())
                        {
                            uint64_t bigbang_timestamp = get_bigbang_timestamp(input_file);
                            uint64_t last_event_timestamp = bigbang_timestamp;
                            uint64_t event_timestamp;
                            std::string event_payload;
                            struct timespec sleep_ts{};
                            while(std::getline(input_file, event_payload))
                            {
                                event_timestamp = get_event_timestamp(event_payload);
                                if(event_timestamp < last_event_timestamp)
                                {
                                    LOGI("An Out-of-Order event is found, sleeping for 1 second ...");
                                    sleep_ts.tv_sec = 1;
                                    sleep_ts.tv_nsec = 0;
                                }
                                else
                                {
                                    sleep_ts.tv_sec = (long)(event_timestamp - last_event_timestamp) / 1000000000;
                                    sleep_ts.tv_nsec = (long)(event_timestamp - last_event_timestamp) % 1000000000;
                                }
                                // TODO: (Kun) work around on failure when daytime changes
                                if(sleep_ts.tv_sec > 3600) sleep_ts.tv_sec = 0;
                                LOGD("Sleeping for %ld.%ld seconds to emulate interval between events ..."
                                     , sleep_ts.tv_sec, sleep_ts.tv_nsec);
                                nanosleep(&sleep_ts, nullptr);
                                last_event_timestamp = event_timestamp;
                                total_event_payload_size += event_payload.size();
                                test_write_event(story_handle, event_payload);
                                if(workload_args.barrier)
                                    MPI_Barrier(MPI_COMM_WORLD);

                                if(workload_args.event_interval > 0)
                                    usleep(workload_args.event_interval);
                            }

                            input_file.close();
                        }
                        else
                        {
                            std::cout << "Unable to open the file";
                        }
                    }
                }

                // release story test
                test_release_story(client, chronicle_name, story_name);
                if(workload_args.barrier)
                    MPI_Barrier(MPI_COMM_WORLD);

                // destroy story test
                test_destroy_story(client, chronicle_name, story_name);
                if(workload_args.barrier)
                    MPI_Barrier(MPI_COMM_WORLD);
            }

            // destroy chronicle test
            test_destroy_chronicle(client, chronicle_name);
            if(workload_args.barrier)
                MPI_Barrier(MPI_COMM_WORLD);
        }
    }
    t2 = std::chrono::steady_clock::now();

    ret = client.Disconnect();
    assert(ret == chronolog::CL_SUCCESS);

    std::cout << "Total payload written: " << total_event_payload_size << std::endl;
    double duration = (double)(t2 - t1).count() / 1.0e9;
    std::cout << "Time used: " << duration << " seconds" << std::endl;
    std::cout << "Bandwidth: " << (double)total_event_payload_size / duration / 1e6 << " MB/s" << std::endl;

    MPI_Finalize();

    return 0;
}
