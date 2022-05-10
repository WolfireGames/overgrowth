//-----------------------------------------------------------------------------
//           Name: transition_mixer.cpp
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
#include "transition_mixer.h"

#include <Logging/logdata.h>
#include <Utility/assert.h>

#include <cmath>
#include <algorithm>

#define PI 3.141592653589793238462643383279502884197169399375105820974944592308

TransitionMixer::TransitionMixer(TransitionType trans_type, float transition_length_seconds) : transition_type(trans_type),
                                                                                               transition_length_sec(transition_length_seconds),
                                                                                               transition_position(0) {
}

bool TransitionMixer::Step(HighResBufferSegment* out, const HighResBufferSegment* first, const HighResBufferSegment* second) {
    LOG_ASSERT(first->sample_rate == second->sample_rate);
    LOG_ASSERT(first->channels == second->channels);

    out->sample_rate = first->sample_rate;
    out->channels = first->channels;
    out->data_size = out->SampleSize() * std::max(first->FlatSampleCount(), second->FlatSampleCount());

    size_t start = transition_position;
    // How many bytes for a second of transition times the time of transition.
    size_t end = (size_t)(transition_length_sec * first->sample_rate * first->channels);

    size_t i = 0;

    // This casting union is possible because we take endianess into account when we get the first from the underlying loader.
    // If we use architecture inspecific endianess we have to bitshift this data instead.
    // Also note that use of a union for aliasing is not strictly allowed until C99, but generally implemented and faster than shifting.
    union casterunion {
        int32_t full;
        char part[4];
    };

    casterunion out1;
    casterunion buf1;
    casterunion buf2;

    // Crossfade the two firsts together.
    for (i = 0; i < out->FlatSampleCount(); i++) {
        float d = (float)(start + i) / (float)(end);
        if (d > 1.0f) {
            d = 1.0f;
        }

        float dl;
        float dr;
        if (transition_type == Sinusoid) {
            // Crossfade in a sinusoid to preserve the energy.
            float dp = d * (float)PI / 2;
            dl = std::sin(dp);
            dr = std::cos(dp);
        } else if (transition_type == Linear) {
            dl = d;
            dr = 1.0f - d;
        } else {
            LOGE << "No transition function used" << std::endl;

            dl = 1.0f;
            dr = 0.0f;
        }

        if (i < first->FlatSampleCount()) {
            buf1.part[0] = first->buf[i * 4 + 0];
            buf1.part[1] = first->buf[i * 4 + 1];
            buf1.part[2] = first->buf[i * 4 + 2];
            buf1.part[3] = first->buf[i * 4 + 3];
        } else {
            buf1.full = 0;
        }

        if (i < second->FlatSampleCount()) {
            buf2.part[0] = second->buf[i * 4 + 0];
            buf2.part[1] = second->buf[i * 4 + 1];
            buf2.part[2] = second->buf[i * 4 + 2];
            buf2.part[3] = second->buf[i * 4 + 3];
        } else {
            buf2.full = 0;
        }

        out1.full = (int32_t)(buf1.full * dr + buf2.full * dl);

        out->buf[i * 4 + 0] = out1.part[0];
        out->buf[i * 4 + 1] = out1.part[1];
        out->buf[i * 4 + 2] = out1.part[2];
        out->buf[i * 4 + 3] = out1.part[3];
    }

    // If it happened that the second first had more data than the first, indicate that
    //  we continued writing into the main first so we don't miss that segment of audio
    //  that was streamed.
    transition_position += i;

    if (transition_position >= end) {
        transition_position = end;
        return true;
    }
    return false;
}

void TransitionMixer::Reset() {
    transition_position = 0;
}
