//-----------------------------------------------------------------------------
//           Name: reactionscript.cpp
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
#include "reactionscript.h"

#include <Scripting/angelscript/ascontext.h>
#include <Main/engine.h>

void ReactionScriptGetter::Load( std::string _path ) {
    path = _path;
    if(_path[_path.size()-2] == ' ' &&
       _path[_path.size()-1] == 'm') 
    {
       mirror = true;
       path.resize(path.size()-2);
    } else {
        mirror = false;
    }
    for(auto & item : items){
        if(item->HasReactionOverride(path)){
            path = item->GetReactionOverride(path);
        }
    }
    //reaction_ref = Reactions::Instance()->ReturnRef(path);
    reaction_ref = Engine::Instance()->GetAssetManager()->LoadSync<Reaction>(path);
}

std::string ReactionScriptGetter::GetAnimPath(float severity) {
    return reaction_ref->GetAnimPath(severity);
}

void ReactionScriptGetter::ItemsChanged( const std::vector<ItemRef> &_items ) {
    items = _items;
}

void ReactionScriptGetter::AttachToScript( ASContext *as_context, const std::string& as_name ) {
    as_context->RegisterObjectType("ReactionScriptGetter", 0, asOBJ_REF | asOBJ_NOHANDLE);
    as_context->RegisterObjectMethod("ReactionScriptGetter",
                                     "void Load(string path)",
                                     asMETHOD(ReactionScriptGetter, Load), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ReactionScriptGetter",
                                     "string GetAnimPath(float severity)",
                                     asMETHOD(ReactionScriptGetter, GetAnimPath), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ReactionScriptGetter",
                                     "int GetMirrored()",
                                     asMETHOD(ReactionScriptGetter, GetMirrored), asCALL_THISCALL);
    as_context->DocsCloseBrace();

    as_context->RegisterGlobalProperty(("ReactionScriptGetter "+as_name).c_str(), this);
}

void ReactionScriptGetter::AttachExtraToScript( ASContext *as_context, const std::string& as_name ) {
    as_context->RegisterGlobalProperty(("ReactionScriptGetter "+as_name).c_str(), this);
}

int ReactionScriptGetter::GetMirrored() {
    if(reaction_ref->IsMirrored() == 2){
        return 2;
    } else {
        bool mirrored = mirror;
        if(reaction_ref->IsMirrored()){
            mirrored = !mirrored;
        }
        return (int)(mirrored);
    }
}
