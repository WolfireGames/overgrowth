//-----------------------------------------------------------------------------
//           Name: modloading.h
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

#include <Internal/modid.h>
#include <Internal/integer.h>
#include <Internal/filesystem.h>

#include <XML/Parsers/activemodsparser.h>
#include <Utility/fixed_string.h>
#include <Steam/ugc_id.h>

#include <vector>
#include <string>
#include <iostream>
#include <set>

class SteamworksUGCItem;
class TiXmlElement;

enum class ModVisibilityOptions {
    Public = 0,
    Friends = 1,
    Private = 2,
};

static const char* mod_visibility_options[] = {"Public", "Friends", "Private"};

#if ENABLE_STEAMWORKS
const ERemoteStoragePublishedFileVisibility mod_visibility_map[] = {k_ERemoteStoragePublishedFileVisibilityPublic,
                                                                    k_ERemoteStoragePublishedFileVisibilityFriendsOnly,
                                                                    k_ERemoteStoragePublishedFileVisibilityPrivate};
static char edit_tags_field[k_cchTagListMax] = "";
#endif

const int update_to_visibilty_options_count = sizeof(mod_visibility_options) / sizeof(mod_visibility_options[0]);

class ModInstance {
   public:
    struct Attribute {
        fixed_string<MOD_ATTRIBUTE_ID_MAX_LENGTH> id;
        fixed_string<MOD_ATTRIBUTE_VALUE_MAX_LENGTH> value;
    };

    struct ModDependency {
        std::string id;
        std::vector<std::string> versions;
    };

    struct MenuItem {
        fixed_string<MOD_MENU_ITEM_TITLE_MAX_LENGTH> title;
        fixed_string<MOD_MENU_ITEM_CATEGORY_MAX_LENGTH> category;
        fixed_string<MOD_MENU_ITEM_PATH_MAX_LENGTH> path;
        fixed_string<MOD_MENU_ITEM_THUMBNAIL_MAX_LENGTH> thumbnail;
    };

    struct Parameter {
        Parameter();

        fixed_string<MOD_LEVEL_PARAMETER_NAME_MAX_LENGTH> name;
        fixed_string<MOD_LEVEL_PARAMETER_TYPE_MAX_LENGTH> type;
        fixed_string<MOD_LEVEL_PARAMETER_VALUE_MAX_LENGTH> value;

        std::vector<Parameter> parameters;
    };

    struct Level {
        fixed_string<MOD_LEVEL_ID_MAX_LENGTH> id;
        fixed_string<MOD_LEVEL_TITLE_MAX_LENGTH> title;
        fixed_string<MOD_LEVEL_THUMBNAIL_MAX_LENGTH> thumbnail;
        fixed_string<MOD_LEVEL_PATH_MAX_LENGTH> path;
        bool completion_optional;
        bool supports_online;
        bool requires_online;
        Parameter parameter;
    };

    struct Item {
        fixed_string<MOD_ITEM_TITLE_MAX_LENGTH> title;
        fixed_string<MOD_ITEM_CATEGORY_MAX_LENGTH> category;
        fixed_string<MOD_ITEM_PATH_MAX_LENGTH> path;
        fixed_string<MOD_ITEM_PATH_MAX_LENGTH> thumbnail;
    };

    struct Campaign {
        fixed_string<MOD_CAMPAIGN_ID_MAX_LENGTH> id;
        fixed_string<MOD_CAMPAIGN_TITLE_MAX_LENGTH> title;
        fixed_string<MOD_CAMPAIGN_THUMBNAIL_MAX_LENGTH> thumbnail;
        fixed_string<MOD_CAMPAIGN_MENU_SCRIPT_PATH_MAX_LENGTH> menu_script;
        fixed_string<MOD_CAMPAIGN_MAIN_SCRIPT_PATH_MAX_LENGTH> main_script;

        std::vector<ModInstance::Attribute> attributes;
        std::vector<ModInstance::Level> levels;
        Parameter parameter;
        bool supports_online;
        bool requires_online;
    };

    struct Pose {
        fixed_string<MOD_POSE_NAME_MAX_LENGTH> name;
        fixed_string<MOD_POSE_NAME_MAX_LENGTH> command;
        fixed_string<MOD_POSE_PATH_MAX_LENGTH> path;
    };

