//-----------------------------------------------------------------------------
//           Name: steam_net_framework.cpp
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
#include "steam_net_framework.h"

#ifdef USE_STEAM_NETWORK_FRAMEWORK
#include <Network/net_framework_common.h>
#include <Logging/logdata.h>
#include <Online/online.h>
#include <Steam/steamworks_util.h>

#include <cassert>
#include <vector>
#include <iostream>
#include <map>


using std::vector;
using std::endl;
using std::map;

static const uint16_t default_port = 25742;

static const map<ESteamNetworkingConnectionState, NetFrameworkConnectionState> connection_state_map = {
    {ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_Connecting, NetFrameworkConnectionState::Connecting},
    {ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_ClosedByPeer, NetFrameworkConnectionState::ClosedByPeer},
    {ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_Connected, NetFrameworkConnectionState::Connected},
    {ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_FindingRoute, NetFrameworkConnectionState::FindingRoute},
    {ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_ProblemDetectedLocally, NetFrameworkConnectionState::ProblemDetectedLocally}
};

static void SteamDebugOutputFunction(ESteamNetworkingSocketsDebugOutputType nType, const char * pszMsg) {
    LOGI << pszMsg << endl;
}

NetworkingMessage::NetworkingMessage() : msg(nullptr){

}

NetworkingMessage::NetworkingMessage(SteamNetworkingMessage_t* msg) : msg(msg) {
}

const void* NetworkingMessage::GetData() const {
    return msg->GetData();
}

uint64_t NetworkingMessage::GetSize() const {
    return msg->GetSize();
}

void NetworkingMessage::Release() {
    msg->Release();
}

SteamNetFramework::SteamNetFramework(Online* online_callback) : online(online_callback), isns(nullptr), utils(nullptr) {

}

NetConnectionID SteamNetFramework::GetFreeConnectionID() {
    for(int i = 1; i < 255; i++) {
        if(connection_id_map.find(i) == connection_id_map.end()) {
            return i;
        }
    }
    LOGF << "Ran out of id's for connections, this is a fatal issue." << endl;
    return 0;
}

NetListenSocketID SteamNetFramework::GetFreeListenSocketID() {
    for(int i = 1; i < 255; i++) {
        if(listen_socket_map.find(i) == listen_socket_map.end()) {
            return i;
        }
    }
    LOGF << "Ran out of id's for listening sockets, this is a fatal issue." << endl;
    return 0;
}


void SteamNetFramework::Initialize() {
    isns = SteamNetworkingSockets();
    utils = SteamNetworkingUtils();

    utils->SetDebugOutputFunction(ESteamNetworkingSocketsDebugOutputType::k_ESteamNetworkingSocketsDebugOutputType_Error, 
        SteamDebugOutputFunction);

    utils->InitRelayNetworkAccess();

    // this is some random numbers that just seems to solve a bug in SteamSDK defaulting into too small values.
    int mtuPacketsize = 1200;
    int outgoingbuffersize = 4096 * 60;
    SteamNetworkingConfigValue_t config_1[4];

    config_1[0].SetInt32(ESteamNetworkingConfigValue::k_ESteamNetworkingConfig_MTU_PacketSize, mtuPacketsize + 100);

    config_1[1].SetInt32(ESteamNetworkingConfigValue::k_ESteamNetworkingConfig_SendBufferSize, outgoingbuffersize);

    config_1[2].SetInt32(ESteamNetworkingConfigValue::k_ESteamNetworkingConfig_SendRateMin, 1024 * 1024 * 9);

    config_1[3].SetInt32(ESteamNetworkingConfigValue::k_ESteamNetworkingConfig_SendRateMax, 1024 * 1024 * 10);


    utils->SetConfigValueStruct(config_1[0], ESteamNetworkingConfigScope::k_ESteamNetworkingConfig_Global, 0);
    utils->SetConfigValueStruct(config_1[1], ESteamNetworkingConfigScope::k_ESteamNetworkingConfig_Global, 0);

    utils->SetConfigValueStruct(config_1[2], ESteamNetworkingConfigScope::k_ESteamNetworkingConfig_Global, 0);
    utils->SetConfigValueStruct(config_1[3], ESteamNetworkingConfigScope::k_ESteamNetworkingConfig_Global, 0);
}

