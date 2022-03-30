//-----------------------------------------------------------------------------
//           Name: levelset_script.cpp
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
#include "levelset_script.h"

#include <Asset/Asset/levelset.h>
#include <Scripting/angelscript/ascontext.h>
#include <Utility/assert.h>
#include <Main/engine.h>

#include <cassert>

class LevelSetReader {
private:
    LevelSetRef lsr_;
    LevelSet::LevelPaths::const_iterator iter_;
    int ref_count_;
public:
    LevelSetReader(){
        LOG_ASSERT(false);
    }
    LevelSetReader(const std::string &path){
        //lsr_ = LevelSets::Instance()->ReturnRef(path);
        lsr_ = Engine::Instance()->GetAssetManager()->LoadSync<LevelSet>(path);
        iter_ = lsr_->level_paths().begin();
        ref_count_ = 1;
    }
    void AddRef() {
        ++ref_count_;
    }
    void Release() {
        --ref_count_;
        if(ref_count_ == 0){
            delete this;
        }
    }
    bool Next(std::string& str){
        if(iter_ == lsr_->level_paths().end()){
            return false;
        }
        str = (*iter_);
        ++iter_;
        return true;
    }
};

LevelSetReader *LevelSetReader_DefaultFactory() {
    return new LevelSetReader();
}

LevelSetReader *LevelSetReader_Factory(const std::string& path) {
    return new LevelSetReader(path);
}

void AttachLevelSet( ASContext* as_context ) {
    as_context->RegisterObjectType("LevelSetReader", 0, asOBJ_REF);
    as_context->RegisterObjectBehaviour("LevelSetReader", asBEHAVE_FACTORY, "LevelSetReader@ f(const string &in path)", asFUNCTION(LevelSetReader_Factory), asCALL_CDECL);
    as_context->RegisterObjectBehaviour("LevelSetReader", asBEHAVE_FACTORY, "LevelSetReader@ f()", asFUNCTION(LevelSetReader_DefaultFactory), asCALL_CDECL);
    as_context->RegisterObjectBehaviour("LevelSetReader", asBEHAVE_ADDREF, "void f()", asMETHOD(LevelSetReader,AddRef), asCALL_THISCALL);
    as_context->RegisterObjectBehaviour("LevelSetReader", asBEHAVE_RELEASE, "void f()", asMETHOD(LevelSetReader,Release), asCALL_THISCALL);    
    as_context->RegisterObjectMethod("LevelSetReader", "bool Next(string &out str)", asMETHOD(LevelSetReader, Next), asCALL_THISCALL);
    as_context->DocsCloseBrace();
}
