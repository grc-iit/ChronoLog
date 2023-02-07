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

#ifndef CHRONOLOG_MACRO_H
#define CHRONOLOG_MACRO_H

#include <thallium.hpp>
#include "ConfigurationManager.h"
#include "singleton.h"

#define CHRONOLOG_CONF ChronoLog::Singleton<ChronoLog::ConfigurationManager>::GetInstance()

#define CHRONOLOG_RPC_CALL_WRAPPER_THALLIUM_SOCKETS() case CHRONOLOG_THALLIUM_SOCKETS:
#define CHRONOLOG_RPC_CALL_WRAPPER_THALLIUM_TCP() case CHRONOLOG_THALLIUM_TCP:
#define CHRONOLOG_RPC_CALL_WRAPPER_THALLIUM_ROCE() case CHRONOLOG_THALLIUM_ROCE:

#define CHRONOLOG_RPC_CALL_WRAPPER_THALLIUM(funcname, serverVar, ret, args...) \
{ \
    return rpc->call<ret>(serverVar, func_prefix + funcname, args); \
    break; \
}

#define CHRONOLOG_RPC_CALL_WRAPPER(funcname, serverVar, ret, args...) [& ]()-> ret { \
    switch (CHRONOLOG_CONF->RPC_IMPLEMENTATION) { \
        CHRONOLOG_RPC_CALL_WRAPPER_THALLIUM_SOCKETS() \
        CHRONOLOG_RPC_CALL_WRAPPER_THALLIUM_TCP() \
        CHRONOLOG_RPC_CALL_WRAPPER_THALLIUM_ROCE() \
        CHRONOLOG_RPC_CALL_WRAPPER_THALLIUM(funcname, serverVar, ret, args) \
    } \
}();
// this is how it is called: CHRONOLOG_RPC_CALL_WRAPPER("Connect", 0, bool, uri, client_id);

#define CHRONOLOG_THALLIUM_DEFINE(name, args,args_t...) \
void Thallium##name(const tl::request &thallium_req, args_t) \
{ \
    thallium_req.respond(name args); \
}

#define ASSERT(left,operator,right) { if(!((left) operator (right))){ \
std::cerr << "ASSERT FAILED: " << #left << #operator << #right \
<< " @ " << __FILE__ << " (" << __LINE__ << "). " \
<< #left << "=" << (left) << "; " << #right << "=" << (right) << std::endl; } }

#endif //CHRONOLOG_MACRO_H
