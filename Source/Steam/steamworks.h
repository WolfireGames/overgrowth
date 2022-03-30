//-----------------------------------------------------------------------------
//           Name: steamworks.h
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

#include <Steam/ugc.h>

#if ENABLE_STEAMWORKS
#include <Steam/steamworks_friends.h>
#include <Steam/steamworks_matchmaking.h>

class SteamworksUGCItem;

class Steamworks {
private:
    bool connected;
    bool can_use_workshop;
    bool needs_to_accept_license;

    int poll_counter;
    const static int poll_freq;

    
    friend class SteamworksUGCItem;
    SteamworksUGC* ugc;
    SteamworksFriends* friends;
    SteamworksMatchmaking* matchmaking;

    STEAM_CALLBACK(Steamworks, OnGameOverlayActivated, GameOverlayActivated_t, m_SteamGameOverlayActivated);
public:
    static Steamworks* Instance();

    Steamworks();
    
    SteamworksFriends* GetFriends();
    SteamworksUGC* GetUGC();
    SteamworksMatchmaking* GetMatchmaking();
    void Initialize();

    void Update(bool updateUGC);
    void Dispose();

    bool WaitingForResults();

    bool UserNeedsToAcceptWorkshopAgreement();
    bool UserCanAccessWorkshop();

    void OpenWebPageToMod(ModID &id);
    void OpenWebPageToModAuthor(ModID &id);
    void OpenWebPageToWorkshop();
    void OpenWebPage(const char* url);

    bool IsConnected();
};
#endif
