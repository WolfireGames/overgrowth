//-----------------------------------------------------------------------------
//           Name: modloading.cpp
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
#include "modloading.h"

#include <Internal/filesystem.h>
#include <Internal/config.h>
#include <Internal/common.h>
#include <Internal/modid.h>
#include <Internal/locale.h>

#include <Utility/strings.h>
#include <Utility/set.h>

#include <Logging/logdata.h>
#include <Version/version.h>
#include <Compat/platformsetup.h>

#if ENABLE_STEAMWORKS
#include <Steam/ugc_item.h>
#include <Steam/ugc.h>
#include <Steam/steamworks.h>
#endif

#include <trex/trex.h>
#include <tinyxml.h>

#include <sstream>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <set>

#define PARSE_ERROR(elem, flag)                                                                                                                 \
    do {                                                                                                                                        \
        TiXmlElement* v = elem;                                                                                                                 \
        if (v) {                                                                                                                                \
            LOGE << "Error when parsing mod.xml for mod " << id << " error " << ModValidityString(flag) << " on row " << v->Row() << std::endl; \
        } else {                                                                                                                                \
            LOGE << "Error when parsing mod.xml for mod " << id << " error " << ModValidityString(flag) << std::endl;                           \
        }                                                                                                                                       \
        valid |= flag;                                                                                                                          \
    } while (0)

#define PARSE_ERROR2(elem, flag, file)                                                                                                                                                 \
    do {                                                                                                                                                                               \
        TiXmlElement* v = elem;                                                                                                                                                        \
        if (v) {                                                                                                                                                                       \
            LOGE << "Error when parsing mod.xml for mod " << id << " error " << ModValidityString(flag) << " on row " << v->Row() << " with the path \"" << file << "\"" << std::endl; \
        } else {                                                                                                                                                                       \
            LOGE << "Error when parsing mod.xml" << std::endl;                                                                                                                         \
        }                                                                                                                                                                              \
        valid |= flag;                                                                                                                                                                 \
    } while (0)

static const char* menu_item_id_regex_string = "^[a-z-_]+$";
static TRexpp menu_item_id_regex;
static const char* id_regex_string = "^[a-z0-9-]+$";
static TRexpp id_regex;
static const char* version_regex_string = "^[0-9a-z-\\.]+$";
static TRexpp version_regex;

static bool has_compiled_regex = false;

int ModInstance::sid_counter = 1;

static const char* ModValidityString(const ModValidity& value) {
    if (ModInstance::kValidityValid.Intersects(value)) {
        return "kValidityValid";
    }
    if (ModInstance::kValidityBrokenXml.Intersects(value)) {
        return "kValidityBrokenXml";
    }
    if (ModInstance::kValidityInvalidVersion.Intersects(value)) {
        return "kValidityInvalidVersion";
    }
    if (ModInstance::kValidityMissingReadRights.Intersects(value)) {
        return "kValidityMissingReadRights";
    }
    if (ModInstance::kValidityMissingXml.Intersects(value)) {
        return "kValidityMissingXml";
    }
    if (ModInstance::kValidityUnloaded.Intersects(value)) {
        return "kValidityUnloaded";
    }
    if (ModInstance::kValidityUnsupportedOvergrowthVersion.Intersects(value)) {
        return "kValidityUnsupportedOvergrowthVersion";
    }
    if (ModInstance::kValidityNoDataOnDisk.Intersects(value)) {
        return "kValidityNoDataOnDisk";
    }
    if (ModInstance::kValiditySteamworksError.Intersects(value)) {
        return "kValiditySteamworksError";
    }
    if (ModInstance::kValidityNotInstalled.Intersects(value)) {
        return "kValidityNotInstalled";
    }
    if (ModInstance::kValidityNotSubscribed.Intersects(value)) {
        return "kValidityNotSubscribed";
    }
    if (ModInstance::kValidityIDTooLong.Intersects(value)) {
        return "kValidityIDTooLong";
    }
    if (ModInstance::kValidityIDMissing.Intersects(value)) {
        return "kValidityIDMissing";
    }
    if (ModInstance::kValidityIDInvalid.Intersects(value)) {
        return "kValidityIDInvalid";
    }
    if (ModInstance::kValidityNameTooLong.Intersects(value)) {
        return "kValidityNameTooLong";
    }
    if (ModInstance::kValidityVersionTooLong.Intersects(value)) {
        return "kValidityVersionTooLong";
    }
    if (ModInstance::kValidityAuthorTooLong.Intersects(value)) {
        return "kValidityAuthorTooLong";
    }
    if (ModInstance::kValidityDescriptionTooLong.Intersects(value)) {
        return "kValidityDescriptionTooLong";
    }
    if (ModInstance::kValidityThumbnailMaxLength.Intersects(value)) {
        return "kValidityThumbnailMaxLength";
    }
    if (ModInstance::kValidityTagsListTooLong.Intersects(value)) {
        return "kValidityTagsListTooLong";
    }
    if (ModInstance::kValidityMenuItemTitleTooLong.Intersects(value)) {
        return "kValidityMenuItemTitleTooLong";
    }
    if (ModInstance::kValidityMenuItemTitleMissing.Intersects(value)) {
        return "kValidityMenuItemTitleMissing";
    }
    if (ModInstance::kValidityMenuItemCategoryTooLong.Intersects(value)) {
        return "kValidityMenuItemCategoryTooLong";
    }
    if (ModInstance::kValidityMenuItemCategoryMissing.Intersects(value)) {
        return "kValidityMenuItemCategoryMissing";
    }
    if (ModInstance::kValidityMenuItemPathTooLong.Intersects(value)) {
        return "kValidityMenuItemPathTooLong";
    }
    if (ModInstance::kValidityMenuItemPathMissing.Intersects(value)) {
        return "kValidityMenuItemPathMissing";
    }
    if (ModInstance::kValidityMenuItemPathInvalid.Intersects(value)) {
        return "kValidityMenuItemPathInvalid";
    }
    if (ModInstance::kValidityInvalidTag.Intersects(value)) {
        return "kValidityInvalidTag";
    }
    if (ModInstance::kValidityIDCollision.Intersects(value)) {
        return "kValidityIDCollision";
    }
    if (ModInstance::kValidityActiveModCollision.Intersects(value)) {
        return "kValidityActiveModCollision";
    }
    if (ModInstance::kValidityMissingName.Intersects(value)) {
        return "kValidityMissingName";
    }
    if (ModInstance::kValidityMissingAuthor.Intersects(value)) {
        return "kValidityMissingAuthor";
    }
    if (ModInstance::kValidityMissingDescription.Intersects(value)) {
        return "kValidityMissingDescription";
    }
    if (ModInstance::kValidityMissingThumbnail.Intersects(value)) {
        return "kValidityMissingThumbnail";
    }
    if (ModInstance::kValidityMissingThumbnailFile.Intersects(value)) {
        return "kValidityMissingThumbnailFile";
    }
    if (ModInstance::kValidityMissingVersion.Intersects(value)) {
        return "kValidityMissingVersion";
    }
    if (ModInstance::kValidityCampaignTitleTooLong.Intersects(value)) {
        return "kValidityCampaignTitleTooLong";
    }
    if (ModInstance::kValidityCampaignTitleMissing.Intersects(value)) {
        return "kValidityCampaignTitleMissing";
    }
    if (ModInstance::kValidityCampaignTypeTooLong.Intersects(value)) {
        return "kValidityCampaignTypeTooLong";
    }
    if (ModInstance::kValidityCampaignTypeMissing.Intersects(value)) {
        return "kValidityCampaignTypeMissing";
    }
    if (ModInstance::kValidityCampaignIsLinearInvalidEnum.Intersects(value)) {
        return "kValidityCampaignIsLinearInvalidEnum";
    }
    if (ModInstance::kValidityLevelTitleTooLong.Intersects(value)) {
        return "kValidityLevelTitleTooLong";
    }
    if (ModInstance::kValidityLevelTitleMissing.Intersects(value)) {
        return "kValidityLevelTitleMissing";
    }
    if (ModInstance::kValidityLevelPathTooLong.Intersects(value)) {
        return "kValidityLevelPathTooLong";
    }
    if (ModInstance::kValidityLevelPathMissing.Intersects(value)) {
        return "kValidityLevelPathMissing";
    }
    if (ModInstance::kValidityLevelPathInvalid.Intersects(value)) {
        return "kValidityLevelPathInvalid";
    }
    if (ModInstance::kValidityLevelThumbnailTooLong.Intersects(value)) {
        return "kValidityLevelThumbnailTooLong";
    }
    if (ModInstance::kValidityLevelThumbnailMissing.Intersects(value)) {
        return "kValidityLevelThumbnailMissing";
    }
    if (ModInstance::kValidityLevelThumbnailInvalid.Intersects(value)) {
        return "kValidityLevelThumbnailInvalid";
    }
    if (ModInstance::kValidityInvalidPreviewImage.Intersects(value)) {
        return "kValidityInvalidPreviewImage";
    }
    if (ModInstance::kValidityInvalidSupportedVersion.Intersects(value)) {
        return "kValidityInvalidSupportedVersion";
    }
    if (ModInstance::kValidityInvalidLevelHookFile.Intersects(value)) {
        return "kValidityInvalidLevelHookFile";
    }
    if (ModInstance::kValidityInvalidNeedRestart.Intersects(value)) {
        return "kValidityInvalidNeedRestart";
    }
    if (ModInstance::kValidityItemTitleTooLong.Intersects(value)) {
        return "kValidityItemTitleTooLong";
    }
    if (ModInstance::kValidityItemTitleMissing.Intersects(value)) {
        return "kValidityItemTitleMissing";
    }
    if (ModInstance::kValidityItemCategoryTooLong.Intersects(value)) {
        return "kValidityItemCategoryTooLong";
    }
    if (ModInstance::kValidityItemCategoryMissing.Intersects(value)) {
        return "kValidityItemCategoryMissing";
    }
    if (ModInstance::kValidityItemPathTooLong.Intersects(value)) {
        return "kValidityItemPathTooLong";
    }
    if (ModInstance::kValidityItemPathMissing.Intersects(value)) {
        return "kValidityItemPathMissing";
    }
    if (ModInstance::kValidityItemPathFileMissing.Intersects(value)) {
        return "kValidityItemPathFileMissing";
    }
    if (ModInstance::kValidityItemThumbnailTooLong.Intersects(value)) {
        return "kValidityItemThumbnailTooLong";
    }
    if (ModInstance::kValidityItemThumbnailMissing.Intersects(value)) {
        return "kValidityItemThumbnailMissing";
    }
    if (ModInstance::kValidityItemThumbnailFileMissing.Intersects(value)) {
        return "kValidityItemThumbnailFileMissing";
    }
    if (ModInstance::kValidityCategoryTooLong.Intersects(value)) {
        return "kValidityCategoryTooLong";
    }
    if (ModInstance::kValidityCategoryMissing.Intersects(value)) {
        return "kValidityCategoryMissing";
    }
    if (ModInstance::kValidityCampaignThumbnailMissing.Intersects(value)) {
        return "kValidityCampaignThumbnailMissing";
    }
    if (ModInstance::kValidityCampaignThumbnailTooLong.Intersects(value)) {
        return "kValidityCampaignThumbnailTooLong";
    }
    if (ModInstance::kValidityCampaignThumbnailInvalid.Intersects(value)) {
        return "kValidityCampaignThumbnailInvalid";
    }
    if (ModInstance::kValidityMenuItemThumbnailTooLong.Intersects(value)) {
        return "kValidityMenuItemThumbnailTooLong";
    }
    if (ModInstance::kValidityMenuItemThumbnailInvalid.Intersects(value)) {
        return "kValidityMenuItemThumbnailInvalid";
    }
    if (ModInstance::kValidityCampaignMenuMusicTooLong.Intersects(value)) {
        return "kValidityCampaignMenuMusicTooLong";
    }
    if (ModInstance::kValidityCampaignMenuMusicMissing.Intersects(value)) {
        return "kValidityCampaignMenuMusicMissing";
    }
    if (ModInstance::kValidityCampaignMenuScriptTooLong.Intersects(value)) {
        return "kValidityCampaignMenuScriptTooLong";
    }
    if (ModInstance::kValidityMainScriptTooLong.Intersects(value)) {
        return "kValidityMainScriptTooLong";
    }
    if (ModInstance::kValidityAttributeIdTooLong.Intersects(value)) {
        return "kValidityAttributeIdTooLong";
    }
    if (ModInstance::kValidityAttributeIdInvalid.Intersects(value)) {
        return "kValidityAttributeIdInvalid";
    }
    if (ModInstance::kValidityAttributeValueTooLong.Intersects(value)) {
        return "kValidityAttributeValueTooLong";
    }
    if (ModInstance::kValidityAttributeValueInvalid.Intersects(value)) {
        return "kValidityAttributeValueInvalid";
    }
    if (ModInstance::kValidityLevelIdTooLong.Intersects(value)) {
        return "kValidityLevelIdTooLong";
    }
    if (ModInstance::kValidityCampaignIDTooLong.Intersects(value)) {
        return "kValidityCampaignIDTooLong";
    }
    if (ModInstance::kValidityCampaignIDMissing.Intersects(value)) {
        return "kValidityCampaignIDMissing";
    }
    if (ModInstance::kValidityLevelParameterNameTooLong.Intersects(value)) {
        return "kValidityLevelParameterNameTooLong";
    }
    if (ModInstance::kValidityLevelParameterNameMissing.Intersects(value)) {
        return "kValidityLevelParameterNameMissing";
    }
    if (ModInstance::kValidityLevelParameterValueTooLong.Intersects(value)) {
        return "kValidityLevelParameterValueTooLong";
    }
    if (ModInstance::kValidityLevelParameterValueMissing.Intersects(value)) {
        return "kValidityLevelParameterValueMissing";
    }
    if (ModInstance::kValidityLevelParameterTypeTooLong.Intersects(value)) {
        return "kValidityLevelParameterTypeTooLong";
    }
    return "UNKNOWN";
}

