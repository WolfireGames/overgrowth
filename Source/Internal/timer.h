//-----------------------------------------------------------------------------
//           Name: timer.h
//      Developer: Wolfire Games LLC
//         Author: David Rosen
//    Description: The timer handles fixed or dynamic timeSteps
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

#include <vector>
#include <cstdint>

struct TimedSlowMotionLayer {
    uint32_t end_time;
	uint32_t start_time;
	float target_time_scale;
};

class Timer {
public:
	Timer();

	float target_time_scale;
	float time_scale;
	float timestep;
	float timestep_error;
	float game_time;
    float wall_time;
    uint32_t wall_ticks;
    int updates_since_last_frame;
    int simulations_per_second;
    uint64_t frame_count;

	void AddTimedSlowMotionLayer( float target_time_scale, float how_long, float delay = 0.0f );
    void UpdateWallTime();
	void Update();
	void SetStepFrequency(int sims);
	int GetStepsNeeded();
	void ReportFrameForFPSCount();
	int GetFramesPerSecond();
	float GetFrameTime();
	float GetSlowestFrameTime();
	float GetFastestFrameTime();
	float GetInterpWeight();
	float GetInterpWeightX(int num, int progress);
    float GetRenderTime();
    float GetWallTime();
    uint32_t GetWallTicks();
	float GetAverageFrameTime();

private:
    uint32_t last_tick;
	static const int NUM_AVERAGED_FRAMES = 30;
    uint64_t frame[NUM_AVERAGED_FRAMES];
    std::vector<TimedSlowMotionLayer> timed_slow_motion_layers;
};
