//-----------------------------------------------------------------------------
//           Name: steamworks_matchmaking.cpp
//      Developer: Wolfire Games LLC
//    Description: Steamworks wrapper for Overgrowth, to simplify use and
//                 minimize state.
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
#include "steamworks_matchmaking.h"

#include <Online/online.h>
#include <Internal/config.h>
#include <Logging/logdata.h>
#include <Steam/steamworks_util.h>

#if ENABLE_STEAMWORKS
#include <isteamnetworkingutils.h>
#endif

#include <algorithm>

static const char *LEVEL_PATH = "levelpath";

#if ENABLE_STEAMWORKS
const char *SteamworksMatchmaking::StatusToString(ESteamNetworkingConnectionState state) {
    switch (state) {
        case k_ESteamNetworkingConnectionState_None:
            return "None";
            break;

        case k_ESteamNetworkingConnectionState_Connecting:
            return "Connecting";
            break;

        case k_ESteamNetworkingConnectionState_FindingRoute:
            return "FindingRoute";
            break;

        case k_ESteamNetworkingConnectionState_Connected:
            return "Connected";
            break;

        case k_ESteamNetworkingConnectionState_ClosedByPeer:
            return "ClosedByPeer";
            break;

        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
            return "ProblemDetectedLocally";
            break;

        default:
            return "impossible internal state";
            break;
    }

    return "unknown";
}

SteamworksMatchmaking::SteamworksMatchmaking() {
    // initializing relay access on startup helps to start multiplayer faster
    // but if the game is closed before it has finished initializing
    // it will crash inside steam libraries
    // so we don't do it
    // FIXME: enable if steam fixes their lib
#if 0
    LOGI << "Initializing steam relay network access ..." << std::endl;
    SteamNetworkingUtils()->InitRelayNetworkAccess();
#endif  // 0
}

SteamworksMatchmaking::~SteamworksMatchmaking() {
    // if we free these it can cause crashes on exit on linux
    // apparently steam api doesn't properly wait for its internal threads
    // to shut down

    if (!lobbies.empty()) {
        LOGW << lobbies.size() << " lobbies remain at shutdown" << std::endl;
#if 0
        for (unsigned int i = 0; i < lobbies.size(); i++) {
            SteamMatchmaking()->LeaveLobby(lobbies[i]);
        }
#endif  // 0

        lobbies.clear();
    }
}

HSteamListenSocket SteamworksMatchmaking::ActivateGameOverlayInviteDialog() {
    LOGI << "SteamworksMatchmaking::ActivateGameOverlayInviteDialog()" << std::endl;

    SteamMatchmaking()->CreateLobby(k_ELobbyTypePrivate, 10);

    ISteamNetworkingSockets *isns = SteamNetworkingSockets();
    assert(isns);

    HSteamListenSocket listen = isns->CreateListenSocketP2P(0, 0, nullptr);

    return listen;
}

void SteamworksMatchmaking::OpenInviteDialog() {
    if (lobbies.begin() != lobbies.end()) {
        if (SteamMatchmaking != nullptr) {
            ISteamFriends *friends = SteamFriends();
            if (friends) {
                friends->ActivateGameOverlayInviteDialog(*lobbies.begin());
            }
        }
    }
}

void SteamworksMatchmaking::JoinLobby(const std::string &lobbyIDStr) {
    LOGI << "SteamworksMatchmaking::JoinLobby " << lobbyIDStr << std::endl;

    char *end = NULL;
    // We presume uint64 to be the same as the unsigned long returned by strtoul
    uint64 l = strtoul(lobbyIDStr.c_str(), &end, 10);

    CSteamID lobbyID(l);

    LOGI << "parsed lobby id: \"" << lobbyID.ConvertToUint64() << "\"" << std::endl;

    SteamMatchmaking()->JoinLobby(lobbyID);
}

void SteamworksMatchmaking::LeaveLobby(CSteamID lobbyID) {
    SteamMatchmaking()->LeaveLobby(lobbyID);
    auto it = std::find(lobbies.begin(), lobbies.end(), lobbyID);
    if (it != lobbies.end()) {
        lobbies.erase(it);
    }
}

