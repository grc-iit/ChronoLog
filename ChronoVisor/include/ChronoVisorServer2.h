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
// Created by kfeng on 7/19/22.
//

#ifndef CHRONOLOG_CHRONOVISORSERVER2_H
#define CHRONOLOG_CHRONOVISORSERVER2_H

#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include <log.h>
#include <thallium.hpp>
#include <margo.h>
#include "TimeManager.h"
#include <TimeRecord.h>
#include "ClocksourceManager.h"
#include <ChronicleMetaDirectory.h>
#include <ClientRegistryManager.h>
#include <RPCVisor.h>

namespace ChronoVisor {
    namespace tl = thallium;

    class ChronoVisorServer2 {
    public:
        ChronoVisorServer2();

        ~ChronoVisorServer2() {
            if (pTimeManager) {
                delete pTimeManager;
                pTimeManager = nullptr;
            }
        }

        void setClocksourceType(ClocksourceType clocksourceType) { pTimeManager->setClocksourceType(clocksourceType); }

        int start();

    private:
        std::string protocol_;
        std::string baseIP_;
        int basePorts_{};
        int numPorts_;
        std::vector<std::string> serverAddrVec_;
        int numStreams_{};
        std::vector<tl::engine> engineVec_;
        std::vector<margo_instance_id> midVec_;

        /**
         * @name Clock-related variables
         */
        TimeManager *pTimeManager;

        /**
         * @name ClientRegistry related variables
         */
        ///@{
        std::shared_ptr<ClientRegistryManager> clientRegistryManager_;
        ///@}

        /**
         * @name Chronicle Meta Directory related variables
         */
        ///@{
        std::shared_ptr<ChronicleMetaDirectory> chronicleMetaDirectory_;
        ///@}

        /**
         * @name RPC related variables
         */
        ///@{
        std::shared_ptr<RPCVisor> rpcProxy_;
    };
}


#endif //CHRONOLOG_CHRONOVISORSERVER2_H
