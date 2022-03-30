//-----------------------------------------------------------------------------
//           Name: connection_closed_reason.h
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
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

#include <string>

/// Important: Steam expects "usual" connection ends to be in 1000 - 1999
/// while "unusual" connection ends need to be in 2000 - 2999!
/// Failure to keep this range will always result in error code 1999 to be returned instead of the error code specified
/// TODO: in the future, wrap these somehow so we can still use them even if we're using a different underlying networking system.
enum class ConnectionClosedReason {
    UNSPECIFIED = 1000,
    DISCONNECTED, // Connection closed as part of natural disconnections

    BAD_REQUEST = 2000, // Client has sent a malformed package
    LOBBY_FULL, // Max capacity reached, connection denied
    LOBBY_PRIVATE, // Lobby is private, client was not invited
    CLIENT_BANNED, // Server has banned this client from joining
    HOST_STOPPED_HOSTING, // Server has stopped hosting
    MISSING_FILES, // Joining client is missing files
    EDITOR_FORBIDDEN, // Client tried to send editor instructions
    CLIENT_OUTDATED, // The client is on a lower version than the host
    SERVER_OUTDATED, // The host runs a lower version than the client
    MOD_MISMATCH, // Host and Client have different mods enabled
};

class ConnectionClosedReasonUtil {
public:
    static std::string GetErrorMessage(ConnectionClosedReason reason) {
        switch (reason) {
            case ConnectionClosedReason::UNSPECIFIED: return "No error specified!";
            case ConnectionClosedReason::BAD_REQUEST: return "Host has received a malformed request and has closed the connection with you!";
            case ConnectionClosedReason::LOBBY_FULL: return "This lobby is full!";
            case ConnectionClosedReason::LOBBY_PRIVATE: return "This lobby is private, ask for an invite!";
            case ConnectionClosedReason::CLIENT_BANNED: return "You've been banned from this lobby";
            case ConnectionClosedReason::HOST_STOPPED_HOSTING: return "The host has stopped hosting!";
            case ConnectionClosedReason::MISSING_FILES: return "You are missing files, make sure to have the right mods enabled!";
            case ConnectionClosedReason::EDITOR_FORBIDDEN: return "You are not allowed to use the editor in this lobby!";
            case ConnectionClosedReason::CLIENT_OUTDATED: return "Your game version is too old, upgrade your game! Unable to join!";
            case ConnectionClosedReason::SERVER_OUTDATED: return "The host is playing on an older version of the game! Unable to join!";
            case ConnectionClosedReason::MOD_MISMATCH: return "You are not using the same mods the host is! The host has received a full list of expected mods!";
            
            default:
                LOGE << "Unspecified error code in ConnectionClosedReason::GetErrorMessage: " + std::to_string(static_cast<int>(reason)) << std::endl;
                return "";
        }
    }

    static bool IsUnusualReason(ConnectionClosedReason reason) {
        return static_cast<int>(reason) >= static_cast<int>(ConnectionClosedReason::BAD_REQUEST);
    }
};
