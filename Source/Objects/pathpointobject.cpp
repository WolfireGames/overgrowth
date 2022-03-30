//-----------------------------------------------------------------------------
//           Name: pathpointobject.cpp
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

#include <Graphics/pxdebugdraw.h>
#include <Graphics/graphics.h>

#include <Objects/pathpointobject.h>
#include <Internal/memwrite.h>
#include <Game/EntityDescription.h>
#include <Editors/map_editor.h>
#include <Scripting/angelscript/ascontext.h>
#include <Main/scenegraph.h>

#include <algorithm>

extern bool g_debug_runtime_disable_pathpoint_object_draw;

//-----------------------------------------------------------------------------
//Functions
//-----------------------------------------------------------------------------

PathPointObject::PathPointObject() {
    permission_flags &= ~(Object::CAN_SCALE | Object::CAN_ROTATE);
    box_.dims = vec3(1.0f);
}

bool PathPointObject::ConnectTo( Object& other, bool checking_other /*= false*/ ) {
    if(other.GetType() == _movement_object){
        return other.ConnectTo(*this, true);
    } else if(other.GetType() == _hotspot_object) {
        return Object::ConnectTo(other, checking_other);
    } else if(other.GetType() != _path_point_object){
        return false;
    } else { // Connecting to another pathpoint
        PathPointObject* ppo = (PathPointObject*)&other;
        if(ppo == this){ // Can't connect to itself
            return false;
        } else {
            int other_id = ppo->GetID();
            for(unsigned i=0; i<connection_ids.size(); ++i){ // Disconnect if we're already connected
                if(connection_ids[i] == other_id){
                   Disconnect(other, checking_other);
                }       
            }    
            if(!checking_other){
                connection_ids.insert(connection_ids.begin(), other_id); // We're first, so add to beginning
                ppo->ConnectTo(*this, true);  // Make sure other path point logs the connection also
            } else {
                connection_ids.push_back(other_id); // We're second, so add to end
            }
            return true;
        }
    }
}

bool PathPointObject::AcceptConnectionsFrom(Object::ConnectionType type, Object& object) {
    return type == kCTMovementObjects || type == kCTPathPoints;
}

bool PathPointObject::Disconnect( Object& other, bool checking_other ) {
    if(other.GetType() == _movement_object){
        return other.Disconnect(*this, true);
    } else if(other.GetType() == _hotspot_object) {
        return Object::Disconnect(other, checking_other);
    } else if(other.GetType() != _path_point_object){
        return false;
    } else {
        PathPointObject* ppo = (PathPointObject*)&other;
        if(ppo == this){
            return false;
        } else {
            int other_id = ppo->GetID();
            std::vector<int>::iterator iter = std::find(connection_ids.begin(), connection_ids.end(), other_id);
            if(iter != connection_ids.end()){
                connection_ids.erase(iter);
            }
            if(!checking_other){
                ppo->Disconnect(*this, true);
            }
            return true;
        }
    }
}

void PathPointObject::GetConnectionIDs(std::vector<int>* cons) {
    for( uint32_t i = 0; i < connection_ids.size(); i++ ) {
        cons->push_back(connection_ids[i]);
    } 
}

void PathPointObject::NotifyDeleted( Object* o ) {
    Object::NotifyDeleted(o);
    std::vector<int>::iterator iter = 
        std::find(connection_ids.begin(), connection_ids.end(), o->GetID());
    if(iter != connection_ids.end()){
        connection_ids.erase(iter);
    }
}

void PathPointObject::GetDesc(EntityDescription &desc) const {
    Object::GetDesc(desc);
    desc.AddString(EDF_FILE_PATH, "");
    desc.AddIntVec(EDF_CONNECTIONS, connection_ids);
}

bool PathPointObject::SetFromDesc( const EntityDescription& desc ) {
    bool ret = Object::SetFromDesc(desc);
    if( ret ) {
        for(unsigned i=0; i<desc.fields.size(); ++i){
            const EntityDescriptionField& field = desc.fields[i];
            switch(field.type){
                case EDF_CONNECTIONS:
                    field.ReadIntVec(&connection_ids);
                    break;
            }
        }
    }
    return ret;
}

bool PathPointObject::Initialize() {
    sp.ASAddFloat("Wait", 0.0f);
    sp.ASAddString("Type", "Stand");
    obj_file = "path_point_object";
    return true;
}

static int ASNumConnectionIDs(PathPointObject* obj){
    return (int) obj->connection_ids.size();
}

static int ASGetConnectionID(PathPointObject* obj, int which){
    return obj->connection_ids[which];
}

void PathPointObject::RegisterToScript(ASContext* as_context) {
    as_context->RegisterObjectType("PathPointObject", 0, asOBJ_REF | asOBJ_NOCOUNT);
    as_context->RegisterObjectMethod("PathPointObject",
        "int NumConnectionIDs()",
        asFUNCTION(ASNumConnectionIDs), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("PathPointObject",
        "int GetConnectionID(int which)",
        asFUNCTION(ASGetConnectionID), asCALL_CDECL_OBJFIRST);
    as_context->DocsCloseBrace();
}

void PathPointObject::Draw() {
    if (g_debug_runtime_disable_pathpoint_object_draw) {
        return;
    }

    if(scenegraph_->map_editor->IsTypeEnabled(_path_point_object) &&
       !Graphics::Instance()->media_mode() && 
       scenegraph_->map_editor->state_ != MapEditor::kInGame)
    {
        DebugDraw::Instance()->AddWireSphere(GetTranslation(), 0.5f, vec4(0.0f,0.5f,0.5f,0.5f), _delete_on_draw);
        for(unsigned i=0; i<connection_ids.size(); ++i){
            Object* obj = scenegraph_->GetObjectFromID(connection_ids[i]);
            DebugDraw::Instance()->AddLine(GetTranslation(), obj->GetTranslation(), vec4(0.0f,0.5f,0.5f,0.5f), _delete_on_draw);
        }
    }
}

void PathPointObject::RemapReferences(std::map<int,int> id_map) {
    for( unsigned i = 0; i < connection_ids.size(); i++ ) {
        if( id_map.find(connection_ids[i]) != id_map.end() ) {
            connection_ids[i] = id_map[connection_ids[i]];
        } else {
            //The remapped id could belong to this, in which case it is valid
            bool is_this = false;
            for(std::map<int,int>::iterator iter = id_map.begin(); iter != id_map.end(); ++iter) {
                if(iter->second == this->GetID()) {
                    is_this = true;
                    break;
                }
            }
            if(!is_this)
                connection_ids[i] = -1;
        }
    }
}
