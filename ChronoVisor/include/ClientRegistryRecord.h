//
// Created by kfeng on 10/27/21.
//

#ifndef SOCKETPP_CLIENTREGISTRYRECORD_H
#define SOCKETPP_CLIENTREGISTRYRECORD_H

#include <SocketPP.h>

class ClientRegistryRecord {
public:
    ClientRegistryRecord() {}
    ClientRegistryRecord(int fd, SocketPP::TCPStream stream) : fd_(fd), stream_(stream) {}

    int fd_;
    SocketPP::TCPStream stream_;
};


#endif //SOCKETPP_CLIENTREGISTRYRECORD_H
