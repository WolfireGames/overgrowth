//-----------------------------------------------------------------------------
//           Name: online_client_connection_manager.cpp
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
#include "online_client_connection_manager.h"

#include <Online/online_datastructures.h>
#include <Online/online_connection_states.h>

#include <Online/Message/pcs_build_version_message.h>
#include <Online/Message/pcs_loading_completed_message.h>
#include <Online/Message/pcs_client_parameters_message.h>

#include <Online/online.h>

bool ClientConnectionManager::IsEveryClientLoaded() const {
    for (auto& it : connection_states) {
        if (!it.second.actor.finished_loading) {
            return false;
        }
    }
    return true;
}

bool ClientConnectionManager::HasClientGottenPersistentQueue(const PeerID peer_id) const {
    if (HasConnection(peer_id))
        return connection_states.at(peer_id).actor.finished_sending_persistent_queue;

    LOGW << "Attempted to query persistent queue state of a non-connected client. This will always return false!" << std::endl;
    return false;
}

bool ClientConnectionManager::IsClientLoaded(const PeerID peer_id) const {
    if (HasConnection(peer_id))
        return connection_states.at(peer_id).actor.finished_loading;

    LOGW << "Attempted to query loading state of a non-connected client. This will always return false!" << std::endl;
    return false;
}

bool ClientConnectionManager::HasConnection(const PeerID peer_id) const {
    return connection_states.find(peer_id) != connection_states.end();
}

void ClientConnectionManager::Update() {
    for (auto& it : connection_states) {
        it.second.Update();
    }
}

void ClientConnectionManager::RemoveConnection(const PeerID peer_id) {
    connection_states.erase(peer_id);
}

void ClientConnectionManager::AddConnection(const PeerID peer_id, const NetConnectionID net_connection_id) {
    connection_states.emplace(std::piecewise_construct, std::make_tuple(peer_id), std::make_tuple(new StateVersionCheck(), PendingConnection(net_connection_id)));
}

void ClientConnectionManager::ResetConnections() {
    connection_states.clear();
    for (auto& peer : Online::Instance()->GetPeers()) {
        connection_states.emplace(std::piecewise_construct, std::make_tuple(peer.peer_id), std::make_tuple(new StateVersionCheck(), PendingConnection(peer.conn_id)));
    }
}

void ClientConnectionManager::ApplyPackage(const PeerID peer_id, PCSBuildVersionMessage* build_version) {
    connection_states[peer_id].actor.received_version = true;
    connection_states[peer_id].actor.build_id = build_version->build_id;
}

void ClientConnectionManager::ApplyPackage(const PeerID peer_id, PCSLoadingCompletedMessage* loading_completed) {
    connection_states[peer_id].actor.finished_loading = true;
}

void ClientConnectionManager::ApplyPackage(const PeerID peer_id, PCSClientParametersMessage* client_parameters) {
    connection_states[peer_id].actor.player_name = client_parameters->player_name;
    connection_states[peer_id].actor.active_mods_string = client_parameters->enabled_mods_string;
    connection_states[peer_id].actor.received_client_params = true;
}

void ClientConnectionManager::SetClientGottenPersistentQueue(const PeerID peer_id) {
    connection_states[peer_id].actor.finished_sending_persistent_queue = true;
}
