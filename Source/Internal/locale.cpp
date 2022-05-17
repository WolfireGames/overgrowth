//-----------------------------------------------------------------------------
//           Name: locale.cpp
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
#include "locale.h"

#include <Utility/strings.h>

static LocaleMap locales;
static LocalizedLevelMap localized_levels;

const LocaleMap& GetLocales() {
    return locales;
}

const LocalizedLevelMap& GetLocalizedLevelMaps() {
    return localized_levels;
}

void ClearLocale() {
    localized_levels.clear();
    locales.clear();
}

void AddLocale(const char* shortcode, const char* name, const char* local_name) {
    if (local_name && strlen(local_name) > 0) {
        locales.insert(std::pair<std::string, std::string>(std::string(shortcode), std::string(name) + "/" + std::string(local_name)));
    } else {
        locales.insert(std::pair<std::string, std::string>(std::string(shortcode), std::string(name)));
    }
}

void AddLevelName(const char* shortcode, const char* level, const char* name) {
    localized_levels[shortcode][level].name = name;
}

void AddLevelTip(const char* shortcode, const char* level, const char* tip) {
    localized_levels[shortcode][level].loading_tip = tip;
}

const char* GetLevelTip(const char* shortcode, const char* level) {
    LocalizedLevelMap::iterator loc_it = localized_levels.find(shortcode);
    if (loc_it != localized_levels.end()) {
        MapDataMap::iterator it = loc_it->second.find(level);
        if (it != loc_it->second.end()) {
            return it->second.loading_tip.c_str();
        }
    }
    return NULL;
}
