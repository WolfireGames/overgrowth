//-----------------------------------------------------------------------------
//           Name: soundplayinfo.cpp
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
#include "soundplayinfo.h"

#include <Threading/rand.h>
#include <Threading/sdl_wrapper.h>

#include <Math/enginemath.h>
#include <Logging/logdata.h>

#include <sstream>

SoundPlayInfo::SoundPlayInfo() : position(0.0f),
                                 occlusion_position(0.0f),
                                 pitch(1.0f),
                                 volume(1.0f),
                                 volume_mult(1.0f),
                                 max_gain(1.0f),
                                 max_distance(30.0f),
                                 looping(false),
                                 flags(0),
                                 priority(_default_priority),
                                 created_by_id(-1),
                                 play_past_tick(0) {
}

SoundGroupPlayInfo::SoundGroupPlayInfo(const SoundGroup& sg, const vec3& _pos) : position(_pos),
                                                                                 occlusion_position(_pos),
                                                                                 gain(1.0f),
                                                                                 max_gain(1.0f),
                                                                                 volume_mult(1.0f),
                                                                                 pitch_shift(1.0f),
                                                                                 path(sg.GetPath()),
                                                                                 created_by_id(-1),
                                                                                 flags(0),
                                                                                 priority(_sound_priority_low),
                                                                                 num_variants(sg.GetNumVariants()),
                                                                                 volume(sg.GetVolume()),
                                                                                 max_distance(sg.GetMaxDistance()),
                                                                                 play_past_tick() {
    SetNumVariants(num_variants);
    play_past_tick = SDL_TS_GetTicks() + 1000 * (unsigned int)sg.GetDelay();
    sound_base_path = path.substr(0, path.size() - 4);
}

std::string SoundGroupPlayInfo::GetARandomOrderedSoundPath(pcg32_random_t* reseed) {
    const char* fstring = "%s_%d.wav";
    char target_s[kPathSize];

    int i = 0;
    if (num_variants > 0) {
        i = pcg32_random_r(reseed) % num_variants;

        if (previous_sound_id == i) {
            i = (i + 1) % num_variants;
        }
    }

    previous_sound_id = i;

    FormatString(target_s, kPathSize, fstring, sound_base_path.c_str(), i + 1);

    return std::string(target_s);
}

void SoundGroupPlayInfo::SetNumVariants(int _num_variants) {
    num_variants = _num_variants;
}

const std::string& SoundGroupPlayInfo::GetPath() const {
    return path;
}
