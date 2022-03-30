//-----------------------------------------------------------------------------
//           Name: object.cpp
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
#include "object.h"

#include <Internal/common.h>
#include <Internal/memwrite.h>

#include <Math/vec3math.h>
#include <Math/vec4math.h>

#include <Objects/prefab.h>
#include <Asset/Asset/material.h>
#include <Game/level.h>
#include <Editors/map_editor.h>
#include <Utility/ieee.h>
#include <GUI/gui.h>

#include <tinyxml.h>
#include <SDL_assert.h>

#include <cstdarg>

extern Timer game_timer;

void Object::SaveHistoryState( std::list<SavedChunk> &chunk_list, int state_id ) {
    SavedChunk saved_chunk;
    saved_chunk.obj_id = GetID();
    if(GetType() != _group && GetType() != _prefab){
        saved_chunk.type = ChunkType::OBJECT;
    } else {
        saved_chunk.type = ChunkType::GROUP;
    }

    GetDesc(saved_chunk.desc);

    AddChunkToHistory(chunk_list, state_id, saved_chunk);
}

int Object::lineCheck(const vec3 &start, const vec3 &end, vec3 *point, vec3 *normal) {
    return LineCheckEditorCube(start, end, point, normal);
}

int Object::LineCheckEditorCube( const vec3 &start, const vec3 &end, vec3 *point, vec3 *normal/*=0*/ ) {
    int intersecting = box_.lineCheck(invert(transform_)*start,
                                               invert(transform_)*end,
                                               point,
                                               normal);
    if(intersecting!=-1){
        if(point) {
            *point = transform_ * *point;
        }
        if(normal) {
            *normal = rotation_ * *normal;
        }
    }
    return intersecting;
}

void Object::HandleTransformationOccurred(){
    Online* online = Online::Instance();
    if (online->IsActive()) {
        online->Send<OnlineMessages::EditorTransformChange>(GetID(), translation_, scale_, rotation_);
    }
}

const MaterialEvent& Object::GetMaterialEvent( const std::string &the_event, const vec3 &event_pos, int *tri ) {
    //MaterialRef ref = Materials::Instance()->ReturnRef("Data/Materials/default.xml");
    MaterialRef ref = Engine::Instance()->GetAssetManager()->LoadSync<Material>("Data/Materials/default.xml");
    return ref->GetEvent(the_event);
}

const MaterialEvent& Object::GetMaterialEvent( const std::string &the_event, const vec3 &event_pos, const std::string &mod, int *tri ) {
    //MaterialRef ref = Materials::Instance()->ReturnRef("Data/Materials/default.xml");
    MaterialRef ref = Engine::Instance()->GetAssetManager()->LoadSync<Material>("Data/Materials/default.xml");
    return ref->GetEvent(the_event, mod);
}

const MaterialDecal& Object::GetMaterialDecal( const std::string &type, const vec3 &pos, int *tri ) {
    //MaterialRef ref = Materials::Instance()->ReturnRef("Data/Materials/default.xml");
    MaterialRef ref = Engine::Instance()->GetAssetManager()->LoadSync<Material>("Data/Materials/default.xml");
    return ref->GetDecal(type);
}

const MaterialParticle& Object::GetMaterialParticle( const std::string &type, const vec3 &pos, int *tri ) {
    //MaterialRef ref = Materials::Instance()->ReturnRef("Data/Materials/default.xml");
    MaterialRef ref = Engine::Instance()->GetAssetManager()->LoadSync<Material>("Data/Materials/default.xml");
    return ref->GetParticle(type);
}

vec3 Object::GetColorAtPoint( const vec3 &pos, int *tri ) {
    return vec3(1.0f);
}

void Object::InfrequentUpdate() {
    if( parent ) {
        selectable_ = !parent->LockedChildren();
    }  else {
        selectable_ = true;
    }

    if( selectable_ == false ) {
        selected_ = false;
    }
}

MaterialRef Object::GetMaterial( const vec3 &pos, int* tri ) {
    //return (*Materials::Instance()->ReturnRef("Data/Materials/default.xml"));
    return Engine::Instance()->GetAssetManager()->LoadSync<Material>("Data/Materials/default.xml");
}

const ScriptParamMap& Object::GetScriptParamMap() {
    return sp.GetParameterMap();
}

void Object::SetScriptParams( const ScriptParamMap& spm ) {
    sp.SetParameterMap(spm);
    UpdateScriptParams();
}

