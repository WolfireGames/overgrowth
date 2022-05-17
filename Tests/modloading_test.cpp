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

#include <gtest/gtest.h>

#include <cmath>
#include <cstdlib>
#include <set>

namespace tut {
    struct datamodloading  //
    {
        int placeholder;
    };

    TEST(Modloading, GenerateValidity)
    {
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityValid).size(), 0);

        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityBrokenXml).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityInvalidVersion).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMissingReadRights).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMissingXml).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityUnloaded).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityUnsupportedOvergrowthVersion).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityNoDataOnDisk).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValiditySteamworksError).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityNotInstalled).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityNotSubscribed).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityIDTooLong).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityIDMissing).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityIDInvalid).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityNameTooLong).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityVersionTooLong).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityAuthorTooLong).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityDescriptionTooLong).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityThumbnailMaxLength).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityTagsListTooLong).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMenuItemTitleTooLong).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMenuItemTitleMissing).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMenuItemCategoryTooLong).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMenuItemCategoryMissing).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMenuItemPathTooLong).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMenuItemPathMissing).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMenuItemPathInvalid).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityInvalidTag).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityIDCollision).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityActiveModCollision).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMissingName).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMissingAuthor).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMissingDescription).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMissingThumbnail).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMissingThumbnailFile).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityMissingVersion).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityCampaignTitleTooLong).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityCampaignTitleMissing).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityCampaignTypeTooLong).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityCampaignTypeMissing).size(), 1);

        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityLevelTitleTooLong).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityLevelTitleMissing).size(), 1);

        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityLevelPathTooLong).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityLevelPathMissing).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityLevelPathInvalid).size(), 1);

        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityLevelThumbnailTooLong).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityLevelThumbnailMissing).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityLevelThumbnailInvalid).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityInvalidPreviewImage).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityInvalidSupportedVersion).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityInvalidLevelHookFile).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityInvalidNeedRestart).size(), 1);

        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityItemTitleTooLong).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityItemTitleMissing).size(), 1);

        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityItemCategoryTooLong).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityItemCategoryMissing).size(), 1);

        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityItemPathTooLong).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityItemPathMissing).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityItemPathFileMissing).size(), 1);

        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityItemThumbnailTooLong).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityItemThumbnailMissing).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityItemThumbnailFileMissing).size(), 1);

        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityCategoryTooLong).size(), 1);
        EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(ModInstance::kValidityCategoryMissing).size(), 1);

        for (unsigned i = 0; i < ModInstance::kValidityFlagCount; i++) {
            ModValidity m;
            if (i < 64) {
                m = ModValidity(0ULL, 1ULL << i);
            }
            else {
                m = ModValidity(1ULL << (i - 64), 0ULL);
            }
            EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(m).size(), 1) << "expected used flags have error string";
        }

        for (int i = ModInstance::kValidityFlagCount; i < 128; i++) {
            ModValidity m;
            if (i < 64) {
                m = ModValidity(0ULL, 1ULL << i);
            }
            else {
                m = ModValidity(1ULL << (i - 64), 0ULL);
            }
            EXPECT_EQ(ModInstance::GenerateValidityErrorsArr(m).size(), 0) << "expected unused flags lack error string";
        }

        EXPECT_EQ(sizeof(ModInstance::kValidityValid), 16) << "expect validity 64 bit";
    }
}  // namespace tut
