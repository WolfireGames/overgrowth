//-----------------------------------------------------------------------------
//           Name: creak.as
//      Developer: Wolfire Games LLC
//    Script Type: Hotspot
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

float delay = 5.0f;
array<string> sounds = {
    "Data/Sounds/ambient/amb_forest_wood_creak_1.wav",
    "Data/Sounds/ambient/amb_forest_wood_creak_2.wav",
    "Data/Sounds/ambient/amb_forest_wood_creak_3.wav"
};

void UpdateSounds() {
    delay -= time_step;
    if (delay >= 0.0f) {
        return;
    }
    delay = RangedRandomFloat(0.1, 10.0f);
    PlayRandomCreakSound();
}

void PlayRandomCreakSound() {
    MovementObject@ player = ReadCharacterID(player_id);
    vec3 random_offset = vec3(
        RangedRandomFloat(-10.0f, 10.0f),
        RangedRandomFloat(-10.0f, 10.0f),
        RangedRandomFloat(-10.0f, 10.0f)
    );
    vec3 position = player.position + random_offset;
    string sound = sounds[rand() % sounds.size()];
    PlaySound(sound, position);
}