OGPalette *Object::GetPalette() {
    DisplayError("Error", "Getting palette of Object that doesn't have one.");
    return NULL;
}

bool Object::ConnectTo(Object& other, bool checking_other/*= false*/) {
    if(other.GetType() == _hotspot_object) {
        connected_to.push_back(other.GetID());
        other.ConnectedFrom(*this);
        return true;
    } else {
        return false;
    }
}

bool Object::Disconnect(Object& other, bool from_socket, bool checking_other/*= false*/) {
    if(other.GetType() == _hotspot_object) {
        std::vector<int>::iterator iter = std::remove(connected_to.begin(), connected_to.end(), other.GetID());
        if(iter == connected_to.end())
            return false;
        other.DisconnectedFrom(*this);
        connected_to.erase(iter, connected_to.end());
        return true;
    }
    return false;
}

void Object::ConnectedFrom(Object& object) {
    connected_from.push_back(object.GetID());
}

void Object::DisconnectedFrom(Object& object) {
    connected_from.erase(std::remove(connected_from.begin(), connected_from.end(), object.GetID()), connected_from.end());
}

void Object::SaveToXML( TiXmlElement* parent ) {
    // Don't save if object has "No Save" parameter with value 1
    bool no_save = false;
    const ScriptParamMap &script_param_map = sp.GetParameterMap();
    const ScriptParamMap::const_iterator it = script_param_map.find("No Save");
    if(it != script_param_map.end()){
        const ScriptParam &param = it->second;
        if(param.GetInt() == 1){
            no_save = true;
        }
    }
    if(!no_save) {
        EntityDescription desc;
        GetDesc(desc);
        desc.SaveToXML(parent);
    }
}

bool Object::SetFromDesc( const EntityDescription& desc ) {
    for(unsigned i=0; i<desc.fields.size(); ++i){
        const EntityDescriptionField& field = desc.fields[i];
        switch(field.type){
            case EDF_ID: {
                int id;
                field.ReadInt(&id);
                SetID(id);
                break;}
            case EDF_NAME: {
                std::string ename;
                field.ReadString(&ename);
                SetName(ename);
                break;}
            case EDF_TRANSLATION: {
                vec3 translation;
                memread(translation.entries, sizeof(float), 3, field.data);
                loaded_translation_ = translation;
                SetTranslation(translation);
                break;}
            case EDF_SCALE:{
                vec3 scale;
                memread(scale.entries, sizeof(float), 3, field.data);
                loaded_scale_ = scale;
                SetScale(scale);
                break;}
            case EDF_ROTATION:{
                vec4 rotation;
                memread(rotation.entries, sizeof(float), 4, field.data);
                loaded_rotation_ = quaternion(rotation.entries[0], rotation.entries[1], rotation.entries[2], rotation.entries[3]);
                SetRotation(loaded_rotation_);
                break;}
            case EDF_ROTATION_EULER:{
                vec3 rotation;
                memread(rotation.entries, sizeof(float), 3, field.data);
                rotation_euler_ = rotation;
                break;}
            case EDF_SCRIPT_PARAMS:{
                ScriptParamMap spm;
                ReadScriptParametersFromRAM(spm, field.data);
                SetScriptParams(spm);
                break;}
            case EDF_HOTSPOT_CONNECTED_TO: {
                field.ReadIntVec(&unfinalized_connected_to);
                break;}
            case EDF_HOTSPOT_CONNECTED_FROM: {
                field.ReadIntVec(&unfinalized_connected_from);
                break;}
        }
    }
    return true;
}

void Object::DrawImGuiEditor() {

}

bool IsDifferentEnough(const vec3 &a, const vec3 &b){
    bool same = true;
    for(int i=0; i<3; ++i){
        if(fabs(a[i] - b[i]) > 0.0001){
            same = false;
        }
    }
    return !same;
}

bool IsDifferentEnough(const quaternion &a, const quaternion &b){
    bool same = true;
    for(int i=0; i<4; ++i){
        if(fabs(a[i] - b[i]) > 0.0001){
            same = false;
        }
    }
    return !same;
}


