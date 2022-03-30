//-----------------------------------------------------------------------------
//           Name: buffer_segment.cpp
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
#include "buffer_segment.h" 

BufferSegment::BufferSegment(): sample_rate(0), channels(0), data_size(0), error_code(0) {

}

size_t BufferSegment::FullSampleCount() const {
    if( channels > 0 ) {
        return data_size/SAMPLE_SIZE/channels;
    } else {
        return 0;
    }
}

size_t BufferSegment::FlatSampleCount() const {
    return data_size/SAMPLE_SIZE;
}

size_t BufferSegment::SampleSize() const {
    return SAMPLE_SIZE;
}