const ModValidity ModInstance::kValidityValid = ModValidity();
const ModValidity ModInstance::kValidityBrokenXml = ModValidity(0);
const ModValidity ModInstance::kValidityInvalidVersion = ModValidity(1);
const ModValidity ModInstance::kValidityMissingReadRights = ModValidity(2);
const ModValidity ModInstance::kValidityMissingXml = ModValidity(3);
const ModValidity ModInstance::kValidityUnloaded = ModValidity(4);
const ModValidity ModInstance::kValidityUnsupportedOvergrowthVersion = ModValidity(5);
const ModValidity ModInstance::kValidityNoDataOnDisk = ModValidity(6);
const ModValidity ModInstance::kValiditySteamworksError = ModValidity(7);
const ModValidity ModInstance::kValidityNotInstalled = ModValidity(8);
const ModValidity ModInstance::kValidityNotSubscribed = ModValidity(9);

const ModValidity ModInstance::kValidityIDTooLong = ModValidity(10);
const ModValidity ModInstance::kValidityIDMissing = ModValidity(11);
const ModValidity ModInstance::kValidityIDInvalid = ModValidity(12);

const ModValidity ModInstance::kValidityNameTooLong = ModValidity(13);
const ModValidity ModInstance::kValidityVersionTooLong = ModValidity(14);
const ModValidity ModInstance::kValidityAuthorTooLong = ModValidity(15);

const ModValidity ModInstance::kValidityDescriptionTooLong = ModValidity(16);
const ModValidity ModInstance::kValidityThumbnailMaxLength = ModValidity(17);
const ModValidity ModInstance::kValidityTagsListTooLong = ModValidity(18);

const ModValidity ModInstance::kValidityMenuItemTitleTooLong = ModValidity(19);
const ModValidity ModInstance::kValidityMenuItemTitleMissing = ModValidity(20);

const ModValidity ModInstance::kValidityMenuItemCategoryTooLong = ModValidity(21);
const ModValidity ModInstance::kValidityMenuItemCategoryMissing = ModValidity(22);

const ModValidity ModInstance::kValidityMenuItemPathTooLong = ModValidity(23);
const ModValidity ModInstance::kValidityMenuItemPathMissing = ModValidity(24);
const ModValidity ModInstance::kValidityMenuItemPathInvalid = ModValidity(25);

const ModValidity ModInstance::kValidityInvalidTag = ModValidity(26);

const ModValidity ModInstance::kValidityIDCollision = ModValidity(27);
const ModValidity ModInstance::kValidityActiveModCollision = ModValidity(28);

const ModValidity ModInstance::kValidityMissingName = ModValidity(29);
const ModValidity ModInstance::kValidityMissingAuthor = ModValidity(30);
const ModValidity ModInstance::kValidityMissingDescription = ModValidity(31);
const ModValidity ModInstance::kValidityMissingThumbnail = ModValidity(32);
const ModValidity ModInstance::kValidityMissingThumbnailFile = ModValidity(33);
const ModValidity ModInstance::kValidityMissingVersion = ModValidity(34);

const ModValidity ModInstance::kValidityCampaignTitleTooLong = ModValidity(35);
const ModValidity ModInstance::kValidityCampaignTitleMissing = ModValidity(36);

const ModValidity ModInstance::kValidityCampaignTypeTooLong = ModValidity(37);
const ModValidity ModInstance::kValidityCampaignTypeMissing = ModValidity(38);

const ModValidity ModInstance::kValidityCampaignIsLinearInvalidEnum = ModValidity(39);

const ModValidity ModInstance::kValidityLevelTitleTooLong = ModValidity(40);
const ModValidity ModInstance::kValidityLevelTitleMissing = ModValidity(41);

const ModValidity ModInstance::kValidityLevelPathTooLong = ModValidity(42);
const ModValidity ModInstance::kValidityLevelPathMissing = ModValidity(43);
const ModValidity ModInstance::kValidityLevelPathInvalid = ModValidity(44);

const ModValidity ModInstance::kValidityLevelThumbnailTooLong = ModValidity(45);
const ModValidity ModInstance::kValidityLevelThumbnailMissing = ModValidity(46);
const ModValidity ModInstance::kValidityLevelThumbnailInvalid = ModValidity(47);
const ModValidity ModInstance::kValidityInvalidPreviewImage = ModValidity(48);
const ModValidity ModInstance::kValidityInvalidSupportedVersion = ModValidity(49);
const ModValidity ModInstance::kValidityInvalidLevelHookFile = ModValidity(50);
const ModValidity ModInstance::kValidityInvalidNeedRestart = ModValidity(51);

const ModValidity ModInstance::kValidityItemTitleTooLong = ModValidity(52);
const ModValidity ModInstance::kValidityItemTitleMissing = ModValidity(53);

const ModValidity ModInstance::kValidityItemCategoryTooLong = ModValidity(54);
const ModValidity ModInstance::kValidityItemCategoryMissing = ModValidity(55);

const ModValidity ModInstance::kValidityItemPathTooLong = ModValidity(56);
const ModValidity ModInstance::kValidityItemPathMissing = ModValidity(57);
const ModValidity ModInstance::kValidityItemPathFileMissing = ModValidity(58);

const ModValidity ModInstance::kValidityItemThumbnailTooLong = ModValidity(59);
const ModValidity ModInstance::kValidityItemThumbnailMissing = ModValidity(60);
const ModValidity ModInstance::kValidityItemThumbnailFileMissing = ModValidity(61);

const ModValidity ModInstance::kValidityCategoryTooLong = ModValidity(62);
const ModValidity ModInstance::kValidityCategoryMissing = ModValidity(63);

const ModValidity ModInstance::kValidityCampaignThumbnailMissing = ModValidity(64);
const ModValidity ModInstance::kValidityCampaignThumbnailTooLong = ModValidity(65);
const ModValidity ModInstance::kValidityCampaignThumbnailInvalid = ModValidity(66);

const ModValidity ModInstance::kValidityMenuItemThumbnailTooLong = ModValidity(67);
const ModValidity ModInstance::kValidityMenuItemThumbnailInvalid = ModValidity(68);

const ModValidity ModInstance::kValidityCampaignMenuMusicTooLong = ModValidity(69);
const ModValidity ModInstance::kValidityCampaignMenuMusicMissing = ModValidity(70);

const ModValidity ModInstance::kValidityCampaignMenuScriptTooLong = ModValidity(71);
const ModValidity ModInstance::kValidityMainScriptTooLong = ModValidity(72);

const ModValidity ModInstance::kValidityAttributeIdTooLong = ModValidity(73);
const ModValidity ModInstance::kValidityAttributeIdInvalid = ModValidity(74);

const ModValidity ModInstance::kValidityAttributeValueTooLong = ModValidity(75);
const ModValidity ModInstance::kValidityAttributeValueInvalid = ModValidity(76);

const ModValidity ModInstance::kValidityLevelIdTooLong = ModValidity(77);

const ModValidity ModInstance::kValidityCampaignIDTooLong = ModValidity(78);
const ModValidity ModInstance::kValidityCampaignIDMissing = ModValidity(79);

const ModValidity ModInstance::kValidityLevelParameterNameTooLong = ModValidity(80);
const ModValidity ModInstance::kValidityLevelParameterNameMissing = ModValidity(81);

const ModValidity ModInstance::kValidityLevelParameterValueTooLong = ModValidity(82);
const ModValidity ModInstance::kValidityLevelParameterValueMissing = ModValidity(83);

const ModValidity ModInstance::kValidityLevelParameterTypeTooLong = ModValidity(84);

const ModValidity ModInstance::kValidityPoseNameTooLong = ModValidity(85);
const ModValidity ModInstance::kValidityPoseNameMissing = ModValidity(86);
const ModValidity ModInstance::kValidityPosePathTooLong = ModValidity(87);
const ModValidity ModInstance::kValidityPosePathMissing = ModValidity(88);
const ModValidity ModInstance::kValidityPoseCommandTooLong = ModValidity(89);
const ModValidity ModInstance::kValidityPoseCommandMissing = ModValidity(90);

const ModValidity ModInstance::kValidityUploadSteamworksBlockingMask =
    ~ModValidity() & ~ModInstance::kValidityIDCollision & ~ModInstance::kValidityActiveModCollision;

const ModValidity ModInstance::kValidityActivationBreakingErrorMask =
    ModInstance::kValidityBrokenXml | ModInstance::kValidityInvalidVersion | ModInstance::kValidityMissingReadRights | ModInstance::kValidityMissingXml | ModInstance::kValidityNoDataOnDisk | ModInstance::kValiditySteamworksError | ModInstance::kValidityNotInstalled | ModInstance::kValidityNotSubscribed | ModInstance::kValidityIDTooLong | ModInstance::kValidityIDMissing | ModInstance::kValidityIDInvalid | ModInstance::kValidityNameTooLong | ModInstance::kValidityVersionTooLong | ModInstance::kValidityAuthorTooLong | ModInstance::kValidityDescriptionTooLong | ModInstance::kValidityTagsListTooLong | ModInstance::kValidityMenuItemTitleTooLong | ModInstance::kValidityMenuItemCategoryTooLong | ModInstance::kValidityMenuItemPathTooLong | ModInstance::kValidityIDCollision | ModInstance::kValidityActiveModCollision | ModInstance::kValidityMissingName | ModInstance::kValidityMissingAuthor | ModInstance::kValidityMissingDescription | ModInstance::kValidityMissingVersion | ModInstance::kValidityCampaignTitleTooLong | ModInstance::kValidityCampaignTypeTooLong | ModInstance::kValidityCampaignIsLinearInvalidEnum | ModInstance::kValidityCampaignMenuMusicTooLong | ModInstance::kValidityLevelTitleTooLong | ModInstance::kValidityLevelPathTooLong | ModInstance::kValidityLevelThumbnailTooLong | ModInstance::kValidityInvalidSupportedVersion | ModInstance::kValidityItemTitleTooLong | ModInstance::kValidityItemCategoryTooLong | ModInstance::kValidityItemPathTooLong | ModInstance::kValidityItemThumbnailTooLong | ModInstance::kValidityCampaignMenuScriptTooLong | ModInstance::kValidityMainScriptTooLong | ModInstance::kValidityAttributeIdTooLong | ModInstance::kValidityAttributeIdInvalid | ModInstance::kValidityAttributeValueTooLong | ModInstance::kValidityAttributeValueInvalid | ModInstance::kValidityLevelIdTooLong | ModInstance::kValidityCampaignIDTooLong | ModInstance::kValidityCampaignIDMissing | ModInstance::kValidityLevelParameterNameTooLong | ModInstance::kValidityLevelParameterNameMissing | ModInstance::kValidityLevelParameterValueTooLong | ModInstance::kValidityLevelParameterValueMissing | ModInstance::kValidityLevelParameterTypeTooLong | ModInstance::kValidityPoseNameTooLong | ModInstance::kValidityPoseNameMissing | ModInstance::kValidityPosePathTooLong | ModInstance::kValidityPosePathMissing | ModInstance::kValidityPoseCommandTooLong | ModInstance::kValidityPoseCommandMissing;