void Object::GetDesc( EntityDescription &desc ) const {
    desc.AddEntityType(EDF_ENTITY_TYPE,   GetType());
    desc.AddString(EDF_NAME, name);
    // Use loaded transforms if they haven't changed much, avoids spamming
    // level xml diffs with changes from slight rounding errors
    if(IsDifferentEnough(loaded_translation_, GetTranslation())){
        desc.AddVec3( EDF_TRANSLATION, GetTranslation());
    } else {
        desc.AddVec3( EDF_TRANSLATION, loaded_translation_ );
    }

    if(IsDifferentEnough(loaded_scale_, GetScale())){
        desc.AddVec3( EDF_SCALE, GetScale());
    } else {
        desc.AddVec3( EDF_SCALE, loaded_scale_ );
    }

    if(IsDifferentEnough(loaded_rotation_, GetRotation())){
        desc.AddQuaternion( EDF_ROTATION, GetRotation() );
    } else {
        desc.AddQuaternion( EDF_ROTATION, loaded_rotation_ );
    }

    if(!rotation_updated) {
        desc.AddVec3( EDF_ROTATION_EULER, rotation_euler_ );
    } else {
        desc.AddVec3( EDF_ROTATION_EULER, QuaternionToEuler(GetRotation()) );
    }

    desc.AddInt(         EDF_ID,            GetID());
    desc.AddScriptParams(EDF_SCRIPT_PARAMS, sp.GetParameterMap());
    desc.AddIntVec(      EDF_HOTSPOT_CONNECTED_TO, connected_to);
    desc.AddIntVec(      EDF_HOTSPOT_CONNECTED_FROM, connected_from);
}

void Object::ReceiveObjectMessage(OBJECT_MSG::Type type, ...) {
    va_list args;
    va_start(args, type);
    ReceiveObjectMessageVAList(type, args);
    va_end(args);
}

void Object::ReceiveObjectMessageVAList( OBJECT_MSG::Type type, va_list args ) {
    switch(type){
    case OBJECT_MSG::TOGGLE_IMPOSTER:             ToggleImposter(); break;
    case OBJECT_MSG::RESET:                       Reset(); break;
    case OBJECT_MSG::RELOAD:                      Reload(); break;
    case OBJECT_MSG::FINALIZE_LOADED_CONNECTIONS: FinalizeLoadedConnections(); break;
    case OBJECT_MSG::DRAW:                        Draw(); break;
    case OBJECT_MSG::UPDATE:                      
        Update((float)va_arg(args,double)); 
        walltime_last_update = game_timer.GetWallTime();
        break;
    case OBJECT_MSG::INFREQUENT_UPDATE:           InfrequentUpdate(); break;
    default:
        break;
    }
}

void Object::ReceiveScriptMessage( const std::string &msg ) {
    receive_depth++;
    if(receive_depth > 8 ) {
        LOGW << "Deep stack on receive scripts... message: \"" << msg << "\" count: " << receive_depth << std::endl;
    }
    if(receive_depth > 32) {
        LOGE << "I've gotten recursed 32 times now, assuming we're in a loop, queueing this message instead \"" << msg << "\"" << std::endl;
        ReceiveObjectMessage(OBJECT_MSG::QUEUE_SCRIPT, &msg);
    } else {
        ReceiveObjectMessage(OBJECT_MSG::SCRIPT, &msg);
    }
    receive_depth--;
}

void Object::QueueScriptMessage( const std::string &msg ) {
    ReceiveObjectMessage(OBJECT_MSG::QUEUE_SCRIPT, &msg);
}

void Object::SetParent(Object* new_parent) {
    if(parent && parent != new_parent){
        parent->ChildLost(this);
    }
    parent = new_parent;
    UpdateParentHierarchy();
}

bool Object::HasParent() {
    return parent != NULL;
}

void Object::NotifyDeleted(Object* o) {
    if(parent == o){
        SetParent(NULL);
    }
    connected_to.erase(std::remove(connected_to.begin(), connected_to.end(), o->GetID()), connected_to.end());
}

void Object::Moved(Object::MoveType type) {
    if(parent){
        parent->ChildMoved(type);
    }

    online_transform_dirty = true;
}

void Object::FinalizeLoadedConnections() {
    for(size_t i = 0; i < unfinalized_connected_to.size(); ++i) {
        Object* object = scenegraph_->GetObjectFromID(unfinalized_connected_to[i]);
        if(object) {
            if(!ConnectTo(*object, false)) {
                LOGW << "Failed to connect to the object with id " << unfinalized_connected_to[i] << " after loading" << std::endl;
            }
        } else {
            LOGW << "Connected to an invalid id " << unfinalized_connected_to[i] << " after loading" << std::endl;
        }
    }

    for(size_t i = 0; i < unfinalized_connected_from.size(); ++i) {
        Object* object = scenegraph_->GetObjectFromID(unfinalized_connected_from[i]);
        if(object) {
            if(!object->ConnectTo(*this, false)) {
                LOGW << "Failed to connect to the object with id " << unfinalized_connected_from[i] << " after loading" << std::endl;
            }
        } else {
            LOGW << "Connected to an invalid id " << unfinalized_connected_from[i] << " after loading" << std::endl;
        }
    }

    unfinalized_connected_to.resize(0);
    unfinalized_connected_from.resize(0);
}

