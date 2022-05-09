//-----------------------------------------------------------------------------
//           Name: ugc_item.cpp
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
#include "ugc_item.h"

#include <Internal/integer.h>
#include <Internal/modloading.h>
#include <Internal/filesystem.h>

#include <JSON/json.h>
#include <JSON/jsonhelper.h>

#include <Steam/steamworks_util.h>
#include <Steam/steamworks.h>

#include <Logging/logdata.h>
#include <Utility/assert.h>
#include <Memory/allocation.h>
#include <Compat/compat.h>

#if ENABLE_STEAMWORKS
int SteamworksUGCItem::ugc_id_counter = 1;

SteamworksUGCItem::SteamworksUGCItem(ModID _upload_sid_source) : ugc_id(ugc_id_counter++),
                                                                 steamworks_id(0),
                                                                 upload_sid_source(_upload_sid_source),
                                                                 waiting_for_details(false),
                                                                 waiting_for_create(false),
                                                                 waiting_for_update(false),
                                                                 was_installed(false),
                                                                 has_eresult_error(false),
                                                                 visibility(k_ERemoteStoragePublishedFileVisibilityPrivate),
                                                                 user_vote(k_VoteUnknown),
                                                                 favorite(false),
                                                                 favorite_clean_for_query(true),
                                                                 mod_data_size(0),
                                                                 need_update_installed_size(true),
                                                                 update_progress(0.0f) {
    RequestCreation();
}

SteamworksUGCItem::SteamworksUGCItem(PublishedFileId_t s_id) : ugc_id(ugc_id_counter++),
                                                               steamworks_id(s_id),
                                                               waiting_for_details(false),
                                                               waiting_for_create(false),
                                                               waiting_for_update(false),
                                                               was_installed(false),
                                                               has_eresult_error(false),
                                                               visibility(k_ERemoteStoragePublishedFileVisibilityPrivate),
                                                               user_vote(k_VoteUnknown),
                                                               favorite(false),
                                                               favorite_clean_for_query(true),
                                                               mod_data_size(0),
                                                               need_update_installed_size(true),
                                                               update_progress(0.0f) {
    RequestDetails();
}

UGCID SteamworksUGCItem::GetUGCID() {
    return ugc_id;
}

void SteamworksUGCItem::RequestCreation() {
    if (waiting_for_create == false) {
        if (steamworks_id == 0) {
            SteamAPICall_t hSteamAPICall = SteamUGC()->CreateItem(OVERGROWTH_APP_ID, k_EWorkshopFileTypeCommunity);
            m_callResultsCreateItemResult.Set(hSteamAPICall, this, &SteamworksUGCItem::OnUGCCreateItemResult);

            waiting_for_create = true;
        } else {
            LOGE << "Requested creation of an already created steamworks item" << std::endl;
        }
    } else {
        LOGE << "Already creating, can't queue another attempt" << std::endl;
    }
}

void SteamworksUGCItem::RequestDetails() {
    if (waiting_for_details == false) {
        LOGI << "Requesting detail info for id: " << id << std::endl;
        LOGI << "Item state:" << ItemStateString(ItemState()) << std::endl;
        // SteamAPICall_t hSteamAPICall = SteamUGC()->RequestUGCDetails(steamworks_id, 60);
        // m_callResultSteamUGCRequestUGCDetailsResult.Set( hSteamAPICall, this, &SteamworksUGCItem::OnFindUGCDetailsResult );

        UGCQueryHandle_t handle = SteamUGC()->CreateQueryUGCDetailsRequest(&steamworks_id, 1);

        SteamUGC()->SetReturnMetadata(handle, true);

        SteamAPICall_t apicall = SteamUGC()->SendQueryUGCRequest(handle);

        waiting_for_details = true;

        m_callSteamUGCQueryCompleted.Set(apicall, this, &SteamworksUGCItem::OnUGCSteamUGCQueryCompleted);

        apicall = SteamUGC()->GetUserItemVote(steamworks_id);

        m_callResultGetUserItemVoteResult.Set(apicall, this, &SteamworksUGCItem::OnUGCGetUserItemVoteResult);
    } else {
        LOGE << "Already waiting for a response on details" << std::endl;
    }
}

