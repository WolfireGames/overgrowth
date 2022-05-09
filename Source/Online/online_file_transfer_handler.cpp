//-----------------------------------------------------------------------------
//           Name: online_file_transfer_handler.cpp
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
#include "online_file_transfer_handler.h"

#include <Internal/modloading.h>
#include <Main/engine.h>

#include <vector>
#include <unordered_map>
#include <mutex>

/*
void OnlineFileTransferHandler::SendLevelFiles(const HSteamNetConnection peer, const LevelInfo& level_info, const std::string& level_name) {
    SendFile(peer, level_info.terrain_info_.heightmap.c_str(), FileTransfer::TEXTURE);
    SendFile(peer, std::string("Data/Scripts/" + level_info.script_).c_str(), FileTransfer::SCRIPT);
    SendFile(peer, (level_name).c_str(), FileTransfer::LEVEL);
    SendFile(peer, level_info.terrain_info_.colormap.c_str(), FileTransfer::TEXTURE);
    SendFile(peer, level_info.terrain_info_.weightmap.c_str(), FileTransfer::TEXTURE);
    SendFile(peer, std::string("Data/Scripts/" + level_info.pc_script_).c_str(), FileTransfer::SCRIPT);
    SendFile(peer, std::string("Data/Scripts/" + level_info.npc_script_).c_str(), FileTransfer::SCRIPT);
    SendFile(peer, std::string(level_info.loading_screen_.image).c_str(), FileTransfer::TEXTURE);
    SendFile(peer, std::string(level_info.sky_info_.dome_texture_path).c_str(), FileTransfer::TEXTURE);
}

void OnlineFileTransferHandler::SendCampaignFiles(HSteamNetConnection peer, const std::string& campaign_id) {
    ModInstance*  temp = ModLoading::Instance().GetModInstance(campaign_id);
    if (temp != nullptr) {

        std::string modpath = "Data/Mods/" + temp->path.substr(temp->path.find("//")).erase(0, 2);
        SendFile(peer, (modpath + "/" + temp->thumbnail.c_str()).c_str(), FileTransfer::Type::TEXTURE); // Send xml

        for (auto image : temp->preview_images) {
            SendFile(peer, image.c_str(), FileTransfer::Type::TEXTURE); // Send xml
        }

        for (auto item : temp->main_menu_items) {
            SendFile(peer, item.thumbnail.c_str(), FileTransfer::Type::TEXTURE); // Send xml
        }

        for (auto campaign : temp->campaigns) {
            SendFile(peer, (modpath + "/Data/" + campaign.thumbnail.c_str()).c_str(), FileTransfer::Type::TEXTURE); // Send xml
        }

        SendFile(peer, (modpath + "/mod.xml").c_str(), FileTransfer::Type::CAMPAIGN); // Send xml+
    } else {
        LOGI << "I can't resolve the currently loaded campagin" << std::endl;
    }
}

void OnlineFileTransferHandler::SendFile(HSteamNetConnection peer, const char * path, FileTransfer::Type filetype) {
    if (FileExists(path, kAnyPath)) {
        Path level_path = FindFilePath(path, kAnyPath);

        std::vector<unsigned char> dat = readFile(level_path.GetFullPath());
        std::vector<FileTransfer*> payLoads = SplitFile(path, dat, filetype);

        for (auto& it : payLoads) {
            //Online::Instance()->QueueFilesForTranfser(it, peer);
        }

        LOGI << "Sending file: " << path << " size: " << dat.size() * sizeof(unsigned char) << " we are sending the file in: " << payLoads.size() << " messages." << std::endl;
    } else {
        LOGI << "Sending file: " << path << " failed. Could not resolve path " << std::endl;
    }
}

void OnlineFileTransferHandler::SaveFile(FileTransfer file) {
    std::string file_location = GetTempFileTransferPath(file);

    if (file.number_of_messages > 1) {

        // First file is a corner case when you have multiple packages
        if (file.file_id == 0) {
            CreateDirectoriesForFile(file, file_location);

            file_handle.open(file_location, std::fstream::out | std::ios::binary);
        }

        StreamFilePackageToDisk(file);

        // Close if it's the last file number
        if (file.file_id == (file.number_of_messages - 1)) {
            file_handle.close();

            FinishedStreamingFileToDisk(file_location);
        }
    } else {
        CreateDirectoriesForFile(file, file_location);
        file_handle.open(file_location, std::fstream::out | std::ios::binary);
        StreamFilePackageToDisk(file);
        file_handle.close();
        FinishedStreamingFileToDisk(file_location);
    }

    delete file.file_data;
    delete file.file_name;
}

void OnlineFileTransferHandler::StreamFilePackageToDisk(FileTransfer file) {
    if (file_handle.is_open()) {
        file_handle.write(reinterpret_cast<char*>(file.file_data), file.file_len);
    } else {
        LOGI << "Failed to create file" << std::endl;
    }
}

void OnlineFileTransferHandler::FinishedStreamingFileToDisk(const std::string& file_location) {
    std::string real_name = file_location;
    size_t pos = file_location.find(std::string(".tmp"));

    if (pos != std::string::npos) {
        real_name.erase(pos, std::string(".tmp").size());
        rename(file_location.c_str(), real_name.c_str());
    }
}

std::string OnlineFileTransferHandler::GetTempFileTransferPath(const FileTransfer file_transfer) const {
    std::string file_location = std::string(write_path) + std::string("/multiplayercache/") + std::string(file_transfer.file_name) + std::string(".tmp");
    return SanitizePath(file_location);
}

void OnlineFileTransferHandler::CreateDirectoriesForFile(FileTransfer file, const std::string& file_location) {
    CreateDirsFromPath(std::string(write_path) + std::string("/multiplayercache/"), file.file_name);
}

std::vector<FileTransfer*> OnlineFileTransferHandler::SplitFile(std::string path, std::vector<unsigned char> buffer, FileTransfer::Type filetype) {
    // Even though steamnetworkingsockets splits up messages into package sizes they still impose
    // a limit on how big a message can be.

    int totalFileSizeInBytes = buffer.size() * sizeof(unsigned char);
    int totalTransferSizeInBytes = totalFileSizeInBytes + path.size() * sizeof(char);
    const uint32_t overhead = sizeof(FileTransfer);
    const uint32_t maxBytesPerMessage = k_cbMaxSteamNetworkingSocketsMessageSizeSend - overhead;
    uint32_t number_of_messages = totalTransferSizeInBytes / maxBytesPerMessage;
    const uint32_t left_over = totalTransferSizeInBytes % maxBytesPerMessage ? 1 : 0;

    number_of_messages += left_over;

    uint32_t totalAllocatedBytes = 0;
    uint32_t allocatedBytesForMessage = 0;
    FileTransfer * file = new FileTransfer();
    allocatedBytesForMessage += path.size() * sizeof(char) + 1;

    file->type = filetype;
    file->file_id = 0;
    file->name_len = allocatedBytesForMessage;
    file->number_of_messages = number_of_messages;

    file->file_name = new char[file->name_len];
    memcpy(file->file_name, path.c_str(), file->name_len);
    file->file_name[file->name_len] = '\0';

    // If it fits in one message, don't overallocate or go over the bounds.
    if (file->number_of_messages > 1) {
        file->file_len = maxBytesPerMessage - allocatedBytesForMessage;
    } else {
        file->file_len = totalFileSizeInBytes;
    }

    file->file_data = new unsigned char[file->file_len];
    memcpy(file->file_data, &buffer.data()[totalAllocatedBytes], file->file_len);

    totalAllocatedBytes += file->file_len;

    std::vector<FileTransfer*> payLoads;
    payLoads.push_back(file);
    for (uint32_t i = 1; i < number_of_messages; i++) {
        allocatedBytesForMessage = 0;
        FileTransfer * file = new FileTransfer();
        allocatedBytesForMessage = (totalTransferSizeInBytes - totalAllocatedBytes) > maxBytesPerMessage
            ? maxBytesPerMessage : totalTransferSizeInBytes - totalAllocatedBytes;

        file->type = filetype;
        file->file_id = i;
        file->file_len = allocatedBytesForMessage;
        file->number_of_messages = number_of_messages;
        file->name_len = 0;
        file->file_data = new unsigned char[allocatedBytesForMessage];
        memcpy(file->file_data, &buffer.data()[totalAllocatedBytes], allocatedBytesForMessage);

        totalAllocatedBytes += allocatedBytesForMessage;

        payLoads.push_back(file);
    }
    return payLoads;
}
*/
