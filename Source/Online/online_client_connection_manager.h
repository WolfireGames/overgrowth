//-----------------------------------------------------------------------------
//           Name: online_client_connection_manager.h
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
#include <Online/state_machine.h>
#include <Online/online_connection_states.h>

#include <unordered_map>

namespace OnlineMessages {
    class PCSBuildVersionMessage;
    class PCSLoadingCompletedMessage;
    class PCSClientParametersMessage;
}

class ClientConnectionManager {
private:
    std::unordered_map<PeerID, StateMachine<PendingConnection>> connection_states;

    bool HasConnection(const PeerID peer_id) const;

public:
    bool IsEveryClientLoaded() const;
    bool IsClientLoaded(const PeerID peer_id) const;
    bool HasClientGottenPersistentQueue(const PeerID peer_id) const;

    void Update();
    void RemoveConnection(const PeerID peer_id);
    void AddConnection(const PeerID peer_id, const NetConnectionID net_connection_id);
    void ResetConnections();

    void ApplyPackage(const PeerID peer_id, OnlineMessages::PCSBuildVersionMessage* build_version);
    void ApplyPackage(const PeerID peer_id, OnlineMessages::PCSLoadingCompletedMessage* loading_completed);
    void ApplyPackage(const PeerID peer_id, OnlineMessages::PCSClientParametersMessage* client_parameters);
    void SetClientGottenPersistentQueue(const PeerID peer_id);
};
