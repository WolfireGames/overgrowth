//-----------------------------------------------------------------------------
//           Name: time_interpolator.h
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

class Timestamps {
   private:
    float* timestamps;
    int timestamp_size;
    int timestamp_count;

   public:
    Timestamps();
    ~Timestamps();

    void Clear();
    void PushValue(float value);
    void Pop();
    void SetSize(int size);
    int size();
    float from_begin(int offset);
    float from_end(int offset);
};

class TimeInterpolator {
   public:
    Timestamps timestamps;

    float virtual_host_walltime = 0.f;
    float current_walltime = 0.f;
    float host_walltime_offset = 0.f;

    float interpolation_time = 0.f;
    float interpolation_step = 0.f;

    float full_interpolation = 0.f;
    float host_walltime_offset_shift_acc = 0.f;
    float host_walltime_offset_shift_vel = 0.f;

    static float offset_shift_coefficient_factor;
    static int target_window_size;

    int Update();
};
