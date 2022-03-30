//-----------------------------------------------------------------------------
//           Name: levelxml_script.cpp
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
#include "levelxml_script.h"

#include <Internal/levelxml.h>
#include <Internal/filesystem.h>

#include <Scripting/angelscript/ascontext.h>

namespace {
    struct LevelInfoReader {
        LevelInfo li;
        void Load(const std::string& rel_path){
            char abs_path[kPathSize];
            if(FindFilePath(rel_path.c_str(), abs_path, kPathSize, kDataPaths|kModPaths) == -1){
                FatalError("Error", "Could not find level info: %s", rel_path.c_str());
            } else {
                ParseLevelXML(abs_path, li);       
            }
        }
        const std::string& visible_name(){return li.visible_name_;}
        const std::string& visible_description(){return li.visible_description_;}
    };
    void LevelInfoReader_Constructor(void *memory) {
        new(memory) LevelInfo();
    }
    void LevelInfoReader_Destructor(void *memory) {
        ((LevelInfoReader*)memory)->~LevelInfoReader();
    }
} // ANONYMOUS NAMESPACE

void AttachLevelXML( ASContext* as_context ) {
    as_context->RegisterObjectType("LevelInfoReader", sizeof(LevelInfo), asOBJ_VALUE | asOBJ_APP_CLASS);
    as_context->RegisterObjectBehaviour("LevelInfoReader", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(LevelInfoReader_Constructor), asCALL_CDECL_OBJLAST);
    as_context->RegisterObjectBehaviour("LevelInfoReader", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(LevelInfoReader_Destructor), asCALL_CDECL_OBJLAST);
    as_context->RegisterObjectMethod("LevelInfoReader", "void Load(const string &in)", asMETHOD(LevelInfoReader, Load), asCALL_THISCALL);
    as_context->RegisterObjectMethod("LevelInfoReader", "const string& visible_name()", asMETHOD(LevelInfoReader, visible_name), asCALL_THISCALL);
    as_context->RegisterObjectMethod("LevelInfoReader", "const string& visible_description()", asMETHOD(LevelInfoReader, visible_description), asCALL_THISCALL);
    as_context->DocsCloseBrace();
}
