//-----------------------------------------------------------------------------
//           Name: hotspot.h
//      Developer: Wolfire Games LLC
//         Author: Phillip Isola
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

#include <Objects/object.h>
#include <Scripting/scriptparams.h>

#include <Graphics/Billboard.h>
#include <Graphics/hudimage.h>
#include <Graphics/textureref.h>

#include <Scripting/angelscript/ascollisions.h>
#include <Scripting/angelscript/ascontext.h>
#include <Scripting/angelscript/add_on/scriptarray/scriptarray.h>

#include <iostream>
#include <memory>
#include <queue>

class MovementObject;
class ItemObject;
class HotspotEditor;

class Hotspot : public Object {
public:
    Hotspot();
    virtual ~Hotspot();

    virtual bool Initialize();
    virtual void ReceiveObjectMessageVAList( OBJECT_MSG::Type type, va_list args );
    virtual void Update(float timestep);
    virtual void PreDrawFrame(float curr_game_time);
    virtual void Draw();
    virtual void Dispose();
    virtual void GetDisplayName(char* buf, int buf_size);
    virtual void DrawImGuiEditor();
    void GetDesc(EntityDescription &desc) const;
    virtual void NotifyDeleted(Object* o);
    virtual EntityType GetType() const { return _hotspot_object; }

    std::unique_ptr<ASCollisions> as_collisions;

	void Reset();
	virtual void SetEnabled(bool val);

    virtual int lineCheck(const vec3 &start, const vec3 &end, vec3 *point, vec3 *normal);

    inline const std::string& GetScriptFile() const { return m_script_file; }
    inline void SetScriptFile(const std::string& script_file) { m_script_file = script_file; }

    void SetBillboardColorMap(const std::string& color_map);
	virtual void Reload();
	void Moved(Object::MoveType type);
    void HandleEvent( const std::string &event, MovementObject* mo );
    void HandleEventItem( const std::string &event, ItemObject* obj );
    virtual void SetScriptParams( const ScriptParamMap& spm );
    virtual void UpdateScriptParams();
    std::string GetTypeString();
    bool ASHasVar( const std::string &name );
    int ASGetIntVar( const std::string &name );
    int ASGetArrayIntVar( const std::string &name, int index );
    float ASGetFloatVar( const std::string &name );
    bool ASGetBoolVar( const std::string &name );
    void ASSetIntVar( const std::string &name, int value );
    void ASSetArrayIntVar( const std::string &name, int index, int value );
    void ASSetFloatVar( const std::string &name, float value );
    void ASSetBoolVar( const std::string &name, bool value );
	virtual bool SetFromDesc( const EntityDescription& desc );
	void UpdateCollisionShape();

    bool AcceptConnectionsFromImplemented();

    virtual bool AcceptConnectionsFrom(ConnectionType type, Object& object);
    virtual bool AcceptConnectionsTo(Object& object);
    virtual bool ConnectTo(Object& other, bool checking_other = false);
    virtual bool Disconnect(Object& other, bool checking_other = false);

    virtual void ConnectedFrom(Object& other);
    virtual void DisconnectedFrom(Object& other);

    CScriptArray* ASGetConnectedObjects();

    bool HasCustomGUI();
    void LaunchCustomGUI();
    bool ObjectInspectorReadOnly();

    bool HasFunction(const std::string& function_definition);
    int QueryIntFunction(const std::string& function);
    bool QueryBoolFunction(const std::string& function);
    float QueryFloatFunction(const std::string& function);
    std::string QueryStringFunction(const std::string& function);

	bool abstract_collision;
private:
    struct {
        ASFunctionHandle init;
        ASFunctionHandle set_parameters;

        ASFunctionHandle receive_message;
        ASFunctionHandle update;
        ASFunctionHandle pre_draw;
        ASFunctionHandle draw;
        ASFunctionHandle draw_editor;
        ASFunctionHandle reset;
        ASFunctionHandle set_enabled;
        ASFunctionHandle handle_event;
        ASFunctionHandle handle_event_item;
        ASFunctionHandle get_type_string;
        ASFunctionHandle dispose;
        ASFunctionHandle pre_script_reload;
        ASFunctionHandle post_script_reload;
        ASFunctionHandle connect_to;
        ASFunctionHandle disconnect;
        ASFunctionHandle connected_from;
        ASFunctionHandle disconnected_from;
        ASFunctionHandle accept_connections_from;
        ASFunctionHandle accept_connections_from_obj;
        ASFunctionHandle accept_connections_to_obj;
        ASFunctionHandle launch_custom_gui;
        ASFunctionHandle object_inspector_read_only;
    } as_funcs;
    HUDImages hud_images;
    ASContext *as_context;
    BulletObject *collision_object;

    std::string m_script_file;
    TextureAssetRef billboard_texture_ref_;
    std::queue<std::string> message_queue;
};

void DefineHotspotTypePublic(ASContext* as_context);