uint32_t SteamworksUGCItem::VerifyForUpload() {
    uint32_t status = 0;

    ModInstance* mi = ModLoading::Instance().GetMod(upload_sid_source);

    if (mi) {
        if (mi->preview_images.size() > 0) {
            Path previewpath = FindFilePath(AssemblePath(mi->path, mi->preview_images[0]).c_str(), kAbsPath, true);
            if (previewpath.valid == false) {
                status |= k_UploadVerifyInvalidPreviewImage;
            }
        } else {
            status |= k_UploadVerifyMissingPreview;
        }

        Path modpath = FindFilePath(mi->path.c_str(), kAbsPath, true);

        if (modpath.valid == false) {
            status |= k_UploadVerifyInvalidModFolder;
        }

        if (mi->version == version) {
            status |= k_UploadVerifyUnchangedVersion;
        }
    } else {
        status |= k_UploadVerifyMissingModSource;
    }

    if (waiting_for_update) {
        status |= k_UploadVerifyWaitingForUpload;
    }

    return status;
}

void SteamworksUGCItem::RequestUpload(const char* update_message, ERemoteStoragePublishedFileVisibility new_visibility, uint32_t upload_control) {
    bool accept_upload = false;
    uint32_t verify = VerifyForUpload();

    if (verify == k_UploadVerifyOk) {
        if (waiting_for_update == false) {
            ModInstance* mi = ModLoading::Instance().GetMod(upload_sid_source);
            if (mi) {
                LOGI << "Doing upload" << std::endl;
                id.set(mi->id);
                version.set(mi->version);
                name.set(mi->name);
                description.set(mi->description);
                author.set(mi->author);
                tags = mi->tags;

                update_handle = SteamUGC()->StartItemUpdate(OVERGROWTH_APP_ID, steamworks_id);

                SteamUGC()->SetItemTitle(update_handle, name);

                if (upload_control & k_UploadDataControl_UploadDescription) {
                    SteamUGC()->SetItemDescription(update_handle, description);
                }

                SteamUGC()->SetItemUpdateLanguage(update_handle, "english");

                SimpleJSONWrapper json;

                Json::Value& meta_root = json.getRoot();
                meta_root["id"] = Json::Value(id);
                meta_root["version"] = Json::Value(version);
                meta_root["author"] = Json::Value(author);

                LOG_ASSERT(k_cchDeveloperMetadataMax > json.writeString().length() + 1);
                SteamUGC()->SetItemMetadata(update_handle, json.writeString().c_str());

                SteamUGC()->SetItemVisibility(update_handle, new_visibility);

                if (tags.size() > 0) {
                    const char** tagsc = (const char**)alloc.stack.Alloc(tags.size());

                    std::set<std::string>::iterator tagit = tags.begin();
                    int count = 0;
                    for (; tagit != tags.end(); tagit++) {
                        tagsc[count] = tagit->c_str();
                        count++;
                    }

                    SteamParamStringArray_t pTags;
                    pTags.m_ppStrings = tagsc;
                    pTags.m_nNumStrings = count;
                    SteamUGC()->SetItemTags(update_handle, &pTags);

                    alloc.stack.Free(tagsc);
                }

                if (mi->preview_images.size() > 0) {
                    Path previewpath = FindFilePath(AssemblePath(mi->path, mi->preview_images[0]).c_str(), kAbsPath, true);
                    if (previewpath.valid) {
                        SteamUGC()->SetItemPreview(update_handle, previewpath.GetAbsPathStr().c_str());
                    }
                }

                Path modpath = FindFilePath(mi->path.c_str(), kAbsPath, true);
                if (modpath.valid) {
                    SteamUGC()->SetItemContent(update_handle, modpath.GetAbsPathStr().c_str());
                }

                SteamAPICall_t hSteamAPICall = SteamUGC()->SubmitItemUpdate(update_handle, update_message);

                m_callResultSubmitItemUpdateResults.Set(hSteamAPICall, this, &SteamworksUGCItem::OnUGCSubmitItemUpdateResult);
                waiting_for_update = true;
            }
        }
    } else {
        LOGE << "Will not perform upload, error when verifying upload status: " << verify << std::endl;
    }
}

void SteamworksUGCItem::RequestUnsubscribe() {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        mod->Activate(false);
        mod->PurgeActiveSettings();
    }
    SteamUGC()->UnsubscribeItem(steamworks_id);
}

void SteamworksUGCItem::RequestSubscribe() {
    SteamUGC()->SubscribeItem(steamworks_id);
}

void SteamworksUGCItem::RequestUserVoteSet(bool voteup) {
    SteamAPICall_t apicall = SteamUGC()->SetUserItemVote(steamworks_id, voteup);
    m_callResultSetUserItemVoteResult.Set(apicall, this, &SteamworksUGCItem::OnUGCSetUserItemVoteResult);
}

