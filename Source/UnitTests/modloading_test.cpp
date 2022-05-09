//-----------------------------------------------------------------------------
//           Name: modloading_test.cpp
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
#include <Internal/modloading.h>

#include <tut/tut.hpp>

#include <cmath>
#include <cstdlib>
#include <set>

namespace tut {
struct datamodloading  //
{
    int placeholder;
};

typedef test_group<datamodloading> tg;
tg test_group_m("Modloading");

typedef tg::object modloading;

template <>
template <>
void modloading::test<1>() {
    ensure("ModInstance::kValidityValid has no error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityValid).size() == 0);

    ensure("ModInstance::kValidityBrokenXml has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityBrokenXml).size() == 1);
    ensure("ModInstance::kValidityInvalidVersion has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityInvalidVersion).size() == 1);
    ensure("ModInstance::kValidityMissingReadRights has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMissingReadRights).size() == 1);
    ensure("ModInstance::kValidityMissingXml has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMissingXml).size() == 1);
    ensure("ModInstance::kValidityUnloaded has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityUnloaded).size() == 1);
    ensure("ModInstance::kValidityUnsupportedOvergrowthVersion has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityUnsupportedOvergrowthVersion).size() == 1);
    ensure("ModInstance::kValidityNoDataOnDisk has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityNoDataOnDisk).size() == 1);
    ensure("ModInstance::kValiditySteamworksError has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValiditySteamworksError).size() == 1);
    ensure("ModInstance::kValidityNotInstalled has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityNotInstalled).size() == 1);
    ensure("ModInstance::kValidityNotSubscribed has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityNotSubscribed).size() == 1);
    ensure("ModInstance::kValidityIDTooLong has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityIDTooLong).size() == 1);
    ensure("ModInstance::kValidityIDMissing has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityIDMissing).size() == 1);
    ensure("ModInstance::kValidityIDInvalid has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityIDInvalid).size() == 1);
    ensure("ModInstance::kValidityNameTooLong has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityNameTooLong).size() == 1);
    ensure("ModInstance::kValidityVersionTooLong has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityVersionTooLong).size() == 1);
    ensure("ModInstance::kValidityAuthorTooLong has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityAuthorTooLong).size() == 1);
    ensure("ModInstance::kValidityDescriptionTooLong has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityDescriptionTooLong).size() == 1);
    ensure("ModInstance::kValidityThumbnailMaxLength has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityThumbnailMaxLength).size() == 1);
    ensure("ModInstance::kValidityTagsListTooLong has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityTagsListTooLong).size() == 1);
    ensure("ModInstance::kValidityMenuItemTitleTooLong has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMenuItemTitleTooLong).size() == 1);
    ensure("ModInstance::kValidityMenuItemTitleMissing has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMenuItemTitleMissing).size() == 1);
    ensure("ModInstance::kValidityMenuItemCategoryTooLong has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMenuItemCategoryTooLong).size() == 1);
    ensure("ModInstance::kValidityMenuItemCategoryMissing has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMenuItemCategoryMissing).size() == 1);
    ensure("ModInstance::kValidityMenuItemPathTooLong has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMenuItemPathTooLong).size() == 1);
    ensure("ModInstance::kValidityMenuItemPathMissing has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMenuItemPathMissing).size() == 1);
    ensure("ModInstance::kValidityMenuItemPathInvalid has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMenuItemPathInvalid).size() == 1);
    ensure("ModInstance::kValidityInvalidTag has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityInvalidTag).size() == 1);
    ensure("ModInstance::kValidityIDCollision has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityIDCollision).size() == 1);
    ensure("ModInstance::kValidityActiveModCollision has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityActiveModCollision).size() == 1);
    ensure("ModInstance::kValidityMissingName has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMissingName).size() == 1);
    ensure("ModInstance::kValidityMissingAuthor has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMissingAuthor).size() == 1);
    ensure("ModInstance::kValidityMissingDescription has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMissingDescription).size() == 1);
    ensure("ModInstance::kValidityMissingThumbnail has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMissingThumbnail).size() == 1);
    ensure("ModInstance::kValidityMissingThumbnailFile has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMissingThumbnailFile).size() == 1);
    ensure("ModInstance::kValidityMissingVersion has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMissingVersion).size() == 1);
    ensure("ModInstance::kValidityCampaignTitleTooLong has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityCampaignTitleTooLong).size() == 1);
    ensure("ModInstance::kValidityCampaignTitleMissing has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityCampaignTitleMissing).size() == 1);
    ensure("ModInstance::kValidityCampaignTypeTooLong has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityCampaignTypeTooLong).size() == 1);
    ensure("ModInstance::kValidityCampaignTypeMissing has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityCampaignTypeMissing).size() == 1);

    ensure("ModInstance::kValidityLevelTitleTooLong has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityLevelTitleTooLong).size() == 1);
    ensure("ModInstance::kValidityLevelTitleMissing has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityLevelTitleMissing).size() == 1);

    ensure("ModInstance::kValidityLevelPathTooLong has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityLevelPathTooLong).size() == 1);
    ensure("ModInstance::kValidityLevelPathMissing has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityLevelPathMissing).size() == 1);
    ensure("ModInstance::kValidityLevelPathInvalid has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityLevelPathInvalid).size() == 1);

    ensure("ModInstance::kValidityLevelThumbnailTooLong has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityLevelThumbnailTooLong).size() == 1);
    ensure("ModInstance::kValidityLevelThumbnailMissing has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityLevelThumbnailMissing).size() == 1);
    ensure("ModInstance::kValidityLevelThumbnailInvalid has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityLevelThumbnailInvalid).size() == 1);
    ensure("ModInstance::kValidityInvalidPreviewImage has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityInvalidPreviewImage).size() == 1);
    ensure("ModInstance::kValidityInvalidSupportedVersion has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityInvalidSupportedVersion).size() == 1);
    ensure("ModInstance::kValidityInvalidLevelHookFile has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityInvalidLevelHookFile).size() == 1);
    ensure("ModInstance::kValidityInvalidNeedRestart has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityInvalidNeedRestart).size() == 1);

    ensure("ModInstance::kValidityItemTitleTooLong has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityItemTitleTooLong).size() == 1);
    ensure("ModInstance::kValidityItemTitleMissing has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityItemTitleMissing).size() == 1);

    ensure("ModInstance::kValidityItemCategoryTooLong has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityItemCategoryTooLong).size() == 1);
    ensure("ModInstance::kValidityItemCategoryMissing has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityItemCategoryMissing).size() == 1);

    ensure("ModInstance::kValidityItemPathTooLong has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityItemPathTooLong).size() == 1);
    ensure("ModInstance::kValidityItemPathMissing has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityItemPathMissing).size() == 1);
    ensure("ModInstance::kValidityItemPathFileMissing has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityItemPathFileMissing).size() == 1);

    ensure("ModInstance::kValidityItemThumbnailTooLong has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityItemThumbnailTooLong).size() == 1);
    ensure("ModInstance::kValidityItemThumbnailMissing has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityItemThumbnailMissing).size() == 1);
    ensure("ModInstance::kValidityItemThumbnailFileMissing has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityItemThumbnailFileMissing).size() == 1);

    ensure("ModInstance::kValidityCategoryTooLong has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityCategoryTooLong).size() == 1);
    ensure("ModInstance::kValidityCategoryMissing has error string", ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityCategoryMissing).size() == 1);

    for (unsigned i = 0; i < ModInstance::kValidityFlagCount; i++) {
        ModValidity m;
        if (i < 64) {
            m = ModValidity(0ULL, 1ULL << i);
        } else {
            m = ModValidity(1ULL << (i - 64), 0ULL);
        }
        ensure("Ensure used flags have error string", ModInstance::GenerateValidityErrorsArr(m).size() == 1);
    }

    for (int i = ModInstance::kValidityFlagCount; i < 128; i++) {
        ModValidity m;
        if (i < 64) {
            m = ModValidity(0ULL, 1ULL << i);
        } else {
            m = ModValidity(1ULL << (i - 64), 0ULL);
        }
        ensure("Ensure unused flags lack error string", ModInstance::GenerateValidityErrorsArr(m).size() == 0);
    }

    ensure("Ensure validity 64 bit", sizeof(ModInstance::kValidityValid) == 16);
}
}  // namespace tut
