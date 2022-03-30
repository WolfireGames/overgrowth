//-----------------------------------------------------------------------------
//           Name: al_audio_source.cpp
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
#include "al_audio_source.h"

#include <Sound/soundlogging.h>
#include <Utility/assert.h>

#include <cassert>

alAudioSource::alAudioSource( )
: src( INVALID_SRC )
{
}

alAudioSource::~alAudioSource()
{
    if( IsValid() )
    {
        alSourceStop(src);
        alDeleteSources(1, &src);        
    }
}

void alAudioSource::Allocate()
{
    alGenSources(1,&src); 

    if( !IsValid() )
    {
        ALenum e = alGetError();
        LOGW << "Unable to generate audio source: " << e << std::endl;
        src = INVALID_SRC;
    }
}

void alAudioSource::SetBuffer( rc_alAudioBuffer buf )
{
    LOG_ASSERT( queuedBuffers.size() == 0 ); //Verify that we aren't using this for buffered audio.

    boundBuffer = buf;

    if( boundBuffer->IsValid() )
    {
        alSourcei( src, AL_BUFFER, buf->buf );
    }
}

void alAudioSource::QueueBuffers( std::vector<rc_alAudioBuffer> d )
{
    LOG_ASSERT( !boundBuffer->IsValid() ); //Verify that we haven't bound anything else.

    if( d.size() > 0 )
    {
        ALuint* buffers = (ALuint*)alloca( sizeof( ALuint ) * d.size() );

        for( unsigned i = 0; i < d.size(); i++ )
        {
            buffers[i] = d[i]->buf;
        }

        alSourceQueueBuffers( src, d.size(), buffers );

        ALenum err = alGetError();

        if( err != AL_NO_ERROR )
        {
            LOGW << "Unable to queue sound buffer: " << alErrString(err) << std::endl;
            std::copy( d.begin(), d.end(), back_inserter(failedQueuedBuffers) );
        }
        else
        {
            std::copy( d.begin(), d.end(), back_inserter(queuedBuffers) );
        }
    }
}

std::vector<rc_alAudioBuffer> alAudioSource::DequeueBuffers()
{
    ALint finished_count = GetSourcei( AL_BUFFERS_PROCESSED );
    std::vector<rc_alAudioBuffer> retbuf;
    retbuf.reserve(finished_count);

    if( finished_count > 0 )
    {
        ALuint *finished_ids = (ALuint*)alloca( sizeof( ALuint ) * finished_count );
        alSourceUnqueueBuffers( src, finished_count, finished_ids );

        for( int i = 0; i < finished_count; i++ )
        {
            for( unsigned k = 0; k < queuedBuffers.size(); k++ )
            {
                if( queuedBuffers[k]->buf == finished_ids[i] )
                {
                    retbuf.push_back(queuedBuffers[k]);
                    queuedBuffers.erase(queuedBuffers.begin() + k);
                    break;
                }
            }
        }
    }

    if( failedQueuedBuffers.size() > 0 )
    {
        std::copy( failedQueuedBuffers.begin(), failedQueuedBuffers.end(), back_inserter(retbuf) );
        failedQueuedBuffers.clear();
    }

    ALenum err = alGetError();

    if( err != AL_NO_ERROR )
    {
        LOGE << alErrString(err)  << ":" << src << std::endl;
    }

    return retbuf;
}

void alAudioSource::Sourcef( ALenum param, ALfloat value )
{
    alSourcef( src, param, value );
}

void alAudioSource::Source3f( ALenum param, ALfloat v1, ALfloat v2, ALfloat v3 )
{
    alSource3f( src, param, v1, v2, v3 );
}

void alAudioSource::Sourcei( ALenum param, ALint value )
{
    LOG_ASSERT( param != AL_BUFFER );
    alSourcei( src, param, value );
}

ALint alAudioSource::GetSourcei( ALenum pname )
{
    ALint value;
    alGetSourcei( src, pname, &value );
    return value;
}

void alAudioSource::Play()
{
    alSourcePlay(src);

    ALenum err = alGetError();

    if( err != AL_NO_ERROR )
    {
        LOGE << alErrString(err) << std::endl;
    }
}

void alAudioSource::Stop()
{
    alSourceStop(src);
}

bool alAudioSource::IsValid()
{
    return src != INVALID_SRC && alIsSource(src);
}