void SteamworksUGCItem::RequestFavoriteSet(bool v) {
    SteamAPICall_t hSteamAPICall;
    favorite_clean_for_query = false;
    if (v) {
        hSteamAPICall = SteamUGC()->AddItemToFavorites(OVERGROWTH_APP_ID, steamworks_id);
    } else {
        hSteamAPICall = SteamUGC()->RemoveItemFromFavorites(OVERGROWTH_APP_ID, steamworks_id);
    }
    m_callResultUserFavoriteItemsListChanged.Set(hSteamAPICall, this, &SteamworksUGCItem::OnUGCUserFavoriteItemsListChanged);
}

void SteamworksUGCItem::Reload() {
    if (sid.Valid()) {
        ModInstance* mi = ModLoading::Instance().GetMod(sid);
        if (mi) {
            LOGI << "Requesting that the ModInstance reload data" << std::endl;
            mi->Reload();
        } else {
            LOGE << "There is no engine mod matching the steamworks mod" << std::endl;
        }
    }
}

void SteamworksUGCItem::UpdateModInstallationInfo() {
    uint64 total;
    uint32 timestamp;
    char path[kPathSize];

    bool res = SteamUGC()->GetItemInstallInfo(steamworks_id, &total, path, kPathSize, &timestamp);
    if (res) {
        mod_path.set(path);
        mod_data_size = total;
        LOGI << "Updated install info for " << id << std::endl;
    } else {
        LOGE << "Unable to fetch mod size info for " << id << std::endl;
    }
}

void SteamworksUGCItem::Update() {
    if (steamworks_id != 0) {
        if (was_installed == false && IsInstalled()) {
            OnBecameInstalled();
        }

        if (was_installed == true && IsInstalled() == false) {
            OnBecameUninstalled();
        }

        was_installed = IsInstalled();

        if (IsSubscribed()) {
            if (NeedsUpdate()) {
                uint64 processed, total;
                SteamUGC()->GetItemDownloadInfo(steamworks_id, &processed, &total);
                if (total > 0) {
                    download_progress = (float)((double)processed / (double)total);
                    mod_data_size = total;
                } else {
                    download_progress = 0.0f;
                }
                need_update_installed_size = true;
            } else if (IsInstalled()) {
                if (need_update_installed_size) {
                    UpdateModInstallationInfo();
                    need_update_installed_size = false;
                }
                download_progress = 1.0f;
            } else {
                download_progress = 0.0f;
            }
        } else {
            download_progress = 0.0f;
        }

        if (waiting_for_update) {
            uint64 processed, total;
            update_status = SteamUGC()->GetItemUpdateProgress(update_handle, &processed, &total);
            update_progress = (float)((double)processed / (double)total);
        } else {
            update_status = k_EItemUpdateStatusInvalid;
        }
    }
}

void SteamworksUGCItem::OnBecameInstalled() {
    Reload();
}

void SteamworksUGCItem::OnBecameUninstalled() {
    Reload();
}

