//-----------------------------------------------------------------------------
//           Name: online_connection_states.h
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

#include <Online/state_machine.h>
#include <Online/online_datastructures.h>

struct PendingConnection {
    NetConnectionID net_connection_id;

    // Request replies
    int build_id = 0;
    bool received_version = false;

    bool received_client_params = false;
    std::string player_name;
    std::string active_mods_string;
    PlayerID player_id;

    bool finished_loading = false;
    bool finished_sending_persistent_queue = false;

    PendingConnection(NetConnectionID net_connection_id) : net_connection_id(net_connection_id) {}
    PendingConnection() {}
};

class StateSuspended : public BaseState<PendingConnection> {
    BaseState<PendingConnection>* OnUpdate() override;
};

class StateVersionCheck : public BaseState<PendingConnection> {
    void OnEnter() override;
    BaseState<PendingConnection>* OnUpdate() override;
};

class StateSync : public BaseState<PendingConnection> {
    void OnEnter() override;
    BaseState<PendingConnection>* OnUpdate() override;
};

class StateHandshake : public BaseState<PendingConnection> {
    void OnEnter() override;
    BaseState<PendingConnection>* OnUpdate() override;
};

class StateWaiting : public BaseState<PendingConnection> {
    void OnEnter() override;
    BaseState<PendingConnection>* OnUpdate() override;
};

class StateEstablished : public BaseState<PendingConnection> {
    void OnEnter() override;
    BaseState<PendingConnection>* OnUpdate() override;
};
