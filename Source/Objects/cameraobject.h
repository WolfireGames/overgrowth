//-----------------------------------------------------------------------------
//           Name: cameraobject.h
//      Developer: Wolfire Games LLC
//         Author: David Rosen
//    Description: The camera object is an entity representing the camera (viewer)
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

#include <Scripting/angelscript/asmodule.h>
#include <Scripting/angelscript/asarglist.h>
#include <Scripting/angelscript/ascontext.h>

#include <Objects/object.h>

//-----------------------------------------------------------------------------
// Class Definition
//-----------------------------------------------------------------------------
class ASContext;
class ASCollisions;

class CameraObject: public Object {
    public:
        bool controlled;
        ASContext* as_context;
        ASCollisions *as_collisions;
        float speed;
        float rotation, rotation2;
        float target_rotation, target_rotation2;
        bool frozen;
        vec3 velocity;
        float smooth_speed;
        bool ignore_mouse_input;
        
        bool has_position_initialized;

        struct {
            ASFunctionHandle init;
            ASFunctionHandle frame_selection;
        } as_funcs;

        CameraObject() {
            has_position_initialized = true;
            frozen=false;
            speed=5;
            rotation=0;
            rotation2=0;
            target_rotation=0;
            target_rotation2=0;
            velocity = vec3(0.0f);
            smooth_speed = 0;
            ignore_mouse_input = false;
            as_context = NULL;
            as_collisions = NULL;
            controlled = false;
            permission_flags = 0;
            exclude_from_undo = true;
        }

        ~CameraObject() override;
        
        EntityType GetType() const override {return _camera_type;}
        
        void saveStateToFile(FILE *);
        void Update(float timestep) override;
		void Reload() override;
		void Draw() override;
        void GetDisplayName(char* buf, int buf_size) override;
        
        virtual void IgnoreInput(bool val) {}
        virtual void IgnoreMouseInput(bool val) {}
        bool Initialize() override;
        vec3 GetMouseRay();

        void FrameSelection(bool v);

};
