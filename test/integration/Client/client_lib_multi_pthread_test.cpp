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
*/
//
// Created by kfeng on 7/18/22.
//
#include <client.h>
#include <cassert>
#include <common.h>
#include <thread>
#define STORY_NAME_LEN 32

struct thread_arg
{
  int tid;
};

ChronoLogClient *client;

void thread_body(struct thread_arg *t)
{

    std::string server_ip = "127.0.0.1";
    int base_port = 5555;
    int flags = 0;
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
    ret = client->AcquireChronicle(chronicle_name,flags);
    ret = client->ReleaseChronicle(chronicle_name,flags);
    std::string story_name = gen_random(STORY_NAME_LEN);
    std::unordered_map<std::string, std::string> story_attrs;
    story_attrs.emplace("Priority", "High");
    story_attrs.emplace("IndexGranularity", "Millisecond");
    story_attrs.emplace("TieringPolicy", "Hot");
    flags = 2;
    ret = client->CreateStory(chronicle_name, story_name, story_attrs, flags);
    ret = client->AcquireStory(chronicle_name,story_name,flags);
    ret = client->ReleaseStory(chronicle_name,story_name,flags);
    ret = client->DestroyStory(chronicle_name,story_name,flags);
    ret = client->DestroyChronicle(chronicle_name,flags);
}

int main(int argc,char **argv) {


    int provided;
    std::string client_id = gen_random(8);

    int num_threads = 4;

    std::vector<struct thread_arg> t_args(num_threads);
    std::vector<std::thread> workers(num_threads);

    ChronoLogRPCImplementation protocol = CHRONOLOG_THALLIUM_SOCKETS;
    std::string server_ip = "127.0.0.1";
    int base_port = 5555;
    client = new ChronoLogClient(protocol, server_ip, base_port);

    std::string server_uri = CHRONOLOG_CONF->SOCKETS_CONF.string();
    server_uri += "://"+server_ip+":"+std::to_string(base_port);

    int flags = 0;
    uint64_t offset;
    int ret = client->Connect(server_uri,client_id,flags,offset);

    for(int i=0;i<num_threads;i++)
    {
	t_args[i].tid = i;
	std::thread t{thread_body,&t_args[i]};
	workers[i] = std::move(t);
    }

    for(int i=0;i<num_threads;i++)
	    workers[i].join();

    ret = client->Disconnect(client_id,flags);
    delete client;

    return 0;
}
