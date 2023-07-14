//
// Created by kfeng on 7/18/22.
//
#include <client.h>
#include <common.h>
#include <thread>
#include <abt.h>

#define CHRONICLE_NAME_LEN 32
#define STORY_NAME_LEN 32

ChronoLogClient *client;

struct thread_arg
{
  int tid;

};

void thread_function(void *tt)
{
	struct thread_arg *t = (struct thread_arg*)tt;

	std::string server_ip = CHRONOLOG_CONF->RPC_CONF.CLIENT_VISOR_CONF.VISOR_END_CONF.VISOR_IP.string();
	int base_port = CHRONOLOG_CONF->RPC_CONF.CLIENT_VISOR_CONF.VISOR_END_CONF.VISOR_BASE_PORT;
	/*std::string client_id = gen_random(8);
	std::string server_uri = CHRONOLOG_CONF->SOCKETS_CONF.string();
	server_uri += "://"+server_ip+":"+std::to_string(base_port);*/
    int flags = 0;
<<<<<<< HEAD
	uint64_t offset;
	int ret;
	std::string chronicle_name;
        if(t->tid%2==0) chronicle_name = "gscs5er9TcdJ9mOgUDteDVBcI0oQjozK";
	else chronicle_name = "6RPkwqX2IOpR41dVCqmWauX9RfXIuTAp";
        std::unordered_map<std::string, std::string> chronicle_attrs;
        chronicle_attrs.emplace("Priority", "High");
        chronicle_attrs.emplace("IndexGranularity", "Millisecond");
        chronicle_attrs.emplace("TieringPolicy", "Hot");
        ret = client->CreateChronicle(chronicle_name, chronicle_attrs, flags);
	flags = 1;
	std::string story_name = gen_random(STORY_NAME_LEN);
	std::unordered_map<std::string, std::string> story_attrs;
        story_attrs.emplace("Priority", "High");
        story_attrs.emplace("IndexGranularity", "Millisecond");
        story_attrs.emplace("TieringPolicy", "Hot");
        flags = 2;
        ret = client->AcquireStory(chronicle_name, story_name, story_attrs, flags);
        ret = client->ReleaseStory(chronicle_name,story_name,flags);
	ret = client->DestroyStory(chronicle_name,story_name,flags);
	ret = client->DestroyChronicle(chronicle_name,flags);
=======
    uint64_t offset;
    int ret;
    std::string chronicle_name;
    if (t->tid % 2 == 0) chronicle_name = "Chronicle_1";
    else chronicle_name = "Chronicle_2";
    std::unordered_map<std::string, std::string> chronicle_attrs;
    chronicle_attrs.emplace("Priority", "High");
    chronicle_attrs.emplace("IndexGranularity", "Millisecond");
    chronicle_attrs.emplace("TieringPolicy", "Hot");
    ret = client->CreateChronicle(chronicle_name, chronicle_attrs, flags);
    std::cout << "tid="<<t->tid<<" Createchronicle {"<<chronicle_name<<"} ret: " << ret << std::endl;
    assert(ret == CL_SUCCESS || ret == CL_ERR_CHRONICLE_EXISTS || ret == CL_ERR_NO_KEEPERS);
    flags = 1;
    std::string story_name = gen_random(STORY_NAME_LEN);
    std::unordered_map<std::string, std::string> story_attrs;
    story_attrs.emplace("Priority", "High");
    story_attrs.emplace("IndexGranularity", "Millisecond");
    story_attrs.emplace("TieringPolicy", "Hot");
    flags = 2;
    auto acquire_ret= client->AcquireStory(chronicle_name, story_name, story_attrs, flags);
    std::cout << "tid="<<t->tid<<" AcquireStory {"<<chronicle_name<<":"<<story_name<<"} ret: " << acquire_ret.first << std::endl;
    assert(acquire_ret.first == CL_SUCCESS || acquire_ret.first == CL_ERR_NOT_EXIST || acquire_ret.first == CL_ERR_NO_KEEPERS);
    ret = client->DestroyStory(chronicle_name, story_name);
    std::cout << "tid="<<t->tid<<" DestroyStory {"<<chronicle_name<<":"<<story_name<<"} ret: " << ret<< std::endl;
    assert(ret == CL_ERR_ACQUIRED || ret==CL_SUCCESS || ret==CL_ERR_NOT_EXIST || ret==CL_ERR_NO_KEEPERS);
    ret = client->Disconnect();
    std::cout <<"tid="<<t->tid<< " Disconnect{"<<"} ret: " << ret << std::endl;
    assert(ret == CL_ERR_ACQUIRED || ret==CL_SUCCESS);
    ret = client->ReleaseStory(chronicle_name, story_name);
    std::cout << "tid="<<t->tid<<" ReleaseStory {"<<chronicle_name<<":"<<story_name<<"} ret: " << ret<< std::endl;
    assert(ret == CL_SUCCESS || ret==CL_ERR_NO_KEEPERS || ret == CL_ERR_NOT_EXIST );
     ret = client->DestroyStory(chronicle_name, story_name);
    std::cout << "tid="<<t->tid<<" DestroyStory {"<<chronicle_name<<":"<<story_name<<"} ret: " << ret<< std::endl;
    assert(ret == CL_SUCCESS || ret == CL_ERR_NOT_EXIST || ret == CL_ERR_ACQUIRED || ret == CL_ERR_NO_KEEPERS);
    ret = client->DestroyChronicle(chronicle_name);
    assert(ret == CL_SUCCESS || ret == CL_ERR_NOT_EXIST || ret == CL_ERR_ACQUIRED);
>>>>>>> 105df6b6cebb07312cf919ade4bb90c617fa51f4
}

