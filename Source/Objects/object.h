//-----------------------------------------------------------------------------
//           Name: object.h
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

#include <Editors/entity_type.h>
#include <Editors/object_sanity_state.h>

#include <Math/overgrowth_geometry.h>
#include <Math/quaternions.h>

#include <Objects/object_msg.h>
#include <Graphics/palette.h>
#include <Scripting/scriptparams.h>
#include <Asset/Asset/material.h>

#include <list>
#include <cstdarg>
#include <string>
#include <map>
//-----------------------------------------------------------------------------
// Class Definition
//-----------------------------------------------------------------------------

class SceneGraph;
class Collision;
class Prefab;
struct LineSegment;
struct MaterialEvent;
struct MaterialDecal;
struct MaterialParticle;
class Material;
class BulletObject;
struct CollideInfo;
struct EntityDescription;
struct SavedChunk;
class TiXmlNode;

typedef int32_t ObjectID;

struct SeparatedTransform {
    vec3 translation;
    vec3 scale;
    quaternion rotation;
};

class Object {
   public:
    enum MoveType {
        kTranslate = (1 << 0),
        kRotate = (1 << 1),
        kScale = (1 << 2),
        kAll = kTranslate | kRotate | kScale
    };
    enum DrawType {
        kUnknown,
        kDrawDepthOnly,
        kDrawAllShadowCascades,
        kDrawDepthNoAA,
        kFullDraw,
        kWireframe,
        kDecal
    };
    enum ConnectionType {
        kCTNone,
        kCTMovementObjects,
        kCTItemObjects,
        kCTEnvObjectsAndGroups,
        kCTPathPoints,
        kCTPlaceholderObjects,
        kCTNavmeshConnections,
        kCTHotspots
    };

    // Transforms
    const vec3& GetTranslation() const { return translation_; }
    const vec3& GetScale() const { return scale_; }
    const quaternion& GetRotation() const { return rotation_; }
    const mat4& GetTransform() const { return transform_; }
    vec3 GetRotationEuler();

    virtual void SetTranslation(const vec3& trans);
    virtual void SetScale(const vec3& trans);
    virtual void SetRotation(const quaternion& trans);
    virtual void SetTranslationRotationFast(const vec3& trans, const quaternion& rotation);
    void SetRotationEuler(const vec3& trans);
    virtual void SetTransformationMatrix(const mat4& transform);
    virtual void GetDisplayName(char* buf, int buf_size);

    enum PermissionFlags {
        CAN_ROTATE = (1 << 0),
        CAN_TRANSLATE = (1 << 1),
        CAN_SCALE = (1 << 2),
        CAN_SELECT = (1 << 3),
        CAN_DELETE = (1 << 4),
        CAN_COPY = (1 << 5)
    };

    std::string name;
    int permission_flags;
    bool selectable_;
    bool enabled_;
    vec4 box_color;
    Box box_;
    bool editor_visible;
    SeparatedTransform start_transform;  // transform of object when editor starts to move it

    bool online_transform_dirty = false;
    bool interp_started = false;
    bool interp = false;
    bool overshot = false;
    bool behind = false;

    float walltime_last_update = 0.0f;

    std::string obj_file;  // generic xml data for my entity type
    std::string editor_label;
    vec3 editor_label_offset;
    float editor_label_scale;
    bool collidable;
    bool created_on_the_fly;
    bool transparent;
    bool exclude_from_undo;
    bool exclude_from_save;
    int update_list_entry;
    Object* parent;
    std::vector<int> unfinalized_connected_from;
    std::vector<int> unfinalized_connected_to;
    // These need to be here so copying an object correctly connects
    // it to and from other objects.
    // Currently they are only used when connecting to and from hotspots,
    // since hotspots should be able to connect to all objects, and all
    // objects should be able to connect to hotspots.
    std::vector<int> connected_from;  // Other objects connected to this object
    std::vector<int> connected_to;    // This object connected to other objects

    SceneGraph* scenegraph_;

    Object(SceneGraph* parent_scenegraph = 0)
        : permission_flags(CAN_ROTATE | CAN_TRANSLATE | CAN_SCALE | CAN_SELECT | CAN_COPY | CAN_DELETE),
          selected_(0),
          box_color(1.0f),
          editor_visible(true),
          collidable(false),
          transparent(false),
          exclude_from_undo(false),
          exclude_from_save(false),
          update_list_entry(-1),
          enabled_(true),
          parent(NULL),
          scenegraph_(parent_scenegraph),
          scale_(1.0f),
          rotation_(),
          translation_(0.0f),
          loaded_rotation_(),
          loaded_translation_(0.0f),
          loaded_scale_(0.0f),
          id(-1),
          editor_label(""),
          editor_label_offset(0.0f),
          editor_label_scale(10),
          selectable_(true),
          receive_depth(0),
          rotation_updated(true)

    {
        UpdateTransform();
    }

    virtual EntityType GetType() const = 0;
    virtual ~Object();
    virtual void SetParent(Object* new_parent);
    virtual bool HasParent();

    ObjectID GetID() const { return id; }
    void SetID(const int _id) {
        id = _id;
        sp.SetObjectID(_id);
    }
    std::string GetName() const { return name; }
    void SetName(const std::string& _name) { name = _name; }

    virtual void Collided(const vec3& pos, float impulse, const CollideInfo& collide_info, BulletObject* obj) {}

