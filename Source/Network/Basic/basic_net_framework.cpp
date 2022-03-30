//-----------------------------------------------------------------------------
//           Name: basic_net_framework.cpp
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
#include "basic_net_framework.h"

#ifdef USE_BASIC_NETWORK_FRAMEWORK
#include <Network/net_framework_common.h>
#include <Logging/logdata.h>
#include <Online/online.h>

#include <SDL_endian.h>

#include <cassert>
#include <vector>
#include <iostream>

using std::vector;
using std::endl;

static const uint16_t default_port = 25743;

static const uint16_t MESSAGE_HEADER_SIZE = 4;

NetworkingMessage::NetworkingMessage() : data(nullptr), size(0) {
}

const void* NetworkingMessage::GetData() const {
    return data;
}

uint64_t NetworkingMessage::GetSize() const {
    return size;
}

void NetworkingMessage::Release() {
    delete data;
    data = nullptr;
    size = 0;
}

NetConnectionStatus::NetConnectionStatus() : count(0) {
    buf.resize(1024 * 4);
}

BasicNetFramework::BasicNetFramework(Online* online_callback) : online(online_callback), socket_set(nullptr) {

}

NetConnectionID BasicNetFramework::GetFreeConnectionID() {
    for(int i = 1; i < 255; i++) {
        if(connection_id_map.find(i) == connection_id_map.end()) {
            return i;
        }
    }
    LOGF << "Ran out of id's for connections, this is a fatal issue." << endl;
    return 0;
}

NetListenSocketID BasicNetFramework::GetFreeListenSocketID() {
    for(int i = 1; i < 255; i++) {
        if(listen_socket_map.find(i) == listen_socket_map.end()) {
            return i;
        }
    }
    LOGF << "Ran out of id's for listening sockets, this is a fatal issue." << endl;
    return 0;
}


void BasicNetFramework::Initialize() {
    socket_set = SDLNet_AllocSocketSet(255);
}

void BasicNetFramework::Dispose() {
    SDLNet_FreeSocketSet(socket_set);
}

void BasicNetFramework::Update() {
    //See if there are any new incoming connections.
    for(auto it : listen_socket_map) {
        TCPsocket new_socket = SDLNet_TCP_Accept(it.second);

        if(new_socket != nullptr) {
            NetConnectionID new_connection_id = GetFreeConnectionID();
            if(new_connection_id > 0) {
                SDLNet_AddSocket(socket_set, (SDLNet_GenericSocket)new_socket);
                connection_id_map[new_connection_id] = new_socket;
                connection_id_map_reverse[new_socket] = new_connection_id;
                connection_status.emplace(new_connection_id, NetConnectionStatus());

                NetFrameworkConnectionStatusChanged status_changed;
                status_changed.conn_id = new_connection_id;
                status_changed.conn_info.connection_state = NetFrameworkConnectionState::Connecting;

                //This call must result in the called either calling AcceptConnection or CloseConnection on the given socket.
                online->OnConnectionChange(&status_changed); 
            } else {
                LOGE << "Got an incoming connection, but there are too many already, dropping it" << std::endl;
                SDLNet_TCP_Close(new_socket);
            }
        }
    }

    //Check for, get and buffer data from sockets.
    int nr_sockets = SDLNet_CheckSockets(socket_set, 0);
    if(nr_sockets > 0) {
        for(auto it : connection_id_map) {
            if(SDLNet_SocketReady(it.second)) {
                auto conn_status = connection_status.find(it.first);
                if(conn_status != connection_status.end()) {
                    NetConnectionStatus* ncs = &conn_status->second;
                    //If we don't have a reasonable amount of buffer space left, grow it.
                    if(ncs->buf.size() - ncs->count < 1024) {
                        ncs->buf.resize(ncs->buf.size() + 1024);
                    }

                    int recv_size = SDLNet_TCP_Recv(it.second, ncs->buf.data() + ncs->count, ncs->buf.size() - ncs->count);

                    if(recv_size > 0) {
                        /*
                        char* data_as_string = new char[recv_size+1];
                        data_as_string[recv_size] = '\0';
                        memcpy(data_as_string, ncs->buf.data() + ncs->count, recv_size);

                        LOGW << "Received Data: \"" << data_as_string << "\"" << endl;
                        */

                        ncs->count += recv_size;
                    } else {
                        LOGF << "Got an error on socket, something is broken, this should trigger a disconnect" << std::endl;
                        //TODO, set a flag to trigger a disconnect.
                    }

                    if(ncs->count > MESSAGE_HEADER_SIZE) {

                        uint32_t message_size = SDL_SwapBE16(((uint16_t*)ncs->buf.data())[0]);
                        uint32_t block_size = message_size + MESSAGE_HEADER_SIZE;
                        uint32_t remainder_size = ncs->count - block_size;

                        if(ncs->buf[2] != 0 || ncs->buf[3] != 0) {
                            LOGE << "Corrupt package detected, header bytes that are supposed to be zero aren't." << std::endl;
                        }

                        //Check if we've buffered up enough data for the next incoming message.
                        if(ncs->count >= block_size) {
                            NetworkingMessage msg;
                            msg.data = new uint8_t[message_size];
                            msg.size = message_size;

                            //Copy out the message data from the block.
                            memcpy(msg.data, ncs->buf.data() + MESSAGE_HEADER_SIZE, message_size);

                            //Shift down the next upcoming block, if there's any additional data.
                            if(remainder_size > 0) {
                                memmove(ncs->buf.data(), ncs->buf.data() + block_size, remainder_size);
                            }
                            ncs->count = remainder_size;
                            assert(remainder_size >= 0);

                            ncs->messages.push_back(msg);
                        }
                    }
                } else {
                    LOGF << "Missing connection status structure." << std::endl;
                }
            }
        }
    }
}

