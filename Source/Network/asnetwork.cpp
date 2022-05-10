//-----------------------------------------------------------------------------
//           Name: asnetwork.cpp
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
#include "asnetwork.h"

#include <Logging/logdata.h>

#include <SDL_net.h>

#include <algorithm>

const int ASNetwork::MAX_SOCKETS = 16;

void ASNetwork::Initialize() {
    socketset = SDLNet_AllocSocketSet(MAX_SOCKETS);
    socket_id_counter = 1;
}

void ASNetwork::Dispose() {
    SDLNet_FreeSocketSet(socketset);
}

void ASNetwork::Update() {
    if (sockets.size() > 0) {
        int check_counter = SocketData::BUF_SIZE;
        while (check_counter > 0) {
            int nr_sockets = SDLNet_CheckSockets(socketset, 0);
            if (nr_sockets > 0) {
                std::map<SocketID, SocketData>::iterator socketit = sockets.begin();
                for (; socketit != sockets.end(); socketit++) {
                    SocketData& sd = socketit->second;
                    if (SDLNet_SocketReady(sd.socket) && sd.used < SocketData::BUF_SIZE) {
                        int recv_size = SDLNet_TCP_Recv(sd.socket, sd.buf + sd.used, 1);
                        if (recv_size == 1) {
                            sd.used++;
                        } else if (recv_size <= 0) {
                            sd.valid = false;
                            LOGE << "Connection Recv error on " << socketit->first << " : " << SDLNet_GetError() << std::endl;
                        }
                    }
                }
                check_counter--;
            } else if (nr_sockets == -1) {
                LOGE << "Error using select() on socket" << std::endl;
                check_counter = 0;
            } else {
                check_counter = 0;
            }
        }

        // Offset the calls to allow calls to DestroyTCPSocket.
        int id_count = 0;
        uint8_t* data[MAX_SOCKETS];
        size_t size[MAX_SOCKETS];
        SocketID id[MAX_SOCKETS];

        std::map<SocketID, SocketData>::iterator socketit = sockets.begin();
        for (; socketit != sockets.end(); socketit++) {
            SocketData& sd = socketit->second;
            if (sd.used > 0) {
                data[id_count] = sd.buf;
                size[id_count] = sd.used;
                id[id_count] = socketit->first;
                id_count++;
            } else if (sd.valid == false) {  // Note that we use NULL on data as a destroy marker.
                data[id_count] = NULL;
                size[id_count] = 0;
                id[id_count] = socketit->first;
                id_count++;
            }
            sd.used = 0;
        }

        for (int i = 0; i < id_count; i++) {
            if (data[i] == NULL) {
                DestroySocketTCP(id[i]);
            } else {
                std::vector<ASNetworkCallback*>::iterator cbit = callbacks.begin();
                for (; cbit != callbacks.end(); cbit++) {
                    (*cbit)->IncomingTCPData(id[i], data[i], size[i]);
                }
            }
        }
    }
}

SocketID ASNetwork::CreateSocketTCP(std::string host, uint16_t port) {
    if (sockets.size() < (unsigned)MAX_SOCKETS) {
        IPaddress adr;
        int ret = SDLNet_ResolveHost(&adr, host.c_str(), port);
        if (ret == 0) {
            TCPsocket tcp_socket;
            tcp_socket = SDLNet_TCP_Open(&adr);
            if (tcp_socket) {
                SDLNet_AddSocket(socketset, (SDLNet_GenericSocket)tcp_socket);

                SocketData sd;
                sd.used = 0;
                sd.socket = tcp_socket;
                sd.valid = true;

                SocketID socket_id = socket_id_counter++;

                sockets[socket_id] = sd;

                return socket_id;
            } else {
                LOGE << "Unable to open socket to resolved address: " << host << ":" << port << std::endl;
                LOGE << "Error: " << SDLNet_GetError() << std::endl;
            }
        } else {
            LOGE << "Unable to resolve host " << host << ":" << port << std::endl;
        }
    } else {
        LOGE << "Unable to create another socket connection, reached max of: " << MAX_SOCKETS << std::endl;
    }

    return SOCKET_ID_INVALID;
}

void ASNetwork::DestroySocketTCP(SocketID sock) {
    std::map<SocketID, SocketData>::iterator datait = sockets.find(sock);
    if (datait != sockets.end()) {
        SDLNet_DelSocket(socketset, (SDLNet_GenericSocket)datait->second.socket);
        SDLNet_TCP_Close(datait->second.socket);
        sockets.erase(datait);
    }
}

int ASNetwork::SocketTCPSend(SocketID socket, const uint8_t* buf, size_t len) {
    std::map<SocketID, SocketData>::iterator datait = sockets.find(socket);
    if (datait != sockets.end()) {
        SocketData& sd = datait->second;
        int ret_data_size = SDLNet_TCP_Send(sd.socket, buf, len);
        if (ret_data_size < (int)len) {
            sd.valid = false;
            DestroySocketTCP(socket);
            LOGE << "Error when trying to send data on socket, error: \"" << SDLNet_GetError() << "\", shutting it down." << std::endl;
        }
        return ret_data_size;
    } else {
        LOGE << "unable to send data, no such socket " << socket << std::endl;
        return -1;
    }
}

bool ASNetwork::IsValidSocketTCP(SocketID socket) {
    std::map<SocketID, SocketData>::iterator datait = sockets.find(socket);
    if (datait != sockets.end()) {
        return true;
    } else {
        return false;
    }
}

void ASNetwork::RegisterASNetworkCallback(ASNetworkCallback* cb) {
    callbacks.push_back(cb);
}

void ASNetwork::DeRegisterASNetworkCallback(ASNetworkCallback* cb) {
    std::vector<ASNetworkCallback*>::iterator callit = std::find(callbacks.begin(), callbacks.end(), cb);
    if (callit != callbacks.end()) {
        callbacks.erase(callit);
    }
}