ModInstance::Parameter::Parameter() : type("empty") {
}

ModInstance::ModInstance(const std::string& _path) : valid(kValidityUnloaded),
                                                     active(false),
                                                     modsource(ModSourceLocalModFolder),
                                                     sid(sid_counter++) {
    if (!has_compiled_regex) {
        menu_item_id_regex.Compile(menu_item_id_regex_string);
        id_regex.Compile(id_regex_string);
        version_regex.Compile(version_regex_string);
        has_compiled_regex = true;
    }
    path = _path;

    has_been_activated = false;

    Reload();
}

ModInstance::ModInstance(const UGCID _ugc_id) : valid(kValidityUnloaded),
                                                active(false),
                                                modsource(ModSourceSteamworks),
                                                ugc_id(_ugc_id),
                                                sid(sid_counter++) {
    LOGI << "Creating with ugc_id: " << _ugc_id << std::endl;
    if (!has_compiled_regex) {
        menu_item_id_regex.Compile(menu_item_id_regex_string);
        id_regex.Compile(id_regex_string);
        version_regex.Compile(version_regex_string);
        has_compiled_regex = true;
    }

    has_been_activated = false;
}

void ModInstance::PurgeActiveSettings() {
    ModLoading::Instance().acp.RemoveModInstance(id, modsource);
}

void ModInstance::ParseLevel(ModInstance::Level* level, TiXmlElement* pElemLevel, uint32_t fallback_id) {
    int err = level->title.set(pElemLevel->Attribute("title"));
    if (err == SOURCE_TOO_LONG) {
        PARSE_ERROR(pElemLevel, kValidityLevelTitleTooLong);
    } else if (err == SOURCE_IS_NULL) {
        PARSE_ERROR(pElemLevel, kValidityLevelTitleMissing);
    }

    err = level->id.set(pElemLevel->Attribute("id"));
    if (err == SOURCE_TOO_LONG) {
        PARSE_ERROR(pElemLevel, kValidityLevelIdTooLong);
    } else if (err == SOURCE_IS_NULL) {
        char temp[MOD_LEVEL_ID_MAX_LENGTH];
        FormatString(temp, MOD_LEVEL_ID_MAX_LENGTH, "%u", fallback_id);
        temp[MOD_LEVEL_ID_MAX_LENGTH - 1] = '\0';
        level->id.set(temp);
    }

    err = level->thumbnail.set(pElemLevel->Attribute("thumbnail"));
    if (err == SOURCE_TOO_LONG) {
        PARSE_ERROR(pElemLevel, kValidityLevelThumbnailTooLong);
    } else if (err == SOURCE_IS_NULL) {
        level->thumbnail.set("Images/thumb_fallback.png");
    } else if (strmtch(level->thumbnail, "")) {
        level->thumbnail.set("Images/thumb_fallback.png");
    }  // else if( FileExists(AssemblePath(path,AssemblePath("Data",level->thumbnail)),kAbsPath) == false && FileExists(AssemblePath("Data",level->thumbnail),kDataPaths) == false) {
       //   PARSE_ERROR2(pElemLevel, kValidityLevelThumbnailInvalid, level->thumbnail);
    //}

    const char* supports_online = pElemLevel->Attribute("supports_online");
    level->supports_online = false;

    if (supports_online != NULL) {
        if (strcmp(supports_online, "true") == 0) {
            level->supports_online = true;
        }
    }

    const char* requires_online = pElemLevel->Attribute("requires_online");
    level->requires_online = false;

    if (requires_online != NULL) {
        if (strcmp(requires_online, "true") == 0) {
            level->requires_online = true;
        }
    }

    const char* optional = pElemLevel->Attribute("completion_optional");
    level->completion_optional = false;
    if (optional != NULL) {
        if (strcmp(optional, "true") == 0) {
            level->completion_optional = true;
        } else if (strcmp(optional, "false") != 0) {
            LOGE << "Error when parsing " << path << "/mod.xml: invalid completion_optional value (can only be true/false)" << std::endl;
            PARSE_ERROR(NULL, kValidityBrokenXml);
        }
    }

    if (pElemLevel->Attribute("path")) {  // Check if we're using the modern Level or the old level->format
        err = level->path.set(pElemLevel->Attribute("path"));
        if (err == SOURCE_TOO_LONG) {
            PARSE_ERROR(pElemLevel, kValidityLevelPathTooLong);
        } else if (err == SOURCE_IS_NULL) {
            PARSE_ERROR(pElemLevel, kValidityLevelPathMissing);
        }  // else if( FileExists( AssemblePath( path, AssemblePath( "Data/Levels", level->path ) ), kAbsPath ) == false && FileExists(AssemblePath( "Data/Levels", level->path ), kDataPaths) == false ) {
           //   PARSE_ERROR2(pElemLevel, kValidityLevelPathInvalid, level->path);
        //}

        TiXmlElement* pElemParameter = pElemLevel->FirstChildElement("Parameter");
        if (pElemParameter) {
            ParseParameter(&level->parameter, pElemParameter, "");
        } else {
            level->parameter.type.set("empty");
        }
    } else {  // This is the old format, where the body of the <Level> is the path.
        err = level->path.set(pElemLevel->GetText());
        if (err == SOURCE_TOO_LONG) {
            PARSE_ERROR(pElemLevel, kValidityLevelPathTooLong);
        } else if (err == SOURCE_IS_NULL) {
            PARSE_ERROR(pElemLevel, kValidityLevelPathMissing);
        }  // else if( FileExists(AssemblePath(path, AssemblePath("Data/Levels", level->path)), kAbsPath) == false && FileExists(AssemblePath("Data/Levels", level->path), kDataPaths) == false) {
           //   PARSE_ERROR2(pElemLevel, kValidityLevelPathInvalid, level->path);
        //}

        level->parameter.type.set("empty");
    }
}

void ModInstance::ParseParameter(ModInstance::Parameter* parameter, TiXmlElement* pElemParameter, const char* parent_type) {
    int err = parameter->type.set(pElemParameter->Attribute("type"));
    if (err == SOURCE_TOO_LONG) {
        PARSE_ERROR(pElemParameter, kValidityLevelParameterTypeTooLong);
    } else if (err == SOURCE_IS_NULL) {
        parameter->type.set("string");
    } else if (strmtch(parameter->type, "")) {
        parameter->type.set("string");
    }

    err = parameter->name.set(pElemParameter->Attribute("name"));
    if (err == SOURCE_TOO_LONG) {
        PARSE_ERROR(pElemParameter, kValidityLevelParameterNameTooLong);
    } else if (err == SOURCE_IS_NULL) {
        if (strmtch(parent_type, "array") == false) {
            PARSE_ERROR(pElemParameter, kValidityLevelParameterNameMissing);
        }
    }

    err = parameter->value.set(pElemParameter->Attribute("value"));
    if (err == SOURCE_TOO_LONG) {
        PARSE_ERROR(pElemParameter, kValidityLevelParameterValueTooLong);
    } else if (err == SOURCE_IS_NULL) {
        if (strmtch(parameter->type, "array") == false && strmtch(parameter->type, "table") == false) {
            PARSE_ERROR(pElemParameter, kValidityLevelParameterValueMissing);
        }
    }

    TiXmlElement* pElemParameterSub = pElemParameter->FirstChildElement("Parameter");

    while (pElemParameterSub) {
        parameter->parameters.resize(parameter->parameters.size() + 1);
        ParseParameter(&parameter->parameters[parameter->parameters.size() - 1], pElemParameterSub, parameter->type);
        pElemParameterSub = pElemParameterSub->NextSiblingElement("Parameter");
    }
}

