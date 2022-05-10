//-----------------------------------------------------------------------------
//           Name: limiter_audio_filter.cpp
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
#include "limiter_audio_filter.h"

#include <Logging/logdata.h>

#include <algorithm>
#include <cmath>

LimiterAudioFilter::LimiterAudioFilter() : target_limit_gain(1.0f), current_limit_gain(0.5f) {
    Reset();
}

bool LimiterAudioFilter::Step(HighResBufferSegment* buffer) {
    int peak_limit = 30000;

    int32_t peak = 0;
    int32_t peak_limit_count = 0;

    HighResBufferSegment in = *buffer;

    buffer->data_size = next_f.data_size;
    buffer->sample_rate = next_f.sample_rate;
    buffer->channels = next_f.channels;

    for (unsigned k = 0; k < in.FlatSampleCount(); k++) {
        int32_t buf1;
        char* buf1a = (char*)&buf1;

        buf1a[0] = in.buf[k * 4 + 0];
        buf1a[1] = in.buf[k * 4 + 1];
        buf1a[2] = in.buf[k * 4 + 2];
        buf1a[3] = in.buf[k * 4 + 3];

        if (abs(buf1) > peak) {
            peak = abs(buf1);
        }

        if (abs(buf1) > peak_limit) {
            peak_limit_count++;
        }
    }

    if (peak > peak_limit && peak_limit_count > 0) {
        target_limit_gain = peak_limit / (float)peak;
        frame_delay_limit = 3;  // number of frames buffered
        // LOGI << "Detected peaking, target_limit_gain:" << target_limit_gain << std::endl;
    } else {
        target_limit_gain = 1.0f;
    }

    for (unsigned k = 0; k < next_f.FlatSampleCount(); k++) {
        int32_t buf1;
        char* buf1a = (char*)&buf1;

        if ((k % next_f.channels) == 0) {
            if (target_limit_gain < current_limit_gain) {
                current_limit_gain -= 0.01f;
                if (target_limit_gain > current_limit_gain) {
                    current_limit_gain = target_limit_gain;
                }
            } else if (target_limit_gain > current_limit_gain) {
                if (frame_delay_limit <= 0) {
                    current_limit_gain += 0.005f;
                    if (target_limit_gain < current_limit_gain) {
                        current_limit_gain = target_limit_gain;
                    }
                }
            }
        }

        buf1a[0] = next_f.buf[k * 4 + 0];
        buf1a[1] = next_f.buf[k * 4 + 1];
        buf1a[2] = next_f.buf[k * 4 + 2];
        buf1a[3] = next_f.buf[k * 4 + 3];

        buf1 = buf1 * (int32_t)current_limit_gain;

        buffer->buf[k * 4 + 0] = buf1a[0];
        buffer->buf[k * 4 + 1] = buf1a[1];
        buffer->buf[k * 4 + 2] = buf1a[2];
        buffer->buf[k * 4 + 3] = buf1a[3];
    }

    next1 = in;
    next_f = next1;

    if (frame_delay_limit > 0) {
        frame_delay_limit--;
    }

    return true;
}

void LimiterAudioFilter::Reset() {
    current_limit_gain = 0.5f;
    target_limit_gain = 1.0f;

    memset(next1.buf, 0, HighResBufferSegment::BUF_SIZE);
    next1.data_size = 0;
    next1.channels = 0;

    memset(next_f.buf, 0, HighResBufferSegment::BUF_SIZE);
    next_f.data_size = 0;
    next_f.channels = 0;
}