void SteamworksUGCItem::OnUGCSteamUGCQueryCompleted(SteamUGCQueryCompleted_t* pResult, bool failed) {
    waiting_for_details = false;

    LOGI << "Got Details for steamworks mod " << id << std::endl;
    if (failed == false) {
        if (pResult->m_eResult == k_EResultOK) {
            has_eresult_error = false;
        } else {
            has_eresult_error = true;
            eresult_error = pResult->m_eResult;
        }

        SteamUGCDetails_t m_details;
        SteamUGC()->GetQueryUGCResult(pResult->m_handle, 0, &m_details);

        if (pResult->m_unNumResultsReturned == 1) {
            // If we get this error i'm assuming the mod doesn't exist anymore.
            // As this is what local testing has implied.
            if (m_details.m_eResult == k_EResultFileNotFound) {
                LOGW << "Steamworks mod " << id << " returned error which implies mod doesn't exist anymore" << std::endl;
            } else {
                if (sid.Valid() == false) {
                    sid = ModLoading::Instance().CreateSteamworksMod(ugc_id);
                }

                name.set(m_details.m_rgchTitle);
                description.set(m_details.m_rgchDescription);
                owner_id.SetFromUint64(m_details.m_ulSteamIDOwner);
                visibility = m_details.m_eVisibility;

                char* metadata = (char*)alloc.stack.Alloc(k_cchDeveloperMetadataMax + 1);

                SteamUGC()->GetQueryUGCMetadata(pResult->m_handle, 0, metadata, k_cchDeveloperMetadataMax + 1);

                SimpleJSONWrapper json;

                std::string metadatastr(metadata);

                LOGI << "Metadata string:" << metadata;

                json.parseString(metadatastr);
                Json::Value root = json.getRoot();

                id.set(root["id"].asString().c_str());
                version.set(root["version"].asString().c_str());
                author.set(root["author"].asString().c_str());

                char tagbuf[k_cchTagListMax + 1];
                char* tagbuf_mem = NULL;
                char* curtag = NULL;
                strncpy(tagbuf, m_details.m_rgchTags, k_cchTagListMax);
                curtag = strtok_r(tagbuf, ",", &tagbuf_mem);
                tags.clear();
                while (curtag != NULL) {
                    if (strlen(curtag) > 0) {
                        tags.insert(std::string(curtag));
                    }
                    curtag = strtok_r(NULL, ",", &tagbuf_mem);
                }

                if (m_details.m_eResult != k_EResultOK) {
                    LOGE << "OnUGCDetailsResult() " << m_details.m_eResult << std::endl;
                }

                alloc.stack.Free(metadata);
            }
        } else {
            LOGE << "Got more/less results than expected for " << steamworks_id << std::endl;
        }
    } else {
    }

    Reload();
}

void SteamworksUGCItem::OnUGCCreateItemResult(CreateItemResult_t* pResult, bool failed) {
    waiting_for_create = false;

    LOGI << "OnUGCCreateItemResult() " << pResult->m_eResult << std::endl;

    if (failed == false) {
        if (pResult->m_eResult == k_EResultOK) {
            has_eresult_error = false;
        } else {
            has_eresult_error = true;
            eresult_error = pResult->m_eResult;
        }
        steamworks_id = pResult->m_nPublishedFileId;
        if (sid.Valid() == false) {
            sid = ModLoading::Instance().CreateSteamworksMod(ugc_id);
        }

        Steamworks::Instance()->needs_to_accept_license = pResult->m_bUserNeedsToAcceptWorkshopLegalAgreement;
        if (pResult->m_bUserNeedsToAcceptWorkshopLegalAgreement) {
            LOGW << "User needs to accept workshop legal agreement" << std::endl;
        }
    } else {
        LOGE << "Error creating Steam Workshop item" << std::endl;
    }

    ModInstance* mi = ModLoading::Instance().GetMod(sid);

    Reload();

    RequestUpload("Initial Upload", visibility, k_UploadDataControl_UploadAll);
}

void SteamworksUGCItem::OnUGCSubmitItemUpdateResult(SubmitItemUpdateResult_t* pResult, bool failed) {
    waiting_for_update = false;
    LOGI << "OnUGCSubmitItemUpdateResult() " << pResult->m_eResult << std::endl;
    if (failed == false) {
        if (pResult->m_eResult == k_EResultOK) {
            has_eresult_error = false;
        } else {
            has_eresult_error = true;
            eresult_error = pResult->m_eResult;
        }

        Steamworks::Instance()->needs_to_accept_license = pResult->m_bUserNeedsToAcceptWorkshopLegalAgreement;
        if (pResult->m_bUserNeedsToAcceptWorkshopLegalAgreement) {
            LOGW << "User needs to accept workshop legal agreement" << std::endl;
        }
    } else {
        LOGE << "Error on item update" << std::endl;
    }

    ModInstance* mi = ModLoading::Instance().GetMod(sid);

    Reload();

    RequestDetails();
}

void SteamworksUGCItem::OnUGCGetUserItemVoteResult(GetUserItemVoteResult_t* pResult, bool failed) {
    if (failed == false) {
        if (pResult->m_eResult == k_EResultOK) {
            if (pResult->m_bVotedUp) {
                user_vote = k_VoteUp;
            } else if (pResult->m_bVotedDown) {
                user_vote = k_VoteDown;
            } else if (pResult->m_bVoteSkipped) {
                user_vote = k_VoteNone;
            }
        } else {
            LOGE << "Failed getting item vote result" << std::endl;
        }
    } else {
        LOGE << "Failed getting item vote result" << std::endl;
    }
}

