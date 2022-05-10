//-----------------------------------------------------------------------------
//           Name: al_audio_source.h
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

#include <Sound/al_audio_buffer.h>
#include <Wrappers/openal.h>
#include <Internal/referencecounter.h>

#include <vector>

class alAudioSource {
   public:
    alAudioSource();
    ~alAudioSource();

    void Sourcef(ALenum param, ALfloat value);
    void Source3f(ALenum param, ALfloat v1, ALfloat v2, ALfloat v3);
    void Sourcei(ALenum param, ALint value);

    ALint GetSourcei(ALenum pname);

    void Allocate();
    void SetBuffer(rc_alAudioBuffer buf);
    void QueueBuffers(std::vector<rc_alAudioBuffer> d);
    std::vector<rc_alAudioBuffer> DequeueBuffers();

    void Play();
    void Stop();

    bool IsValid();

   private:
    static const ALuint INVALID_SRC = 0xFFFFFFFF;
    ALuint src;
    rc_alAudioBuffer boundBuffer;
    std::vector<rc_alAudioBuffer> queuedBuffers;
    std::vector<rc_alAudioBuffer> failedQueuedBuffers;
};

typedef ReferenceCounter<alAudioSource> rc_alAudioSource;
