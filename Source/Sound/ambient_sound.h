//-----------------------------------------------------------------------------
//           Name: ambient_sound.h
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

#include <Sound/al_audio.h>

class ambientSound : public AudioEmitter {
   public:
    ambientSound(float volume = 1.0f);

    /// Set to false to allow the emitter to time out (useful only on non-looping sounds)
    bool KeepPlaying() override;
    /// if true is returned, this will be a relative-to-listener sound
    bool GetPosition(vec3 &p) override;
    const vec3 GetPosition() override;
    const vec3 GetOcclusionPosition() override;
    void GetDirection(vec3 &p) override;
    const vec3 &GetVelocity() override;

    /// allows you to change the global alAudio pitch multiplier for this sound
    float GetPitchMultiplier() override;

    /// allows you to change the global alAudio gain multiplier for this sound
    float GetVolume() override;

    /// Indicate the priority of this effect to make room for newer/higher priority effects
    unsigned char GetPriority() override;

    void SetVolume(float volume) override;
    void SetPitch(float pitch);
    bool IsTransient() override { return false; }

   private:
    float m_volume;
    float m_pitch;
    vec3 m_velocity;
};