void SteamNetFramework::Dispose() {

}

void SteamNetFramework::Update() {
}

bool SteamNetFramework::HasFriendInviteOverlay() const {
    return SteamUtils()->IsOverlayEnabled();
}

bool SteamNetFramework::ConnectByIPAddress(const string& address, NetConnectionID* id) {
    assert(isns);

    SteamNetworkingIPAddr ipAddress;
    ipAddress.Clear();

    bool ok = ipAddress.ParseString(address.c_str());
    if (!ok) {
        LOGE << "Invalid host IP address " << address << endl;
        return false;
    } else {
        LOGI << "Connecting to adress: " << address << endl;
    }

    if (ipAddress.m_port == 0) {
        ipAddress.m_port = default_port;
    }

    HSteamNetConnection connection = isns->ConnectByIPAddress(ipAddress, 0, nullptr);
    NetConnectionID connection_id = GetFreeConnectionID();
    vector<char> temp(1024, 0);
    int ret = isns->GetDetailedConnectionStatus(connection, &temp[0], temp.size());
    if (ret >= 0) {
        LOGI << "connection status: " << &temp[0] << endl;
    } else {
        LOGE << "GetDetailedConnectionStatus failed" << endl;
    }

    connection_id_map[connection_id] = connection;
    connection_id_map_reverse[connection] = connection_id;

    *id = connection_id;

    return true;
}

NetListenSocketID SteamNetFramework::CreateListenSocketIP(const string &local_address) {
    assert(isns);

    SteamNetworkingIPAddr ipAddress;
    ipAddress.Clear();

    bool ok = ipAddress.ParseString(local_address.c_str());
    if (ok) {
        LOGI << "Attempting to create IP listen socket at " << local_address << endl;
    } else {
        LOGI << "Attempting to create IP listen socket at default port" << endl;
        ipAddress.m_port = default_port;
    }

    HSteamListenSocket listen = isns->CreateListenSocketIP(ipAddress, 0, nullptr);
    NetListenSocketID socket_id = GetFreeListenSocketID();

    listen_socket_map[socket_id] = listen;
    listen_socket_map_reverse[listen] = socket_id;

    bool data = isns->GetListenSocketAddress(listen, &ipAddress);
    // REWRITE: It will fail if port is busy.
    if (data) {
        vector<char> temp(1024, 0);
        ipAddress.ToString(&temp[0], temp.size(), true);
        LOGI << "IP listen socket address: " << &temp[0] << endl;


    } else {
        LOGE << "IP listen socket has no address" << endl;
    }

    return socket_id;
}

void SteamNetFramework::CloseListenSocket(NetListenSocketID listen_socket_id) {
    auto steam_socket = listen_socket_map.find(listen_socket_id);    
    assert(isns);

    if(steam_socket != listen_socket_map.end()) {
        isns->CloseListenSocket(steam_socket->second);
        listen_socket_map_reverse.erase(listen_socket_map_reverse.find(steam_socket->second));
        listen_socket_map.erase(steam_socket);
    }
}

void SteamNetFramework::CloseConnection(NetConnectionID conn_id, ConnectionClosedReason reason) {
    auto map_ref = connection_id_map.find(conn_id);

    if(map_ref != connection_id_map.end()) {
        isns->CloseConnection(map_ref->second, static_cast<int>(reason), nullptr, false);
        connection_id_map_reverse.erase(connection_id_map_reverse.find(map_ref->second));
        connection_id_map.erase(map_ref);
    }
}

void SteamNetFramework::GetConnectionStatus(NetConnectionID conn_id, ConnectionStatus* status) const {
    SteamNetworkingQuickConnectionStatus steam_status;
    isns->GetQuickConnectionStatus(connection_id_map.at(conn_id), &steam_status);

	status->ms_ping = steam_status.m_nPing;
	status->connection_quality_local = steam_status.m_flConnectionQualityLocal;
	status->connection_quality_remote = steam_status.m_flConnectionQualityRemote;
	status->out_packets_per_sec = steam_status.m_flOutPacketsPerSec;
	status->out_bytes_per_sec = steam_status.m_flOutBytesPerSec;
	status->in_packets_per_sec = steam_status.m_flInPacketsPerSec;
	status->in_bytes_per_sec = steam_status.m_flInBytesPerSec;
	status->send_rate_bytes_per_second = steam_status.m_nSendRateBytesPerSecond;
	status->pending_unreliable = steam_status.m_cbPendingUnreliable;
	status->pending_reliable = steam_status.m_cbPendingReliable;
	status->sent_unacked_reliable = steam_status.m_cbSentUnackedReliable;
	status->usec_queue_time = steam_status.m_usecQueueTime;
}

