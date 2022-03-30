//-----------------------------------------------------------------------------
//           Name: base_loader.h
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

#include <Internal/referencecounter.h>
#include <Internal/integer.h>

class baseLoader
{
public:
    virtual ~baseLoader();

    //If two channels, PCM data is interleaved left then right.
    virtual int stream_buffer_int16(char *buffer, int size) = 0;
    virtual unsigned long get_sample_count() = 0;
    virtual unsigned long get_channels() = 0;
    virtual int get_sample_rate() = 0;
    virtual int rewind() = 0;
    virtual bool is_at_end() = 0;

    virtual int64_t get_pcm_pos() = 0;
    virtual void    set_pcm_pos( int64_t pos ) = 0;
};

typedef ReferenceCounter<baseLoader> rc_baseLoader;
