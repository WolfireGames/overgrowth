//-----------------------------------------------------------------------------
//           Name: steamworks.cpp
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
#include "steamworks.h"

#if ENABLE_STEAMWORKS
#include "steam_api.h"

#include <Logging/logdata.h>
#include <Internal/integer.h>
#include <Memory/allocation.h>
#include <Internal/modloading.h>
#include <Steam/ugc_item.h>
#include <Utility/assert.h>
#include <Internal/snprintf.h>
#include <Internal/profiler.h>
#include <Main/engine.h>

#include <sstream>

const int Steamworks::poll_freq = 60 * 10;

Steamworks::Steamworks() : m_SteamGameOverlayActivated(this, &Steamworks::OnGameOverlayActivated),
                           connected(false),
                           ugc(NULL),
                           friends(NULL),
                           matchmaking(NULL),
                           poll_counter(0),
                           can_use_workshop(false),
                           needs_to_accept_license(false) {
    // Should be empty
}

Steamworks* Steamworks::Instance() {
    static Steamworks instance;
    return &instance;
}

SteamworksUGC* Steamworks::GetUGC() {
    return ugc;
}

SteamworksFriends* Steamworks::GetFriends() {
    return friends;
}

SteamworksMatchmaking* Steamworks::GetMatchmaking() {
    return matchmaking;
}

void Steamworks::Initialize() {
    bool steam_init_success = SteamAPI_Init();
    connected = false;
    if (steam_init_success) {
        LOGI << "Successfully connected to Steam API." << std::endl;
        if (SteamUGC()) {
            connected = true;
            ugc = new SteamworksUGC();
            friends = new SteamworksFriends();
            matchmaking = new SteamworksMatchmaking();
        } else {
            LOGE << "SteamUGC() is null, not initializing steamworks, this is an error." << std::endl;
        }
    } else {
        LOGI << "Could not connect to Steam API." << std::endl;
    }
}

static const CSteamID overgrowth_tester_group(103582791457175036ULL);
void Steamworks::Update(bool updateUGC) {
    {
        PROFILER_ZONE(g_profiler_ctx, "RunCallbacks");
        SteamAPI_RunCallbacks();
    }
    if (updateUGC) {
        if (GetUGC()) {
            PROFILER_ZONE(g_profiler_ctx, "UGC->Update");
            GetUGC()->Update();
        }

        if (friends) {
            PROFILER_ZONE(g_profiler_ctx, "Friends");
            if (poll_counter <= 0) {
                if (config["check_for_workshop_membership"].toBool()) {
                    std::vector<CSteamID> ids = friends->GetClans();
                    can_use_workshop = false;
                    for (unsigned i = 0; i < ids.size(); i++) {
                        if (ids[i] == overgrowth_tester_group) {
                            can_use_workshop = true;
                        }
                    }
                } else {
                    can_use_workshop = true;
                }
                poll_counter = poll_freq;
            }
            poll_counter--;
        } else {
            can_use_workshop = false;
        }
    }
}

void Steamworks::Dispose() {
    while (WaitingForResults()) {
        Update(true);
    }

    if (matchmaking) {
        delete matchmaking;
        matchmaking = NULL;
    }

    SteamAPI_Shutdown();
    delete ugc;
    delete friends;
    ugc = NULL;
    friends = NULL;
    connected = false;
}

// This function indicates of any of the steamworks systems are waiting
// for data to be returned from the underlying api. If this is the case,
// disposing of the systems aren't safe;
bool Steamworks::WaitingForResults() {
    if (ugc) {
        return ugc->WaitingForResults();
    } else {
        return false;
    }
}

bool Steamworks::UserNeedsToAcceptWorkshopAgreement() {
    return needs_to_accept_license;
}

bool Steamworks::UserCanAccessWorkshop() {
    if (IsConnected()) {
        if (can_use_workshop) {
            return true;
        }
    }
    return false;
}

void Steamworks::OpenWebPageToMod(ModID& id) {
    ModInstance* mod = ModLoading::Instance().GetMod(id);
    if (mod) {
        SteamworksUGCItem* ugci = mod->GetUGCItem();
        if (ugci) {
            if (friends) {
                LOGI << "Opening workshop mod overlay page for " << id << std::endl;

                std::stringstream ss;
                ss << "steamcommunity.com/sharedfiles/filedetails/?id=" << ugci->steamworks_id << std::endl;
                std::string url = ss.str();
                friends->ActivateGameOverlayToWebPage(url.c_str());
            } else {
                LOGE << "Couldn't open mod page overlay, mod " << id << " no friends steamworks instance" << std::endl;
            }
        } else {
            LOGE << "Couldn't open mod page overlay, mod " << id << " doesn't have a ugc" << std::endl;
        }
    } else {
        LOGE << "Couldn't open mod page overlay, mod " << id << " no such mod" << std::endl;
    }
}

void Steamworks::OpenWebPageToModAuthor(ModID& id) {
    ModInstance* mod = ModLoading::Instance().GetMod(id);

    if (mod) {
        SteamworksUGCItem* ugci = mod->GetUGCItem();
        if (ugci) {
            if (friends) {
                LOGI << "Opening workshop mod author overlay page for " << id << std::endl;
                friends->ActivateGameOverlayToUser("steamid", ugci->owner_id);
            } else {
                LOGE << "Couldn't open mod author page overlay, mod " << id << " no friends steamworks instance" << std::endl;
            }
        } else {
            LOGE << "Couldn't open mod author page overlay, mod " << id << " doesn't have a ugc" << std::endl;
        }
    } else {
        LOGE << "Couldn't open mod author page overlay, mod " << id << " no such mod" << std::endl;
    }
}

void Steamworks::OpenWebPageToWorkshop() {
    const char* url = "steamcommunity.com/app/" OVERGROWTH_APP_ID_STR "/workshop/";
    if (friends) {
        LOGI << "Opening workshop overlay" << std::endl;
        friends->ActivateGameOverlayToWebPage(url);
    } else {
        LOGE << "Couldn't open workshop overlay page" << std::endl;
    }
}

void Steamworks::OpenWebPage(const char* url) {
    if (friends) {
        LOGI << "Opening workshop overlay" << std::endl;
        friends->ActivateGameOverlayToWebPage(url);
    } else {
        LOGE << "Couldn't open workshop overlay url:" << url << std::endl;
    }
}

bool Steamworks::IsConnected() {
    return connected;
}

void Steamworks::OnGameOverlayActivated(GameOverlayActivated_t* callback) {
    if (callback->m_bActive) {
        LOGI << "Overlay activated" << std::endl;
        UIShowCursor(1);
    } else {
        LOGI << "Overlay deactivated" << std::endl;
        UIShowCursor(0);
    }
}
#endif