void SteamworksMatchmaking::OnLobbyEnter(LobbyEnter_t *data) {
    Online *online = Online::Instance();

    assert(data);

    LOGI << "OnLobbyEnter " << data->m_ulSteamIDLobby << std::endl;

    lobbies.insert(data->m_ulSteamIDLobby);

    if (online->IsHosting()) {
        // we are the host
        // set lobby metadata
        SteamMatchmaking()->SetLobbyData(data->m_ulSteamIDLobby, LEVEL_PATH, "");

        // activate overlay
        SteamFriends()->ActivateGameOverlayInviteDialog(data->m_ulSteamIDLobby);
    } else {
        // we are client
        // get lobby metadata

        // Start multiplayer with host
        CSteamID lobbyHost = SteamMatchmaking()->GetLobbyOwner(data->m_ulSteamIDLobby);
        ISteamNetworkingSockets *isns = SteamNetworkingSockets();
        assert(isns);

        SteamNetworkingIdentity id;
        id.SetSteamID(lobbyHost);

        HSteamNetConnection connection = isns->ConnectP2P(id, 0, 0, nullptr);
        std::vector<char> temp(1024, 0);
        int ret = isns->GetDetailedConnectionStatus(connection, &temp[0], temp.size());
        if (ret >= 0) {
            LOGI << "connection status: " << &temp[0] << std::endl;
        } else {
            LOGE << "GetDetailedConnectionStatus failed" << std::endl;
        }
        steamPendingConnections.insert(connection);

        // Leave lobby - maybe not?
        LeaveLobby(data->m_ulSteamIDLobby);

        if (!online->IsActive())
            online->StartMultiplayer(MultiplayerMode::Client);
    }
}

void SteamworksMatchmaking::OnLobbyDataUpdate(LobbyDataUpdate_t *data) {
    assert(data);

    LOGI << "OnLobbyDataUpdate " << data->m_ulSteamIDLobby << std::endl;

    if (data->m_bSuccess) {
        LOGI << "Transition was a success" << std::endl;
    }
}

void SteamworksMatchmaking::OnLobbyJoinRequested(GameLobbyJoinRequested_t *data) {
    assert(data);

    LOGI << "OnLobbyJoinRequested" << std::endl;

    SteamMatchmaking()->JoinLobby(data->m_steamIDLobby);
}

void SteamworksMatchmaking::OnLobbyChatUpdate(LobbyChatUpdate_t *data) {
    Online *online = Online::Instance();
    assert(data);

    LOGI << "OnLobbyChatUpdate " << data->m_ulSteamIDLobby << std::endl;
    if (data->m_rgfChatMemberStateChange & k_EChatMemberStateChangeEntered) {
        LOGI << "User " << data->m_ulSteamIDUserChanged << " joined lobby" << std::endl;
        if (online->IsHosting()) {
            // Friend accepted invite
            // Leave lobby
            // For single player mode host should maybe stay in lobby and only clients leave as they enter game
            // LeaveLobby(data->m_ulSteamIDLobby);
            // Steam doesn't offer way to close overlay from app, user needs to do that
        }
    } else {
        LOGI << "User " << data->m_ulSteamIDUserChanged << " left lobby" << std::endl;
    }
}

void SteamworksMatchmaking::OnConnectionChange(SteamNetConnectionStatusChangedCallback_t *data) {
    ISteamNetworkingSockets *isns = SteamNetworkingSockets();

    assert(data);
    LOGI << "SteamworksMatchmaking::OnConnectionChange " << data->m_hConn << " old:" << StatusToString(data->m_eOldState) << "  new: " << StatusToString(data->m_info.m_eState) << std::endl;

    /*/
    switch (data->m_info.m_eState) {
            case k_ESteamNetworkingConnectionState_Connecting: {
                    //EResult result = isns->AcceptConnection(data->m_hConn);
                    break;
            }

            default: {

            }
    }
    */

    auto it = steamPendingConnections.find(data->m_hConn);
    if (it != steamPendingConnections.end()) {
        // This is a Steam connection
        LOGI << "Steam connection" << std::endl;
        steamPendingConnections.erase(it);
    }
}

std::string SteamworksMatchmaking::GetSteamNickname(const CSteamID &friendID) {
    std::string name = "<unknown>";

    ISteamFriends *friends = SteamFriends();
    assert(friends);

    if (friendID.IsValid()) {
        // query for friend nickname, might not exist
        const char *nickname = friends->GetPlayerNickname(friendID);
        if (nickname) {
            name = nickname;
        } else {
            // TODO: might not exist immediately when joining lobby
            //  need to wait for PersonaStateChange_t
            // when not exists guaranteed to be non-NULL but possibly empty
            name = friends->GetFriendPersonaName(friendID);
        }
    }

    return name;
}
#endif
