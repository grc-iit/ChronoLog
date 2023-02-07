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
// Created by kfeng on 11/15/21.
//

#include <chrono>
#include "SocketPP.h"
#include "SerDe.h"

//#define NDEBUG 1

using namespace SocketPP;

int main() {
    const int port = 6000;
    TCPClient client("127.0.0.1", port);

    std::chrono::high_resolution_clock::time_point t1, t2;
    std::chrono::duration<double> duration{};
    SerDeFactory *pSerDeFactory = new SerDeFactory(SerDeType::CEREAL);
    SerDe *pSerDe = pSerDeFactory->getSerDe();

    client.setConnHandle([&] (const TCPStream& stream) {
        ClientMessage clientMsg;
        clientMsg.msgType_ = ClientMessage::CONNECTION;
        std::unique_ptr<std::ostringstream> oss;
        {
            oss = pSerDe->serializeClientMessage(clientMsg);
        }
        LOGD("Serialized client msg (len: %ld): %s", oss->str().length(), oss->str().c_str());
        t1 = std::chrono::high_resolution_clock::now();
        client.send(oss->str());
    });

    client.setRecvHandle([&] (const Message& message) {
        std::unique_ptr<ServerMessage> serverMsg;
        {
            serverMsg = pSerDe->deserializeServerMessage(message.rawMsg.data(),
                                                         message.rawMsg.length());
        }
        t2 = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
        LOGD("server message received: %s", serverMsg->toString().c_str());
        std::cout << "Latency: " << duration.count() * 1000 << " ms" << std::endl;

        usleep(3e6);
        ClientMessage clientMsg;
        clientMsg.msgType_ = ClientMessage::GETTIMEINFO;
        std::unique_ptr<std::ostringstream> oss;
        {
            oss = pSerDe->serializeClientMessage(clientMsg);
        }
        LOGD("Serialized client msg (len: %ld): %s", oss->str().length(), oss->str().c_str());
        t1 = std::chrono::high_resolution_clock::now();
        client.send(oss->str());
    });

    return client.loop();
}