void ModInstance::Reload() {
    LOGD << "Reloading mod data" << std::endl;
    valid = kValidityValid;

    bool load_from_disk = false;

    id.set("");
    name.set("");
    category.set("");
    version.set("");
    author.set("");
    description.set("");
    thumbnail.set("");

    campaigns.clear();
    tags.clear();
    preview_images.clear();
    supported_versions.clear();
    level_hook_files.clear();
    mod_dependencies.clear();
    main_menu_items.clear();
    items.clear();
    levels.clear();
    invalid_item_paths.clear();
    poses.clear();

    overload_files.clear();
    manifest.clear();

    if (modsource == ModSourceSteamworks) {
#if ENABLE_STEAMWORKS
        SteamworksUGC* ugc = Steamworks::Instance()->GetUGC();
        if (ugc) {
            std::vector<SteamworksUGCItem*>::iterator ugc_it = ugc->GetItem(ugc_id);
            if (ugc_it != ugc->GetItemEnd()) {
                if ((*ugc_it)->IsSubscribed()) {
                    if ((*ugc_it)->IsInstalled()) {
                        path = (*ugc_it)->GetPath();
                        load_from_disk = true;
                    } else {
                        PARSE_ERROR(NULL, kValidityNotInstalled);
                    }
                } else {
                    PARSE_ERROR(NULL, kValidityNotSubscribed);
                }

                LOGD << "Getting base data from ugc" << std::endl;
                id.set((*ugc_it)->id);
                version.set((*ugc_it)->version);
                name.set((*ugc_it)->name);
                author.set((*ugc_it)->author);
                description.set((*ugc_it)->description);
                append(tags, (*ugc_it)->tags);
            } else {
                LOGE << "Unable to find mod item with ugcid: " << ugc_id << std::endl;
                PARSE_ERROR(NULL, kValiditySteamworksError);
            }
        } else {
            LOGE << "Somehow i have a steamworks mod loaded, but no valid UGC instance. This is unexpected, unable to reload mod" << std::endl;
            PARSE_ERROR(NULL, kValiditySteamworksError);
        }
#else
        LOGE << "Game was not compiled with Steamworks support." << std::endl;
        PARSE_ERROR(NULL, kValiditySteamworksError);
#endif
    } else if (modsource == ModSourceLocalModFolder) {
        load_from_disk = true;
    }

    if (load_from_disk) {
        std::string modxmlpath = path + "/mod.xml";

        GenerateManifest(path.c_str(), manifest);

        // We need to sort to allow intersectiontests
        std::sort(manifest.begin(), manifest.end());

        if (fileExist(modxmlpath.c_str())) {
            int err = 0;

            if (fileReadable(modxmlpath.c_str())) {
                TiXmlDocument doc(modxmlpath.c_str());
                doc.LoadFile();

                if (doc.Error()) {
                    LOGE << "Error when parsing " << path << "/mod.xml: " << doc.ErrorDesc() << std::endl;
                    PARSE_ERROR(NULL, kValidityBrokenXml);
                }

                TiXmlHandle hDoc(&doc);
                TiXmlElement* pElem;
                TiXmlHandle hRoot(0);

                pElem = hDoc.FirstChildElement().Element();
                if (!pElem) {
                    LOGE << "Unable to load mod.xml " << modxmlpath << " broken xml file." << std::endl;
                    PARSE_ERROR(NULL, kValidityBrokenXml);
                } else {
                    hRoot = TiXmlHandle(pElem);

                    pElem = hRoot.FirstChild("Id").Element();
                    if (pElem) {
                        err = id.set(pElem->GetText());

                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityIDTooLong);
                        } else if (err == SOURCE_IS_NULL) {
                            PARSE_ERROR(pElem, kValidityIDMissing);
                        } else if (false == id_regex.Match(id)) {
                            LOGE << "Mod name: " << id << " is illegal, mod is invalid." << std::endl;
                            PARSE_ERROR(pElem, kValidityIDInvalid);
                        }
                    } else {
                        PARSE_ERROR(pElem, kValidityIDMissing);
                    }

                    pElem = hRoot.FirstChild("Name").Element();
                    if (pElem) {
                        LOGD << "Getting name from xml" << std::endl;

                        err = name.set(pElem->GetText());

                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityNameTooLong);
                        } else if (err == SOURCE_IS_NULL) {
                            PARSE_ERROR(pElem, kValidityMissingName);
                        }
                    } else {
                        PARSE_ERROR(pElem, kValidityMissingName);
                    }

                    pElem = hRoot.FirstChild("Category").Element();
                    if (pElem) {
                        LOGD << "Getting category from xml" << std::endl;

                        err = category.set(pElem->GetText());

                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityCategoryTooLong);
                        } else if (err == SOURCE_IS_NULL) {
                            PARSE_ERROR(pElem, kValidityCategoryMissing);
                        }
                    } else {
                        PARSE_ERROR(pElem, kValidityCategoryMissing);
                    }

                    pElem = hRoot.FirstChild("Author").Element();
                    if (pElem) {
                        err = author.set(pElem->GetText());
                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityAuthorTooLong);
                        } else if (err == SOURCE_IS_NULL) {
                            PARSE_ERROR(pElem, kValidityMissingAuthor);
                        }
                    } else {
                        PARSE_ERROR(pElem, kValidityMissingAuthor);
                    }

                    pElem = hRoot.FirstChild("Description").Element();

                    if (pElem) {
                        err = description.set(pElem->GetText());

                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityDescriptionTooLong);
                        } else if (err == SOURCE_IS_NULL) {
                            PARSE_ERROR(pElem, kValidityMissingDescription);
                        }
                    } else {
                        PARSE_ERROR(pElem, kValidityMissingDescription);
                    }

                    pElem = hRoot.FirstChild("Version").Element();
                    if (pElem) {
                        err = version.set(pElem->GetText());

                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityVersionTooLong);
                        } else if (err == SOURCE_IS_NULL) {
                            PARSE_ERROR(pElem, kValidityMissingVersion);
                        } else if (!version_regex.Match(version)) {
                            LOGE << "Mod version: " << version << " is illegal, mod is invalid" << std::endl;
                            PARSE_ERROR(pElem, kValidityInvalidVersion);
                        }
                    } else {
                        PARSE_ERROR(pElem, kValidityMissingVersion);
                    }

                    pElem = hRoot.FirstChild("Thumbnail").Element();
                    if (pElem) {
                        err = thumbnail.set(pElem->GetText());
                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityThumbnailMaxLength);
                        } else if (err == SOURCE_IS_NULL) {
                            PARSE_ERROR(pElem, kValidityMissingThumbnail);
                        } else {
                            Path thumbpath = FindFilePath(AssemblePath(path.c_str(), thumbnail.c_str()).c_str(), kAbsPath, true);
                            if (thumbpath.valid == false) {
                                PARSE_ERROR(pElem, kValidityMissingThumbnailFile);
                            }
                        }
                    } else {
                        PARSE_ERROR(pElem, kValidityMissingThumbnail);
                    }

                    pElem = hRoot.FirstChild("PreviewImage").Element();
                    while (pElem) {
                        if (pElem->GetText()) {
                            preview_images.push_back(pElem->GetText());
                        } else {
                            LOGE << "Got a <PreviewImage> with invalid body" << std::endl;
                            PARSE_ERROR(pElem, kValidityInvalidPreviewImage);
                        }
                        pElem = pElem->NextSiblingElement("PreviewImage");
                    }

                    pElem = hRoot.FirstChild("Tag").Element();
                    while (pElem) {
                        const char* tag = pElem->GetText();
                        if (tag) {
                            if (false == strcont(tag, ",")) {
                                tags.insert(tag);
                            } else {
                                PARSE_ERROR(pElem, kValidityInvalidTag);
                            }

                            LOGD << "Tag: " << tag << std::endl;
                        } else {
                            PARSE_ERROR(pElem, kValidityInvalidTag);
                        }
                        pElem = pElem->NextSiblingElement("Tag");
                    }

                    if (GetTagsListString().size() > (MOD_TAGS_MAX_LENGTH - 1)) {
                        PARSE_ERROR(pElem, kValidityTagsListTooLong);
                    }

                    pElem = hRoot.FirstChild("SupportedVersion").Element();
                    while (pElem) {
                        if (pElem->GetText()) {
                            supported_versions.push_back(pElem->GetText());
                        } else {
                            PARSE_ERROR(pElem, kValidityInvalidSupportedVersion);
                        }
                        LOGD << "SupportedVersion: " << pElem->GetText() << std::endl;
                        pElem = pElem->NextSiblingElement("SupportedVersion");
                    }

                    pElem = hRoot.FirstChild("LevelHookFile").Element();
                    while (pElem) {
                        if (pElem->GetText()) {
                            level_hook_files.push_back(pElem->GetText());
                        } else {
                            PARSE_ERROR(pElem, kValidityInvalidLevelHookFile);
                        }
                        LOGD << "LevelHookFile: " << pElem->GetText() << std::endl;

                        pElem = pElem->NextSiblingElement("LevelHookFile");
                    }

                    pElem = hRoot.FirstChild("MenuItem").Element();
                    while (pElem) {
                        MenuItem mi;

                        err = mi.title.set(pElem->Attribute("title"));
                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityMenuItemTitleTooLong);
                        } else if (err == SOURCE_IS_NULL) {
                            PARSE_ERROR(pElem, kValidityMenuItemTitleMissing);
                        }

                        err = mi.category.set(pElem->Attribute("category"));
                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityMenuItemCategoryTooLong);
                        } else if (err == SOURCE_IS_NULL) {
                            PARSE_ERROR(pElem, kValidityMenuItemCategoryMissing);
                        }

                        err = mi.thumbnail.set(pElem->Attribute("thumbnail"));
                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityMenuItemThumbnailTooLong);
                        } else if (err == SOURCE_IS_NULL) {
                            // We accept this not having a value.
                        }  // else if( FileExists(AssemblePath(path, AssemblePath("Data",mi.thumbnail)),kAbsPath) == false && FileExists(AssemblePath("Data",mi.thumbnail),kDataPaths) == false ) {
                           //   PARSE_ERROR2(pElem, kValidityMenuItemThumbnailInvalid, mi.thumbnail);
                        //}

                        err = mi.path.set(pElem->GetText());
                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityMenuItemPathTooLong);
                        } else if (err == SOURCE_IS_NULL) {
                            PARSE_ERROR(pElem, kValidityMenuItemPathMissing);
                        }

                        main_menu_items.push_back(mi);
                        pElem = pElem->NextSiblingElement("MenuItem");
                    }

                    bool requireID = false;
                    {
                        TiXmlElement* campaignElem = hRoot.FirstChild("Campaign").Element();
                        if (campaignElem) {
                            campaignElem = campaignElem->NextSiblingElement("Campaign");
                            if (campaignElem)
                                requireID = true;
                        }
                    }
                    pElem = hRoot.FirstChild("Campaign").Element();
                    while (pElem) {
                        Campaign campaign;
                        err = campaign.id.set(pElem->Attribute("id"));
                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityCampaignIDTooLong);
                        } else if (err == SOURCE_IS_NULL) {
                            if (requireID)
                                PARSE_ERROR(pElem, kValidityCampaignIDMissing);
                            else
                                campaign.id.set(id);
                        }

                        err = campaign.title.set(pElem->Attribute("title"));
                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityCampaignTitleTooLong);
                        } else if (err == SOURCE_IS_NULL) {
                            PARSE_ERROR(pElem, kValidityCampaignTitleMissing);
                        }

                        err = campaign.thumbnail.set(pElem->Attribute("thumbnail"));
                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityCampaignThumbnailTooLong);
                        } else if (err == SOURCE_IS_NULL) {
                            // We actually accept as being empty.
                            // PARSE_ERROR(pElem, kValidityCampaignThumbnailMissing);
                            campaign.thumbnail.set("Images/thumb_fallback.png");
                        } else if (strmtch(campaign.thumbnail, "")) {
                            campaign.thumbnail.set("Images/thumb_fallback.png");
                        }  // else if( FileExists(AssemblePath(path, AssemblePath("Data",campaign.thumbnail)),kAbsPath) == false && FileExists(AssemblePath("Data",campaign.thumbnail),kDataPaths) == false ) {
                           //   PARSE_ERROR2(pElem, kValidityCampaignThumbnailInvalid, campaign.thumbnail);
                        //}

                        err = campaign.menu_script.set(pElem->Attribute("menu_script"));

                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityCampaignMenuScriptTooLong);
                        } else if (err == SOURCE_IS_NULL) {
                            campaign.menu_script.set("standard_campaign_menu.as");
                        }

                        const char* supports_online = pElem->Attribute("supports_online");
                        campaign.supports_online = false;
                        if (supports_online != NULL) {
                            if (strcmp(supports_online, "true") == 0) {
                                campaign.supports_online = true;
                            }
                        }

                        const char* requires_online = pElem->Attribute("requires_online");
                        campaign.requires_online = false;
                        if (requires_online != NULL) {
                            if (strcmp(requires_online, "true") == 0) {
                                campaign.requires_online = true;
                            }
                        }

                        err = campaign.main_script.set(pElem->Attribute("main_script"));
                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityMainScriptTooLong);
                        } else if (err == SOURCE_IS_NULL) {
                            campaign.main_script.set("campaign/overgrowth_campaign.as");
                        }

                        campaign.parameter.type.set("empty");

                        TiXmlElement* pElemSub = pElem->FirstChildElement();
                        while (pElemSub) {
                            if (strmtch(pElemSub->Value(), "Parameter")) {
                                if (pElemSub) {
                                    ParseParameter(&campaign.parameter, pElemSub, "");
                                }
                            } else if (strmtch(pElemSub->Value(), "Level")) {
                                campaign.levels.resize(campaign.levels.size() + 1);
                                ParseLevel(&campaign.levels[campaign.levels.size() - 1], pElemSub, (unsigned int)campaign.levels.size());
                            } else if (strmtch(pElemSub->Value(), "Attribute")) {
                                Attribute attribute;

                                err = attribute.id.set(pElemSub->Attribute("id"));
                                if (err == SOURCE_TOO_LONG) {
                                    PARSE_ERROR(pElemSub, kValidityAttributeIdTooLong);
                                } else if (err == SOURCE_IS_NULL) {
                                    PARSE_ERROR(pElemSub, kValidityAttributeIdInvalid);
                                }

                                err = attribute.value.set(pElemSub->Attribute("value"));
                                if (err == SOURCE_TOO_LONG) {
                                    PARSE_ERROR(pElemSub, kValidityAttributeValueTooLong);
                                } else if (err == SOURCE_IS_NULL) {
                                    PARSE_ERROR(pElemSub, kValidityAttributeValueInvalid);
                                }

                                campaign.attributes.push_back(attribute);
                            } else {
                                LOGW << "Unknown element name in campaign: " << pElemSub->Value() << " on row " << pElemSub->Row() << std::endl;
                            }

                            pElemSub = pElemSub->NextSiblingElement();
                        }

                        pElem = pElem->NextSiblingElement("Campaign");

                        campaigns.push_back(campaign);
                    }

                    pElem = hRoot.FirstChild("Level").Element();
                    while (pElem) {
                        levels.resize(levels.size() + 1);
                        ParseLevel(&levels[levels.size() - 1], pElem, (unsigned int)levels.size());
                        pElem = pElem->NextSiblingElement("Level");
                    }

                    pElem = hRoot.FirstChild("NeedsRestart").Element();
                    if (pElem) {
                        if (pElem->GetText()) {
                            int v = saysTrue(pElem->GetText());

                            if (v != -1) {
                                needs_restart = (v == 1);
                            } else {
                                LOGW << "NeedsRestart contains invalid value, needs to be true or false, is \"" << pElem->GetText() << "\"" << std::endl;
                                needs_restart = true;
                            }
                        } else {
                            PARSE_ERROR(pElem, kValidityInvalidNeedRestart);
                        }
                    } else {
                        needs_restart = true;
                    }

                    pElem = hRoot.FirstChild("SupportsOnline").Element();
                    if (pElem) {
                        if (pElem->GetText()) {
                            int v = saysTrue(pElem->GetText());

                            if (v != -1) {
                                supports_online = (v == 1);
                            } else {
                                LOGW << "SupportsOnline contains invalid value, needs to be true or false, is \"" << pElem->GetText() << "\"" << std::endl;
                                supports_online = true;
                            }
                        } else {
                            PARSE_ERROR(pElem, kValidityInvalidNeedRestart);
                        }
                    } else {
                        supports_online = false;
                    }

                    pElem = hRoot.FirstChild("ModDependency").Element();
                    while (pElem) {
                        TiXmlElement* pModDepElem;

                        ModDependency moddep;

                        pModDepElem = pElem->FirstChildElement("Id");
                        if (pModDepElem) {
                            if (pModDepElem->GetText()) {
                                moddep.id = pModDepElem->GetText();
                            }
                        }

                        pModDepElem = pElem->FirstChildElement("Version");
                        while (pModDepElem) {
                            if (pModDepElem->GetText()) {
                                moddep.versions.push_back(pModDepElem->GetText());
                            }

                            pModDepElem = pModDepElem->NextSiblingElement("Version");
                        }

                        mod_dependencies.push_back(moddep);

                        pElem = pElem->NextSiblingElement("ModDependency");
                    }

                    pElem = hRoot.FirstChild("OverloadFile").Element();
                    while (pElem) {
                        if (pElem->GetText()) {
                            overload_files.push_back(pElem->GetText());
                        }

                        pElem = pElem->NextSiblingElement("OverloadFile");
                    }

                    pElem = hRoot.FirstChild("Item").Element();
                    while (pElem) {
                        Item item;

                        err = item.title.set(pElem->Attribute("title"));
                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityItemTitleTooLong);
                        } else if (err == SOURCE_IS_NULL) {
                            PARSE_ERROR(pElem, kValidityItemTitleMissing);
                        }

                        err = item.category.set(pElem->Attribute("category"));
                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityItemCategoryTooLong);
                        } else if (err == SOURCE_IS_NULL) {
                            PARSE_ERROR(pElem, kValidityItemCategoryMissing);
                        }

                        err = item.path.set(pElem->Attribute("path"));
                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityItemPathTooLong);
                            invalid_item_paths.push_back(item.path.str());
                        } else if (err == SOURCE_IS_NULL) {
                            PARSE_ERROR(pElem, kValidityItemPathMissing);
                        }  // else if( FileExists( AssemblePath( path.c_str(), item.path.c_str() ), kAbsPath ) == false && FileExists(item.path, kDataPaths) == false) {
                           //   PARSE_ERROR2(pElem, kValidityItemPathFileMissing, item.path);
                           //   invalid_item_paths.push_back(item.path.str());
                        //}

                        err = item.thumbnail.set(pElem->Attribute("thumbnail"));
                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityItemThumbnailTooLong);
                        } else if (err == SOURCE_IS_NULL) {
                            PARSE_ERROR(pElem, kValidityItemThumbnailMissing);
                        }  // else if(
                           // strmtch(item.thumbnail, "") == false
                        // && FileExists( AssemblePath( path.c_str(), item.thumbnail.c_str() ), kAbsPath ) == false
                        // && FileExists(item.thumbnail, kDataPaths) == false) {
                        // PARSE_ERROR2(pElem, kValidityItemThumbnailFileMissing, item.thumbnail);
                        //}

                        pElem = pElem->NextSiblingElement("Item");

                        items.push_back(item);
                    }

                    pElem = hRoot.FirstChild("Pose").Element();
                    while (pElem) {
                        Pose pose;

                        err = pose.name.set(pElem->Attribute("name"));
                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityPoseNameTooLong);
                        } else if (err == SOURCE_IS_NULL) {
                            PARSE_ERROR(pElem, kValidityPoseNameMissing);
                        }

                        err = pose.command.set(pElem->Attribute("command"));
                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityPoseCommandTooLong);
                        } else if (err == SOURCE_IS_NULL) {
                            PARSE_ERROR(pElem, kValidityPoseCommandMissing);
                        }

                        err = pose.path.set(pElem->Attribute("path"));
                        if (err == SOURCE_TOO_LONG) {
                            PARSE_ERROR(pElem, kValidityPosePathTooLong);
                        } else if (err == SOURCE_IS_NULL) {
                            PARSE_ERROR(pElem, kValidityPosePathMissing);
                        }
                        Path animation_path = FindFilePath(pose.path, kAnyPath, true);
                        if (animation_path.valid == false) {
                            LOGE << "Path to animation is invalid " << pose.path << ", file is missing." << std::endl;
                        } else {
                            poses.push_back(pose);
                        }
                        pElem = pElem->NextSiblingElement("Pose");
                    }

                    pElem = hRoot.FirstChild("Languages").FirstChild("Language").Element();
                    while (pElem) {
                        std::string shortcode = pElem->Attribute("shortcode");
                        std::string name = pElem->Attribute("name");
                        const char* local_name = pElem->Attribute("local_name");
                        std::string local_name_str = local_name ? local_name : "";
                        LanguageData new_data;
                        new_data.language = name;
                        new_data.local_language = local_name_str;
                        language_data[shortcode] = new_data;
                        pElem = pElem->NextSiblingElement("Language");
                    }
                }
            } else {
                LOGE << "Unable to load mod.xml " << modxmlpath << ", missing readable rights." << std::endl;
                PARSE_ERROR(NULL, kValidityMissingReadRights);
            }
        } else {
            LOGE << "Unable to load mod.xml " << modxmlpath << ", file is missing." << std::endl;
            PARSE_ERROR(NULL, kValidityMissingXml);
        }

        // Check if mod has files in root folder we don't like.
        for (auto& str : manifest) {
            bool is_root = true;

            for (char k : str) {
                if (k == '/') {
                    is_root = false;
                }
            }

            const static size_t LOC_LENGTH = strlen("Localized/");
            if (memcmp("Localized/", str.c_str(), LOC_LENGTH) == 0) {
                if (str.size() > 9 && strcmp(str.c_str() + str.size() - 9, "_meta.xml") == 0) {
                    size_t second_slash = str.find('/', LOC_LENGTH);

                    std::string shortcode = str.substr(LOC_LENGTH, second_slash - LOC_LENGTH);
                    std::string level = str.substr(second_slash + 1, str.size() - (second_slash + 1));
                    level.replace(level.size() - strlen("_meta.xml"), strlen("_meta.xml"), ".xml");

                    std::string xml_path = path + "/" + str;
                    if (fileExist(xml_path.c_str())) {
                        int err = 0;

                        if (fileReadable(xml_path.c_str())) {
                            TiXmlDocument doc(xml_path.c_str());
                            doc.LoadFile();

                            if (doc.Error()) {
                                LOGE << "Error when parsing " << xml_path << doc.ErrorDesc() << std::endl;
                                PARSE_ERROR(NULL, kValidityBrokenXml);
                            }

                            TiXmlHandle hDoc(&doc);
                            TiXmlElement* pElem;
                            TiXmlHandle hRoot(0);

                            pElem = hDoc.FirstChildElement().Element();
                            if (!pElem) {
                                LOGE << "Unable to load level metadata file " << xml_path << " broken xml file." << std::endl;
                                PARSE_ERROR(NULL, kValidityBrokenXml);
                            } else {
                                hRoot = TiXmlHandle(pElem);
                                pElem = hRoot.FirstChild("LoadTip").Element();
                                if (pElem) {
                                    language_data[shortcode].per_level_data[level].loading_screen_tip = pElem->GetText();
                                }
                                pElem = hRoot.FirstChild("Title").Element();
                                if (pElem) {
                                    language_data[shortcode].per_level_data[level].name = pElem->GetText();
                                }
                            }
                        }
                    }
                }
            }

            if (is_root) {
                if (strmtch("Data", str.c_str()) || strmtch("mod.xml", str.c_str()) || strmtch("Localized", str.c_str())) {
                } else {
                    LOGW << "Mod " << id << " has files in root besides mod.xml and Data, this is bad form. File is question " << str << " consider moving it to Data." << std::endl;
                }
            }
        }

        std::vector<std::string>::iterator supported_versions_it;

        for (supported_versions_it = supported_versions.begin();
             supported_versions_it != supported_versions.end();
             supported_versions_it++) {
            std::vector<std::string> split_string;
            split(*supported_versions_it, '.', split_string);

            if (split_string.size() >= 3) {
                if (split_string[0] == GetVersionMajor() && split_string[1] == GetVersionMinor() && (split_string[2] == GetVersionPatch())) {
                    explicit_version_support = true;
                }
            }

            if (split_string.size() == 2) {
                if (split_string[0] == GetVersionMajor() && split_string[1] == GetVersionMinor()) {
                    explicit_version_support = true;
                }
            }

            // Accept either the short form version or the full name.
            if (GetShortBuildTag() == *supported_versions_it || std::string(GetBuildVersion()) == *supported_versions_it) {
                explicit_version_support = true;
            }
        }
    }

    if (strmtch("com-wolfire-lugaru-campaign", id)) {
        is_core = true;
    } else if (strmtch("com-wolfire-overgrowth-campaign", id)) {
        is_core = true;
    } else if (strmtch("com-wolfire-overgrowth-core", id)) {
        is_core = true;
    } else if (strmtch("com-wolfire-timbles-therium", id)) {
        is_core = true;
    } else if (strmtch("com-wolfire-drika", id)) {
        is_core = true;
    } else if (strmtch("com-wolfire-versus", id)) {
        is_core = true;
    } else if (strmtch("com-wolfire-versus-online", id)) {
        is_core = true;
    } else if (strmtch("com-wolfire-sandbox-levels", id)) {
        is_core = true;
    } else if (strmtch("com-wolfire-arena", id)) {
        is_core = true;
    } else {
        is_core = false;
    }

    if (is_core) {
        if (!Activate(true)) {
            LOGW << "FAILED ACTIVATING CORE MOD \"" << id << "\"" << std::endl;
        }
    } else if (CanActivate()) {
        if (ModLoading::Instance().acp.HasModInstance(id, modsource)) {
            ActiveModsParser::ModInstance mi = ModLoading::Instance().acp.GetModInstance(id, modsource);
            Activate(mi.activated);
        } else {  // Fall backon checking the config file and purging the value if found.
            std::stringstream active_index_ss;
            if (modsource == ModSourceSteamworks) {
                active_index_ss << "mod_activated[" << id << "_" << version << "_steam]";
            } else {
                active_index_ss << "mod_activated[" << id << "_" << version << "]";
            }
            std::string active_index = active_index_ss.str();

            // If we don't have anything in the state file, pull in from config.
            // Then remove value from old config.
            if (config.HasKey(active_index)) {
                Activate(config[active_index].toBool());
                // Save to file immediately to ensure that the information isn't lost.
                // Purging the value as it shouldn't be in the config.txt file in the long run.
                config.RemoveConfig(active_index);
                ModLoading::Instance().acp.Save(AssemblePath(std::string(GetWritePath(CoreGameModID).c_str()), "modconfig.xml"));
            } else {
                switch (modsource) {
                    case ModSourceLocalModFolder:
                        Activate(false);
                        break;
                    case ModSourceSteamworks:
                        Activate(true);
                        break;
                    case ModSourceUnknown:
                        LOGE << "Unhandled mod source" << std::endl;
                        break;
                }
            }
        }
    } else {
        // Don't call Activate because we might not own our id and version, basically we are perpertually deactivated.
        active = false;
    }
}

