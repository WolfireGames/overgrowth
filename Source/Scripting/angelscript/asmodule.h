//-----------------------------------------------------------------------------
//           Name: asmodule.h
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

#include <Asset/assets.h>
#include <Scripting/scriptfile.h>
#include <JSON/jsonhelper.h>
#include <Internal/integer.h>
#include <Logging/logdata.h>

#include <angelscript.h>

#include <string>
#include <vector>
#include <list>
#include <map>

class asIScriptModule;
class asIScriptEngine;
class asIScriptFunction;

struct AppObject;
struct ScriptObject;
struct TemplateObject;

enum VarStorageType {
    _vs_unknown = 1,
    _vs_bool = 2,
    _vs_int8 = 3,
    _vs_int32 = 4,
    _vs_int64 = 5,
    _vs_uint8 = 6,
    _vs_uint32 = 7,
    _vs_uint64 = 8,
    _vs_float = 9,
    _vs_enum = 10,
    _vs_app_obj = 11,
    _vs_script_obj = 12,
    _vs_template = 13,
    _vs_app_obj_handle = 14,
    _vs_script_obj_handle = 15,
    _vs_template_handle = 16,
    _vs_int16 = 17,
    _vs_uint16 = 18
};

enum ValueParent {
    _vp_Global,
    _vp_ScriptObject,
    _vp_Array
};

enum AppObjectType {
    _ao_vec3,
    _ao_vec4,
    _ao_ivec2,
    _ao_ivec3,
    _ao_ivec4,
    _ao_string,
    // -- Begin scenegraph objects --
    _ao_ambient_sound_object,
    _ao_camera_object,
    _ao_decal_object,
    _ao_dynamic_light_object,
    _ao_env_object,
    _ao_game_object,
    _ao_group_object,
    _ao_hotspot_object,
    _ao_item_object,
    _ao_light_probe_object,
    _ao_light_volume_object,
    _ao_movement_object,
    _ao_navmesh_connection_object,
    _ao_navmesh_hint_object,
    _ao_navmesh_region_object,
    _ao_path_point_object,
    _ao_placeholder_object,
    _ao_reflection_capture_object,
    _ao_rigged_object,
    _ao_terrain_object,
    // -- End scenegraph objects --
    _ao_attack_script_getter,
    _ao_bone_transform,
    _ao_quaternion,
    _ao_vec2,
    _ao_nav_path,
    _ao_mat4,
    _ao_json,
    _ao_fontsetup,
    _ao_modlevel,
    _ao_textureassetref,
    _ao_modid,
    _ao_spawneritem,
    _ao_IMFadeIn,
    _ao_IMMoveIn,
    _ao_IMChangeTextFadeOutIn,
    _ao_IMChangeImageFadeOutIn,
    _ao_IMPulseAlpha,
    _ao_IMPulseBorderAlpha,
    _ao_IMMouseOverMove,
    _ao_IMMouseOverScale,
    _ao_IMMouseOverShowBorder,
    _ao_IMMouseOverPulseColor,
    _ao_IMMouseOverPulseBorder,
    _ao_IMMouseOverPulseBorderAlpha,
    _ao_IMMouseOverFadeIn,
    _ao_IMFixedMessageOnMouseOver,
    _ao_IMFixedMessageOnClick,
    _ao_IMMessage,
    _ao_IMGUI,
    _ao_IMElement,
    _ao_IMContainer,
    _ao_IMDivider,
    _ao_IMImage,
    _ao_IMText,
    _ao_IMTextSelectionList,
    _ao_IMSpacer,
    _ao_IMUIImage,
    _ao_IMUIText,
    _ao_dictionary,
    _ao_ignored,
    _ao_unknown
};

struct VarStorage {
    VarStorageType type;
    int size;
    std::string name;
    std::string type_name;
    void* var;
    VarStorage();
    void destroy();
    std::string GetString(unsigned depth = 0);
};

struct ScriptObjectInstance {
    ScriptObjectInstance(uintptr_t ptr, VarStorage& var, bool from_script_obj_value);
    void destroy();
    uintptr_t ptr;
    bool reconstructed;
    bool from_script_obj_value;  // This means there's a value version of this app object that all handles should re-refer to.
    asIScriptObject* new_ptr;    // New value generated
    VarStorage var;
};

struct AppObject {
    AppObjectType type;
    void* original;
    void* var;
    AppObject();
    void destroy();
};

struct AppObjectHandle {
    AppObjectType type;
    void* var;
    AppObjectHandle();
    void destroy();
};

enum TemplateObjectType { _to_array,
                          _to_unknown };

struct TemplateObject {
    TemplateObjectType type;
    std::string sub_type_name;
    void* var;
    TemplateObject();
    void destroy();
};

struct ScriptObject {
    uintptr_t orig_ptr;  // Original ptr.
    std::string type_name;
    std::string name_space;
    std::list<VarStorage> storage;
    void Populate(asIScriptObject* obj, std::list<ScriptObjectInstance>& handle_var_list, asIScriptModule* module);
    void destroy();
};

struct ScriptObjectInstanceHandleRemap {
    ValueParent var_type;
    void* parent_ptr;
    int parent_index;
    uintptr_t orig_ptr;
};

class ASModule {
    asIScriptModule* module_;
    const ScriptFile* script_file_ptr_;

    std::map<void*, void*> fast_var_ptr_map;
    std::map<std::string, asIScriptFunction*> func_map_;
    Path script_path_;
    std::list<VarStorage> saved_vars_;
    std::list<ScriptObjectInstance> saved_handle_vars_;
    int64_t modified_;

    std::string* active_angelscript_error_string;

    ASModule(const ASModule&);
    ASModule& operator=(const ASModule&);

   public:
    ASModule()
        : active_angelscript_error_string(NULL) {}
    ~ASModule();

    void SetErrorStringDestination(std::string* error_string);

    asIScriptModule* GetInternalScriptModule();
    void AttachToEngine(asIScriptEngine* engine);
    int CompileScript(const Path& path);
    bool SourceChanged();
    const Path& GetScriptPath();
    LineFile GetCorrectedLine(int line);
    void ResetGlobals();
    void Recompile();
    void PrintGlobalVars();
    void PrintAllTypes();
    void LogGlobalVars();
    void PrintScriptClassVars(asIScriptObject* obj);
    void GetGlobalVars(std::list<VarStorage>& storage, std::list<ScriptObjectInstance>& handle_var_list);
    void SaveGlobalVars();
    void LoadGlobalVars();
    void LoadVar(ValueParent var_type, void* parent_ptr, int parent_index, const VarStorage& vs, void* ptr, std::list<ScriptObjectInstance>& handle_var_list, std::list<ScriptObjectInstanceHandleRemap>& app_obj_handle_offset, asIScriptModule* module);
    asIScriptFunction* GetFunctionID(const std::string& function_name);
    void* GetVarPtr(const char* name);
    void* GetVarPtrCache(const char* name);  // Only use if passing fixed const char*, not converted from string
    int CompileScriptFromText(const std::string& text);
};

const char* AppObjectTypeString(enum AppObjectType aot);

std::string GetTypeIDString(int type_id);
