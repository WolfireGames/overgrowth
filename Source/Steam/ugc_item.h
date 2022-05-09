//-----------------------------------------------------------------------------
//           Name: ugc_item.h
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
#include <Internal/integer.h>
#include <Internal/modid.h>
#include <Internal/path.h>

#include <Steam/steam_appid.h>
#include <Steam/ugc_id.h>
#include <Steam/steamworks_util.h>

#include <Utility/fixed_string.h>

#include <steam_api.h>

#include <string>
#include <set>

class SteamworksUGCItem {
   public:
    enum UploadVerifyStatus {
        k_UploadVerifyOk = 0,
        k_UploadVerifyInvalidPreviewImage = 1UL << 0,
        k_UploadVerifyMissingPreview = 1UL << 1,
        k_UploadVerifyMissingModSource = 1UL << 2,
        k_UploadVerifyWaitingForUpload = 1UL << 3,
        k_UploadVerifyInvalidModFolder = 1UL << 4,
        k_UploadVerifyUnchangedVersion = 1UL << 5
    };

    enum UploadDataControl {
        k_UploadDataControl_UploadDescription = 1UL << 0,
        k_UploadDataControl_UploadAll = 1UL << 0
    };

    enum ItemWrapperType {
        Normal,
        Upload
    };

    enum UserVote {
        k_VoteUnknown,
        k_VoteNone,
        k_VoteUp,
        k_VoteDown
    };

    PublishedFileId_t steamworks_id;
    static int ugc_id_counter;
    UGCID ugc_id;
    ModID sid;
    ModID upload_sid_source;
    ItemWrapperType item_wrapper_type;
    CSteamID owner_id;
    ERemoteStoragePublishedFileVisibility visibility;

    bool has_eresult_error;
    EResult eresult_error;

    // Stored in the metadata block
    fixed_string<MOD_ID_MAX_LENGTH> id;
    fixed_string<MOD_VERSION_MAX_LENGTH> version;
    fixed_string<MOD_AUTHOR_MAX_LENGTH> author;

    // Not in the metadata block
    fixed_string<MOD_NAME_MAX_LENGTH> name;
    fixed_string<MOD_DESCRIPTION_MAX_LENGTH> description;

    UserVote user_vote;

    std::set<std::string> tags;

    SteamworksUGCItem(ModID sid);
    SteamworksUGCItem(PublishedFileId_t id);

    std::string GetSteamworksIDString();

    UGCID GetUGCID();

    uint32_t VerifyForUpload();

    void RequestCreation();
    void RequestDetails();
    void RequestUpload(const char* update_message, ERemoteStoragePublishedFileVisibility new_visibility, uint32_t upload_control);
    void RequestUnsubscribe();
    void RequestSubscribe();
    void RequestUserVoteSet(bool voteup);
    void RequestFavoriteSet(bool favorite);

    void Reload();

    // Function that keeps tabs on some state changes.
    void Update();

    std::string ItemStateString(uint32_t flag);
    uint32_t ItemState();

    float ItemDownloadProgress();
    uint64_t ItemDataSize();

    float ItemUploadProgress();
    EItemUpdateStatus GetUpdateStatus();
    const char* UpdateStatusString();

    const char* GetLastResultError();

    bool WaitingForResults();

    bool IsSubscribed();
    bool IsInstalled();
    bool IsDownloading();
    bool IsDownloadPending();
    bool NeedsUpdate();
    bool IsFavorite();

    UserVote GetUserVote();

    void SetIntendedUpdateModSource(ModID modid);

    std::string GetPath();

    void OnBecameInstalled();
    void OnBecameUninstalled();

    void OnUGCSteamUGCQueryCompleted(SteamUGCQueryCompleted_t* pResult, bool failed);
    CCallResult<SteamworksUGCItem, SteamUGCQueryCompleted_t> m_callSteamUGCQueryCompleted;

    void OnUGCCreateItemResult(CreateItemResult_t* pResult, bool failed);
    CCallResult<SteamworksUGCItem, CreateItemResult_t> m_callResultsCreateItemResult;

    void OnUGCSubmitItemUpdateResult(SubmitItemUpdateResult_t* pResult, bool failed);
    CCallResult<SteamworksUGCItem, SubmitItemUpdateResult_t> m_callResultSubmitItemUpdateResults;

    void OnUGCGetUserItemVoteResult(GetUserItemVoteResult_t* pResult, bool failed);
    CCallResult<SteamworksUGCItem, GetUserItemVoteResult_t> m_callResultGetUserItemVoteResult;

    void OnUGCSetUserItemVoteResult(SetUserItemVoteResult_t* pResult, bool failed);
    CCallResult<SteamworksUGCItem, SetUserItemVoteResult_t> m_callResultSetUserItemVoteResult;

    void OnUGCUserFavoriteItemsListChanged(UserFavoriteItemsListChanged_t* pResult, bool failed);
    CCallResult<SteamworksUGCItem, UserFavoriteItemsListChanged_t> m_callResultUserFavoriteItemsListChanged;

   private:
    // Disable copying.
    SteamworksUGCItem& operator=(const SteamworksUGCItem& item);
    SteamworksUGCItem(SteamworksUGCItem& other);

    void UpdateModInstallationInfo();

    bool waiting_for_details;
    bool waiting_for_create;

    bool waiting_for_update;
    UGCUpdateHandle_t update_handle;
    EItemUpdateStatus update_status;
    float update_progress;

    bool was_installed;

    bool favorite;
    bool favorite_clean_for_query;  // Marker when querying for favorite list, gets set to false when user changes value via game.

    float download_progress;
    fixed_string<kPathSize> mod_path;
    uint64_t mod_data_size;
    bool need_update_installed_size;

    friend class SteamworksUGC;
};
#endif
