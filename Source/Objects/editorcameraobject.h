//-----------------------------------------------------------------------------
//           Name: editorcameraobject.h
//      Developer: Wolfire Games LLC
//         Author: Phillip Isola
//    Description: Camera object for the editors
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

#include <Objects/cameraobject.h>

#ifdef _DEBUG
#include <stdio.h>
#endif

class EditorCameraObject: public CameraObject {
public:
    virtual EntityType GetType() const { return _camera_type; }
    EditorCameraObject() {
        frozen=false;
        speed=12;
        rotation=0;
        rotation2=0;
    };

    virtual ~EditorCameraObject()
    {
#ifdef _DEBUG
        printf("editor camera destroyed\n");
#endif
    }
    
    virtual void IgnoreInput(bool val);
    void IgnoreMouseInput(bool val);
    virtual void GetDisplayName(char* buf, int buf_size);
};
