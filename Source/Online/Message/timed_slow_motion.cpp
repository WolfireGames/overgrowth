//-----------------------------------------------------------------------------
//           Name: timed_slow_motion.cpp
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
#include "timed_slow_motion.h"

#include <Main/engine.h>
#include <Online/online.h>

extern Timer game_timer;

namespace OnlineMessages {
TimedSlowMotion::TimedSlowMotion(float target_time_scale, float how_long, float delay) : OnlineMessageBase(OnlineMessageCategory::LEVEL_TRANSIENT),
                                                                                         target_time_scale(target_time_scale),
                                                                                         how_long(how_long),
                                                                                         delay(delay) {
}

binn* TimedSlowMotion::Serialize(void* object) {
    TimedSlowMotion* tsm = static_cast<TimedSlowMotion*>(object);
    binn* l = binn_object();

    binn_object_set_float(l, "tts", tsm->target_time_scale);
    binn_object_set_float(l, "hl", tsm->how_long);
    binn_object_set_float(l, "d", tsm->delay);

    return l;
}

void TimedSlowMotion::Deserialize(void* object, binn* l) {
    TimedSlowMotion* tsm = static_cast<TimedSlowMotion*>(object);

    binn_object_get_float(l, "tts", &tsm->target_time_scale);
    binn_object_get_float(l, "hl", &tsm->how_long);
    binn_object_get_float(l, "d", &tsm->delay);
}

void TimedSlowMotion::Execute(const OnlineMessageRef& ref, void* object, PeerID peer) {
    TimedSlowMotion* tsm = static_cast<TimedSlowMotion*>(object);

    game_timer.AddTimedSlowMotionLayer(tsm->target_time_scale, tsm->how_long, tsm->delay);
}

void* TimedSlowMotion::Construct(void* mem) {
    return new (mem) TimedSlowMotion(.0f, .0f, .0f);
}

void TimedSlowMotion::Destroy(void* object) {
    TimedSlowMotion* tsm = static_cast<TimedSlowMotion*>(object);
    tsm->~TimedSlowMotion();
}
}  // namespace OnlineMessages
