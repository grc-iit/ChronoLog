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
// Created by kfeng on 10/24/22.
//

#ifndef CHRONOLOG_GLOBAL_VAR_VISOR_H
#define CHRONOLOG_GLOBAL_VAR_VISOR_H

#include "ClocksourceManager.h"
#include <ConfigurationManager.h>
#include <Chronicle.h>
#include <ChronicleMetaDirectory.h>
#include <rpc.h>
#include <RPCFactory.h>
#include <RPCVisor.h>

/**
 * Configuration-related global variables
 */
std::shared_ptr<ChronoLog::ConfigurationManager> g_confManager =
        ChronoLog::Singleton<ChronoLog::ConfigurationManager>::GetInstance();

/**
 * Clock management-related global variables
 */
ClocksourceManager *ClocksourceManager::clocksourceManager_ = nullptr;

/**
 * ClientRegistry-related global variables
 */
std::shared_ptr<ClientRegistryManager> g_clientRegistryManager =
        ChronoLog::Singleton<ClientRegistryManager>::GetInstance();
std::mutex g_clientRegistryMutex_;

/**
 * ChronicleMetaDirectory-related global variables
 */
std::shared_ptr<std::unordered_map<std::string, Chronicle *>> g_chronicleMap =
        ChronoLog::Singleton<std::unordered_map<std::string, Chronicle *>>::GetInstance();
std::shared_ptr<ChronicleMetaDirectory> g_chronicleMetaDirectory =
        ChronoLog::Singleton<ChronicleMetaDirectory>::GetInstance(); // ChronicleMetaDirectory has to been inited
                                                                     // after global ChronicleMap since it tried to
                                                                     // store a reference to it using Singleton
                                                                     // template class in its constructor
std::mutex g_chronicleMetaDirectoryMutex_;
std::mutex g_acquiredChronicleMapMutex_;
std::mutex g_acquiredStoryMapMutex_;

/**
 * RPC-related global variables
 */
// Shared RPC instance
std::shared_ptr<ChronoLogRPC> g_RPC =
        ChronoLog::Singleton<ChronoLogRPCFactory>::GetInstance()->GetRPC(CHRONOLOG_CONF->RPC_BASE_SERVER_PORT);
// RPC proxy instance
std::shared_ptr<RPCVisor> g_RPCProxy =
        ChronoLog::Singleton<RPCVisor>::GetInstance();

#endif //CHRONOLOG_GLOBAL_VAR_VISOR_H
