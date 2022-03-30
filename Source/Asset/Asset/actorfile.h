//-----------------------------------------------------------------------------
//           Name: actorfile.h
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

#include <Math/vec3.h>
#include <Graphics/palette.h>
#include <Asset/assetbase.h>
#include <Asset/assetinfobase.h>

#include <string>
#include <vector>

using std::string;

class ActorFile : public AssetInfo {
public:
    ActorFile( AssetManager *owner, uint32_t asset_id );
    static AssetType GetType() { return ACTOR_FILE_ASSET; }
    static const char* GetTypeName() { return "ACTOR_FILE_ASSET"; }

    string script;
    string character;

    int load_error;

    int Load(const string &path, uint32_t load_flags);
    const char* GetLoadErrorString();
    const char* GetLoadErrorStringExtended() { return ""; }

    void Unload();
    void Reload();
    virtual void ReportLoad();

    void ReturnPaths(PathSet& path_set);

    virtual AssetLoaderBase* NewLoader();

    static bool AssetWarning() { return true; }
};

typedef AssetRef<ActorFile> ActorFileRef;
