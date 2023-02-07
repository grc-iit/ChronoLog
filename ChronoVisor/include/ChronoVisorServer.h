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
// Created by kfeng on 10/25/21.
//

#ifndef CHRONOLOG_CHRONOVISOR_H
#define CHRONOLOG_CHRONOVISOR_H

#include <unordered_map>
#include <atomic>
#include <rocksdb/db.h>
#include <TCPServer.h>
#include <Socket.h>
#include <type.h>
#include <log.h>
#include "TimeManager.h"
#include <TimeRecord.h>
#include <ClientRegistryInfo.h>
#include <SerDe.h>
#include <ChronicleMetaDirectory.h>
#include <ClientRegistryManager.h>

namespace ChronoVisor {

    class ChronoVisorServer : public SocketPP::TCPServer {
    public:
        ChronoVisorServer(int port, int update_interval);
        explicit ChronoVisorServer(int port);

        ~ChronoVisorServer() override {
            if (pSerDe) { delete pSerDe; pSerDe = nullptr; }
            if (pTimeManager) { delete pTimeManager; pTimeManager = nullptr; }
            if (pChronicleMetaDirectory) { delete pChronicleMetaDirectory; pChronicleMetaDirectory = nullptr; }
        }

        void setClocksourceType(ClocksourceType clocksourceType) { pTimeManager->setClocksourceType(clocksourceType); }

        /**
         * Handler invoked when server recvs sth
         * @param fd
         * @param buf
         * @param len
         */
        void onReceive(int fd, const byte* buf, size_t len) override;

        /**
         * Handler invoked when a new client connects
         * @param fd
         */
        void onConnected(int fd) override;

        int start();

        /**
         * Get lastest time info using maintained timestamp as the key
         * @return the latest TimeInfo with timestamp and drift rate
         */
        TimeRecord getCurTimeInfo();

        /**
         * Append new ChronoKeeper to the list
         * @param chronoKeeperInfo info of new ChronoKeeper
         */
        void addChronoKeeperInfo(const ChronoKeeperInfo& chronoKeeperInfo);

    private:
        /**
         * Initialize ChronoVisor
         */
        void init();

        /**
         * @name TimeDB related functions
         */
        ///@{
        /** Initiate RocksDB */
        void initTimeDB();
        ///@}

        /**
         * @name Serialization/deserialization related functions
         */
        ///@{
        /** Initiate SerDe */
        void initSerDe(SerDeType type = SerDeType::CEREAL);
        ///@}

        /**
         * @name ClientRegistry related functions
         */
        ///@{
        void addToClientRegistry(SocketPP::TCPStream stream);
        void removeFromClientRegistry(SocketPP::TCPStream stream);
        ///@}

        /**
         * @name Periodic TimeDB update thread functions
         */
        void startPeriodicTimeDBUpdateThread();

        /**
         * @name On-demand TimeDB update thread functions
         */
        void startOnDemandTimeDBUpdateThread();


        bool periodicThrdStarted_ = false;
        bool periodicThrdStopped_ = false;
        std::thread* periodicTimeDBUpdateThread_ = nullptr;

        bool onDemandThrdStarted_ = false;
        bool onDemandThrdStopped_ = false;
        std::thread* onDemandTimeDBUpdateThread_ = nullptr;

        TimeManager *pTimeManager;
        /**
         * @name In-memory time info related variables
         */
        ///@{
        double timeDBUpdateInterval_;                    ///< in second
        ///@}

        /**
         * @name TimeDB related variables
         */
        ///@{
        rocksdb::DB *timeDB_{};
        std::mutex timeDBMutex_;
        const std::string timeDBName_ = "/tmp/chronolog_timedb";
        ///@}

        /**
         * @name ClientRegistry related variables
         */
        ///@{
        ClientRegistryManager *pClientRegistryManager;
        std::shared_ptr<std::unordered_map<int, ClientRegistryInfo>> clientRegistry_; ///< use socket fd as the key
        std::mutex clientRegistryMutex_;
        ///@}

        /**
         * @name ChronoKeeper info related variables
         */
        ///@{
        std::vector<ChronoKeeperInfo> chronoKeeperInfoList_;
        std::mutex chronoKeeperInfoListMutex_;
        ///@}

        /**
         * @name Serialization/deserialization related variables
         */
        SerDeFactory *pSerDeFactory{};
        SerDe *pSerDe{};

        /**
         * @name Chronicle Meta Directory related variables
         */
        ChronicleMetaDirectory *pChronicleMetaDirectory;
    };

}

#endif //CHRONOLOG_CHRONOVISOR_H
