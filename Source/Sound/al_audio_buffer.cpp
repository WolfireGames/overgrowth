//-----------------------------------------------------------------------------
//           Name: al_audio_buffer.cpp
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
#include "al_audio_buffer.h"

#include <Sound/soundlogging.h>

alAudioBuffer::alAudioBuffer( )
: buf( INVALID_BUF )
{
}

void alAudioBuffer::Allocate()
{
    ALenum e = alGetError();
    if( e != AL_NO_ERROR ) {
        LOGW << "Entering alAudioBuffer with error: " << e << " " << alGetString(e) << std::endl;
    }

    alGenBuffers(1, &buf);

    if( !IsValid() )
    {
        e = alGetError();
        LOGW << "Unable to generate audio buffer: " << e << " " << alGetString(e) << std::endl;
        buf = INVALID_BUF;
    }
}

alAudioBuffer::~alAudioBuffer()
{
    if( IsValid() )
    {
        alDeleteBuffers(1, &buf);
        buf = INVALID_BUF;
    }
}

void alAudioBuffer::BufferData(ALenum format, const ALvoid *data, ALsizei size, ALsizei freq)
{
    if( !IsValid() )
    {
        Allocate();
    }

    if( IsValid() )
    {
        alBufferData(buf, format, data, size, freq ); 

        ALenum err = alGetError();

        if( err != AL_NO_ERROR )
        {
            LOGE << alErrString(err)  << std::endl;
        }
    }
    else
    {
        LOGE << "Can't buffer, buffer is invalid after attempted (re)allocation" << std::endl;
    }
}

bool alAudioBuffer::IsValid()
{
    return buf != INVALID_BUF && alIsBuffer(buf);
}