bool ModInstance::IsValid() const {
    return GetValidityErrorsArr().empty();
}

bool ModInstance::CanActivate() const {
    return (GetValidity() & kValidityActivationBreakingErrorMask).Empty();
}

bool ModInstance::IsActive() const {
    return active;
}

bool ModInstance::IsCore() const {
    return is_core;
}

ModID ModInstance::GetSid() const {
    return sid;
}

bool ModInstance::Activate(bool value) {
    bool ret_value = false;
    bool prev_active = active;

    if (is_core) {
        value = true;
    }

    if (value == active) {
        ret_value = true;
    } else if (value == false) {
        RemovePath(path.c_str(), kModPaths);
        active = false;
    } else if (CanActivate()) {
        if (AddModPath(path.c_str(), GetSid())) {
            LOGI << "Added mod path to filesystem" << std::endl;
            active = true;
            ret_value = false;
        } else {
            LOGE << "Can't activate mod, can't add path to filesystem" << std::endl;
            active = false;
            ret_value = false;
        }
    } else {
        switch (modsource) {
            case ModSourceLocalModFolder:
                LOGE << "Can't activate invalid local mod " << id << "_" << version << std::endl;
                break;
            case ModSourceSteamworks:
                LOGE << "Can't activate invalid steam mod " << id << "_" << version << std::endl;
                break;
            case ModSourceUnknown:
                LOGE << "Can't activate invalid unknown mod " << id << "_" << version << std::endl;
                break;
        }
        ret_value = false;
    }

    ModLoading::Instance().acp.SetModInstanceActive(id, modsource, active, version);

    if (active != prev_active) {
        ModLoading::Instance().TriggerModActivationCallback(this);
        ModLoading::Instance().RebuildLocalization();
    }

    return ret_value;
}

void ModInstance::SetHasBeenActivated() {
    has_been_activated = true;
}

bool ModInstance::HasBeenActivated() const {
    return has_been_activated;
}

bool ModInstance::NeedsRestart() const {
    return needs_restart;
}

