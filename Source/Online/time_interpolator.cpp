//-----------------------------------------------------------------------------
//           Name: time_interpolator.cpp
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
#include "time_interpolator.h"

#include <Internal/timer.h>
#include <Logging/logdata.h>

#include <cassert>
#include <cmath>

extern Timer game_timer;

Timestamps::Timestamps() : timestamps(nullptr), timestamp_size(0), timestamp_count(0) {
    SetSize(16);
}

Timestamps::~Timestamps() {
    free(timestamps);
    timestamp_size = 0;
    timestamp_count = 0;
}

void Timestamps::Clear() {
    timestamp_count = 0;
}

void Timestamps::PushValue(float value) {
    assert(timestamp_size > 0);

    if (timestamp_size <= timestamp_count) {
        SetSize(timestamp_size + 16);
    }

    timestamps[timestamp_count] = value;
    timestamp_count++;
}

void Timestamps::Pop() {
    assert(timestamp_size > 0);

    for (int i = timestamp_count - 1; i >= 1; i--) {
        timestamps[i - 1] = timestamps[i];
    }
    timestamp_count--;
}

void Timestamps::SetSize(int size) {
    if (size > timestamp_size) {
        float* new_timestamps = (float*)realloc(timestamps, sizeof(float) * size);
        if (new_timestamps != nullptr) {
            timestamps = new_timestamps;
            timestamp_size = size;
        } else {
            LOGF << "realloc() returned a null pointer, out of memory" << std::endl;
        }
    }
}

int Timestamps::size() {
    return timestamp_count;
}

float Timestamps::from_begin(int offset) {
    return timestamps[offset];
}

float Timestamps::from_end(int offset) {
    return timestamps[timestamp_count - 1 - offset];
}

float TimeInterpolator::offset_shift_coefficient_factor = 5.0f;
int TimeInterpolator::target_window_size = 5;

int TimeInterpolator::Update() {
    if (timestamps.size() > 1) {
        current_walltime = game_timer.GetWallTime();

        virtual_host_walltime = current_walltime + host_walltime_offset;

        float current_frame_walltime = timestamps.from_begin(0);
        float next_frame_walltime = timestamps.from_begin(1);
        float second_last_frame_walltime = timestamps.from_end(1);
        float last_frame_walltime = timestamps.from_end(0);

        // The two frames we are trying to interpolate between are on the same time
        if (current_frame_walltime == next_frame_walltime) {
            LOGE << "Trying to interpolate between two identical timestamps! This can lead to a nan value in the system!" << std::endl;
            timestamps.Pop();
            return 3;
        }

        // We are beyond the last frame time, wait for enough to start interpolating and set it to the resting point
        if (virtual_host_walltime > last_frame_walltime) {
            if (timestamps.size() >= 2 * target_window_size) {
                float third_last_frame_walltime = timestamps.from_end(target_window_size);
                // Land on what the algorithm sees as the resting-point
                host_walltime_offset = third_last_frame_walltime - current_walltime;
                LOGI << "Resetting host_walltime_offset to: " << host_walltime_offset << std::endl;
                return 1;
            } else {
                return 3;
            }
        }
        // We are ahead the first frame time, wait for enough to start interpolating and set it to the resting point
        if (virtual_host_walltime < current_frame_walltime) {
            // If we behind the current frame, skip ahead to the resting-position in accordance with the
            // speed adjusting algo, so we don't create a bouncing interpolating effect.
            if (timestamps.size() >= 2 * target_window_size) {
                float third_last_frame_walltime = timestamps.from_end(target_window_size);
                // Land on what the algorithm sees as the resting-point
                host_walltime_offset = third_last_frame_walltime - current_walltime;

                if ((host_walltime_offset + current_walltime) < current_frame_walltime) {
                    // If this is true, then we will get stuck, it's true to what is probably a rounding error
                    // this is a band aid solution
                    float diff = next_frame_walltime - (host_walltime_offset + current_walltime);
                    host_walltime_offset += diff / 2.0f;
                }
                LOGI << "Data missing, jumping ahead to catch up" << std::endl;
                return 1;
            } else {
                return 3;
            }
        }

        // If we're past the next bones timestamp, pop the bones from the stack. and do the next one.
        if (virtual_host_walltime > next_frame_walltime) {
            return 2;
        }

        interpolation_step = (virtual_host_walltime - current_frame_walltime) / (next_frame_walltime - current_frame_walltime);

        // Rough estimate how much data there's left in the buffer.
        float current_window_size = timestamps.size() - interpolation_step;

        // Pow function that ranges from -1.0f to 1.0f returning a shift direction accordingly.
        // If we have target_window_size data left in the buffer, this should return 0.0f, meaning no time shift.
        host_walltime_offset_shift_vel = offset_shift_coefficient_factor * std::pow((current_window_size - (float)target_window_size) / (float)target_window_size, 3.0f);

        host_walltime_offset += host_walltime_offset_shift_vel * game_timer.timestep;
    }
    return 0;
}
