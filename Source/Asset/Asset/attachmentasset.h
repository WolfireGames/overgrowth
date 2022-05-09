//-----------------------------------------------------------------------------
//           Name: attachmentasset.h
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
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
#include <Math/vec4.h>

#include <Asset/assetbase.h>
#include <Asset/assetref.h>

#include <string>

using std::string;

class Attachment : public Asset {
   public:
    Attachment(AssetManager* owner, uint32_t asset_id);
    static AssetType GetType() { return ATTACHMENT_ASSET; }
    static const char* GetTypeName() { return "ATTACHMENT_ASSET"; }

    string anim;
    string ik_chain_label;
    int ik_chain_bone;
    bool mirror_allowed;

    int Load(const string& path, uint32_t load_flags);
    const char* GetLoadErrorString();
    const char* GetLoadErrorStringExtended() { return ""; }

    void Unload();
    void Reload();
    void ReportLoad() override;

    void clear();

    AssetLoaderBase* NewLoader() override;
    static bool AssetWarning() { return true; }
};

typedef AssetRef<Attachment> AttachmentRef;