    static const ModValidity kValidityValid;
    static const ModValidity kValidityBrokenXml;
    static const ModValidity kValidityInvalidVersion;
    static const ModValidity kValidityMissingReadRights;
    static const ModValidity kValidityMissingXml;
    static const ModValidity kValidityUnloaded;
    static const ModValidity kValidityUnsupportedOvergrowthVersion;
    static const ModValidity kValidityNoDataOnDisk;
    static const ModValidity kValiditySteamworksError;
    static const ModValidity kValidityNotInstalled;
    static const ModValidity kValidityNotSubscribed;

    static const ModValidity kValidityIDTooLong;
    static const ModValidity kValidityIDMissing;
    static const ModValidity kValidityIDInvalid;

    static const ModValidity kValidityNameTooLong;
    static const ModValidity kValidityVersionTooLong;
    static const ModValidity kValidityAuthorTooLong;

    static const ModValidity kValidityDescriptionTooLong;
    static const ModValidity kValidityThumbnailMaxLength;
    static const ModValidity kValidityTagsListTooLong;

    static const ModValidity kValidityMenuItemTitleTooLong;
    static const ModValidity kValidityMenuItemTitleMissing;

    static const ModValidity kValidityMenuItemCategoryTooLong;
    static const ModValidity kValidityMenuItemCategoryMissing;

    static const ModValidity kValidityMenuItemPathTooLong;
    static const ModValidity kValidityMenuItemPathMissing;
    static const ModValidity kValidityMenuItemPathInvalid;

    static const ModValidity kValidityInvalidTag;

    static const ModValidity kValidityIDCollision;
    static const ModValidity kValidityActiveModCollision;

    static const ModValidity kValidityMissingName;
    static const ModValidity kValidityMissingAuthor;
    static const ModValidity kValidityMissingDescription;
    static const ModValidity kValidityMissingThumbnail;
    static const ModValidity kValidityMissingThumbnailFile;
    static const ModValidity kValidityMissingVersion;

    static const ModValidity kValidityCampaignTitleTooLong;
    static const ModValidity kValidityCampaignTitleMissing;

    static const ModValidity kValidityCampaignTypeTooLong;
    static const ModValidity kValidityCampaignTypeMissing;

    static const ModValidity kValidityCampaignIsLinearInvalidEnum;

    static const ModValidity kValidityLevelTitleTooLong;
    static const ModValidity kValidityLevelTitleMissing;

    static const ModValidity kValidityLevelPathTooLong;
    static const ModValidity kValidityLevelPathMissing;
    static const ModValidity kValidityLevelPathInvalid;

    static const ModValidity kValidityLevelThumbnailTooLong;
    static const ModValidity kValidityLevelThumbnailMissing;
    static const ModValidity kValidityLevelThumbnailInvalid;
    static const ModValidity kValidityInvalidPreviewImage;
    static const ModValidity kValidityInvalidSupportedVersion;
    static const ModValidity kValidityInvalidLevelHookFile;
    static const ModValidity kValidityInvalidNeedRestart;

    static const ModValidity kValidityItemTitleTooLong;
    static const ModValidity kValidityItemTitleMissing;

    static const ModValidity kValidityItemCategoryTooLong;
    static const ModValidity kValidityItemCategoryMissing;

    static const ModValidity kValidityItemPathTooLong;
    static const ModValidity kValidityItemPathMissing;
    static const ModValidity kValidityItemPathFileMissing;

    static const ModValidity kValidityItemThumbnailTooLong;
    static const ModValidity kValidityItemThumbnailMissing;
    static const ModValidity kValidityItemThumbnailFileMissing;

    static const ModValidity kValidityCategoryTooLong;
    static const ModValidity kValidityCategoryMissing;

    static const ModValidity kValidityCampaignThumbnailMissing;
    static const ModValidity kValidityCampaignThumbnailTooLong;
    static const ModValidity kValidityCampaignThumbnailInvalid;

    static const ModValidity kValidityMenuItemThumbnailTooLong;
    static const ModValidity kValidityMenuItemThumbnailInvalid;

    static const ModValidity kValidityCampaignMenuScriptTooLong;
    static const ModValidity kValidityMainScriptTooLong;

    static const ModValidity kValidityAttributeIdTooLong;
    static const ModValidity kValidityAttributeIdInvalid;

    static const ModValidity kValidityAttributeValueTooLong;
    static const ModValidity kValidityAttributeValueInvalid;

    static const ModValidity kValidityLevelIdTooLong;

    static const ModValidity kValidityCampaignIDTooLong;
    static const ModValidity kValidityCampaignIDMissing;

