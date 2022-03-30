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

    Group();
    bool Initialize();
    virtual ~Group();
    virtual int lineCheck(const vec3 &start, const vec3 &end, vec3 *point, vec3 *normal=0);
    virtual EntityType GetType() const { return _group; }
    virtual bool SetFromDesc( const EntityDescription &desc );
    virtual void PreDrawCamera(float curr_game_time);
    virtual void PreDrawFrame(float curr_game_time);
    virtual void Moved(Object::MoveType type);
    virtual void GetDesc(EntityDescription &desc) const;
    virtual void ChildMoved(Object::MoveType type);
    virtual void ChildLost(Object *obj);
    virtual void UpdateParentHierarchy();
    virtual void PropagateTransformsDown(bool deep);
    virtual void GetDisplayName(char* buf, int buf_size);
    virtual void SetEnabled(bool val);
	virtual void InitShape();
	virtual void InitRelMats();
	virtual void FinalizeLoadedConnections();
    virtual void GetChildren(std::vector<Object*>* ret_children);
    virtual void GetBottomUpCompleteChildren(std::vector<Object*>* ret_children);
    virtual void GetTopDownCompleteChildren(std::vector<Object*>* ret_children);
    virtual void HandleTransformationOccured();
    virtual void ReceiveObjectMessageVAList(OBJECT_MSG::Type type, va_list args);
    virtual void Update(float timestep){}
    virtual void RemapReferences(std::map<int,int> id_map);
	virtual void SetTranslationRotationFast(const vec3& trans, const quaternion& rotation);
	void PropagateTransformsDownFast(bool deep);

    virtual bool IsGroupDerived() const {return true;}

    virtual ObjectSanityState GetSanity();
};
