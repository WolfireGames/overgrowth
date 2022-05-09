//-----------------------------------------------------------------------------
//           Name: al_audio_buffer.h
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

#include <Wrappers/openal.h>
#include <Internal/referencecounter.h>

#include <cstdlib>

class alAudioBuffer {
   public:
    alAudioBuffer();
    ~alAudioBuffer();
    void BufferData(ALenum format, const ALvoid *data, ALsizei size, ALsizei freq);

    void Allocate();
    bool IsValid();

   private:
    static const ALuint INVALID_BUF = 0xFFFFFFFF;
    ALuint buf;
    friend class alAudioSource;
};

typedef ReferenceCounter<alAudioBuffer> rc_alAudioBuffer;
