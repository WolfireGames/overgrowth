//-----------------------------------------------------------------------------
//           Name: soundgroup.cpp
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
#include "soundgroup.h"

#include <Asset/AssetLoader/fallbackassetloader.h>
#include <XML/xml_helper.h>
#include <Internal/scoped_buffer.h>
#include <Logging/logdata.h>
#include <Main/engine.h>

#include <tinyxml.h>
#include <sstream>

SoundGroup::SoundGroup(AssetManager* owner, uint32_t asset_id) : Asset(owner, asset_id), sub_error(0) {
}

void SoundGroup::Reload() {
    Load(path_, 0x0);
}

int SoundGroup::Load(const std::string& path, uint32_t load_flags) {
    // SoundGroupInfoCollection::Instance()->ReturnRef(path);
    sound_group_info = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroupInfo>(path);

    history_size = int(float(sound_group_info->GetNumVariants()) / 2.0f);
    history.clear();
    history.resize(history_size, -1);
    history_index = 0;

    LOGD.Format("Soundgroup: %s\n", path.c_str());
    // LOGD.Format("Num variants: %d\n", num_variants);
    // LOGD.Format("Delay: %f\n", delay);
    // LOGD.Format("Volume: %f\n", volume);
    return kLoadOk;
}

const char* SoundGroup::GetLoadErrorString() {
    switch (sub_error) {
        case 0:
            return "";
        default:
            return "Undefined Errors";
    }
}

void SoundGroup::Unload() {
}

std::string SoundGroup::GetSoundPath() {
    bool bad_choice = true;
    int choice;
    while (bad_choice) {
        choice = rand() % (sound_group_info->GetNumVariants());
        bad_choice = false;
        for (int i = 0; i < history_size; ++i) {
            if (choice == history[i]) {
                bad_choice = true;
                break;
            }
        }
    }

    if (history_size) {
        history[history_index++] = choice;
        history_index = history_index % history_size;
    }

    return sound_group_info->GetSoundPath(choice);
}

std::string SoundGroup::GetPath() const {
    return path_;
}

int SoundGroup::GetNumVariants() const {
    return sound_group_info->GetNumVariants();
}

float SoundGroup::GetVolume() const {
    return sound_group_info->GetVolume();
}

float SoundGroup::GetDelay() const {
    return sound_group_info->GetDelay();
}

float SoundGroup::GetMaxDistance() const {
    return sound_group_info->GetMaxDistance();
}

AssetLoaderBase* SoundGroup::NewLoader() {
    return new FallbackAssetLoader<SoundGroup>();
}
