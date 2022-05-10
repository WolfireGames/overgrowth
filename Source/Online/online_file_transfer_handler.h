//-----------------------------------------------------------------------------
//           Name: online_file_transfer_handler.h
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
#include <Game/levelinfo.h>

#if ENABLE_STEAMWORKS || ENABLE_GAMENETWORKINGSOCKETS
#include <steamnetworkingtypes.h>
#endif

#include <vector>
#include <unordered_map>
#include <mutex>
#include <fstream>

struct FileTransfer {
    enum Type {
        SCRIPT = 1 << 0,
        LEVEL = 1 << 1,
        TEXTURE = 1 << 2,
        CAMPAIGN = 1 << 3

    };
    Type type;
    uint32_t name_len;
    uint32_t file_len;
    int8_t file_id;
    int8_t number_of_messages;
    char* file_name;
    unsigned char* file_data;
};

/// TODO This class has a big assumption that we only send one file at a time!
/// Attempting to send another file while the previous file is not fully send will cause all kinds of corruption issues
class OnlineFileTransferHandler {
   private:
    std::fstream file_handle;  // TODO this shouldn't be a member variable

    std::string GetTempFileTransferPath(const FileTransfer file_transfer) const;
    void CreateDirectoriesForFile(FileTransfer file, const std::string& file_location);
    void FinishedStreamingFileToDisk(const std::string& file_location);
    void StreamFilePackageToDisk(FileTransfer file);
    std::vector<FileTransfer*> SplitFile(std::string path, std::vector<unsigned char> buffer, FileTransfer::Type filetype);
    // void SendFile(HSteamNetConnection peer, const char * path, FileTransfer::Type filetype);

   public:
    // void SendLevelFiles(const HSteamNetConnection peer, const LevelInfo& level_info, const std::string& level_name);
    // void SendCampaignFiles(HSteamNetConnection peer, const std::string& campaign_id);
    void SaveFile(FileTransfer file);
};
