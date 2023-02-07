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
// Created by kfeng on 4/3/22.
//

#ifndef CHRONOLOG_RPCFACTORY_H
#define CHRONOLOG_RPCFACTORY_H

#include <unordered_map>
#include <memory>
#include "rpc.h"

class ChronoLogRPCFactory {
private:
    std::unordered_map<uint16_t, std::shared_ptr<ChronoLogRPC>> rpcs;
public:
    ChronoLogRPCFactory() : rpcs() {}

    std::shared_ptr<ChronoLogRPC> GetRPC(uint16_t server_port) {
        auto iter = rpcs.find(server_port);
        if (iter != rpcs.end()) return iter->second;
        auto temp = CHRONOLOG_CONF->RPC_BASE_SERVER_PORT;
        CHRONOLOG_CONF->RPC_BASE_SERVER_PORT = server_port;
        auto rpc = std::make_shared<ChronoLogRPC>();
        rpcs.emplace(server_port, rpc);
        CHRONOLOG_CONF->RPC_BASE_SERVER_PORT = temp;
        return rpc;
    }
};

#endif //CHRONOLOG_RPCFACTORY_H