    virtual void SetEnabled(bool val) { enabled_ = val; }
    virtual void SetCollisionEnabled(bool val){};
    virtual void HandleTransformationOccurred();
    virtual void UpdateParentHierarchy(){};
    virtual void PropagateTransformsDown(bool deep) {}
    virtual void ChildMoved(Object::MoveType type) {}
    virtual const MaterialEvent& GetMaterialEvent(const std::string& the_event, const vec3& event_pos, int* tri = NULL);
    virtual const MaterialEvent& GetMaterialEvent(const std::string& the_event, const vec3& event_pos, const std::string& mod, int* tri = NULL);
    virtual const MaterialDecal& GetMaterialDecal(const std::string& type, const vec3& pos, int* tri = NULL);
    virtual const MaterialParticle& GetMaterialParticle(const std::string& type, const vec3& pos, int* tri = NULL);
    virtual MaterialRef GetMaterial(const vec3& pos, int* tri = NULL);
    virtual vec3 GetColorAtPoint(const vec3& pos, int* tri = NULL);
    virtual void HandleMaterialEvent(const std::string& the_event, const vec3& event_pos, int* tri = NULL) {}

    virtual void Update(float timestep) {}  // Is only called on certain objects that are registered as needing a frequent update.
    virtual void InfrequentUpdate();        // Is called on all objects, but there is no guarantee on how often.
                                            //  Interpolation boilerplate for all objects in multiplayer

    virtual bool Initialize() = 0;
    virtual void SetImposter(bool set) {}
    virtual void drawShadow(vec3 origin, float distance) {}
    virtual int lineCheck(const vec3& start, const vec3& end, vec3* point, vec3* normal = 0);
    virtual bool AcceptConnectionsFrom(ConnectionType type, Object& object) { return false; }
    virtual bool ConnectTo(Object& other, bool checking_other = false);
    virtual bool Disconnect(Object& other, bool from_socket = false, bool checking_other = false);
    virtual void ConnectedFrom(Object& object);
    virtual void DisconnectedFrom(Object& object);
    virtual void Dispose() {}
    virtual void SaveToXML(TiXmlElement* parent);
    virtual void GetDesc(EntityDescription& desc) const;
    virtual void NotifyDeleted(Object* o);
    void ReceiveObjectMessage(OBJECT_MSG::Type type, ...);
    void ReceiveScriptMessage(const std::string& msg);
    void QueueScriptMessage(const std::string& msg);
    virtual void ReceiveObjectMessageVAList(OBJECT_MSG::Type type, va_list args);
    virtual void GetChildren(std::vector<Object*>* children) {}
    virtual void GetBottomUpCompleteChildren(std::vector<Object*>* ret_children) {}
    virtual void GetTopDownCompleteChildren(std::vector<Object*>* ret_children) {}

    ScriptParams* GetScriptParams() { return &sp; }
    const ScriptParamMap& GetScriptParamMap();
    virtual void SetScriptParams(const ScriptParamMap& spm);
    virtual void ApplyPalette(const OGPalette& palette, bool from_socket = false) {}
    virtual OGPalette* GetPalette();
    virtual void ReceiveASVec3Message(int type, const vec3& vec_a, const vec3& vec_b) {}
    virtual void SaveHistoryState(std::list<SavedChunk>& chunk_list, int state_id);
    virtual bool SetFromDesc(const EntityDescription& desc);
    virtual void UpdateScriptParams() {}
    virtual void ChildLost(Object* obj) {}
    virtual void DrawDepthMap(const mat4& proj_view_matrix, const vec4* cull_planes, int num_cull_planes, Object::DrawType draw_type) {}
    virtual void PreDrawFrame(float curr_game_time) {}
    virtual void PreDrawCamera(float curr_game_time) {}
    virtual bool IsGroupDerived() const { return false; }
    virtual bool LockedChildren() { return !selectable_; }

    virtual void GetConnectionIDs(std::vector<int>* cons) {}

    // Remap all attachment references etc to new id's, used for prefabs.
    virtual void RemapReferences(std::map<int, int> id_map){};

    virtual void DrawImGuiEditor();
    virtual int GetGroupDepth();

    virtual Prefab* GetPrefabParent();
    virtual bool IsInPrefab();

    virtual void Select(bool val);
    virtual unsigned int Selected();

    // If this is non-zero that indicates that there's something wrong with the object data state.
    virtual ObjectSanityState GetSanity();
    virtual void GetShaderNames(std::map<std::string, int>& shaders) {}

    virtual bool IsMultiplayerSupported() { return true; }

   protected:
    unsigned int selected_;
    ScriptParams sp;
    mat4 transform_;
    mat4 rotation_mat_;
    vec3 scale_;
    vec3 translation_;
    quaternion loaded_rotation_;
    vec3 loaded_translation_;
    vec3 loaded_scale_;
    void UpdateTransform();
    virtual void Moved(Object::MoveType type);
    // These are accessed via ReceiveObjectMessage()
    virtual void Draw() {}
    virtual void SaveShadow() {}
    virtual void LoadShadow() {}
    virtual void Reload() {}
    virtual void FinalizeLoadedConnections();
    virtual void Reset() {}
    virtual void ToggleImposter() {}
    int LineCheckEditorCube(const vec3& start, const vec3& end, vec3* point, vec3* normal);

   private:
    int receive_depth;
    ObjectID id;
    bool rotation_updated;
    quaternion rotation_;
    vec3 rotation_euler_;
};

inline std::ostream& operator<<(std::ostream& out, const Object& obj) {
    out << "Object(id:" << obj.GetID() << ",type:" << obj.GetType() << ",file:" << obj.obj_file << ")";
    return out;
}
