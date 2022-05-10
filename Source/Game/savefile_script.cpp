//-----------------------------------------------------------------------------
//           Name: savefile_script.cpp
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
#include "savefile_script.h"

#include <Scripting/angelscript/add_on/scriptarray/scriptarray.h>
#include <Scripting/angelscript/ascontext.h>

#include <Game/savefile.h>

#include <angelscript.h>

static ASContext* local_ctx = NULL;

static CScriptArray* AS_GetArray(SavedLevel* save_file, const std::string& key) {
    asIScriptContext* ctx = local_ctx->ctx;
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<string>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);

    std::vector<std::string> source_arr = save_file->GetArray(key);

    array->Reserve(source_arr.size());

    std::vector<std::string>::iterator layerit = source_arr.begin();

    for (; layerit != source_arr.end(); layerit++) {
        array->InsertLast((void*)&(*layerit));
    }

    return array;
}

void AttachSaveFile(ASContext* ctx, SaveFile* save_file) {
    local_ctx = ctx;
    ctx->RegisterObjectType("SavedLevel", 0, asOBJ_REF | asOBJ_NOCOUNT);
    ctx->RegisterObjectMethod(
        "SavedLevel",
        "void SetValue(const string &in key, const string &in value)",
        asMETHOD(SavedLevel, SetValue),
        asCALL_THISCALL);
    ctx->RegisterObjectMethod(
        "SavedLevel",
        "const string& GetValue(const string &in key)",
        asMETHOD(SavedLevel, GetValue),
        asCALL_THISCALL);

    ctx->RegisterObjectMethod(
        "SavedLevel",
        "void SetArrayValue(const string &in key, const int32 index, const string &in value)",
        asMETHOD(SavedLevel, SetArrayValue),
        asCALL_THISCALL);
    ctx->RegisterObjectMethod(
        "SavedLevel",
        "void DeleteArrayValue(const string &in key, const int32 index)",
        asMETHOD(SavedLevel, DeleteArrayValue),
        asCALL_THISCALL);
    ctx->RegisterObjectMethod(
        "SavedLevel",
        "void AppendArrayValueIfUnique(const string &in key, const string &in val)",
        asMETHOD(SavedLevel, AppendArrayValueIfUnique),
        asCALL_THISCALL);
    ctx->RegisterObjectMethod(
        "SavedLevel",
        "void AppendArrayValue(const string &in key, const string &in val)",
        asMETHOD(SavedLevel, AppendArrayValue),
        asCALL_THISCALL);
    ctx->RegisterObjectMethod(
        "SavedLevel",
        "uint32 GetArraySize(const string &in key)",
        asMETHOD(SavedLevel, GetArraySize),
        asCALL_THISCALL);
    ctx->RegisterObjectMethod(
        "SavedLevel",
        "string GetArrayValue(const string &in key, const int32 index)",
        asMETHOD(SavedLevel, GetArrayValue),
        asCALL_THISCALL);
    ctx->RegisterObjectMethod(
        "SavedLevel",
        "array<string>@ GetArray(const string &in key)",
        asFUNCTION(AS_GetArray),
        asCALL_CDECL_OBJFIRST);

    ctx->RegisterObjectMethod(
        "SavedLevel",
        "void SetInt32Value(const string &in key, const int32 value)",
        asMETHOD(SavedLevel, SetInt32Value),
        asCALL_THISCALL);
    ctx->RegisterObjectMethod(
        "SavedLevel",
        "int32 GetInt32Value(const string &in key)",
        asMETHOD(SavedLevel, GetInt32Value),
        asCALL_THISCALL);
    ctx->RegisterObjectMethod(
        "SavedLevel",
        "bool HasInt32Value(const string &in key)",
        asMETHOD(SavedLevel, HasInt32Value),
        asCALL_THISCALL);

    ctx->RegisterObjectMethod(
        "SavedLevel",
        "void SetKey(const string &in modsource_id, const string &in campaign_name, const string &in level_name)",
        asMETHOD(SavedLevel, SetKey),
        asCALL_THISCALL);
    ctx->DocsCloseBrace();

    ctx->RegisterObjectType("SaveFile", 0, asOBJ_REF | asOBJ_NOHANDLE);
    ctx->RegisterObjectMethod(
        "SaveFile",
        "SavedLevel& GetSavedLevel(const string &in name)",
        asMETHOD(SaveFile, GetSavedLevelDeprecated),
        asCALL_THISCALL);
    ctx->RegisterObjectMethod(
        "SaveFile",
        "SavedLevel& GetSave(const string campaign_id, const string save_category, const string save_name)",
        asMETHOD(SaveFile, GetSave),
        asCALL_THISCALL);
    ctx->RegisterObjectMethod(
        "SaveFile",
        "bool SaveExist(const string modsource_id, const string save_category, const string save_name)",
        asMETHOD(SaveFile, SaveExist),
        asCALL_THISCALL);
    ctx->RegisterObjectMethod(
        "SaveFile",
        "bool WriteInPlace()",
        asMETHOD(SaveFile, WriteInPlace),
        asCALL_THISCALL);
    ctx->RegisterObjectMethod(
        "SaveFile",
        "void QueueWriteInPlace()",
        asMETHOD(SaveFile, WriteInPlace),
        asCALL_THISCALL);
    ctx->RegisterObjectMethod(
        "SaveFile",
        "uint GetLoadedVersion()",
        asMETHOD(SaveFile, GetLoadedVersion),
        asCALL_THISCALL);
    ctx->DocsCloseBrace();

    ctx->RegisterGlobalProperty("SaveFile save_file", save_file);
}
