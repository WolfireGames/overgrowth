//-----------------------------------------------------------------------------
//           Name: asnetwork.h
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
//   Copyright 2022 Wolfire Games LLC
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//-----------------------------------------------------------------------------
#pragma once

#include <Threading/threadsafequeue.h>

#include <SDL_net.h>

#include <map>

#define SOCKET_ID_INVALID 0xFFFFFFFF

typedef uint32_t SocketID;

class SocketData {
   public:
    TCPsocket socket;

    static const unsigned BUF_SIZE = 128;
    bool valid;
    size_t used;
    uint8_t buf[BUF_SIZE];
};

class ASNetworkCallback {
   public:
    virtual void IncomingTCPData(SocketID socket, uint8_t* data, size_t len) = 0;
};

class ASNetwork {
   private:
    std::vector<ASNetworkCallback*> callbacks;

    static const int MAX_SOCKETS;
    uint32_t socket_id_counter;
    std::map<SocketID, SocketData> sockets;

    SDLNet_SocketSet socketset;

   public:
    void Initialize();
    void Dispose();

    void Update();

    SocketID CreateSocketTCP(std::string host, uint16_t port);
    void DestroySocketTCP(SocketID sock);

    int SocketTCPSend(SocketID socket, const uint8_t* buf, size_t len);

    bool IsValidSocketTCP(SocketID socket);

    void RegisterASNetworkCallback(ASNetworkCallback* cb);
    void DeRegisterASNetworkCallback(ASNetworkCallback* cb);
};
