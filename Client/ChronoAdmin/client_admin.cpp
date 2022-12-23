//
// Created by kfeng on 7/11/22.
//

#include <client.h>
#include <common.h>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>
#include <string>

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
    char ip_add[16];
    std::string host_ip;

    struct hostent *he = gethostbyname(hostname.c_str());
    in_addr **addr_list = (struct in_addr **) he->h_addr_list;
    strcpy(ip_add, inet_ntoa(*addr_list[0]));
    host_ip = std::string(ip_add);

    ChronoLogClient client;

    std::string conf = "ofi+sockets";    
    std::string conn = "://";
    std::string server_uri = conf + conn + host_ip + ":" + std::to_string(portno);

    int flags = 0;
    bool ret = false;

    std::string client_id = gen_random(8);

    ret = client.Connect(server_uri, client_id);

    std::cout <<" connected to server address : "<<server_uri<<std::endl;

    ret = client.Disconnect(client_id, flags);

    return 0;
}
