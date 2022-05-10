//-----------------------------------------------------------------------------
//           Name: ambient_sound.cpp
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
#include "ambient_sound.h"

ambientSound::ambientSound(float volume) : m_volume(volume), m_pitch(1.0) {
}

/// Set to false to allow the emitter to time out (useful only on non-looping sounds)
bool ambientSound::KeepPlaying() {
    return true;
}

/// if true is returned, this will be a relative-to-listener sound
bool ambientSound::GetPosition(vec3 &p) {
    return true;
}

const vec3 ambientSound::GetPosition() {
    return vec3(0.0f);
}

void ambientSound::GetDirection(vec3 &p) {
    p.x() = 0.0f;
    p.y() = 0.0f;
    p.y() = 0.0f;
}

const vec3 &ambientSound::GetVelocity() {
    return m_velocity;
}

/// allows you to change the global alAudio pitch multiplier for this sound
float ambientSound::GetPitchMultiplier() {
    return m_pitch;
}

/// allows you to change the global alAudio gain multiplier for this sound
float ambientSound::GetVolume() {
    return m_volume;
}

/// Indicate the priority of this effect to make room for newer/higher priority effects
unsigned char ambientSound::GetPriority() {
    return _sound_priority_max;
}

void ambientSound::SetVolume(float volume) {
    m_volume = volume;
}

void ambientSound::SetPitch(float pitch) {
    m_pitch = pitch;
}

const vec3 ambientSound::GetOcclusionPosition() {
    return vec3(0.0f);
}