int main(int argc,char **argv) {
    std::atomic<long> duration_connect{}, duration_disconnect{};
    std::vector<std::thread> thread_vec;
    uint64_t offset;


    ChronoLogRPCImplementation protocol = CHRONOLOG_THALLIUM_SOCKETS;
    std::string server_ip = CHRONOLOG_CONF->RPC_CONF.CLIENT_VISOR_CONF.VISOR_END_CONF.VISOR_IP.string();
    int base_port = CHRONOLOG_CONF->RPC_CONF.CLIENT_VISOR_CONF.VISOR_END_CONF.VISOR_BASE_PORT;
    client = new ChronoLogClient(protocol, server_ip, base_port);

    int num_xstreams = 8;
    int num_threads = 8;

    ABT_xstream *xstreams = (ABT_xstream *)malloc(sizeof(ABT_xstream) * num_xstreams);
    ABT_pool *pools = (ABT_pool *)malloc(sizeof(ABT_pool) * num_xstreams);
    ABT_thread *threads = (ABT_thread*)malloc(sizeof(ABT_thread)*num_threads);
    struct thread_arg *t_args = (struct thread_arg*)malloc(num_threads*sizeof(struct thread_arg));

    std::string client_id = gen_random(8);;
    std::string server_uri = CHRONOLOG_CONF->RPC_CONF.CLIENT_VISOR_CONF.PROTO_CONF.string();
    server_uri += "://"+server_ip+":"+std::to_string(base_port);
    int flags = 0;

    int ret = client->Connect(server_uri,client_id,flags,offset);

    for(int i=0;i<num_threads;i++)
	    t_args[i].tid = i;

    ABT_init(argc, argv);

    ABT_xstream_self(&xstreams[0]);

    for (int i = 1; i < num_xstreams; i++) 
    {
        ABT_xstream_create(ABT_SCHED_NULL, &xstreams[i]);
    }


    for (int i = 0; i < num_xstreams; i++) 
    {
        ABT_xstream_get_main_pools(xstreams[i], 1, &pools[i]);
    }


   for(int i=0;i<num_threads;i++)
   {
       ABT_thread_create(pools[i],thread_function,&t_args[i],ABT_THREAD_ATTR_NULL, &threads[i]);
   }

   for(int i=0;i<num_threads;i++)
   ABT_thread_free(&threads[i]);

   for (int i = 1; i < num_xstreams; i++) 
   {
        ABT_xstream_join(xstreams[i]);
        ABT_xstream_free(&xstreams[i]);
   }

   ABT_finalize();

   free(pools);
   free(xstreams);
   free(threads);
   free(t_args);

<<<<<<< HEAD
   ret = client->Disconnect(client_id,flags);
=======
    ret = client->Disconnect();//client_id, flags);
    std::cout << "main Disconnect ret: " << ret << std::endl;
    assert(ret == CL_SUCCESS);
>>>>>>> 105df6b6cebb07312cf919ade4bb90c617fa51f4

   delete client;

    return 0;
}