    static const ModValidity kValidityLevelParameterNameTooLong;
    static const ModValidity kValidityLevelParameterNameMissing;

    static const ModValidity kValidityLevelParameterValueTooLong;
    static const ModValidity kValidityLevelParameterValueMissing;

    static const ModValidity kValidityLevelParameterTypeTooLong;

    static const ModValidity kValidityPoseNameTooLong;
    static const ModValidity kValidityPoseNameMissing;
    static const ModValidity kValidityPosePathTooLong;
    static const ModValidity kValidityPosePathMissing;
    static const ModValidity kValidityPoseCommandTooLong;
    static const ModValidity kValidityPoseCommandMissing;

    static const uint64_t kValidityFlagCount = 91;

    static const ModValidity kValidityUploadSteamworksBlockingMask;
    static const ModValidity kValidityActivationBreakingErrorMask;

    static const ModValidity kValidityCampaignMenuMusicTooLong;
    static const ModValidity kValidityCampaignMenuMusicMissing;

    static const uint32_t k_UploadValidityOk = 0;
    static const uint32_t k_UploadValidityGenericValidityError = 1UL << 0;  // If any of the standard kValidity values are set, we don't allow an upload.
    static const uint32_t k_UploadValidityInvalidPreviewImage = 1UL << 1;
    static const uint32_t k_UploadValidityMissingPreview = 1UL << 2;
    static const uint32_t k_UploadValidityInvalidModFolder = 1UL << 3;
    static const uint32_t k_UploadValidityBrokenXml = 1UL << 4;
    static const uint32_t k_UploadValidityInvalidID = 1UL << 5;
    static const uint32_t k_UploadValidityInvalidVersion = 1UL << 6;
    static const uint32_t k_UploadValidityMissingReadRights = 1UL << 7;
    static const uint32_t k_UploadValidityMissingXml = 1UL << 8;
    static const uint32_t k_UploadValidityInvalidThumbnail = 1UL << 9;
    static const uint32_t k_UploadValidityOversizedPreviewImage = 1UL << 10;

    enum UserVote {
        k_VoteUnknown,
        k_VoteNone,
        k_VoteUp,
        k_VoteDown
    };

   private:
    ModValidity valid;

    bool is_core;
    bool active;
    bool needs_restart;
    bool supports_online;
    bool explicit_version_support;
    ModID sid;

    // Steamworks specific data
    UGCID ugc_id;
    //---

    bool has_been_activated;

    static int sid_counter;

   public:
    ModInstance(const std::string& _path);
    ModInstance(const UGCID _ugc_id);

    void ParseLevel(ModInstance::Level* dest, TiXmlElement* pElemLevel, uint32_t id_fallback);
    void ParseParameter(ModInstance::Parameter* dest, TiXmlElement* pElemParameter, const char* parent_type);

    void PurgeActiveSettings();
    void Reload();

    bool IsValid() const;
    bool CanActivate() const;
    bool IsActive() const;
    bool IsCore() const;

    ModID GetSid() const;
    bool Activate(bool value);
    void SetHasBeenActivated();
    bool HasBeenActivated() const;
    bool NeedsRestart() const;
    bool SupportsOnline() const;
    bool ExplicitVersionSupport() const;

    /* Evaluate if this mod could be uploaded to steam or other */
    uint32_t GetUploadValidity();

    /** These only really applies to a mod from steamworks **/
    SteamworksUGCItem* GetUGCItem();

    bool IsOwnedByCurrentUser();
    bool IsSubscribed();
    bool IsInstalled();
    bool IsDownloading();
    bool IsDownloadPending();
    bool IsNeedsUpdate();
    bool IsFavorite();

    UserVote GetUserVote();

    void RequestUnsubscribe();
    void RequestSubscribe();
    void RequestVoteSet(bool voteup);
    void RequestFavoriteSet(bool fav);

    void RequestUpdate(ModID source_sid, const char* update_message, ModVisibilityOptions visibility);
    /** End to steamworks specific **/

    const char* GetModsourceString() const;

    const char* GetStatus();

    std::string GetTagsListString();

    std::vector<ModID> GetCollisionsWithActiveMods() const;
    std::vector<ModID> GetIDCollisionsWithActiveMods() const;

    static std::vector<std::string> GenerateValidityErrorsArr(const ModValidity& validity, const ModInstance* instance = NULL);
    static std::string GenerateValidityErrors(const ModValidity& v, const ModInstance* instance = NULL);
    static std::vector<std::string> GetFileCollisions(const ModInstance* lhs, const ModInstance* rhs);

