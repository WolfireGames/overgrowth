//-----------------------------------------------------------------------------
//           Name: cameraobject.cpp
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

#include <Internal/common.h>
#include <Internal/profiler.h>
#include <Internal/timer.h>

#include <Graphics/camera.h>
#include <Graphics/graphics.h>

#include <Scripting/angelscript/ascontext.h>
#include <Scripting/angelscript/asfuncs.h>
#include <Scripting/angelscript/ascollisions.h>

#include <Sound/sound.h>
#include <UserInput/input.h>
#include <Objects/cameraobject.h>
#include <Game/level.h>
#include <Editors/map_editor.h>
#include <Physics/bulletworld.h>
#include <Online/online.h>
#include <Main/scenegraph.h>

#include <SDL_assert.h>

extern std::string script_dir_path;

//Camera cannot see itself
void CameraObject::Draw() {
}

void CameraObject::GetDisplayName(char* buf, int buf_size) {
    if( GetName().empty() ) {
        FormatString(buf, buf_size, "%d, Camera", GetID());
    } else {
        FormatString(buf, buf_size, "%s, Camera", GetName().c_str());
    }
}

void CameraObject::saveStateToFile(FILE *){
}

//Give camera fps controls
void CameraObject::Update(float timestep) {
    static const std::string kUpdateStr = "void Update()";
	as_context->CallScriptFunction(kUpdateStr);
}

void CameraObject::Reload() {
	as_context->Reload();
}

bool CameraObject::Initialize() {
    ASData as_data;
    as_data.scenegraph = scenegraph_;
    as_data.gui = scenegraph_->map_editor->gui;
    as_context = new ASContext("camera_object",as_data);
    AttachUIQueries(as_context);
    AttachActiveCamera(as_context);
    AttachScreenWidth(as_context);
    AttachPhysics(as_context);
    AttachEngine(as_context);
    AttachScenegraph(as_context, scenegraph_);
    AttachTextCanvasTextureToASContext(as_context);
    AttachLevel(as_context);
    AttachInterlevelData(as_context);
    AttachMessages(as_context);
    AttachTokenIterator(as_context);
    AttachStringConvert(as_context);
    AttachOnline(as_context);

    as_collisions = new ASCollisions(scenegraph_);
    as_collisions->AttachToContext(as_context);
    
    as_context->RegisterObjectType("CameraObject", 0, asOBJ_REF | asOBJ_NOHANDLE);
    as_context->RegisterObjectProperty("CameraObject","vec3 velocity",asOFFSET(CameraObject,velocity));
    as_context->RegisterObjectProperty("CameraObject","bool controlled",asOFFSET(CameraObject,controlled));
    as_context->RegisterObjectProperty("CameraObject","bool frozen",asOFFSET(CameraObject,frozen));
    as_context->RegisterObjectProperty("CameraObject","bool ignore_mouse_input",asOFFSET(CameraObject,ignore_mouse_input));
    as_context->RegisterObjectProperty("CameraObject","bool has_position_initialized",asOFFSET(CameraObject,has_position_initialized));
    as_context->RegisterObjectMethod("CameraObject","const vec3& GetTranslation()",asMETHOD(CameraObject,GetTranslation), asCALL_THISCALL);
    as_context->RegisterObjectMethod("CameraObject","const quaternion& GetRotation()",asMETHOD(CameraObject,GetRotation), asCALL_THISCALL);
    as_context->RegisterObjectMethod("CameraObject","void SetTranslation(const vec3 &in vec)",asMETHOD(CameraObject,SetTranslation), asCALL_THISCALL);
    as_context->RegisterObjectMethod("CameraObject","void SetRotation(const quaternion &in quat)",asMETHOD(CameraObject,SetRotation), asCALL_THISCALL);
    as_context->DocsCloseBrace();
    as_context->RegisterGlobalProperty("CameraObject co", this);

    as_funcs.init               = as_context->RegisterExpectedFunction("void Init()",true);
    as_funcs.frame_selection    = as_context->RegisterExpectedFunction("void FrameSelection(bool)",true);

    PROFILER_ENTER(g_profiler_ctx, "Exporting docs");
    char path[kPathSize];
    FormatString(path, kPathSize, "%sascameraobject_docs.h", GetWritePath(CoreGameModID).c_str());
    as_context->ExportDocs(path);
    PROFILER_LEAVE(g_profiler_ctx);

    Path script_path = FindFilePath(script_dir_path+"cam.as", kDataPaths | kModPaths);

    if( script_path.isValid() )  {
        if( as_context->LoadScript(script_path)) {
            as_context->CallScriptFunction(as_funcs.init);

            SDL_assert(update_list_entry == -1);
            update_list_entry = scenegraph_->LinkUpdateObject(this);
            return true;
        } else {
            FatalError("Error", "We don't handle the case of an invalid camera, fix the camera script");
            return false;
        }
    } else {
        FatalError("Error", "We don't handle the case of an invalid camera, missing camera script file.");
        return false;

    }
}

void CameraObject::FrameSelection(bool v) {
    if(as_context) {
        ASArglist args;
        args.Add(v);
        as_context->CallScriptFunction(as_funcs.frame_selection, &args);
    }
}

CameraObject::~CameraObject() {
    delete as_context;
    delete as_collisions;
}

