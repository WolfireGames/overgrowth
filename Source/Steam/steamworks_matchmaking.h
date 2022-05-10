//-----------------------------------------------------------------------------
//           Name: steamworks_matchmaking.h
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
#pragma once

// steam_api.h requires cstddef for offsetof but doesn't include it itself
#include <cstddef>

#if ENABLE_STEAMWORKS || ENABLE_GAMENETWORKINGSOCKETS
#include <isteamnetworkingsockets.h>
#endif

#if ENABLE_STEAMWORKS
#include <steam_api.h>
#endif

#include <string>
#include <unordered_set>

#if ENABLE_STEAMWORKS

namespace std {

template <>
struct hash<CSteamID> {
    size_t operator()(const CSteamID &id) const {
        return hash<uint64_t>()(id.ConvertToUint64());
    }
};

}  // namespace std

class SteamworksMatchmaking {
   private:
    friend class Steamworks;
    SteamworksMatchmaking();

    std::unordered_set<CSteamID> lobbies;  // we are we assuming multiple??
    std::unordered_set<HSteamNetConnection> steamPendingConnections;

   public:
    ~SteamworksMatchmaking();

    // activate game overlay to the invite dialog
    // invitations sent from this dialog will be for the provided lobby

    // This function has a ton of side effects and requires refactor
    // it basically inits an entire mp state.
    HSteamListenSocket ActivateGameOverlayInviteDialog();

    //
    void OpenInviteDialog();

    // for command-line jobby loining
    void JoinLobby(const std::string &lobbyIDStr);

    // leave lobby
    void LeaveLobby(CSteamID lobbyID);

    // get Steam friend name
    std::string GetSteamNickname(const CSteamID &friendID);

    static const char *StatusToString(ESteamNetworkingConnectionState state);

    STEAM_CALLBACK(SteamworksMatchmaking, OnLobbyEnter, LobbyEnter_t);
    STEAM_CALLBACK(SteamworksMatchmaking, OnLobbyDataUpdate, LobbyDataUpdate_t);
    STEAM_CALLBACK(SteamworksMatchmaking, OnLobbyJoinRequested, GameLobbyJoinRequested_t);
    STEAM_CALLBACK(SteamworksMatchmaking, OnLobbyChatUpdate, LobbyChatUpdate_t);
    STEAM_CALLBACK(SteamworksMatchmaking, OnConnectionChange, SteamNetConnectionStatusChangedCallback_t);
};
#endif