void SteamNetFramework::OnConnectionChange(SteamNetConnectionStatusChangedCallback_t *data) {
    NetFrameworkConnectionStatusChanged status_changed;

    LOGI << "Online::OnConnectionChange " << data->m_hConn
            << " old:" << SteamworksMatchmaking::StatusToString(data->m_eOldState)
            << " new: " << SteamworksMatchmaking::StatusToString(data->m_info.m_eState)
            << endl;

    if(data->m_info.m_eState == k_ESteamNetworkingConnectionState_None) {
        LOGW << "We've gotten an unexpected connection change, this is an error state in the Steam sockets. We likely told steam to do something with this connection after a disconnection" << std::endl;
        return;
    }

    if(data->m_info.m_eState == k_ESteamNetworkingConnectionState_Connecting) {
        NetConnectionID connection_id = GetFreeConnectionID();
        connection_id_map[connection_id] = data->m_hConn;
        connection_id_map_reverse[data->m_hConn] = connection_id;
    }

    status_changed.conn_id = connection_id_map_reverse.at(data->m_hConn);
    status_changed.conn_info.end_reason = static_cast<ConnectionClosedReason>(data->m_info.m_eEndReason);

    auto connection_state_it = connection_state_map.find(data->m_info.m_eState);
    if(connection_state_it != connection_state_map.end()) {
        status_changed.conn_info.connection_state = connection_state_it->second;
    } else {
        status_changed.conn_info.connection_state = NetFrameworkConnectionState::Unknown;
    }

    online->OnConnectionChange(&status_changed);
}

int SteamNetFramework::ReceiveMessageOnConnection(NetConnectionID conn_id, NetworkingMessage* networking_message) {
    SteamNetworkingMessage_t* msg;
    int ret = isns->ReceiveMessagesOnConnection(connection_id_map[conn_id], &msg, 1);
    *networking_message = NetworkingMessage(msg);
    return ret;
}

bool SteamNetFramework::AcceptConnection(NetConnectionID conn_id, int* result) {
    EResult r = isns->AcceptConnection(connection_id_map[conn_id]);
    *result = (int)r;
    return r == EResult::k_EResultOK;
}

string SteamNetFramework::GetResultString(int result) {
    EResult r = (EResult)result;
    return string(GetEResultString(r));
}

void SteamNetFramework::SendMessageToConnection(NetConnectionID conn_id, char* buffer, uint32_t bytes, bool reliable, bool flush) {

    int send_flags = k_nSteamNetworkingSend_UseCurrentThread | k_nSteamNetworkingSend_AutoRestartBrokenSession;

    if(reliable) {
        send_flags |= k_nSteamNetworkingSend_Reliable;
    } else {
        send_flags |= k_nSteamNetworkingSend_Unreliable | k_nSteamNetworkingSend_NoDelay;
    }

    if(flush) {
        send_flags |= k_nSteamNetworkingSend_NoNagle;
    }

    EResult result = isns->SendMessageToConnection(connection_id_map[conn_id], buffer,
        bytes,
        send_flags,
        0);

    if (k_EResultOK != result) {
        switch (result) {
            LOGE << "Failed to send message: Error " << result << endl;
            case k_EResultInvalidParam:
                LOGE << "Invalid connection handle!" << endl;
                break;

            case k_EResultInvalidState:
                LOGE << "Connection is in an invalid state!" << endl;
                break;

            case k_EResultNoConnection:
                LOGE << "Connection has ended!" << endl;
                break;

            case k_EResultIgnored:
                LOGE << "We weren't (yet) connected, so this operation has no effect.!" << endl;
                break;
            
            case k_EResultLimitExceeded: {
                LOGE << "Internal outgoing buffer full" << endl;
                break;
            }
        }
    }
}

#endif