void SteamworksUGCItem::OnUGCUserFavoriteItemsListChanged(UserFavoriteItemsListChanged_t* pResult, bool failed) {
    if (pResult) {
        if (failed == false) {
            favorite = pResult->m_bWasAddRequest;
        } else {
            LOGE << "Error when settings favorite: " << pResult->m_eResult << std::endl;
        }
    } else {
        LOGE << "pResult was null in favorite change callback." << std::endl;
    }
}

void SteamworksUGCItem::OnUGCSetUserItemVoteResult(SetUserItemVoteResult_t* pResult, bool failed) {
    if (failed == false) {
        if (pResult->m_eResult == k_EResultOK) {
            if (pResult->m_bVoteUp) {
                user_vote = k_VoteUp;
            } else {
                user_vote = k_VoteDown;
            }
        } else {
            LOGE << "Failed getting item vote result" << std::endl;
        }
    } else {
        LOGE << "Failed getting item vote result" << std::endl;
    }
}

std::string SteamworksUGCItem::ItemStateString(uint32_t flag) {
    std::stringstream ss;
    if (flag & k_EItemStateSubscribed)
        ss << "k_EItemStateSubscribed ";
    if (flag & k_EItemStateLegacyItem)
        ss << "k_EItemStateLegacyItem ";
    if (flag & k_EItemStateInstalled)
        ss << "k_EItemStateInstalled ";
    if (flag & k_EItemStateNeedsUpdate)
        ss << "k_EItemStateNeedsUpdate ";
    if (flag & k_EItemStateDownloading)
        ss << "k_EItemStateDownloading ";
    if (flag & k_EItemStateDownloadPending)
        ss << "k_EItemStateDownloadPending ";
    if (k_EItemStateNone == flag)
        ss << "k_EItemStateNone";
    return ss.str();
}

float SteamworksUGCItem::ItemDownloadProgress() {
    return download_progress;
}

uint64_t SteamworksUGCItem::ItemDataSize() {
    return mod_data_size;
}

float SteamworksUGCItem::ItemUploadProgress() {
    return update_progress;
}

EItemUpdateStatus SteamworksUGCItem::GetUpdateStatus() {
    return update_status;
}

const char* SteamworksUGCItem::UpdateStatusString() {
    switch (update_status) {
        case k_EItemUpdateStatusInvalid:
            return "No Status";
        case k_EItemUpdateStatusPreparingConfig:
            return "Preparing Config";
        case k_EItemUpdateStatusPreparingContent:
            return "Preparing Content";
        case k_EItemUpdateStatusUploadingContent:
            return "Uploading Content";
        case k_EItemUpdateStatusUploadingPreviewFile:
            return "Uploading Preview File";
        case k_EItemUpdateStatusCommittingChanges:
            return "Commiting Changes";
        default:
            return "UNKNOWN";
    }
}

uint32_t SteamworksUGCItem::ItemState() {
    return SteamUGC()->GetItemState(steamworks_id);
}

bool SteamworksUGCItem::WaitingForResults() {
    return waiting_for_details || waiting_for_create || waiting_for_update;
}

const char* SteamworksUGCItem::GetLastResultError() {
    if (has_eresult_error) {
        return GetEResultString(eresult_error);
    } else {
        return "";
    }
}

bool SteamworksUGCItem::IsSubscribed() {
    return ItemState() & k_EItemStateSubscribed;
}

bool SteamworksUGCItem::IsInstalled() {
    return ItemState() & k_EItemStateInstalled;
}

bool SteamworksUGCItem::IsDownloading() {
    return ItemState() & k_EItemStateDownloading;
}

bool SteamworksUGCItem::IsDownloadPending() {
    return ItemState() & k_EItemStateDownloadPending;
}

bool SteamworksUGCItem::NeedsUpdate() {
    return ItemState() & k_EItemStateNeedsUpdate;
}

bool SteamworksUGCItem::IsFavorite() {
    return favorite;
}

SteamworksUGCItem::UserVote SteamworksUGCItem::GetUserVote() {
    return user_vote;
}

void SteamworksUGCItem::SetIntendedUpdateModSource(ModID mod) {
    upload_sid_source = mod;
}

std::string SteamworksUGCItem::GetPath() {
    if (ItemState() & k_EItemStateInstalled) {
        if (need_update_installed_size) {
            UpdateModInstallationInfo();
            need_update_installed_size = false;
        }
        return std::string(mod_path);
    } else {
        return std::string("");
    }
}

std::string SteamworksUGCItem::GetSteamworksIDString() {
    std::stringstream ss;
    ss << steamworks_id;
    return ss.str();
}
#endif