bool ModInstance::SupportsOnline() const {
    return supports_online;
}

bool ModInstance::ExplicitVersionSupport() const {
    return explicit_version_support;
}

uint32_t ModInstance::GetUploadValidity() {
    uint32_t status = 0;
    ModValidity val = GetValidity();

    if (preview_images.size() > 0) {
        Path previewpath = FindFilePath(AssemblePath(path, preview_images[0]).c_str(), kAbsPath, true);
        if (previewpath.valid == false) {
            status |= k_UploadValidityInvalidPreviewImage;
        }

        // Check that the file isn't larger than 1MB (SI). Steam API limit for all preview files.
        if (GetFileSize(previewpath) > 1 * 1000 * 1000) {
            status |= k_UploadValidityOversizedPreviewImage;
        }
    } else {
        status |= k_UploadValidityMissingPreview;
    }

    Path thumbpath = FindFilePath(AssemblePath(path.c_str(), thumbnail.c_str()).c_str(), kAbsPath, true);
    if (thumbpath.valid == false) {
        status |= k_UploadValidityInvalidThumbnail;
    }

    Path modpath = FindFilePath(path.c_str(), kAbsPath, true);

    if (modpath.valid == false) {
        status |= k_UploadValidityInvalidModFolder;
    }

    if (val.Intersects(kValidityBrokenXml)) {
        status |= k_UploadValidityBrokenXml;
    }

    if (val.Intersects(kValidityIDInvalid)) {
        status |= k_UploadValidityInvalidID;
    }

    if (val.Intersects(kValidityInvalidVersion)) {
        status |= k_UploadValidityInvalidVersion;
    }

    if (val.Intersects(kValidityMissingReadRights)) {
        status |= k_UploadValidityMissingReadRights;
    }

    if (val.Intersects(kValidityMissingXml)) {
        status |= k_UploadValidityMissingXml;
    }

    ModValidity ignored =
        kValidityBrokenXml | kValidityIDInvalid | kValidityInvalidVersion | kValidityMissingReadRights | kValidityMissingXml;

    if ((val & ~ignored).Intersects(kValidityUploadSteamworksBlockingMask)) {
        status |= k_UploadValidityGenericValidityError;
    }

    return status;
}

#if ENABLE_STEAMWORKS
SteamworksUGCItem* ModInstance::GetUGCItem() {
    SteamworksUGC* ugc = Steamworks::Instance()->GetUGC();
    if (ugc) {
        std::vector<SteamworksUGCItem*>::iterator ugc_it = ugc->GetItem(ugc_id);
        if (ugc_it != ugc->GetItemEnd()) {
            return (*ugc_it);
        } else {
            return NULL;
        }
    } else {
        return NULL;
    }
}
#endif

bool ModInstance::IsOwnedByCurrentUser() {
    if (modsource == ModSourceSteamworks) {
#if ENABLE_STEAMWORKS
        if (SteamUser() && SteamUGC()) {
            if (GetUGCItem()) {
                CSteamID userid = SteamUser()->GetSteamID();
                if (GetUGCItem()->owner_id == userid) {
                    return true;
                } else {
                    return false;
                }
            } else {
                return false;
            }
        } else {
            return false;
        }
#else
        return false;
#endif
    } else {
        return false;
    }
}

bool ModInstance::IsSubscribed() {
    if (modsource == ModSourceSteamworks) {
#if ENABLE_STEAMWORKS
        if (GetUGCItem()) {
            return GetUGCItem()->IsSubscribed();
        }
#endif
        return false;
    } else {
        return true;
    }
}

bool ModInstance::IsInstalled() {
    if (modsource == ModSourceSteamworks) {
#if ENABLE_STEAMWORKS
        if (GetUGCItem()) {
            return GetUGCItem()->IsInstalled();
        }
#endif
        return false;
    } else {
        return true;
    }
}

bool ModInstance::IsDownloading() {
    if (modsource == ModSourceSteamworks) {
#if ENABLE_STEAMWORKS
        if (GetUGCItem()) {
            return GetUGCItem()->IsDownloading();
        }
#endif
        return false;
    } else {
        return false;
    }
}

bool ModInstance::IsDownloadPending() {
    if (modsource == ModSourceSteamworks) {
#if ENABLE_STEAMWORKS
        if (GetUGCItem()) {
            return GetUGCItem()->IsDownloadPending();
        }
#endif
        return false;
    } else {
        return false;
    }
}

bool ModInstance::IsNeedsUpdate() {
    if (modsource == ModSourceSteamworks) {
#if ENABLE_STEAMWORKS
        if (GetUGCItem()) {
            return GetUGCItem()->NeedsUpdate();
        }
#endif
        return false;
    } else {
        return false;
    }
}

bool ModInstance::IsFavorite() {
    if (modsource == ModSourceSteamworks) {
#if ENABLE_STEAMWORKS
        if (GetUGCItem()) {
            return GetUGCItem()->IsFavorite();
        }
#endif
        return false;
    } else {
        return false;
    }
}

ModInstance::UserVote ModInstance::GetUserVote() {
#if ENABLE_STEAMWORKS
    if (GetUGCItem()) {
        switch (GetUGCItem()->GetUserVote()) {
            case SteamworksUGCItem::k_VoteUnknown:
                return k_VoteUnknown;
            case SteamworksUGCItem::k_VoteNone:
                return k_VoteNone;
            case SteamworksUGCItem::k_VoteUp:
                return k_VoteUp;
            case SteamworksUGCItem::k_VoteDown:
                return k_VoteDown;
        }
    }
#endif
    return k_VoteUnknown;
}

void ModInstance::RequestUnsubscribe() {
#if ENABLE_STEAMWORKS
    if (GetUGCItem()) {
        GetUGCItem()->RequestUnsubscribe();
    }
#endif
}

void ModInstance::RequestSubscribe() {
#if ENABLE_STEAMWORKS
    if (GetUGCItem()) {
        GetUGCItem()->RequestSubscribe();
    }
#endif
}

void ModInstance::RequestVoteSet(bool voteup) {
#if ENABLE_STEAMWORKS
    if (GetUGCItem()) {
        GetUGCItem()->RequestUserVoteSet(voteup);
    }
#endif
}

void ModInstance::RequestFavoriteSet(bool fav) {
#if ENABLE_STEAMWORKS
    if (GetUGCItem()) {
        GetUGCItem()->RequestFavoriteSet(fav);
    }
#endif
}

void ModInstance::RequestUpdate(ModID source_sid, const char* update_message, ModVisibilityOptions visibility) {
#if ENABLE_STEAMWORKS
    ERemoteStoragePublishedFileVisibility new_visibility = mod_visibility_map[static_cast<int>(visibility)];
    if (GetUGCItem()) {
        GetUGCItem()->upload_sid_source = source_sid;
        GetUGCItem()->RequestUpload(update_message, new_visibility, 0U);
    } else {
        LOGE << "Missing ugc item for mod as update target" << std::endl;
    }
#endif
}

const char* ModInstance::GetModsourceString() const {
    switch (modsource) {
        case ModSourceLocalModFolder:
            return "Local Mod";
            break;
        case ModSourceSteamworks:
            return "Steam Workshop";
            break;
        case ModSourceUnknown:
            return "Unknown";
            break;
    }
    return "";
}

/*
const char* GetStatus() {
    switch(modsource) {
        case LocalModFolder:
            return "Installed";
            break;
        case Steamworks;
            SteamworksUGC* ugc = Steamworks::Instance()->GetUGC();
            std::vector<SteamworksUGCItem*>::iterator ugc_it = ugc->GetItem(ugc_id);
            if( ugc_it != ugc->GetItemEnd() ) {
                return (*ugc_it)->GetStatus();
            } else {
                return "Unknown";
            }
            break;
        default:
            return "Unknown Modsource";
            break;
    }
}
*/

std::string ModInstance::GetTagsListString() {
    std::stringstream ss;
    std::set<std::string>::iterator tag_it = tags.begin();

    while (tag_it != tags.end()) {
        if (tag_it == tags.begin()) {
            ss << *tag_it;
        } else {
            ss << "," << *tag_it;
        }
        tag_it++;
    }

    return ss.str();
}

std::vector<std::string> ModInstance::GetFileCollisions(const ModInstance* lhs, const ModInstance* rhs) {
    std::vector<std::string> uncollidable;
    uncollidable.push_back("mod.xml");

    std::vector<std::string> intersection;

    std::set_intersection(lhs->manifest.begin(),
                          lhs->manifest.end(),
                          rhs->manifest.begin(),
                          rhs->manifest.end(),
                          std::back_inserter(intersection));

    std::vector<std::string> intersection_cleaned;

    std::set_difference(intersection.begin(),
                        intersection.end(),
                        uncollidable.begin(),
                        uncollidable.end(),
                        std::back_inserter(intersection_cleaned));

    return intersection_cleaned;
}

std::vector<ModID> ModInstance::GetCollisionsWithActiveMods() const {
    std::vector<ModID> colliding_sids;

    std::vector<ModInstance*>::iterator modit = ModLoading::Instance().mods.begin();

    for (; modit != ModLoading::Instance().mods.end(); modit++) {
        if ((*modit)->sid != this->sid) {
            // Files that don't collide.
            std::vector<std::string> intersection_cleaned = GetFileCollisions(this, *modit);

            if (intersection_cleaned.size() > 0 && (*modit)->active) {
                colliding_sids.push_back((*modit)->sid);

                // Not logged due to spam when having the dear imgui mods menu open
                /*LOGW << "Mod " << id << "_" << version << "has colliding files with " << (*modit)->id << "_" << (*modit)->version << std::endl;
                for( unsigned i = 0; i < intersection_cleaned.size(); i++ ) {
                    LOGW << "Colliding file: " << intersection_cleaned[i] << std::endl;
                }*/
            }
        }
    }

    return colliding_sids;
}

struct CharEqual {
    bool operator()(const char* lhs, const char* rhs) const {
        return strmtch(lhs, rhs);
    }
};

struct CharLess {
    bool operator()(const char* lhs, const char* rhs) const {
        return std::strcmp(lhs, rhs) < 0;
    }
};

std::vector<ModID> ModInstance::GetIDCollisionsWithActiveMods() const {
    std::vector<ModID> colliding_sids;

    std::vector<ModInstance*>::iterator modit = ModLoading::Instance().mods.begin();

    std::set<const char*, CharLess> campaign_ids;
    for (const auto& campaign : campaigns) {
        if (campaign_ids.count(campaign.id) != 0)
            colliding_sids.push_back(sid);
        else
            campaign_ids.insert(campaign.id);
    }

    for (; modit != ModLoading::Instance().mods.end(); modit++) {
        bool collides = false;

        if ((*modit)->sid != this->sid) {
            if (strmtch((*modit)->id, id) && (*modit)->active) {
                colliding_sids.push_back((*modit)->sid);
                collides = true;
            }

            if (!collides) {
                for (size_t i = 0; i < (*modit)->campaigns.size(); i++) {
                    if (campaign_ids.count((*modit)->campaigns[i].id)) {
                        colliding_sids.push_back((*modit)->sid);
                        break;
                    }
                }
            }
        }
    }
    return colliding_sids;
}

ModValidity ModInstance::GetValidity() const {
    ModValidity out = valid;

    if (GetIDCollisionsWithActiveMods().size() > 0) {
        out |= kValidityIDCollision;
    }

    if (GetCollisionsWithActiveMods().size() > 0) {
        out |= kValidityActiveModCollision;
    }

    if (!explicit_version_support) {
        out |= kValidityUnsupportedOvergrowthVersion;
    }

    return out;
}

std::vector<std::string> ModInstance::GetInvalidItemPaths() const {
    return invalid_item_paths;
}

