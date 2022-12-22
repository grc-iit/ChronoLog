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

    ChronoLogClient client;

    std::string conf = "ofi+sockets";    
    std::string conn = "://";
    std::string server_uri = conf + conn + hostname+":"+std::to_string(portno);

    int flags = 0;
    bool ret = false;

    std::string client_id = gen_random(8);

    ret = client.Connect(server_uri, client_id);

    std::cout <<" connected to server address : "<<server_uri<<std::endl;

    ret = client.Disconnect(client_id, flags);

    return 0;
}
