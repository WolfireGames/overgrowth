//-----------------------------------------------------------------------------
//           Name: basic_net_framework.h
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

#ifdef USE_BASIC_NETWORK_FRAMEWORK
#include <Game/connection_closed_reason.h>
#include <Network/net_framework_common.h>

#include <SDL_net.h>

#include <map>
#include <string>
#include <vector>
#include <list>

using std::list;
using std::map;
using std::string;
using std::vector;

typedef uint8_t NetConnectionID;
typedef uint8_t NetListenSocketID;

class Online;

class NetworkingMessage {
   private:
    friend class BasicNetFramework;

    uint8_t* data;
    uint16_t size;

   public:
    NetworkingMessage();
    const void* GetData() const;
    uint64_t GetSize() const;
    void Release();
};

struct NetConnectionInfo {
    ConnectionClosedReason end_reason;
    NetFrameworkConnectionState connection_state;
};

struct NetFrameworkConnectionStatusChanged {
    NetConnectionID conn_id;
    NetConnectionInfo conn_info;
};

struct NetConnectionStatus {
    NetConnectionStatus();

    list<NetworkingMessage> messages;

    vector<uint8_t> buf;
    uint32_t count;
};

class BasicNetFramework {
    Online* online;

   private:
    SDLNet_SocketSet socket_set;

    map<NetConnectionID, TCPsocket> connection_id_map;
    map<TCPsocket, NetConnectionID> connection_id_map_reverse;
    map<NetConnectionID, NetConnectionStatus> connection_status;

    map<NetListenSocketID, TCPsocket> listen_socket_map;
    map<TCPsocket, NetListenSocketID> listen_socket_map_reverse;

    NetConnectionID GetFreeConnectionID();
    NetListenSocketID GetFreeListenSocketID();

   public:
    BasicNetFramework(Online* online_callback);

    void Initialize();
    void Dispose();
    void Update();

    bool HasFriendInviteOverlay() const;

    bool ConnectByIPAddress(const string& address, NetConnectionID* id);

    NetListenSocketID CreateListenSocketIP(const string& local_address);
    void CloseListenSocket(NetListenSocketID listen_socket_id);

    void GetConnectionStatus(NetConnectionID conn_id, ConnectionStatus* status) const;

    void CloseConnection(NetConnectionID conn_id, ConnectionClosedReason reason);

    int ReceiveMessageOnConnection(NetConnectionID conn_id, NetworkingMessage* msg);

    bool AcceptConnection(NetConnectionID conn_id, int* result);

    string GetResultString(int result);

    void SendMessageToConnection(NetConnectionID conn_id, char* buffer, uint32_t bytes, bool reliable, bool flush);
};
#endif