std::vector<std::string> ModInstance::GenerateValidityErrorsArr(const ModValidity& val, const ModInstance* instance /*= NULL*/) {
    std::vector<std::string> e;

    if (val.Intersects(kValidityIDCollision)) {
        e.push_back("ID Collision");
    }

    if (val.Intersects(kValidityActiveModCollision)) {
        if (instance) {
            std::vector<ModInstance*>::iterator modit = ModLoading::Instance().mods.begin();
            for (; modit != ModLoading::Instance().mods.end(); modit++) {
                if ((*modit)->sid != instance->sid) {
                    // Files that don't collide.
                    std::vector<std::string> intersection_cleaned = GetFileCollisions(instance, *modit);

                    if (intersection_cleaned.size() > 0 && (*modit)->active) {
                        std::string text = std::string("File collisions with mod \"") + (*modit)->name.c_str() + "\" (" + (*modit)->id.c_str() + "): ";

                        for (unsigned i = 0; i < intersection_cleaned.size(); i++) {
                            text += intersection_cleaned[i];
                            if (i != intersection_cleaned.size() - 1) {
                                text += ", ";
                            }
                        }
                        e.push_back(text);
                    }
                }
            }
        }
    }

    if (val.Intersects(kValidityBrokenXml)) {
        e.push_back("Mod Xml Is Broken");
    }

    if (val.Intersects(kValidityInvalidVersion)) {
        e.push_back("Version Invalid");
    }

    if (val.Intersects(kValidityMissingReadRights)) {
        e.push_back("MissingReadRights");
    }

    if (val.Intersects(kValidityMissingXml)) {
        e.push_back("Missing Mod Xml");
    }

    if (val.Intersects(kValidityUnloaded)) {
        e.push_back("Unloaded");
    }

    if (val.Intersects(kValidityUnsupportedOvergrowthVersion)) {
        e.push_back("Current version of the game not explicitly supported");
    }

    if (val.Intersects(kValidityNoDataOnDisk)) {
        e.push_back("No Data On Disk");
    }

    if (val.Intersects(kValiditySteamworksError)) {
        e.push_back("Steamworks error");
    }

    if (val.Intersects(kValidityNotInstalled)) {
        e.push_back("Not Installed");
    }

    if (val.Intersects(kValidityNotSubscribed)) {
        e.push_back("Not Subscribed");
    }

    if (val.Intersects(kValidityIDTooLong)) {
        e.push_back("ID Is Too Long");
    }

    if (val.Intersects(kValidityIDMissing)) {
        e.push_back("ID Missing");
    }

    if (val.Intersects(kValidityIDInvalid)) {
        e.push_back("ID Invalid, it must only contain lower-case characters, numbers, or dashes");
    }

    if (val.Intersects(kValidityNameTooLong)) {
        e.push_back("Name Too Long");
    }

    if (val.Intersects(kValidityVersionTooLong)) {
        e.push_back("Version Too Long");
    }

    if (val.Intersects(kValidityAuthorTooLong)) {
        e.push_back("Author Too Long");
    }

    if (val.Intersects(kValidityDescriptionTooLong)) {
        e.push_back("Description Too Long");
    }

    if (val.Intersects(kValidityThumbnailMaxLength)) {
        e.push_back("Thumbnail Max Length");
    }

    if (val.Intersects(kValidityTagsListTooLong)) {
        e.push_back("Tags List Too Long");
    }

    if (val.Intersects(kValidityMenuItemTitleTooLong)) {
        e.push_back("Menu Item Title Too Long");
    }

    if (val.Intersects(kValidityMenuItemTitleMissing)) {
        e.push_back("Menu Item Title Missing");
    }

    if (val.Intersects(kValidityMenuItemPathTooLong)) {
        e.push_back("Menu Item Path Too Long");
    }

    if (val.Intersects(kValidityMenuItemPathMissing)) {
        e.push_back("Menu Item Path Missing");
    }

    if (val.Intersects(kValidityMenuItemPathInvalid)) {
        e.push_back("Menu Item Path Invalid");
    }

    if (val.Intersects(kValidityMenuItemCategoryTooLong)) {
        e.push_back("Menu Item Category Too Long");
    }

    if (val.Intersects(kValidityMenuItemCategoryMissing)) {
        e.push_back("Menu Item Category Missing");
    }

    if (val.Intersects(kValidityInvalidTag)) {
        e.push_back("Invalid Tag");
    }

    if (val.Intersects(kValidityMissingName)) {
        e.push_back("Missing Name");
    }

    if (val.Intersects(kValidityMissingAuthor)) {
        e.push_back("Missing Author");
    }

    if (val.Intersects(kValidityMissingDescription)) {
        e.push_back("Missing Description");
    }

    if (val.Intersects(kValidityMissingThumbnail)) {
        e.push_back("Missing Thumbnail");
    }

    if (val.Intersects(kValidityMissingThumbnailFile)) {
        e.push_back("Missing Thumbnail File");
    }

    if (val.Intersects(kValidityMissingVersion)) {
        e.push_back("Missing Version");
    }

    if (val.Intersects(kValidityCampaignTitleTooLong)) {
        e.push_back("Campaign Title Too Long");
    }

    if (val.Intersects(kValidityCampaignTitleMissing)) {
        e.push_back("Campaign Title Missing");
    }

    if (val.Intersects(kValidityCampaignTypeTooLong)) {
        e.push_back("Campaign Type Too Long");
    }

    if (val.Intersects(kValidityCampaignTypeMissing)) {
        e.push_back("Campaign Type Missing");
    }

    if (val.Intersects(kValidityLevelTitleTooLong)) {
        e.push_back("Level Title Too Long");
    }

    if (val.Intersects(kValidityLevelTitleMissing)) {
        e.push_back("Level Title Missing");
    }

    if (val.Intersects(kValidityLevelPathTooLong)) {
        e.push_back("Level Path Too Long");
    }

    if (val.Intersects(kValidityLevelPathMissing)) {
        e.push_back("Level Path Missing");
    }

    if (val.Intersects(kValidityLevelPathInvalid)) {
        e.push_back("Level Path Invalid");
    }

    if (val.Intersects(kValidityLevelThumbnailTooLong)) {
        e.push_back("Level Thumbnail Too Long");
    }

    if (val.Intersects(kValidityLevelThumbnailMissing)) {
        e.push_back("Level Thumbnail Missing");
    }

    if (val.Intersects(kValidityLevelThumbnailInvalid)) {
        e.push_back("Level Thumbnail Invalid");
    }

    if (val.Intersects(kValidityItemTitleTooLong)) {
        e.push_back("Item Title Too Long");
    }

    if (val.Intersects(kValidityItemTitleMissing)) {
        e.push_back("Item Title Missing");
    }

    if (val.Intersects(kValidityItemCategoryTooLong)) {
        e.push_back("Item Category Too Long");
    }

    if (val.Intersects(kValidityItemCategoryMissing)) {
        e.push_back("Item Category Missing");
    }

    if (val.Intersects(kValidityItemPathTooLong)) {
        e.push_back("Item Path Too Long");
    }

    if (val.Intersects(kValidityItemPathMissing)) {
        e.push_back("Item Path Missing");
    }

    if (val.Intersects(kValidityItemPathFileMissing)) {
        e.push_back("Item Path File Missing");
    }

    if (val.Intersects(kValidityItemThumbnailTooLong)) {
        e.push_back("Item Thumbnail Too Long");
    }

    if (val.Intersects(kValidityItemThumbnailMissing)) {
        e.push_back("Item Thumbnail Missing");
    }

    if (val.Intersects(kValidityItemThumbnailFileMissing)) {
        e.push_back("Item Thumbnail File Missing");
    }

    if (val.Intersects(kValidityCategoryTooLong)) {
        e.push_back("Category Too Long");
    }

    if (val.Intersects(kValidityCategoryMissing)) {
        e.push_back("Category Missing");
    }

    if (val.Intersects(kValidityInvalidPreviewImage)) {
        e.push_back("Invalid Preview Image");
    }

    if (val.Intersects(kValidityInvalidSupportedVersion)) {
        e.push_back("Invalid Supported Version");
    }

    if (val.Intersects(kValidityInvalidLevelHookFile)) {
        e.push_back("Invalid Level Hook File");
    }

    if (val.Intersects(kValidityInvalidNeedRestart)) {
        e.push_back("Invalid Need Restart");
    }

    if (val.Intersects(kValidityCampaignIsLinearInvalidEnum)) {
        e.push_back("Invalid is_linear Campaign Attribute");
    }

    if (val.Intersects(kValidityCampaignThumbnailMissing)) {
        e.push_back("Campaign Thumbnail Attribute Missing");
    }

    if (val.Intersects(kValidityCampaignThumbnailTooLong)) {
        e.push_back("Campaign Thumbnail Attribute Too Long");
    }

    if (val.Intersects(kValidityCampaignThumbnailInvalid)) {
        e.push_back("Campaign Thumbnail Attribute Invalid");
    }

    if (val.Intersects(kValidityMenuItemThumbnailTooLong)) {
        e.push_back("Menu Item Thumbnail Attribute Too Long");
    }

    if (val.Intersects(kValidityMenuItemThumbnailInvalid)) {
        e.push_back("Menu Item Thumbnail Attribute Invalid");
    }

    if (val.Intersects(kValidityCampaignIDTooLong)) {
        e.push_back("Campaign ID too long");
    }

    if (val.Intersects(kValidityCampaignIDMissing)) {
        e.push_back("Campaign IDs are missing");
    }

    return e;
}

std::vector<std::string> ModInstance::GetValidityErrorsArr() const {
    return GenerateValidityErrorsArr(GetValidity(), this);
}

Path ModInstance::GetFullAbsThumbnailPath() const {
    return FindFilePath(AssemblePath(path.c_str(), thumbnail.c_str()).c_str(), kAbsPath, true);
}

std::string ModInstance::GetValidityErrors() const {
    return GenerateValidityErrors(GetValidity(), this);
}

std::string ModInstance::GenerateValidityErrors(const ModValidity& v, const ModInstance* instance /*= NULL*/) {
    std::vector<std::string> e = GenerateValidityErrorsArr(v, instance);

    std::stringstream ss;
    for (size_t i = 0; i < e.size(); i++) {
        ss << e[i];
        if (i < e.size() - 1) {
            ss << ", ";
        }
    }

    return ss.str();
}

ModLoading::ModLoading() {
}

std::vector<ModInstance*>::iterator ModLoading::GetMod(const std::string& path) {
    std::vector<ModInstance*>::iterator modit = mods.begin();

    for (; modit != mods.end(); modit++) {
        if ((*modit)->path == path) {
            break;
        }
    }

    return modit;
}

