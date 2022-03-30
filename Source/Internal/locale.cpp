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
#include <Scripting/angelscript/add_on/scriptarray/scriptarray.h>

#include <map>
#include <string>

typedef std::map<std::string, std::string> LocaleMap;
static LocaleMap locales;

struct LevelLocalizationData {
    std::string name;
    std::string loading_tip;
};
typedef std::map<std::string, LevelLocalizationData> MapDataMap; // Maps level path -> per-level data
typedef std::map<std::string, MapDataMap> LocalizedLevelMap; // Maps locale shortcode -> map of level data
static LocalizedLevelMap localized_levels;

void ClearLocale() {
    localized_levels.clear();
    locales.clear();
}

void AddLocale(const char* shortcode, const char* name, const char* local_name) {
    if(local_name && strlen(local_name) > 0) {
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
    if(loc_it != localized_levels.end()) {
        MapDataMap::iterator it = loc_it->second.find(level);
        if(it != loc_it->second.end()) {
            return it->second.loading_tip.c_str();
        }
    }
    return NULL;
}

static CScriptArray* ASGetLocaleShortcodes() {
    asIScriptContext *ctx = asGetActiveContext();
    asIScriptEngine *engine = ctx->GetEngine();
    asITypeInfo *arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<string>"));
    CScriptArray *array = CScriptArray::Create(arrayType, (asUINT)0);
    array->Reserve(locales.size());

    for(LocaleMap::iterator iter = locales.begin(); iter != locales.end(); ++iter) {
        // InsertLast doesn't actually do anything but copy from the pointer,
        // so a const_cast would be fine, but maybe an update to AS could change
        // that
        std::string str = iter->first;
        array->InsertLast(&str);
    }

    return array;
}

static CScriptArray* ASGetLocaleNames() {
    asIScriptContext *ctx = asGetActiveContext();
    asIScriptEngine *engine = ctx->GetEngine();
    asITypeInfo *arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<string>"));
    CScriptArray *array = CScriptArray::Create(arrayType, (asUINT)0);
    array->Reserve(locales.size());

    for(LocaleMap::iterator iter = locales.begin(); iter != locales.end(); ++iter) {
        // InsertLast doesn't actually do anything but copy from the pointer,
        // so a const_cast would be fine, but maybe an update to AS could change
        // that
        std::string str = iter->second;
        array->InsertLast(&str);
    }

    return array;
}

static std::string ASGetLevelName(const std::string& shortcode, const std::string& path) {
    LocalizedLevelMap::iterator loc_it = localized_levels.find(shortcode);
    if(loc_it != localized_levels.end()) {
        MapDataMap::iterator it = loc_it->second.find("Data/Levels/" + path);
        if(it != loc_it->second.end()) {
            return it->second.name;
        }
    }
    return "";
}

void AttachLocale(ASContext* context) {
    context->RegisterGlobalFunction("array<string>@ GetLocaleShortcodes()", asFUNCTION(ASGetLocaleShortcodes), asCALL_CDECL);
    context->RegisterGlobalFunction("array<string>@ GetLocaleNames()", asFUNCTION(ASGetLocaleNames), asCALL_CDECL);
    context->RegisterGlobalFunction("string GetLocalizedLevelName(const string &in locale_shortcode, const string &in path)", asFUNCTION(ASGetLevelName), asCALL_CDECL);
}
