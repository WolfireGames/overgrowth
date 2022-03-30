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
#pragma once

#include <vector>

class IMGUI;
class IMElement;

class IMEventListener {
public:
    virtual void DestroyedIMElement( IMElement* element ) = 0;
    virtual void DestroyedIMGUI( IMGUI* IMGUI ) = 0;
};

class IMEvents {
private:
    std::vector<IMEventListener*> listeners;
public:
    void RegisterListener( IMEventListener *eventlistener );
    void DeRegisterListener( IMEventListener *eventlistener );

    void TriggerDestroyed(IMElement* elem);
    void TriggerDestroyed(IMGUI* imgui);
};

extern IMEvents imevents;
