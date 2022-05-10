//-----------------------------------------------------------------------------
//           Name: high_res_buffer_segment.h
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

#include <Internal/integer.h>

#include <cmath>
#include <cstring>

class HighResBufferSegment {
   public:
    HighResBufferSegment();

    size_t FullSampleCount() const;
    size_t FlatSampleCount() const;
    size_t SampleSize() const;

    static const size_t SAMPLE_SIZE = 4;
    static const size_t BUF_SIZE = 4096 * 2 * SAMPLE_SIZE;

    size_t sample_rate;
    size_t channels;

    size_t data_size;
    int error_code;
    char buf[BUF_SIZE];
};
