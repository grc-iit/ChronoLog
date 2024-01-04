#include <chronolog_client.h>
#include <cassert>
#include <common.h>
#include <thread>
#include <cmd_arg_parse.h>

#define STORY_NAME_LEN 32

struct thread_arg
{
    int tid;
    std::string client_id;
};

chronolog::Client*client;

void thread_body(struct thread_arg*t)
{
    Logger::getLogger()->info("[ClientLibMultiPThreadTest] Thread (ID: {}) - Starting execution.", t->tid);
    //std::string server_ip = CHRONOLOG_CONF->RPC_CONF.CLIENT_VISOR_CONF.VISOR_END_CONF.VISOR_IP.string();
    //int base_port = CHRONOLOG_CONF->RPC_CONF.CLIENT_VISOR_CONF.VISOR_END_CONF.VISOR_BASE_PORT;
    int flags = 0;
    uint64_t offset;
    int ret;
    std::string chronicle_name;
    if(t->tid % 2 == 0) chronicle_name = "Chronicle_2";
    else chronicle_name = "Chronicle_1";

    Logger::getLogger()->info("[ClientLibMultiPThreadTest] Thread (ID: {}) - Creating Chronicle: {}", t->tid
                              , chronicle_name);
    std::unordered_map <std::string, std::string> chronicle_attrs;
    chronicle_attrs.emplace("Priority", "High");
    chronicle_attrs.emplace("IndexGranularity", "Millisecond");
    chronicle_attrs.emplace("TieringPolicy", "Hot");
    ret = client->CreateChronicle(chronicle_name, chronicle_attrs, flags);
    Logger::getLogger()->info("[ClientLibMultiPThreadTest] Thread (ID: {}) - CreateChronicle result for {}: {}", t->tid
                              , chronicle_name, ret);

    flags = 1;
    std::string story_name = gen_random(STORY_NAME_LEN);
    Logger::getLogger()->info("[ClientLibMultiPThreadTest] Thread (ID: {}) - Generating Story: {}", t->tid, story_name);

    std::unordered_map <std::string, std::string> story_attrs;
    story_attrs.emplace("Priority", "High");
    story_attrs.emplace("IndexGranularity", "Millisecond");
    story_attrs.emplace("TieringPolicy", "Hot");
    flags = 2;
    auto acquire_ret = client->AcquireStory(chronicle_name, story_name, story_attrs, flags);
    Logger::getLogger()->info("[ClientLibMultiPThreadTest] Thread (ID: {}) - AcquireStory result for {}:{} - {}", t->tid
                              , chronicle_name, story_name, acquire_ret.first);

    assert(acquire_ret.first == chronolog::CL_SUCCESS || acquire_ret.first == chronolog::CL_ERR_NOT_EXIST);
    ret = client->DestroyStory(chronicle_name, story_name);//, flags);
    Logger::getLogger()->info("[ClientLibMultiPThreadTest] Thread (ID: {}) - DestroyStory result for {}:{} - {}", t->tid
                              , chronicle_name, story_name, ret);

    assert(ret == chronolog::CL_ERR_ACQUIRED || ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NOT_EXIST);
    ret = client->Disconnect(); //t->client_id, flags);
    Logger::getLogger()->info("[ClientLibMultiPThreadTest] Thread (ID: {}) - Disconnect result: {}", t->tid, ret);

    assert(ret == chronolog::CL_ERR_ACQUIRED || ret == chronolog::CL_SUCCESS);
    ret = client->ReleaseStory(chronicle_name, story_name);//, flags);
    Logger::getLogger()->info("[ClientLibMultiPThreadTest] Thread (ID: {}) - ReleaseStory result for {}:{} - {}", t->tid
                              , chronicle_name, story_name, ret);

    assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NO_CONNECTION);
    ret = client->DestroyStory(chronicle_name, story_name);//, flags);
    Logger::getLogger()->info("[ClientLibMultiPThreadTest] Thread (ID: {}) - DestroyStory result for {}:{} - {}", t->tid
                              , chronicle_name, story_name, ret);

    assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NOT_EXIST || ret == chronolog::CL_ERR_ACQUIRED ||
           ret == chronolog::CL_ERR_NO_CONNECTION);

    ret = client->DestroyChronicle(chronicle_name);//, flags);
    assert(ret == chronolog::CL_SUCCESS || ret == chronolog::CL_ERR_NOT_EXIST || ret == chronolog::CL_ERR_ACQUIRED ||
           ret == chronolog::CL_ERR_NO_CONNECTION);
    Logger::getLogger()->info("[ClientLibMultiPThreadTest] Thread (ID: {}) - DestroyChronicle result for {}: {}", t->tid
                              , chronicle_name, ret);
    Logger::getLogger()->info("[ClientLibMultiPThreadTest] Thread (ID: {}) - Execution completed.", t->tid);
}

int main(int argc, char**argv)
{
    Logger::initialize("file", "../../logs/ChronoClient_initlogfile.txt", spdlog::level::debug, "ChronoClient");
    Logger::getLogger()->info("[ClientLibMultiPthreadTest] Init configuration process.");
    int provided;
    std::string client_id = gen_random(8);

    int num_threads = 4;

    std::vector <struct thread_arg> t_args(num_threads);
    std::vector <std::thread> workers(num_threads);

    std::string default_conf_file_path = "./default_conf.json";
    std::string conf_file_path;
    conf_file_path = parse_conf_path_arg(argc, argv);
    if(conf_file_path.empty())
    {
        conf_file_path = default_conf_file_path;
    }
    ChronoLog::ConfigurationManager confManager(conf_file_path);
    Logger::getLogger()->info("[ClientLibMultiPthreadTest] Configuration process completed.");
    Logger::changeConfiguration(confManager.CLIENT_CONF.CLIENT_LOG_CONF.LOGTYPE
                                , confManager.CLIENT_CONF.CLIENT_LOG_CONF.LOGFILE
                                , confManager.CLIENT_CONF.CLIENT_LOG_CONF.LOGLEVEL
                                , confManager.CLIENT_CONF.CLIENT_LOG_CONF.LOGNAME);
    Logger::getLogger()->info("[ClientLibMultiPthreadTest] Running test.");

    std::string server_ip = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.IP;
    int base_port = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.BASE_PORT;
    client = new chronolog::Client(confManager);//protocol, server_ip, base_port);

    std::string server_uri = confManager.CLIENT_CONF.VISOR_CLIENT_PORTAL_SERVICE_CONF.RPC_CONF.PROTO_CONF;
    server_uri += "://" + server_ip + ":" + std::to_string(base_port);

    int flags = 0;
    uint64_t offset;
    int ret = client->Connect(); //;server_uri, client_id, flags);//, offset);

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
