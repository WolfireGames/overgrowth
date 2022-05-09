//-----------------------------------------------------------------------------
//           Name: online_peer.h
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

#include <Online/online_datastructures.h>
#include <Game/connection_closed_reason.h>
#include <Network/net_framework.h>

#include <string>

// work around due to how the locks hinders us to  call close connection
// while inside the state_manager.update
struct ConnectionToBeClosed {
    NetConnectionID conn_id;
    ConnectionClosedReason reason;
};

struct Peer {
    PeerID peer_id = 0;
    NetConnectionID conn_id;
    NetConnectionInfo m_info;
    OnlineMessageID last_sent_package_id = 0;

    map<OnlineFlags, bool> host_session_flags;

    // Host side flag to mark that we need to do an initial sync with this peer.
    bool should_send_state_messages = false;
    float current_ping_delta = 0.0f;
};