    Path GetFullAbsThumbnailPath() const;

    std::vector<std::string> GetValidityErrorsArr() const;
    std::string GetValidityErrors() const;
    ModValidity GetValidity() const;

    std::vector<std::string> invalid_item_paths;
    std::vector<std::string> GetInvalidItemPaths() const;

    ModSource modsource;

    std::string path;

    fixed_string<MOD_ID_MAX_LENGTH> id;
    fixed_string<MOD_NAME_MAX_LENGTH> name;
    fixed_string<MOD_CATEGORY_MAX_LENGTH> category;
    fixed_string<MOD_VERSION_MAX_LENGTH> version;
    fixed_string<MOD_AUTHOR_MAX_LENGTH> author;
    fixed_string<MOD_DESCRIPTION_MAX_LENGTH> description;
    fixed_string<MOD_THUMBNAIL_MAX_LENGTH> thumbnail;

    std::vector<std::string> preview_images;
    std::set<std::string> tags;

    std::vector<std::string> supported_versions;
    std::vector<std::string> level_hook_files;
    std::vector<ModDependency> mod_dependencies;

    std::vector<MenuItem> main_menu_items;
    std::vector<Item> items;

    std::vector<Level> levels;

    std::vector<Campaign> campaigns;

    std::vector<Pose> poses;

    std::vector<std::string> overload_files;
    std::vector<std::string> manifest;  // List of all files in the mod.

    struct LevelLocalizationData {
        std::string name;
        std::string loading_screen_tip;
    };
    struct LanguageData {
        std::string language;        // Language name in english
        std::string local_language;  // Language name in the language
        std::map<std::string, LevelLocalizationData> per_level_data;
    };
    // Language shortcode
    std::map<std::string, LanguageData> language_data;

    friend std::ostream& operator<<(std::ostream& os, const ModInstance& mi);
};

class ModLoadingCallback {
   public:
    virtual void ModActivationChange(const ModInstance* mod) = 0;

    virtual ~ModLoadingCallback() {}
};

class ModLoading {
   private:
    std::vector<ModInstance*> mods;
    std::vector<ModLoadingCallback*> callbacks;

    static ModLoading instance;
    ModLoading();

    std::vector<ModInstance*>::iterator GetMod(const std::string& path);

    // void AddMod( const std::string& path );
    void RemoveMod(const std::string& path);
    void ReloadMod(const std::string& path);

    void SetMods(std::vector<std::string> modpaths);

    friend class ModInstance;
    void TriggerModActivationCallback(ModInstance* mod);

    ActiveModsParser acp;

   public:
    void AddMod(const std::string& path);
    std::string WhichCampaignLevelBelongsTo(const std::string& level);
    bool IsCampaignPresent(const std::string& campaign);
    bool CampaignHasLevel(const std::string& campaign, const std::string& level_path);
    ModInstance* GetMod(ModID sid);
    std::string GetModName(ModID sid);
    std::string GetModID(ModID sid);
    bool IsActive(ModID sid);
    bool IsActive(std::string id);

    ModInstance::Campaign GetCampaign(std::string& campaign_id);
    ModInstance* GetModInstance(const std::string& campaign_id);
    std::vector<ModInstance::Campaign> GetCampaigns();

    std::vector<ModInstance*> GetModsMatchingID(std::string id);
    std::vector<ModInstance*> GetSteamModsMatchingID(std::string id);
    std::vector<ModInstance*> GetLocalModsMatchingID(std::string id);
    std::vector<ModInstance*> GetLocalMods();
    std::vector<ModInstance*> GetAllMods();

    const std::vector<ModInstance*>& GetMods();
    const std::vector<ModID> GetModsSid();

    void InitMods();
    void DetectMods();
    void Reload();

#if ENABLE_STEAMWORKS
    ModID CreateSteamworksMod(UGCID steamworks_id);
#endif

    void RegisterCallback(ModLoadingCallback* callback);
    void DeRegisterCallback(ModLoadingCallback* callback);

    static ModLoading& Instance();
    void Initialize();
    void Dispose();
    void SaveModConfig();

    std::vector<std::string> ActiveLevelHooks();

    void RebuildLocalization();
};

std::ostream& operator<<(std::ostream& os, const ModInstance::ModDependency& md);
std::ostream& operator<<(std::ostream& os, const ModInstance& mi);
