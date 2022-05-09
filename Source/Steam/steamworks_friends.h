//-----------------------------------------------------------------------------
//           Name: steamworks_friends.h
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

#include <Logging/logdata.h>
#include <Internal/config.h>

#if ENABLE_STEAMWORKS
#include <steam_api.h>
#endif

#include <vector>

#if ENABLE_STEAMWORKS

class SteamworksFriends {
   private:
    friend class Steamworks;
    SteamworksFriends();

   public:
    ~SteamworksFriends();

    // activates the game overlay, with an optional dialog to open
    // valid options are "Friends", "Community", "Players", "Settings", "OfficialGameGroup", "Stats", "Achievements"
    void ActivateGameOverlay(const char *pchDialog);

    // activates game overlay to a specific place
    // valid options are
    //		"steamid" - opens the overlay web browser to the specified user or groups profile
    //		"chat" - opens a chat window to the specified user, or joins the group chat
    //		"jointrade" - opens a window to a Steam Trading session that was started with the ISteamEconomy/StartTrade Web API
    //		"stats" - opens the overlay web browser to the specified user's stats
    //		"achievements" - opens the overlay web browser to the specified user's achievements
    //		"friendadd" - opens the overlay in minimal mode prompting the user to add the target user as a friend
    //		"friendremove" - opens the overlay in minimal mode prompting the user to remove the target friend
    //		"friendrequestaccept" - opens the overlay in minimal mode prompting the user to accept an incoming friend invite
    //		"friendrequestignore" - opens the overlay in minimal mode prompting the user to ignore an incoming friend invite
    void ActivateGameOverlayToUser(const char *pchDialog, CSteamID steamID);

    // activates game overlay web browser directly to the specified URL
    // full address with protocol type is required, e.g. http://www.steamgames.com/
    void ActivateGameOverlayToWebPage(const char *pchURL);

    std::vector<CSteamID> GetClans();

    std::string GetPersonaName();
};

#endif
