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

#include <algorithm>
#include <cstdio>
#include <ChronoVisorServer.h>
#include "ClocksourceManager.h"
#include "../../ChronoAPI/ChronoLog/include/common.h"

namespace ChronoVisor {

    ChronoVisorServer::ChronoVisorServer(int port)
            : SocketPP::TCPServer(port) {
        timeDBUpdateInterval_ = TIME_DB_UPDATE_INTERVAL;
        pTimeManager = new TimeManager();
        pChronicleMetaDirectory = new ChronicleMetaDirectory();
        pClientRegistryManager = new ClientRegistryManager();
        initTimeDB();
        initSerDe();
        startPeriodicTimeDBUpdateThread();
        startOnDemandTimeDBUpdateThread();
    }

    ChronoVisorServer::ChronoVisorServer(int port, int update_interval)
            : SocketPP::TCPServer(port), timeDBUpdateInterval_(update_interval) {
        pTimeManager = new TimeManager(update_interval);
        pChronicleMetaDirectory = new ChronicleMetaDirectory();
        pClientRegistryManager = new ClientRegistryManager();
        initTimeDB();
        initSerDe();
        startPeriodicTimeDBUpdateThread();
        startOnDemandTimeDBUpdateThread();
    }

    TimeRecord ChronoVisorServer::getCurTimeInfo() {
        return *(pTimeManager->genOnDemandTimeRecord());
    }

    void ChronoVisorServer::addChronoKeeperInfo(const ChronoKeeperInfo& chronoKeeperInfo) {
        std::unique_lock<std::mutex> lock(chronoKeeperInfoListMutex_);
        chronoKeeperInfoList_.push_back(chronoKeeperInfo);
    }

    int ChronoVisorServer::start() {
//        LOGD("ChronoVisor server initializing ...");
//        init();

        LOGI("ChronoVisor server starting, socket=%p, port=%d ...", this, port_);
        return this->loop();
    }

    void ChronoVisorServer::init() {
        SocketPP::TCPServer::setConnHandle([this] (const SocketPP::TCPStream& stream) {
            LOGI("add a new client to registry: fd=%d", stream.fd);
            addToClientRegistry(stream);
        });
        SocketPP::TCPServer::setDiscHandle([this] (const SocketPP::TCPStream& stream) {
            LOGI("remove a client from registry: fd=%d", stream.fd);
            removeFromClientRegistry(stream);
        });
    }

    void ChronoVisorServer::initTimeDB() {
//        assert(remove(timeDBName_.c_str()) != 0);
        rocksdb::Options options;
        options.create_if_missing = true;
        rocksdb::Status status =
                rocksdb::DB::Open(options, timeDBName_, &timeDB_);
        assert(status.ok());
    }

    void ChronoVisorServer::initSerDe(SerDeType type) {
        pSerDeFactory = new SerDeFactory(type);
        assert((pSerDe = pSerDeFactory->getSerDe()) != nullptr);
    }

    void ChronoVisorServer::addToClientRegistry(SocketPP::TCPStream stream) {
        ClientRegistryInfo client_record;
        std::unique_lock<std::mutex> lock(clientRegistryMutex_);
        clientRegistry_->emplace(std::make_pair(stream.fd, client_record));
    }

    void ChronoVisorServer::removeFromClientRegistry(SocketPP::TCPStream stream) {
        std::unique_lock<std::mutex> lock(clientRegistryMutex_);
        auto iter = clientRegistry_->find(stream.fd);

        if (iter == clientRegistry_->end()) {
            LOGE("fd=%d is not in client registry!", stream.fd);
            return;
        }

        clientRegistry_->erase(iter);
    }

    void ChronoVisorServer::startPeriodicTimeDBUpdateThread() {
        periodicTimeDBUpdateThread_ = new std::thread([this] {
            LOGD("periodic TimeDB update thread running...");
            TimeRecord record = *(pTimeManager->genPeriodicTimeRecord());
            rocksdb::Status status = timeDB_->Put(rocksdb::WriteOptions(),
                                                  std::to_string(record.getTimestamp()).c_str(),
                                                  std::to_string(record.getDriftRate()).c_str()); // first record
            LOGI("big bang time record is generated, %s, appended to timeDB ...",
                record.toString().c_str());

            pTimeManager->refTimestampUpdateInterval_ = timeDBUpdateInterval_;
            while (true) {
                {
                    // Append a record every interval
                    record = *(pTimeManager->genPeriodicTimeRecord());

                    std::ostringstream out;
                    out.precision(10);
                    out << std::fixed << record.getDriftRate();
                    std::string driftRateStr = out.str();

                    // Get TimeDB lock
                    {
                        std::unique_lock<std::mutex> lock(timeDBMutex_);

                        // Write to TimeDB
                        status = timeDB_->Put(rocksdb::WriteOptions(),
                                              std::to_string(record.getTimestamp()).c_str(),
                                              driftRateStr.c_str());
                    }
                    LOGI("every %lf seconds, new time record %s is appended to timeDB ...", \
                         timeDBUpdateInterval_, record.toString().c_str());

//                    LOGD("time record appended");

                    LOGD("sleeping for %lf seconds ...", timeDBUpdateInterval_);
                    usleep(timeDBUpdateInterval_ * 1000000);

                    if (periodicThrdStopped_) return;
                }
            }
        });
    }