bool BasicNetFramework::HasFriendInviteOverlay() const {
    return false;
}

bool BasicNetFramework::ConnectByIPAddress(const string& address, NetConnectionID* id) {
    *id = GetFreeConnectionID(); 
    if(*id != 0) {
        IPaddress addr;

        int ret = SDLNet_ResolveHost(&addr, address.c_str(), default_port);
        if(ret == 0) {
            TCPsocket socket;
            
            socket = SDLNet_TCP_Open(&addr);
            if(socket != nullptr) {
                SDLNet_AddSocket(socket_set, (SDLNet_GenericSocket)socket);
                connection_id_map[*id] = socket;
                connection_id_map_reverse[socket] = *id;
                connection_status.emplace(*id, NetConnectionStatus());

                NetFrameworkConnectionStatusChanged status_changed;
                status_changed.conn_id = *id;
                status_changed.conn_info.connection_state = NetFrameworkConnectionState::Connected;

                online->OnConnectionChange(&status_changed); 

                return true;
            } else {
                LOGE << "Unable to open socket to resolved address: " << address << ":" << default_port << std::endl;
                LOGE << "Error: " <<  SDLNet_GetError() << std::endl;
            }
        } else {
            LOGE << "Unable to resolve host " << address << ":" << default_port << std::endl;
        }
    } else {
        LOGE << "Ran out of NetConnectionID's" << std::endl;
    }
    *id = 0;
    return false;
}

NetListenSocketID BasicNetFramework::CreateListenSocketIP(const string &local_address) {
    NetListenSocketID socket_id = GetFreeListenSocketID();

    if(socket_id != 0) {
        IPaddress addr;
        SDLNet_ResolveHost(&addr, NULL, default_port);

        TCPsocket socket;
        socket = SDLNet_TCP_Open(&addr);
        if(socket != nullptr) {
            listen_socket_map[socket_id] = socket;
            listen_socket_map_reverse[socket] = socket_id;
            return socket_id;
        } else {
            LOGE << "Unable to open a listen socket on port " << default_port << "(" <<  SDLNet_GetError()  << ")" << std::endl;
        }
    } else {
        LOGE << "Ran out of NetListenSocketID's" << std::endl;
    }

    return 0;
}

void BasicNetFramework::CloseListenSocket(NetListenSocketID listen_socket_id) {
    auto socket_it = listen_socket_map.find(listen_socket_id);

    if(socket_it != listen_socket_map.end()) {
        SDLNet_TCP_Close(socket_it->second);
        listen_socket_map_reverse.erase(socket_it->second);
        listen_socket_map.erase(socket_it->first);
    }
}

void BasicNetFramework::CloseConnection(NetConnectionID conn_id, ConnectionClosedReason reason) {
    auto socket_it = connection_id_map.find(conn_id);
    //TODO: Send the close reason before closing the socket

    if(socket_it != connection_id_map.end()) {
        SDLNet_DelSocket(socket_set, (SDLNet_GenericSocket)socket_it->second);
        SDLNet_TCP_Close(socket_it->second);
        connection_id_map_reverse.erase(socket_it->second);
        connection_status.erase(socket_it->first);
        connection_id_map.erase(socket_it->first);
    } 
}

void BasicNetFramework::GetConnectionStatus(NetConnectionID conn_id, ConnectionStatus* status) const {
	status->ms_ping = 0;
	status->connection_quality_local = 0;
	status->connection_quality_remote = 0;
	status->out_packets_per_sec = 0;
	status->out_bytes_per_sec = 0;
	status->in_packets_per_sec = 0;
	status->in_bytes_per_sec = 0;
	status->send_rate_bytes_per_second = 0;
	status->pending_unreliable = 0;
	status->pending_reliable = 0;
	status->sent_unacked_reliable = 0;
	status->usec_queue_time = 0;
}

int BasicNetFramework::ReceiveMessageOnConnection(NetConnectionID conn_id, NetworkingMessage* msg) {
    auto status = connection_status.find(conn_id);

    if(status != connection_status.end()) {
        auto first_message = status->second.messages.begin();  
        if(first_message != status->second.messages.end()) {
            if(msg != nullptr) {
                *msg = *first_message;
            }
            status->second.messages.erase(first_message);
            return 1;
        }
    } 

    return 0;
}

bool BasicNetFramework::AcceptConnection(NetConnectionID conn_id, int* result) {
    *result = 0;

    NetFrameworkConnectionStatusChanged status_changed;
    status_changed.conn_id = conn_id;
    status_changed.conn_info.connection_state = NetFrameworkConnectionState::Connected;

    online->OnConnectionChange(&status_changed); 
    return true;
}

void BasicNetFramework::SendMessageToConnection(NetConnectionID conn_id, char* buffer, uint32_t bytes, bool reliable, bool flush) {
    auto conn = connection_id_map.find(conn_id);
    uint16_t header[MESSAGE_HEADER_SIZE/2];
    header[0] = SDL_SwapBE16(bytes);
    header[1] = 0;

    if(conn != connection_id_map.end()) {
        int ret_count = SDLNet_TCP_Send(conn->second, header, MESSAGE_HEADER_SIZE);
        if(ret_count != MESSAGE_HEADER_SIZE) {
            LOGE << "ERROR ON CONNECTION" << std::endl;
            return;
        }

        ret_count = SDLNet_TCP_Send(conn->second, buffer, bytes);

        if(ret_count != bytes) {
            LOGE << "Error when sending data to connection" << std::endl;
        }
    }
}

string BasicNetFramework::GetResultString(int result) {
    return string("Unknown result code");
}
#endif
