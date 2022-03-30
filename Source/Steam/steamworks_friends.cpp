//-----------------------------------------------------------------------------
//           Name: steamworks_friends.cpp
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
#include "steamworks_friends.h"

#if ENABLE_STEAMWORKS
SteamworksFriends::SteamworksFriends() {

}

SteamworksFriends::~SteamworksFriends() {

}

void SteamworksFriends::ActivateGameOverlay( const char *pchDialog ) {
    SteamFriends()->ActivateGameOverlay(pchDialog);
}

void SteamworksFriends::ActivateGameOverlayToUser( const char *pchDialog, CSteamID steamID ) { 
    SteamFriends()->ActivateGameOverlayToUser(pchDialog, steamID);
}

void SteamworksFriends::ActivateGameOverlayToWebPage( const char *pchURL ) {
    SteamFriends()->ActivateGameOverlayToWebPage(pchURL);
}

std::vector<CSteamID> SteamworksFriends::GetClans() {
    std::vector<CSteamID> ret;
    for( int i = 0; i < SteamFriends()->GetClanCount(); i++ ) {
        CSteamID clanid = SteamFriends()->GetClanByIndex(i);
        ret.push_back(clanid);
    }
    return ret;
}


std::string SteamworksFriends::GetPersonaName() {
    return SteamFriends()->GetPersonaName();
}
#endif