Object::~Object() {
    if(scenegraph_ && update_list_entry != -1){
        scenegraph_->UnlinkUpdateObject(this, update_list_entry);
        update_list_entry = -1;
    }
    SDL_assert(update_list_entry == -1);
}

void Object::UpdateTransform() {
    LOG_ASSERT(!IsNan(rotation_[0]));
    rotation_mat_ = Mat4FromQuaternion(rotation_);
    mat4 translation_mat;
    translation_mat.SetTranslation(translation_);
    mat4 scale_mat;
    scale_mat.SetScale(scale_);
    transform_ = translation_mat * rotation_mat_ * scale_mat;
}

void Object::SetTranslation(const vec3& trans) {
    if(translation_ != trans){
        translation_ = trans;
        UpdateTransform();
        Moved(Object::kTranslate);
    }
}

void Object::SetScale(const vec3& trans) {
    if(scale_ != trans){
        scale_ = trans;
        UpdateTransform();
        Moved(Object::kScale);
    }
}

void Object::SetRotation(const quaternion& trans) {
    if(IsDifferentEnough(rotation_, trans)){
        rotation_ = trans;
        LOG_ASSERT(!IsNan(rotation_[0]));
        UpdateTransform();
        Moved(Object::kRotate);
        rotation_updated = true;
    }
}

void Object::SetTranslationRotationFast(const vec3& trans, const quaternion& rotation) {
    bool changed_something = false;
    if(translation_ != trans){
        translation_ = trans;
        changed_something = true;
    }
    if(IsDifferentEnough(rotation_, rotation)){
        rotation_ = rotation;
        LOG_ASSERT(!IsNan(rotation_[0]));
        rotation_updated = true;
        changed_something = true;
    }
    if(changed_something){
        UpdateTransform();
    }
}

void Object::SetTransformationMatrix(const mat4& transform) {
    if(transform_ != transform){
        translation_ = transform.GetTranslationPart();
        rotation_ = QuaternionFromMat4(transform);
        LOG_ASSERT(!IsNan(rotation_[0]));
        scale_[0] = length(transform * vec4(1,0,0,0));
        scale_[1] = length(transform * vec4(0,1,0,0));
        scale_[2] = length(transform * vec4(0,0,1,0));
        UpdateTransform();
        Moved(Object::kAll);
    }
}

void Object::GetDisplayName(char* buf, int buf_size) {
    if( GetName().empty() ) {
        FormatString(buf, buf_size, "%d, Object from %s", GetID(), CStringFromEntityType(GetType()));
    } else {
        FormatString(buf, buf_size, "%s, Object from %s", GetName().c_str(), CStringFromEntityType(GetType()));
    }
}

int Object::GetGroupDepth() {
    if( parent == NULL ) {
        return 1;
    } else {
        return parent->GetGroupDepth()+1;
    }
}

Prefab* Object::GetPrefabParent() {
    if( parent != NULL ) {
        if( parent->GetType() == _prefab ) {
            return static_cast<Prefab*>(parent);
        } else  {
            return parent->GetPrefabParent();
        }
    }
    return NULL;
}

bool Object::IsInPrefab() {
    return GetPrefabParent() != NULL;
}

void Object::Select(bool val) {
    static unsigned int selection_counter_ = 1;

    if( val && selectable_ ) {
        selected_ = selection_counter_++;
    } else {
        selected_ = false;
    }
}

ObjectSanityState Object::GetSanity() {
    return ObjectSanityState(GetType(),id,0);
}

unsigned int Object::Selected() {
    return selected_;
}

void Object::SetRotationEuler(const vec3& trans) {
    rotation_euler_ = trans;
    SetRotation(EulerToQuaternion(rotation_euler_));
    rotation_updated = false;
}

vec3 Object::GetRotationEuler() {
    if(rotation_updated) {
        rotation_euler_ = QuaternionToEuler(rotation_);
        rotation_updated = false;
    }

    return rotation_euler_;
}
