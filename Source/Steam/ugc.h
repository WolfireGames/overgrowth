//-----------------------------------------------------------------------------
//           Name: ugc.h
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

#if ENABLE_STEAMWORKS
#include <Steam/ugc_id.h>
#include <Internal/modid.h>

#include <steam_api.h>

#include <vector>
#include <string>
#include <set>

class SteamworksUGCItem;

class SteamworksUGC {
   private:
    friend class Steamworks;
    SteamworksUGC();

    // After this many update calls, every item should be updated
    // So if 120 mods are activated, 2 will be updated each frame when this is 60
    const static int MAX_UPDATE_FRAMES = 60;
    int update_item;

   public:
    ~SteamworksUGC();

    void Update();

    UGCID TryUploadMod(ModID sid);

    std::vector<SteamworksUGCItem *>::iterator LoadModIntoGame(PublishedFileId_t steamworks_id);

    std::vector<SteamworksUGCItem *>::iterator GetItem(PublishedFileId_t steamworks_id);
    std::vector<SteamworksUGCItem *>::iterator GetItem(ModID sid);
    std::vector<SteamworksUGCItem *>::iterator GetItem(UGCID id);
    std::vector<SteamworksUGCItem *>::iterator GetItemBegin();
    std::vector<SteamworksUGCItem *>::iterator GetItemEnd();

    size_t SubscribedNotInstalledCount();
    size_t DownloadingCount();
    size_t DownloadPendingCount();
    size_t NeedsUpdateCount();

    float TotalDownloadProgress();

    bool WaitingForResults();

    void QueryPersonalWorkshopItems();
    void QueryFavoritesWorkshopItems();

    std::vector<SteamworksUGCItem *> items;

    void OnUGCRemoteStoragePublishedFileSubscribed(RemoteStoragePublishedFileSubscribed_t *pCallback);
    CCallback<SteamworksUGC, RemoteStoragePublishedFileSubscribed_t, false> m_callRemoteStoragePublishedFileSubscribed;

    void OnUGCRemoteStoragePublishedFileUnsubscribed(RemoteStoragePublishedFileUnsubscribed_t *pCallback);
    CCallback<SteamworksUGC, RemoteStoragePublishedFileUnsubscribed_t, false> m_callRemoteStoragePublishedFileUnsubscribed;

    void OnUGCRemoteStoragePublishedFileDeleted(RemoteStoragePublishedFileDeleted_t *pCallback);
    CCallback<SteamworksUGC, RemoteStoragePublishedFileDeleted_t, false> m_callRemoteStoragePublishedFileDeleted;

    void OnUGCSteamUGCQueryCompleted(SteamUGCQueryCompleted_t *pResult, bool failed);
    CCallResult<SteamworksUGC, SteamUGCQueryCompleted_t> m_callSteamUGCQueryCompleted;

    void OnUGCSteamUGCQueryFavoritesCompleted(SteamUGCQueryCompleted_t *pResult, bool failed);
    CCallResult<SteamworksUGC, SteamUGCQueryCompleted_t> m_callSteamUGCQueryFavoritesCompleted;
};
#endif
