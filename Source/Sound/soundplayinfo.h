//-----------------------------------------------------------------------------
//           Name: soundplayinfo.h
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
#include <Internal/integer.h>
#include <Asset/Asset/soundgroup.h>
#include <Utility/pcg_basic.h>

#include <string>

enum FilterType {
    _simple_filter = 0,
    _convolution_filter = 1
};

enum SoundPriority {
    _sound_priority_max = 0,
    _sound_priority_very_high = 1,
    _sound_priority_high = 2,
    _sound_priority_med = 3,
    _sound_priority_low = 4
};

const unsigned char _default_priority = _sound_priority_med;

struct FilterInfo {
    std::string path;
    float wet;
    FilterType type;
    FilterInfo() : wet(0.0f) {}
};

struct SoundPlayInfo {
    vec3 position;
    vec3 occlusion_position;
    float pitch;
    float volume;
    float volume_mult;
    float max_gain;
    float max_distance;
    bool looping;
    uint8_t flags;
    unsigned char priority;
    std::string path;
    int created_by_id;
    FilterInfo filter_info;

    unsigned int play_past_tick;

    SoundPlayInfo();
};

struct SoundGroupPlayInfo {
    vec3 position;
    vec3 occlusion_position;
    float gain;
    float max_gain;
    float volume_mult;
    float pitch_shift;

   private:
    std::string path;
    int previous_sound_id;

   public:
    std::string specific_path;
    std::string sound_base_path;
    int created_by_id;
    uint8_t flags;
    unsigned char priority;
    FilterInfo filter_info;

    // Values from SoundGroup
    float volume;
    float max_distance;

    SoundGroupPlayInfo(const SoundGroup& sg, const vec3& _pos);

    std::string GetARandomOrderedSoundPath(pcg32_random_t* reseed);

    void SetNumVariants(int _num_variants);
    inline int GetNumVariants() { return num_variants; };

    unsigned int play_past_tick;

    const std::string& GetPath() const;

   private:
    int num_variants;
};
