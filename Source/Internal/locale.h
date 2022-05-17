//-----------------------------------------------------------------------------
//           Name: locale.h
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

#include <vector>
#include <string>
#include <map>

typedef std::map<std::string, std::string> LocaleMap;

struct LevelLocalizationData {
    std::string name;
    std::string loading_tip;
};
typedef std::map<std::string, LevelLocalizationData> MapDataMap;  // Maps level path -> per-level data
typedef std::map<std::string, MapDataMap> LocalizedLevelMap;      // Maps locale shortcode -> map of level data

const LocaleMap& GetLocales();
const LocalizedLevelMap& GetLocalizedLevelMaps();

void ClearLocale();

void AddLocale(const char* shortcode, const char* name, const char* local_name);
void AddLevelName(const char* shortcode, const char* level, const char* name);
void AddLevelTip(const char* shortcode, const char* level, const char* tip);

const char* GetLevelTip(const char* shortcode, const char* level);