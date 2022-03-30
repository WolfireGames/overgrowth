//-----------------------------------------------------------------------------
//           Name: online_connection_states.cpp
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
#include "online_connection_states.h"

#include <Online/online.h>
#include <Online/online_utility.h>

#include <Main/engine.h>
#include <Version/version.h>

// ---------------- StateStateSuspended ----------------

BaseState<PendingConnection>* StateSuspended::OnUpdate() {
    return this;
}

// ---------------- StateVersionCheck ----------------

void StateVersionCheck::OnEnter() {
    Online::Instance()->SendTo<OnlineMessages::PCSBuildVersionRequestMessage>(actor->net_connection_id);
}

BaseState<PendingConnection>* StateVersionCheck::OnUpdate() {
    if (actor->received_version) {
        // Does the client have the same version as us?
        if (actor->build_id == GetBuildID()) {
            return new StateHandshake();
        }

        // Is there an editor build doing the version check? Allow it, but send a warning.
        if (actor->build_id == -1 || GetBuildID() == -1) {
            Online::Instance()->SendRawChatMessage("[Warning] Client with another version connected, but was granted access since the host or client is running a Development version");
            return new StateHandshake();
        }

        // Failed version check, close connection
        ConnectionClosedReason reason = (actor->build_id < GetBuildID()) ? ConnectionClosedReason::CLIENT_OUTDATED : ConnectionClosedReason::SERVER_OUTDATED;
        Online::Instance()->CloseConnection(actor->net_connection_id, reason);
        return nullptr;
    }
    return this;
}


// ---------------- StateHandshake ----------------

void StateHandshake::OnEnter() {
    // Send session parameters
    Online::Instance()->SendTo<OnlineMessages::PCSSessionParametersMessage>(actor->net_connection_id, Online::Instance()->online_session->binding_id_map_);
}

BaseState<PendingConnection>* StateHandshake::OnUpdate() {
    if (actor->received_client_params) {
        if(!OnlineUtility::IsValidPlayerName(actor->player_name)) {
            actor->player_name = "InvalidName";
        }

        if (actor->active_mods_string != OnlineUtility::GetActiveModsString()) {
            Online::Instance()->AddLocalChatMessage("\"" + actor->player_name + "\" tried to connect, but was rejected entry due to a mod mismatch");
            Online::Instance()->AddLocalChatMessage(" - Expected mods: \"" + OnlineUtility::GetActiveModsString() + "\"");
            Online::Instance()->AddLocalChatMessage(" - Client's mods: \"" + actor->active_mods_string + "\"");

            Online::Instance()->CloseConnection(actor->net_connection_id, ConnectionClosedReason::MOD_MISMATCH);
            return new StateSuspended();
        }

        return new StateSync();
    }
    return this;
}


// ---------------- StateSync ----------------

void StateSync::OnEnter() {
    // TODO This package is a placeholder, we don't actually send any level files here right now, as this would require a bigger refactor to file transfer
    // For now, we simply tell the client what map to load and *pray* the client loads the exact same map as the host! (pretty much like it did before)
    Online::Instance()->SendTo<OnlineMessages::PCSFileTransferMetadataMessage>(actor->net_connection_id, Online::Instance()->level_name, Online::Instance()->campaign_id);

    // TODO initiate level file transfers to client
}

BaseState<PendingConnection>* StateSync::OnUpdate() {
    if (actor->finished_loading)
        return new StateWaiting();
    return this;
}


// ---------------- StateWaiting ----------------

void StateWaiting::OnEnter() {

}

BaseState<PendingConnection>* StateWaiting::OnUpdate() {
    if (Online::Instance()->ForceMapStartOnLoad())
        return new StateEstablished();
    return this;
}


// ---------------- StateEstablished ----------------

void StateEstablished::OnEnter() {
    actor->player_id = Online::Instance()->GetPeerFromConnection(actor->net_connection_id)->peer_id; // TODO We assume that peer_id == player_id here
    Online::Instance()->SendTo<OnlineMessages::PCSAssignPlayerID>(actor->net_connection_id, actor->player_id);

    // Create player state for the new player
    PlayerState player_state;
    player_state.playername = actor->player_name;
    Online::Instance()->online_session->player_states[actor->player_id] = player_state;
    Online::Instance()->Send<OnlineMessages::SetPlayerState>(actor->player_id, &player_state);

    Online::Instance()->Send<OnlineMessages::PCSLoadingCompletedMessage>();

    // Create new controllers, assign them via player_id
    Online::Instance()->AssignNewControllerForPlayer(actor->player_id);

    // "Reserve" character for peer
    ObjectID free_id = Online::Instance()->GetFreeAvatarID();
    if (free_id == -1) {
        free_id = Online::Instance()->CreateCharacter();
    }

    // Posess avatar
    Online::Instance()->PossessAvatar(actor->player_id, free_id);

    // Display Join Message
    Online::Instance()->SendRawChatMessage(actor->player_name + " just joined!");

    // Dump playerstates onto new player
    for (const auto& it : Online::Instance()->online_session->player_states) {
        Online::Instance()->SendTo<OnlineMessages::SetPlayerState>(actor->net_connection_id, it.first, &it.second);
    }

    // This seems to be an important package
    Online::Instance()->GenerateStateForNewConnection(actor->net_connection_id);
}

BaseState<PendingConnection>* StateEstablished::OnUpdate() {
    return nullptr;
}