bool ModLoading::IsCampaignPresent(const std::string& campaign) {
    for (auto& mod : mods) {
        if (mod->IsActive()) {
            for (uint32_t j = 0; j < mod->campaigns.size(); j++) {
                if (strmtch(mod->campaigns[j].id, campaign)) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool ModLoading::CampaignHasLevel(const std::string& campaign, const std::string& level_path) {
    for (auto& mod : mods) {
        if (mod->IsActive()) {
            for (uint32_t j = 0; j < mod->campaigns.size(); j++) {
                if (strmtch(mod->campaigns[j].id, campaign)) {
                    for (const auto& level : mod->campaigns[j].levels) {
                        LOGI << "path: " << std::string("Data/Levels/") + level.path.str() << std::endl;
                        LOGI << "Level path: " << level_path << std::endl;
                        if (level.path.str() == level_path) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

ModInstance* ModLoading::GetMod(ModID sid) {
    std::vector<ModInstance*>::iterator modit = mods.begin();

    for (; modit != mods.end(); modit++) {
        if ((*modit)->GetSid() == sid) {
            return (*modit);
        }
    }
    return NULL;
}

std::string ModLoading::GetModName(ModID sid) {
    std::vector<ModInstance*>::iterator modit = mods.begin();

    if (sid == CoreGameModID) {
        return "Core Engine";
    }

    for (; modit != mods.end(); modit++) {
        if ((*modit)->GetSid() == sid) {
            return (*modit)->name.str();
        }
    }

    return "Unknown";
}

std::string ModLoading::GetModID(ModID sid) {
    if (sid == CoreGameModID) {
        return "";
    }

    std::vector<ModInstance*>::iterator modit = mods.begin();
    for (; modit != mods.end(); modit++) {
        if ((*modit)->GetSid() == sid) {
            return (*modit)->id.str();
        }
    }

    LOGW << "Got an invalid mod id, returning core id" << std::endl;
    return "";
}

bool ModLoading::IsActive(ModID sid) {
    std::vector<ModInstance*>::iterator modit = mods.begin();

    for (; modit != mods.end(); modit++) {
        if ((*modit)->GetSid() == sid) {
            if ((*modit)->IsActive()) {
                return true;
            } else {
                return false;
            }
        }
    }
    return false;
}

bool ModLoading::IsActive(std::string id) {
    std::vector<ModInstance*>::iterator modit = mods.begin();

    for (; modit != mods.end(); modit++) {
        if (strmtch((*modit)->id, id.c_str())) {
            if ((*modit)->IsActive()) {
                return true;
            } else {
                return false;
            }
        }
    }
    return false;
}

ModInstance::Campaign ModLoading::GetCampaign(std::string& campaign_id) {
    for (auto& mod : mods) {
        if (mod->IsActive()) {
            for (uint32_t j = 0; j < mod->campaigns.size(); j++) {
                if (strmtch(mod->campaigns[j].id, campaign_id)) {
                    return mod->campaigns[j];
                }
            }
        }
    }
    return ModInstance::Campaign();
}

ModInstance* ModLoading::GetModInstance(const std::string& campaign_id) {
    for (auto& mod : mods) {
        if (mod->IsActive()) {
            for (uint32_t j = 0; j < mod->campaigns.size(); j++) {
                if (strmtch(mod->campaigns[j].id, campaign_id)) {
                    return mod;
                }
            }
        }
    }
    return nullptr;
}

std::vector<ModInstance::Campaign> ModLoading::GetCampaigns() {
    std::vector<ModInstance::Campaign> campaigns;
    for (auto& mod : mods) {
        if (mod->IsActive()) {
            for (uint32_t j = 0; j < mod->campaigns.size(); j++) {
                if (strlen(mod->campaigns[j].title) > 0) {
                    campaigns.push_back(mod->campaigns[j]);
                }
            }
        }
    }
    return campaigns;
}

std::vector<ModInstance*> ModLoading::GetModsMatchingID(std::string id) {
    std::vector<ModInstance*> lmods;
    for (auto& mod : mods) {
        if (strmtch(mod->id, id)) {
            lmods.push_back(mod);
        }
    }
    return lmods;
}

std::vector<ModInstance*> ModLoading::GetSteamModsMatchingID(std::string id) {
    std::vector<ModInstance*> lmods;
    for (auto& mod : mods) {
        if (strmtch(mod->id, id) && mod->modsource == ModSourceSteamworks) {
            lmods.push_back(mod);
        }
    }
    return lmods;
}

std::vector<ModInstance*> ModLoading::GetLocalModsMatchingID(std::string id) {
    std::vector<ModInstance*> lmods;
    for (auto& mod : mods) {
        if (strmtch(mod->id, id) && mod->modsource == ModSourceLocalModFolder) {
            lmods.push_back(mod);
        }
    }
    return lmods;
}

std::vector<ModInstance*> ModLoading::GetLocalMods() {
    std::vector<ModInstance*> lmods;
    for (auto& mod : mods) {
        if (mod->modsource == ModSourceLocalModFolder) {
            lmods.push_back(mod);
        }
    }
    return lmods;
}

std::vector<ModInstance*> ModLoading::GetAllMods() {
    std::vector<ModInstance*> lmods;
    for (auto& mod : mods) {
        lmods.push_back(mod);
    }
    return lmods;
}

void ModLoading::AddMod(const std::string& path) {
    std::vector<ModInstance*>::iterator modit = GetMod(path);
    LOGI << "Added mod: " << path << std::endl;
    if (modit == mods.end()) {
        ModInstance* modInstance = new ModInstance(path);
        LOGD << modInstance << std::endl;
        mods.push_back(modInstance);
    }
}

std::string ModLoading::WhichCampaignLevelBelongsTo(const std::string& level_path) {
    for (auto& mod : mods) {
        if (mod->IsActive()) {
            for (uint32_t j = 0; j < mod->campaigns.size(); j++) {
                for (const auto& level : mod->campaigns[j].levels) {
                    if (std::string("Data/Levels/") + level.path.str() == level_path) {
                        return mod->campaigns[j].id.str();
                    }
                }
            }
        }
    }
    return "";
}

void ModLoading::RemoveMod(const std::string& path) {
    std::vector<ModInstance*>::iterator modit = GetMod(path);
    LOGI << "Removed mod: " << path << std::endl;
    if (modit != mods.end()) {
        delete *modit;
        mods.erase(modit);
    }
}

void ModLoading::ReloadMod(const std::string& path) {
    std::vector<ModInstance*>::iterator modit = GetMod(path);
    LOGI << "Reload mod: " << path << std::endl;
    if (modit != mods.end()) {
        (*modit)->Reload();
    }
}

void ModLoading::Reload() {
    DetectMods();

    std::vector<ModInstance*>::iterator modit = mods.begin();
    for (; modit != mods.end(); modit++) {
        (*modit)->Reload();
    }
}

void ModLoading::InitMods() {
    DetectMods();
}

void ModLoading::DetectMods() {
    std::vector<std::string> newMods;

    for (int i = 0; i < vanilla_data_paths.num_paths; ++i) {
        std::stringstream ss;
        ss << vanilla_data_paths.paths[i];
        // TODO: Use config var instead.
        ss << "Data/Mods/";

        std::string path;
        path = caseCorrect(ss.str());
        if (CheckFileAccess(path.c_str())) {
            getSubdirectories(path.c_str(), newMods);
        }
    }

    std::stringstream ss;
    ss << write_path;
    // TODO: Use config var instead.
    ss << "Data/Mods/";

    std::string path;
    path = caseCorrect(ss.str());

    LOGI << "Looking for mods in " << path << std::endl;
    if (CheckFileAccess(path.c_str())) {
        getSubdirectories(path.c_str(), newMods);
    }

    std::vector<std::string> verifiedMods;

    for (auto& newMod : newMods) {
        bool already_in_list = false;
        bool has_mod_xml = false;

        if (CheckFileAccess(std::string(newMod + "/mod.xml").c_str())) {
            has_mod_xml = true;
        }

        if (has_mod_xml) {
            for (auto& verifiedMod : verifiedMods) {
                if (AreSame(std::string(newMod + "/mod.xml").c_str(), std::string(verifiedMod + "/mod.xml").c_str())) {
                    already_in_list = true;
                }
            }
            if (already_in_list == false) {
                verifiedMods.push_back(newMod);
            }
        }
    }

    SetMods(verifiedMods);
}

#if ENABLE_STEAMWORKS
ModID ModLoading::CreateSteamworksMod(UGCID ugc_id) {
    LOGI << "Adding steamworks mod: " << ugc_id << std::endl;
    ModInstance* modInstance = new ModInstance(ugc_id);
    mods.push_back(modInstance);
    return modInstance->GetSid();
}
#endif

void ModLoading::SetMods(std::vector<std::string> modpaths) {
    std::vector<std::string> currentPaths;

    std::vector<ModInstance*>::iterator modit = mods.begin();
    for (; modit != mods.end(); modit++) {
        if ((*modit)->path.empty() == false) {
            if ((*modit)->modsource == ModSourceLocalModFolder) {
                currentPaths.push_back((*modit)->path);
            }
        }
    }

    std::sort(currentPaths.begin(), currentPaths.end());
    std::sort(modpaths.begin(), modpaths.end());

    std::vector<std::string> newPaths;
    std::vector<std::string> missingPaths;
    std::vector<std::string> stillPaths;

    if (modpaths.size() == 0 || currentPaths.size() == 0) {
        newPaths = modpaths;
        missingPaths = currentPaths;
        stillPaths = std::vector<std::string>();  // Empty as intersection is guaranteed to be zero
    } else {
        for (auto& modpath : modpaths) {
            bool found = false;
            for (auto& currentPath : currentPaths) {
                if (modpath == currentPath) {
                    found = true;
                }
            }
            if (found == false) {
                newPaths.push_back(modpath);
            }
        }
        for (auto& currentPath : currentPaths) {
            bool found = false;
            for (auto& modpath : modpaths) {
                if (modpath == currentPath) {
                    found = true;
                }
            }
            if (found == false) {
                missingPaths.push_back(currentPath);
            }
        }
        for (auto& currentPath : currentPaths) {
            bool found = false;
            for (auto& modpath : modpaths) {
                if (modpath == currentPath) {
                    found = true;
                }
            }
            if (found) {
                stillPaths.push_back(currentPath);
            }
        }
    }

    std::vector<std::string>::iterator pathit;
    for (pathit = newPaths.begin(); pathit != newPaths.end(); pathit++) {
        AddMod(*pathit);
    }

    for (pathit = missingPaths.begin(); pathit != missingPaths.end(); pathit++) {
        RemoveMod(*pathit);
    }

    for (pathit = stillPaths.begin(); pathit != stillPaths.end(); pathit++) {
        ReloadMod(*pathit);
    }
}

void ModLoading::TriggerModActivationCallback(ModInstance* mod) {
    std::vector<ModLoadingCallback*>::iterator callbackit;

    for (callbackit = callbacks.begin();
         callbackit != callbacks.end();
         callbackit++) {
        (*callbackit)->ModActivationChange(mod);
    }
}

const std::vector<ModInstance*>& ModLoading::GetMods() {
    return mods;
}

const std::vector<ModID> ModLoading::GetModsSid() {
    std::vector<ModID> vals;
    for (auto& mod : mods) {
        vals.push_back(mod->GetSid());
    }
    return vals;
}

void ModLoading::RegisterCallback(ModLoadingCallback* callback) {
    callbacks.push_back(callback);
}

void ModLoading::DeRegisterCallback(ModLoadingCallback* callback) {
    std::vector<ModLoadingCallback*>::iterator callbackit = std::find(callbacks.begin(), callbacks.end(), callback);

    if (callbackit != callbacks.end()) {
        callbacks.erase(callbackit);
    }
}

ModLoading ModLoading::instance;

ModLoading& ModLoading::Instance() {
    return instance;
}

void ModLoading::Initialize() {
    std::string path = AssemblePath(std::string(GetWritePath(CoreGameModID).c_str()), "modconfig.xml");
    if (isFile(path.c_str()) == false) {
        acp.Save(path);
    } else {
        acp.Load(path);
    }
}

void ModLoading::Dispose() {
    SaveModConfig();
}

void ModLoading::SaveModConfig() {
    acp.Save(AssemblePath(std::string(GetWritePath(CoreGameModID).c_str()), "modconfig.xml"));
}

std::vector<std::string> ModLoading::ActiveLevelHooks() {
    std::vector<ModInstance*>::iterator modit = mods.begin();
    std::vector<std::string> files;

    for (; modit != mods.end(); modit++) {
        if ((*modit)->IsActive()) {
            files.insert(files.end(), (*modit)->level_hook_files.begin(), (*modit)->level_hook_files.end());
        }
    }

    return files;
}

void ModLoading::RebuildLocalization() {
    std::vector<ModInstance*>::iterator modit = mods.begin();
    ClearLocale();
    const std::string current_language = config["language"].str();
    for (; modit != mods.end(); modit++) {
        if ((*modit)->IsActive()) {
            std::map<std::string, ModInstance::LanguageData>::iterator lditer = (*modit)->language_data.begin();
            for (; lditer != (*modit)->language_data.end(); ++lditer) {
                AddLocale(lditer->first.c_str(), lditer->second.language.c_str(), lditer->second.local_language.c_str());

                std::map<std::string, ModInstance::LevelLocalizationData>::iterator per_level_iter = lditer->second.per_level_data.begin();
                for (; per_level_iter != lditer->second.per_level_data.end(); ++per_level_iter) {
                    AddLevelName(lditer->first.c_str(), per_level_iter->first.c_str(), per_level_iter->second.name.c_str());
                    AddLevelTip(lditer->first.c_str(), per_level_iter->first.c_str(), per_level_iter->second.loading_screen_tip.c_str());
                }
            }
        }
    }
}

std::ostream& operator<<(std::ostream& os, const ModInstance::ModDependency& md) {
    os << "[ModDependency]" << std::endl;
    os << "Id:" << md.id << std::endl;

    os << "-Versions-" << std::endl;

    for (const auto& version : md.versions) {
        os << version << std::endl;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, const ModInstance& mi) {
    os << "[ModInstance]" << std::endl;
    os << "Path:" << mi.path << std::endl;
    os << "Id:" << mi.id << std::endl;
    os << "Name:" << mi.name << std::endl;
    os << "Version:" << mi.version << std::endl;
    os << "Valid:" << mi.GetValidity() << std::endl;
    os << "-Supported Versions-" << std::endl;

    for (const auto& supported_version : mi.supported_versions) {
        os << supported_version << std::endl;
    }

    os << "-Mod Dependencies-" << std::endl;

    for (const auto& mod_dependencie : mi.mod_dependencies) {
        os << mod_dependencie << std::endl;
    }

    os << "-Manifest -" << std::endl;

    for (const auto& sit : mi.manifest) {
        os << sit << std::endl;
    }

    os << "-Overloaded Files-" << std::endl;

    for (const auto& overload_file : mi.overload_files) {
        os << overload_file << std::endl;
    }

    return os;
}
