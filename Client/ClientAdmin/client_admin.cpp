//
// Created by kfeng on 7/11/22.
//

#include <client.h>
#include <global_var_client.h>
#include <log.h>
#include <common.h>
#include <cassert>
#include <iostream>
#include <fstream>

#define NUM_CONNECTION (1)

int main() 
{

    std::string hostname = "localhost:1200";
    std::string filename = "server_list";

    std::ofstream fs(filename,std::ios_base::out);
    fs << hostname;

    fs.close();

    ChronoLogClient client(filename);
    
    std::string conn = "://";
    std::string server_uri = CHRONOLOG_CONF->SOCKETS_CONF.string() + 
	    	            conn +CHRONOLOG_CONF->RPC_SERVER_IP.string()+":"+
			    std::to_string(CHRONOLOG_CONF->RPC_BASE_SERVER_PORT);

    std::vector<std::string> client_ids;
    int flags = 0;
    bool ret = false;

    client_ids.reserve(NUM_CONNECTION);
    for (int i = 0; i < NUM_CONNECTION; i++) client_ids.emplace_back(gen_random(8));
    for (int i = 0; i < NUM_CONNECTION; i++) 
    {
        ret = client.Connect(server_uri, client_ids[i]);
    }

    for (int i = 0; i < NUM_CONNECTION; i++) 
    {
        ret = client.Disconnect(client_ids[i], flags);
    };

    return 0;
}
