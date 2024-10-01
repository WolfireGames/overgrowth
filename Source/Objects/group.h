//-----------------------------------------------------------------------------
//           Name: group.h
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

#include <Math/mat4.h>
#include <Math/quaternions.h>

#include <Game/EntityDescription.h>
#include <Objects/object.h>

class Group : public Object {
   public:
    struct Child {
        Object* direct_ptr;
        vec3 rel_translation;
        quaternion rel_rotation;
        vec3 rel_scale;
    };
    std::vector<Child> children;
    bool child_transforms_need_update;
    bool child_moved;

    //Glimpse - Group no navmesh.
    bool children_no_navmesh;

    Group();
    bool Initialize() override;
    ~Group() override;
    int lineCheck(const vec3& start, const vec3& end, vec3* point, vec3* normal = 0) override;
    EntityType GetType() const override { return _group; }
    bool SetFromDesc(const EntityDescription& desc) override;
    void PreDrawCamera(float curr_game_time) override;
    void PreDrawFrame(float curr_game_time) override;
    void Moved(Object::MoveType type) override;
    void GetDesc(EntityDescription& desc) const override;
    void ChildMoved(Object::MoveType type) override;
    void ChildLost(Object* obj) override;
    void UpdateParentHierarchy() override;
    void PropagateTransformsDown(bool deep) override;
    void GetDisplayName(char* buf, int buf_size) override;
    void SetEnabled(bool val) override;
    virtual void InitShape();
    virtual void InitRelMats();
    void FinalizeLoadedConnections() override;
    void GetChildren(std::vector<Object*>* ret_children) override;
    void GetBottomUpCompleteChildren(std::vector<Object*>* ret_children) override;
    void GetTopDownCompleteChildren(std::vector<Object*>* ret_children) override;
    virtual void HandleTransformationOccured();
    void ReceiveObjectMessageVAList(OBJECT_MSG::Type type, va_list args) override;
    void Update(float timestep) override {}
    void RemapReferences(std::map<int, int> id_map) override;
    void SetTranslationRotationFast(const vec3& trans, const quaternion& rotation) override;
    void PropagateTransformsDownFast(bool deep);

    bool IsGroupDerived() const override { return true; }

    ObjectSanityState GetSanity() override;
};
