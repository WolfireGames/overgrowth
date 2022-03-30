//-----------------------------------------------------------------------------
//           Name: scriptstdstring_extend.cpp
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
#include "scriptstdstring_extend.h"

#include <Scripting/angelscript/ascontext.h>
#include <Math/vec3.h>

#include <string>
#include <sstream>

using namespace std;

static void AssignVec32StringGeneric(asIScriptGeneric *gen) {
    vec3 *a = static_cast<vec3*>(gen->GetAddressOfArg(0));
    string *self = static_cast<string*>(gen->GetObject());
    std::stringstream sstr;
    sstr << *a;
    *self = sstr.str();
    gen->SetReturnAddress(self);
}

static void AddAssignVec32StringGeneric(asIScriptGeneric * gen) {
    vec3 *a = static_cast<vec3*>(gen->GetAddressOfArg(0));
    string * self = static_cast<string *>(gen->GetObject());
    std::stringstream sstr;
    sstr << *a;
    *self += sstr.str();
    gen->SetReturnAddress(self);
}
static void AddString2Vec3Generic(asIScriptGeneric * gen) {
    string * a = static_cast<string *>(gen->GetObject());
    vec3 *b = static_cast<vec3*>(gen->GetAddressOfArg(0));
    std::stringstream sstr;
    sstr << *a << *b;
    std::string ret_val = sstr.str();
    gen->SetReturnObject(&ret_val);
}

static void AddVec32StringGeneric(asIScriptGeneric * gen) {
    vec3 *a = static_cast<vec3*>(gen->GetAddressOfArg(0));
    string *b = static_cast<string *>(gen->GetObject());
    std::stringstream sstr;
    sstr << *a << *b;
    std::string ret_val = sstr.str();
    gen->SetReturnObject(&ret_val);
}

void RegisterStdString_Extend(ASContext* ctx) {
    ctx->RegisterObjectMethod("string", "string &opAssign(vec3)", asFUNCTION(AssignVec32StringGeneric), asCALL_GENERIC, "These 4 functions allow us to append vec3s to strings");
    ctx->RegisterObjectMethod("string", "string &opAddAssign(vec3)", asFUNCTION(AddAssignVec32StringGeneric), asCALL_GENERIC);
    ctx->RegisterObjectMethod("string", "string opAdd(vec3) const", asFUNCTION(AddString2Vec3Generic), asCALL_GENERIC);
    ctx->RegisterObjectMethod("string", "string opAdd_r(vec3) const", asFUNCTION(AddVec32StringGeneric), asCALL_GENERIC);
}
