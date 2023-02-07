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
// Created by kfeng on 3/30/22.
//

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

template<typename F>
void ChronoLogRPC::bind(const ChronoLogCharStruct &str, F func) {
    switch (CHRONOLOG_CONF->RPC_IMPLEMENTATION) {
        case CHRONOLOG_THALLIUM_TCP:
        case CHRONOLOG_THALLIUM_SOCKETS:
        case CHRONOLOG_THALLIUM_ROCE: {
            for (int i = 0; i < numPorts_; i++)
                thalliumServerList_[i].define(str.string(), func);
//            thalliumClient_->define(str.string(), func);
            break;
        }
    }
}

template<typename Response, typename... Args>
Response ChronoLogRPC::callWithTimeout(uint16_t server_index, int timeout_ms,
                                       ChronoLogCharStruct const &func_name, Args... args) {
    int16_t port = baseServerPort_ + server_index;

    switch (CHRONOLOG_CONF->RPC_IMPLEMENTATION) {
        case CHRONOLOG_THALLIUM_TCP:
        case CHRONOLOG_THALLIUM_SOCKETS:
        case CHRONOLOG_THALLIUM_ROCE: {
            tl::remote_procedure remote_procedure = thalliumClient_->define(func_name.c_str());
            return remote_procedure.on(thallium_endpoints[server_index])(std::forward<Args>(args)...);
            break;
        }
    }
}

template<typename Response, typename... Args>
Response ChronoLogRPC::call(uint16_t server_index,
                   ChronoLogCharStruct const &func_name,
                   Args... args) {
//    int16_t port = baseServerPort_ + server_index;

    switch (CHRONOLOG_CONF->RPC_IMPLEMENTATION) {
        case CHRONOLOG_THALLIUM_TCP:
        case CHRONOLOG_THALLIUM_SOCKETS:
        case CHRONOLOG_THALLIUM_ROCE: {
            tl::remote_procedure remote_procedure = thalliumClient_->define(func_name.c_str());
            return remote_procedure.on(thallium_endpoints[server_index])(std::forward<Args>(args)...);
            break;
        }
    }
}

template<typename Response, typename... Args>
Response ChronoLogRPC::call(ChronoLogCharStruct &server,
                   uint16_t &port,
                   ChronoLogCharStruct const &func_name,
                   Args... args) {
    switch (CHRONOLOG_CONF->RPC_IMPLEMENTATION) {
        case CHRONOLOG_THALLIUM_TCP:
        case CHRONOLOG_THALLIUM_SOCKETS:
        case CHRONOLOG_THALLIUM_ROCE: {
            tl::remote_procedure remote_procedure = thalliumClient_->define(func_name.c_str());
            auto end_point = get_endpoint(CHRONOLOG_CONF->SOCKETS_CONF,
                                          server, port);
            return remote_procedure.on(end_point)(std::forward<Args>(args)...);
            break;
        }
    }
}

template<typename Response, typename... Args>
std::future<Response> ChronoLogRPC::async_call(uint16_t server_index,
                                      ChronoLogCharStruct const &func_name,
                                      Args... args) {
//    int16_t port = baseServerPort_ + server_index;

    switch (CHRONOLOG_CONF->RPC_IMPLEMENTATION) {
        case CHRONOLOG_THALLIUM_TCP: {
            // TODO: NotImplemented error
            std::cout << "async call with protocol TCP IS NOT SUPPORTED YET" << std::endl;
            break;
        }
        case CHRONOLOG_THALLIUM_SOCKETS: {
            // TODO: NotImplemented error
            std::cout << "async call with protocol SOCKETS IS NOT SUPPORTED YET" << std::endl;
            break;
        }
        case CHRONOLOG_THALLIUM_ROCE: {
            // TODO: NotImplemented error
            std::cout << "async call with protocol ROCE IS NOT SUPPORTED YET" << std::endl;
            break;
        }
    }
}

template<typename Response, typename... Args>
std::future<Response> ChronoLogRPC::async_call(ChronoLogCharStruct &server,
                                      uint16_t &port,
                                      ChronoLogCharStruct const &func_name,
                                      Args... args) {
    switch (CHRONOLOG_CONF->RPC_IMPLEMENTATION) {
        case CHRONOLOG_THALLIUM_TCP: {
            // TODO: NotImplemented error
            break;
        }
        case CHRONOLOG_THALLIUM_SOCKETS: {
            // TODO: NotImplemented error
        }
        case CHRONOLOG_THALLIUM_ROCE: {
            // TODO: NotImplemented error
            break;
        }
    }
}
