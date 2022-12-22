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
#include <sstream>

#define NUM_CONNECTION (1)

int main() 
{

    std::string hostname;
    std::string portnum;
    std::string filename = "server_list";

    std::stringstream ss;

    std::cout <<" Enter server hostname : "<<std::endl;
    std::cin >> hostname;

    std::cout <<" Enter server portnumber : "<<std::endl;
    std::cin >> portnum;
    int portno = std::stoi(portnum);

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

    std::cout <<" connected to server address : "<<server_uri<<std::endl;

    for (int i = 0; i < NUM_CONNECTION; i++) 
    {
        ret = client.Disconnect(client_ids[i], flags);
    };

    return 0;
}
