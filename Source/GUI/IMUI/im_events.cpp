//-----------------------------------------------------------------------------
//           Name: im_events.cpp
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
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
#include "im_events.h"

#include <GUI/IMUI/imgui.h>
#include <GUI/IMUI/im_element.h>

IMEvents imevents;

void IMEvents::RegisterListener(IMEventListener* listener) {
    listeners.push_back(listener);
}

void IMEvents::DeRegisterListener(IMEventListener* listener) {
    for (int i = listeners.size() - 1; i >= 0; i--) {
        if (listeners[i] == listener) {
            listeners.erase(listeners.begin() + i);
        }
    }
}

void IMEvents::TriggerDestroyed(IMElement* elem) {
    for (auto& listener : listeners) {
        listener->DestroyedIMElement(elem);
    }
}

void IMEvents::TriggerDestroyed(IMGUI* imgui) {
    for (auto& listener : listeners) {
        listener->DestroyedIMGUI(imgui);
    }
}