    void ChronoVisorServer::startOnDemandTimeDBUpdateThread() {
        onDemandTimeDBUpdateThread_ = new std::thread([this] {
            LOGD("on-demand TimeDB update thread running...");
            TimeRecord record;
            rocksdb::Status status;
            while (true) {
                {
                    // Take a TimeRecord from the shared queue
                    {
                        // Lock the queue
                        std::unique_lock<std::mutex> lock(timeDBWriteQueueMutex_);
                        while (timeDBWriteQueueSize_ == 0) timeDBWriteQueueCV_.wait(lock);
                        timeDBWriteQueueSize_--;

                        record = timeDBWriteQueue_.front();
                        timeDBWriteQueue_.pop();
                    }

                    std::ostringstream out;
                    out.precision(10);
                    out << std::fixed << record.getDriftRate();
                    std::string driftRateStr = out.str();

                    {
                        // Get TimeDB lock
                        std::unique_lock<std::mutex> lock(timeDBMutex_);

                        // Write to TimeDB
                        status = timeDB_->Put(rocksdb::WriteOptions(),
                                              std::to_string(record.getTimestamp()).c_str(),
                                              driftRateStr.c_str());
                    }
                    LOGI("upon client requests, new time record %s is appended to timeDB ...", \
                         record.toString().c_str());

                    if (onDemandThrdStopped_) return;
                }
            }
        });
    }

    void ChronoVisorServer::onReceive(int fd, const byte* buf, size_t len) {
        SocketPP::TCPServer::onReceive(fd, buf, len);
        std::unique_ptr<ClientMessage> clientMsg;
        {
            clientMsg = pSerDe->deserializeClientMessage(const_cast<unsigned char *>(buf), len);
        }

        SocketPP::Message reply;
        reply.target = SocketPP::TCPStream(fd);
        std::unique_ptr<std::ostringstream> oss;
        ServerMessage serverMsg;

        switch (clientMsg->msgType_) {
            case ClientMessage::CONNECTION: {
                LOGI("connection message has been handled by onConnected");
                break;
            }
            case ClientMessage::GETTIMEINFO: {
                serverMsg.msgType_ = ServerMessage::GETTIMEINFORESPONSE;
                TimeRecord record = *(pTimeManager->genOnDemandTimeRecord());
                serverMsg.timeInfo_.timestamp_ = std::to_string(record.getTimestamp());
                serverMsg.timeInfo_.driftRate_ = record.getDriftRate();
                {
                    oss = pSerDe->serializeServerMessage(serverMsg);
                }
                LOGD("Msg to client: %s", serverMsg.toString().c_str());
                reply.rawMsg.initMsg((const byte *) oss->str().c_str(), oss->str().length());

                // Upon each get_clock request from clients, append a record to TimeDB
                {
                    std::unique_lock<std::mutex> lock(timeDBWriteQueueMutex_);
                    timeDBWriteQueueSize_++;
                    timeDBWriteQueueCV_.notify_all();
                    timeDBWriteQueue_.emplace(record);
                }
                break;
            }
            case ClientMessage::GETCHRONOKEEPERINFO: {
                serverMsg.msgType_ = ServerMessage::GETCHRONOKEEPERINFORESPONSE;
                serverMsg.chronoKeeperList_ = chronoKeeperInfoList_;
                {
                    oss = pSerDe->serializeServerMessage(serverMsg);
                }
                LOGD("Msg to client: %s", serverMsg.toString().c_str());
                reply.rawMsg.initMsg((const byte *) oss->str().c_str(), oss->str().length());
                break;
            }
            case ClientMessage::DISCONNECTION: {
                removeFromClientRegistry(fd);
                break;
            }
            case ClientMessage::CREATECHRONICLE: {
                pChronicleMetaDirectory->create_chronicle(clientMsg->chronicleName_);
                serverMsg.msgType_ = ServerMessage::CREATECHRONICLERESPONSE;
                serverMsg.chronicleId_ = CityHash64(clientMsg->chronicleName_.c_str(),
                                                    clientMsg->chronicleName_.size());
                {
                    oss = pSerDe->serializeServerMessage(serverMsg);
                }
                LOGD("Msg to client: %s", serverMsg.toString().c_str());
                reply.rawMsg.initMsg((const byte *) oss->str().c_str(), oss->str().length());
                break;
            }
            case ClientMessage::ACQUIRECHRONICLE: {
                pChronicleMetaDirectory->acquire_chronicle(clientMsg->chronicleName_, 0);
                break;
            }
            case ClientMessage::RELEASECHRONICLE: {
                pChronicleMetaDirectory->release_chronicle(clientMsg->chronicleId_, 0);
                break;
            }
            case ClientMessage::DESTROYCHRONICLE: {
                pChronicleMetaDirectory->destroy_chronicle(clientMsg->chronicleName_, 0);
                break;
            }
            default: {
                LOGE("unknown message from client");
            }
        }

        this->send(reply);
    }

    void ChronoVisorServer::onConnected(int fd) {
        SocketPP::TCPServer::onConnected(fd);
        std::unique_ptr<std::ostringstream> oss;

        ServerMessage serverMsg;
        TimeInfo timeInfo(std::string("1234567890"), 0.00001111);
        serverMsg.msgType_ = ServerMessage::CONNRESPONSE;
        serverMsg.timeInfo_ = timeInfo;
        serverMsg.chronoKeeperList_ = chronoKeeperInfoList_;
        {
            oss = pSerDe->serializeServerMessage(serverMsg);
        }

//        LOGD("Msg to client: %s", serverMsg.toString().c_str());
        SocketPP::Message reply;
        reply.rawMsg.initMsg((const byte *)oss->str().c_str(), oss->str().length());
        reply.target = SocketPP::TCPStream(fd);
        this->send(reply);
    }
}
