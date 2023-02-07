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
// Created by kfeng on 5/17/22.
//

#ifndef CHRONOLOG_CLIENT_H
#define CHRONOLOG_CLIENT_H

#include "RPCClient.h"
#include "errcode.h"
#include "ClocksourceManager.h"

ClocksourceManager *ClocksourceManager::clocksourceManager_ = nullptr;

class ChronoLogClient {
public:
    ChronoLogClient() { CHRONOLOG_CONF->ConfigureDefaultClient("../../test/communication/server_list"); }

    ChronoLogClient(const std::string& server_list_file_path) {
        CHRONOLOG_CONF->ConfigureDefaultClient(server_list_file_path);
        rpcProxy_ = ChronoLog::Singleton<RPCClient>::GetInstance();
    }

    ChronoLogClient(const ChronoLogRPCImplementation& protocol, const std::string& visor_ip, int visor_port) {
        CHRONOLOG_CONF->IS_SERVER = false;
        CHRONOLOG_CONF->RPC_IMPLEMENTATION = protocol;
        CHRONOLOG_CONF->RPC_SERVER_IP = visor_ip;
        CHRONOLOG_CONF->RPC_BASE_SERVER_PORT = visor_port;
        rpcProxy_ = ChronoLog::Singleton<RPCClient>::GetInstance();
    }

    int Connect(const std::string &server_uri,
                std::string &client_id,
                int &flags,
                uint64_t &clock_offset);
    int Disconnect(const std::string &client_id, int &flags);
    int CreateChronicle(std::string &name,
                        const std::unordered_map<std::string, std::string> &attrs,
                        int &flags);
    int DestroyChronicle(std::string &name, int &flags);
    int AcquireChronicle(std::string &name, int &flags);
    int ReleaseChronicle(std::string &name, int &flags);
    int CreateStory(std::string &chronicle_name,
                    std::string &story_name,
                    const std::unordered_map<std::string, std::string> &attrs,
                    int &flags);
    int DestroyStory(std::string &chronicle_name, std::string &story_name, int &flags);
    int AcquireStory(std::string &chronicle_name, std::string &story_name, int &flags);
    int ReleaseStory(std::string &chronicle_name, std::string &story_name, int &flags);
    int GetChronicleAttr(std::string &chronicle_name, const std::string &key, std::string &value);
    int EditChronicleAttr(std::string &chronicle_name, const std::string &key, const std::string &value);

private:
    std::shared_ptr<RPCClient> rpcProxy_;
};
#endif //CHRONOLOG_CLIENT_H
