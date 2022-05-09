//-----------------------------------------------------------------------------
//           Name: actorfile.cpp
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

#include "actorfile.h"

#include <Asset/Asset/character.h>
#include <Asset/Asset/material.h>
#include <Asset/AssetLoader/fallbackassetloader.h>
#include <Asset/assetloaderrors.h>

#include <XML/xml_helper.h>
#include <Internal/filesystem.h>
#include <Graphics/shaders.h>
#include <Scripting/scriptfile.h>
#include <Logging/logdata.h>
#include <Main/engine.h>

#include <tinyxml.h>

#include <cmath>
#include <map>
#include <string>

using std::string;

extern string script_dir_path;

ActorFile::ActorFile(AssetManager* owner, uint32_t asset_id) : AssetInfo(owner, asset_id), load_error(0) {
}

int ActorFile::Load(const string& rel_path, uint32_t load_flags) {
    load_error = 0;
    char abs_path[kPathSize];
    if (FindFilePath(rel_path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths | kAbsPath) == -1) {
        DisplayFormatError(_ok_cancel, true, "Error", "Actor file %s not found", rel_path.c_str());
        return kLoadErrorNoFile;
    }

    TiXmlDocument doc(abs_path);
    doc.LoadFile();

    if (doc.Error()) {
        return kLoadErrorCorruptFile;
    }

    if (!XmlHelper::getNodeValue(doc, "Actor/Character", character)) {
        DisplayError("Error", "Character name not found. Aborting actor load.");
        load_error = 1;
        return kLoadErrorIncompleteXML;
    }

    if (!XmlHelper::getNodeValue(doc, "Actor/ControlScript", script)) {
        DisplayError("Error", "Control script not found. Aborting actor load.");
        load_error = 2;
        return kLoadErrorIncompleteXML;
    }

    return kLoadOk;
}

const char* ActorFile::GetLoadErrorString() {
    switch (load_error) {
        case 0:
            return "";
        case 1:
            return "Character name not found";
        case 2:
            return "Control script not found";
        default:
            return "undefined error code";
    }
}

void ActorFile::Unload() {
}

void ActorFile::Reload() {
}

void ActorFile::ReportLoad() {
}

void ActorFile::ReturnPaths(PathSet& path_set) {
    path_set.insert("actor " + path_);
    // Characters::Instance()->ReturnRef(character)->ReturnPaths(path_set);
    Engine::Instance()->GetAssetManager()->LoadSync<Character>(character)->ReturnPaths(path_set);
    Path script_path = FindFilePath(script_dir_path + script, kDataPaths | kModPaths);
    ScriptFileUtil::ReturnPaths(script_path, path_set);
}

AssetLoaderBase* ActorFile::NewLoader() {
    return new FallbackAssetLoader<ActorFile>();
}
