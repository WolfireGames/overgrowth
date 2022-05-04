//-----------------------------------------------------------------------------
//           Name: online_utility.cpp
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
#include <Online/online_utility.h>
#include <Main/engine.h>
#include <Internal/config.h>

#if ENABLE_STEAMWORKS
#include <isteamfriends.h>
#endif

using std::stringstream;

// Name is invalid if (less than 3 letters) or (contains any non alpha numerical characters)
bool OnlineUtility::IsValidPlayerName(const std::string& name) {
    if (name.length() <= 2)
        return false;

    for (char c : name) {
        bool valid_char = (48 <= c && c <= 57) || (65 <= c && c <= 90) || (97 <= c && c <= 122) || c == 32;
        if (!valid_char) {
            return false;
        }
    }

    return true;
}

std::string OnlineUtility::GetDefaultPlayerName() {
#if ENABLE_STEAMWORKS
    if(SteamFriends() != nullptr) {
        const char * name_ptr = SteamFriends()->GetPersonaName();
        if(name_ptr != nullptr) {
            std::string default_name(name_ptr);
            if(IsValidPlayerName(default_name)) {
                return default_name;
            }
        }
    }
#endif

    // Steam name isn't valid, use fallback
    return "Unknown Rabbit";
}

std::string OnlineUtility::GetPlayerName() {
    return config.GetRef("playername").str();
}

std::string OnlineUtility::GetActiveModsString() {
    bool active_mods = false;
    stringstream modlist;
    vector<ModInstance*> mods = ModLoading::Instance().GetAllMods();
    for(auto & mod : mods) {
        if(mod->IsActive() && !mod->IsCore()) {
            if(active_mods) {
                modlist << ", ";
            }
            active_mods = true;

            if(mod->SupportsOnline()) {
                modlist << mod->id;
            } else {
                modlist << "[" << mod->id << "]";
            }
        }
    }

    return modlist.str();
}

bool OnlineUtility::HasActiveIncompatibleMods() {
    for (auto& it : ModLoading::Instance().GetAllMods()) {
        if (it->IsActive() && !it->IsCore() && !it->SupportsOnline()) {
            return true;
        }
    }
    return false;
}

std::string OnlineUtility::GetActiveIncompatibleModsString() {
    bool active_mods = false;
    stringstream modlist;
    vector<ModInstance*> mods = ModLoading::Instance().GetAllMods();
    for(auto & mod : mods) {
        if(mod->IsActive() && !mod->IsCore() && !mod->SupportsOnline()) {
            if(active_mods) {
                modlist << ", ";
            }
            active_mods = true;
            modlist << "\"" << mod->id << "\"";
        }
    }

    return modlist.str();
}

void OnlineUtility::HandleCommand(PlayerID playerID, const std::string& message)
{
    auto command = OnlineUtility::CommandFromString(message);
    //TODO send command to angelscript
}

bool OnlineUtility::IsCommand(const std::string& message)
{
    return message.size() > 1 && message.front() == '/';
}

std::vector<std::string> OnlineUtility::CommandFromString(const std::string& message)
{
    std::string command_str = message.substr(1);
    std::vector<std::string> command;

    size_t start = command_str.find_first_not_of(' ');
    size_t end = command_str.find_first_of(' ', start);
    while (start != std::string::npos) {
        command.push_back(command_str.substr(start, end != std::string::npos ? end - start : std::string::npos));
        start = command_str.find_first_not_of(' ', end);
        end = command_str.find_first_of(' ', start);
    }
    return command;
}
