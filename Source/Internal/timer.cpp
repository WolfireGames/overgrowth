//-----------------------------------------------------------------------------
//           Name: timer.cpp
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

#include <Internal/timer.h>
#include <Internal/common.h>
#include <Internal/datemodified.h>
#include <Internal/filesystem.h>

#include <Compat/fileio.h>
#include <Compat/time.h>

#include <Threading/sdl_wrapper.h>
#include <Math/enginemath.h>

#include <SDL.h>

#include <ctime>
#include <cstdlib>

//-----------------------------------------------------------------------------
//Functions
//-----------------------------------------------------------------------------

static const float _time_inertia = 0.6f;

Timer game_timer;
Timer ui_timer;

void Timer::UpdateWallTime() {
    wall_ticks = SDL_TS_GetTicks();
    wall_time = wall_ticks / 1000.0f;
}

void Timer::Update() {
    frame_count++;
	uint32_t tick = SDL_TS_GetTicks();
	float min_target_time_scale = target_time_scale;
    for(std::vector<TimedSlowMotionLayer>::iterator iter = timed_slow_motion_layers.begin(); 
		iter != timed_slow_motion_layers.end();)
	{
        TimedSlowMotionLayer& layer = (*iter);
		if(layer.start_time <= tick){
			min_target_time_scale = min(min_target_time_scale, layer.target_time_scale);
        }
        if(layer.end_time <= tick){
            iter = timed_slow_motion_layers.erase(iter);
        } else {
            iter++;
        }
    }
    time_scale = mix(min_target_time_scale, time_scale, _time_inertia);
    game_time += timestep;
}

float Timer::GetInterpWeight(){
    return timestep_error;
}

float Timer::GetInterpWeightX(int num, int progress){
    float mult = (1.0f/(float)num);
    return mult*(float)progress + timestep_error*mult;
}


float Timer::GetRenderTime() {
    return game_time + timestep_error * timestep;
}

float Timer::GetAverageFrameTime() { 
	float sum = .0f;
	for (unsigned long i : frame) {
		sum += i;
	}

	return sum / (float)NUM_AVERAGED_FRAMES;
}

//Set simulations per second
void Timer::SetStepFrequency(int sims) {
	simulations_per_second = sims;
	uint32_t tick = SDL_TS_GetTicks();
    last_tick = tick;
    timestep = 1/((float)simulations_per_second);
    srand(tick);
}

//Get number of timeSteps to use this display frame
int Timer::GetStepsNeeded() {
	uint32_t tick = SDL_TS_GetTicks();
    int num; 
	num=((int)((tick-last_tick)*simulations_per_second*time_scale)/1000);
	last_tick+=(int)((num*1000)/(simulations_per_second*time_scale));
	timestep_error=(tick-last_tick)/1000.0f*simulations_per_second*time_scale;
    return num;
}

//We have rendered another frame, so gather fps data
void Timer::ReportFrameForFPSCount() {
    for (int i = 0;i<NUM_AVERAGED_FRAMES-1;i++){
        frame[i]=frame[i+1];
    }
    frame[NUM_AVERAGED_FRAMES-1]=GetPrecisionTime();
}

//Calculate fps
int Timer::GetFramesPerSecond() {
    uint64_t frame_diff = ToNanoseconds(frame[NUM_AVERAGED_FRAMES-1] - frame[0]);
    if (frame_diff != 0) {
        return (int) (1000*NUM_AVERAGED_FRAMES/(frame_diff * 0.000001));
    } else {
		return 0;
	}
}

float Timer::GetFrameTime() {
    uint64_t frame_diff = ToNanoseconds(frame[NUM_AVERAGED_FRAMES-1] - frame[0]);
    if (frame_diff != 0) {
        return (frame_diff * 0.000001f)/NUM_AVERAGED_FRAMES;
    } else {
		return 0;
	}
}

float Timer::GetWallTime() {
    return wall_time;
}

uint32_t Timer::GetWallTicks() {
    return wall_ticks;
}

float Timer::GetSlowestFrameTime() {
    uint64_t slowest = frame[1] - frame[0];
    for(int i = 1; i < NUM_AVERAGED_FRAMES; ++i) {
        slowest = std::max(slowest, frame[i] - frame[i - 1]);
    }
    return ToNanoseconds(slowest) * 0.000001f;
}

float Timer::GetFastestFrameTime() {
    int64_t fastest = frame[1] - frame[0];
    for(int i = 1; i < NUM_AVERAGED_FRAMES; ++i) {
        int64_t current = frame[i] - frame[i - 1];
        if(fastest == 0 || current < fastest)
            fastest = current;
    }
    return ToNanoseconds(fastest) * 0.000001f;
}

void Timer::AddTimedSlowMotionLayer( float target_time_scale, float duration, float delay ) {
    timed_slow_motion_layers.resize(timed_slow_motion_layers.size()+1);
    TimedSlowMotionLayer& layer = timed_slow_motion_layers.back();
    layer.target_time_scale = target_time_scale;
	layer.start_time = SDL_TS_GetTicks() + (int)(delay * 1000.0f);
	layer.end_time = layer.start_time + (int)(duration * 1000.0f);
}

Timer::Timer():
    frame_count(0),
	target_time_scale(1.0f),
	time_scale(1.0f),
	game_time(0),
	simulations_per_second(30),
	last_tick(0),
    wall_time(0)
{
    for (auto& i : frame) {
        i = 0;
    }
}
