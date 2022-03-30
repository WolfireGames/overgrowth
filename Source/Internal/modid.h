//-----------------------------------------------------------------------------
//           Name: modid.h
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

#include <Internal/integer.h>

#include <ostream>


//Some strict limitations on datatypes to simplify handling and storage assumptions.
static const size_t MOD_PATH_MAX_LENGTH = 256 + 1;
static const size_t MOD_ID_MAX_LENGTH = 64 + 1;
static const size_t MOD_NAME_MAX_LENGTH = 128 + 1; //Number borrowed from steam ugc
static const size_t MOD_VERSION_MAX_LENGTH = 32 + 1;
static const size_t MOD_AUTHOR_MAX_LENGTH = 128 + 1;
static const size_t MOD_CATEGORY_MAX_LENGTH = 64 + 1;

static const size_t MOD_DESCRIPTION_MAX_LENGTH = 8000; //Number borrowed from steam ugc
static const size_t MOD_THUMBNAIL_MAX_LENGTH = 128 + 1;
static const size_t MOD_TAGS_MAX_LENGTH = 1024 + 1; //Number borrowed from steam ugc

static const size_t MOD_ATTRIBUTE_ID_MAX_LENGTH = 32 + 1;
static const size_t MOD_ATTRIBUTE_VALUE_MAX_LENGTH = 256 + 1;

static const size_t MOD_MENU_ITEM_TITLE_MAX_LENGTH = 64 + 1;
static const size_t MOD_MENU_ITEM_CATEGORY_MAX_LENGTH = 32 + 1;
static const size_t MOD_MENU_ITEM_PATH_MAX_LENGTH = 256 + 1;
static const size_t MOD_MENU_ITEM_THUMBNAIL_MAX_LENGTH = 256 + 1;

static const size_t MOD_LEVEL_PARAMETER_NAME_MAX_LENGTH = 32+1;
static const size_t MOD_LEVEL_PARAMETER_TYPE_MAX_LENGTH = 16+1;
static const size_t MOD_LEVEL_PARAMETER_VALUE_MAX_LENGTH = 128+1;

static const size_t MOD_LEVEL_ID_MAX_LENGTH = 32 + 1;
static const size_t MOD_LEVEL_TITLE_MAX_LENGTH = 64 + 1;
static const size_t MOD_LEVEL_PATH_MAX_LENGTH = 256 + 1;
static const size_t MOD_LEVEL_THUMBNAIL_MAX_LENGTH = 256 + 1;

static const size_t MOD_CAMPAIGN_ID_MAX_LENGTH = 64 + 1;
static const size_t MOD_CAMPAIGN_TITLE_MAX_LENGTH = 64 + 1;
static const size_t MOD_CAMPAIGN_THUMBNAIL_MAX_LENGTH = 256 + 1;
static const size_t MOD_CAMPAIGN_MENU_MUSIC_PATH_MAX_LENGTH = 256 + 1;
static const size_t MOD_CAMPAIGN_MENU_SCRIPT_PATH_MAX_LENGTH = 256 + 1;
static const size_t MOD_CAMPAIGN_MAIN_SCRIPT_PATH_MAX_LENGTH = 256 + 1;

static const size_t MOD_ITEM_TITLE_MAX_LENGTH = 128 + 1;
static const size_t MOD_ITEM_CATEGORY_MAX_LENGTH = 128 + 1;
static const size_t MOD_ITEM_PATH_MAX_LENGTH = 256 + 1;

static const size_t MOD_POSE_NAME_MAX_LENGTH = 32 + 1;
static const size_t MOD_POSE_COMMAND_MAX_LENGTH = 32 + 1;
static const size_t MOD_POSE_PATH_MAX_LENGTH = 256 + 1;

enum ModSource {
    ModSourceUnknown,
    ModSourceLocalModFolder,
    ModSourceSteamworks
};

struct ModID {
    ModID();
    ModID(int id);
    int id;

    bool Valid() const;
    bool operator==( const ModID& modid ) const;
    bool operator!=( const ModID& modid ) const;
};

extern const ModID CoreGameModID;

struct ModValidity {
    ModValidity();
    ModValidity(uint16_t bit);
    ModValidity(uint64_t upper, uint64_t lower);
    
    ModValidity Intersection(const ModValidity& other) const;
    ModValidity Union(const ModValidity& other) const;
    ModValidity& Append(const ModValidity& other);

    bool Empty() const;
    bool NotEmpty() const;
    bool Intersects(const ModValidity& other) const;

    ModValidity operator&(const ModValidity& rhs) const;
    ModValidity operator|(const ModValidity& rhs) const;
    ModValidity operator~() const;

    ModValidity& operator|=(const ModValidity& rhs);
    bool operator!=(const ModValidity& rhs) const;

private:
    uint64_t upper;
    uint64_t lower;
    friend std::ostream& operator<<(std::ostream& os, const ModValidity &mi );
};

std::ostream& operator<<(std::ostream& os, const ModID &mi );
std::ostream& operator<<(std::ostream& os, const ModValidity &mi );
