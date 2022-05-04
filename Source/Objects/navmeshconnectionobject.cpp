//-----------------------------------------------------------------------------
//           Name: navmeshconnectionobject.cpp
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
#include "navmeshconnectionobject.h"

#include <Graphics/pxdebugdraw.h>
#include <Graphics/graphics.h>

#include <Internal/memwrite.h>
#include <Game/EntityDescription.h>
#include <Editors/map_editor.h>
#include <Scripting/angelscript/ascontext.h>
#include <Main/scenegraph.h>
#include <Math/vec3math.h>
#include <Logging/logdata.h>

#include <algorithm>

extern bool g_debug_runtime_disable_navmesh_connection_object_draw;

//-----------------------------------------------------------------------------
//Functions
//-----------------------------------------------------------------------------

NavmeshConnectionObject::NavmeshConnectionObject() {
    permission_flags &= ~(Object::CAN_SCALE | Object::CAN_ROTATE);
    box_.dims = vec3(1.0f);
}

bool NavmeshConnectionObject::ConnectTo( Object& other, bool checking_other /*= false*/ ) {
    if(other.GetType() != this->GetType()){
        return false;
    } else if(other.GetType() == _hotspot_object) {
        return Object::ConnectTo(other, checking_other);
    } else {
        NavmeshConnectionObject* ppo = (NavmeshConnectionObject*)&other;
        if(ppo == this){
            return false;
        } else {
            int other_id = ppo->GetID();
            for(auto & connection : connections) {
                if(connection.other_object_id == other_id) {
                   return false;
                }       
            }    
            NavMeshConnectionData d;
            d.other_object_id = other_id;
            d.offmesh_connection_id = -1;
            d.poly_area = SAMPLE_POLYAREA_DISABLED;
            connections.push_back(d);

            if(!checking_other) {
                ppo->ConnectTo(*this, true);
            }
            return true;
        }
    }
}

bool NavmeshConnectionObject::AcceptConnectionsFrom(Object::ConnectionType type, Object& object) {
    return type == kCTNavmeshConnections;
}

bool NavmeshConnectionObject::Disconnect( Object& other, bool checking_other ) {
    if(other.GetType() == _movement_object) {
        return Object::Disconnect(other, checking_other);
    } else if(other.GetType() != this->GetType()){
        return false;
    } else {
        NavmeshConnectionObject* ppo = (NavmeshConnectionObject*)&other;
        if(ppo == this){
            return false;
        } else {
            int other_id = ppo->GetID();
            std::vector<NavMeshConnectionData>::iterator iter = connections.begin();

            for(; iter != connections.end(); iter++ )
            {
                if( iter->other_object_id == other_id )
                {
                    connections.erase(iter);
                    break;
                }
            }

            if(!checking_other){
                ppo->Disconnect(*this, true);
            }
            return true;
        }
    }
}

void NavmeshConnectionObject::GetConnectionIDs(std::vector<int>* cons) {
    for(auto & connection : connections) { 
        cons->push_back(connection.other_object_id);
    }
}

void NavmeshConnectionObject::ResetConnectionOffMeshReference()
{
    std::vector<NavMeshConnectionData>::iterator iter = connections.begin();

    for(; iter != connections.end(); iter++ )
    {
        iter->offmesh_connection_id = -1;

        //We have to sync up the connection back to us.
        Object *other_obj = scenegraph_->GetObjectFromID( iter->other_object_id );
        if( other_obj && other_obj->GetType() == _navmesh_connection_object )
        {
            NavmeshConnectionObject* nmco = static_cast<NavmeshConnectionObject*>(other_obj);
			std::vector<NavMeshConnectionData>::iterator other = nmco->GetConnectionTo( this->GetID() );
			if( other != nmco->connections.end() )
			{
				other->offmesh_connection_id = -1;
			}
        }
    }
}


void NavmeshConnectionObject::ResetPolyAreaAssignment()
{
    std::vector<NavMeshConnectionData>::iterator iter = connections.begin();

    for(; iter != connections.end(); iter++ )
    {
        iter->poly_area = SAMPLE_POLYAREA_DISABLED;

        //We have to sync up the connection back to us.
        Object *other_obj = scenegraph_->GetObjectFromID( iter->other_object_id );
        if( other_obj && other_obj->GetType() == _navmesh_connection_object )
        {
            NavmeshConnectionObject* nmco = static_cast<NavmeshConnectionObject*>(other_obj);
            std::vector<NavMeshConnectionData>::iterator other = nmco->GetConnectionTo( this->GetID() );
			if( other != nmco->connections.end() )
			{
				other->poly_area = SAMPLE_POLYAREA_DISABLED; 
			}
        }
    }
}

std::vector<NavMeshConnectionData>::iterator NavmeshConnectionObject::GetConnectionTo(int other_object_id)
{
    std::vector<NavMeshConnectionData>::iterator iter = connections.begin();

    for(; iter != connections.end(); iter++ )
    {
        if( iter->other_object_id == other_object_id )
        {
            return iter;
        }
    }
    return connections.end();
}

void NavmeshConnectionObject::NotifyDeleted( Object* o ) {
    Object::NotifyDeleted(o);
    Disconnect( *o, true );
}

void NavmeshConnectionObject::GetDesc(EntityDescription &desc) const {
    Object::GetDesc(desc);
    desc.AddString(EDF_FILE_PATH, "");
    desc.AddNavMeshConnnectionVec(EDF_NAV_MESH_CONNECTIONS, connections);
}

bool NavmeshConnectionObject::SetFromDesc( const EntityDescription& desc ) {
    bool ret = Object::SetFromDesc(desc);
    if( ret ) {
        //Keeping this to maintain compatability for now. There shouldn't be any alphas running this though.
        for(const auto & field : desc.fields){
            switch(field.type){
                case EDF_NAV_MESH_CONNECTIONS:
                    field.ReadNavMeshConnectionDataVec(&connections);
                    break;
            }
        }
    }
    return ret;
}

bool NavmeshConnectionObject::Initialize() {
    assert(update_list_entry == -1);
    update_list_entry = scenegraph_->LinkUpdateObject(this);
    return true;
}

void NavmeshConnectionObject::Update(float timestep)
{
    //We do this in update aswell, in case we're not drawing.
    UpdatePolyAreas();
}

void NavmeshConnectionObject::RegisterToScript(ASContext* as_context) {
}

void NavmeshConnectionObject::UpdatePolyAreas()
{
    std::vector<NavMeshConnectionData>::iterator iter = connections.begin();
    for( ; iter != connections.end(); iter++ )
    {
        //Categorize the poly
        if( iter->poly_area == SAMPLE_POLYAREA_DISABLED )
        {
            Object *other = scenegraph_->GetObjectFromID(iter->other_object_id);
            if( other )
            {
                float dist = length( this->GetTranslation() - other->GetTranslation() );
                if( dist < 2.0f )
                {
                    iter->poly_area = SAMPLE_POLYAREA_JUMP1; 
                }
                else if( dist < 5.0f )
                {
                    iter->poly_area = SAMPLE_POLYAREA_JUMP2; 
                }
                else if( dist < 9.0f )
                {
                    iter->poly_area = SAMPLE_POLYAREA_JUMP3; 
                }
                else if( dist < 14.0f )
                {
                    iter->poly_area = SAMPLE_POLYAREA_JUMP4; 
                }
                else
                {
                    iter->poly_area = SAMPLE_POLYAREA_JUMP5;
                }
            }
        }
    }
}

void NavmeshConnectionObject::Draw() {
    if (g_debug_runtime_disable_navmesh_connection_object_draw) {
        return;
    }

    if(!Graphics::Instance()->media_mode() && scenegraph_->map_editor->state_ != MapEditor::kInGame && this->editor_visible){
        DebugDraw::Instance()->AddWireSphere(GetTranslation(), 0.5f, vec4(0.2f,0.2f,0.9f,0.5f), _delete_on_draw);

        //Need to do this to ensure the state is updated for rendering.
        UpdatePolyAreas();

        std::vector<NavMeshConnectionData>::iterator iter = connections.begin();
        for( ; iter != connections.end(); iter++ )
        {
            Object* obj = scenegraph_->GetObjectFromID(iter->other_object_id);
            if(obj == NULL) {
                LOGW_ONCE("Navemesh connection object is connected to -1");
                return;
            }
            
            //All connections are two way right now, so we only draw in one direction.
            if( this->GetID() > obj->GetID() )
            {
                vec4 color = vec4(1.0f,0,0,1.0f);
                DDFlag dd_flag = _DD_NO_FLAG;

                if( iter->offmesh_connection_id == -1 )
                {
                    dd_flag = _DD_DASHED;
                }

                SamplePolyAreas poly_area = iter->poly_area;
                if( poly_area == SAMPLE_POLYAREA_JUMP1 )
                {
                    color = vec4(0, 0.9f,0,1.0f);
                }
                else if( poly_area == SAMPLE_POLYAREA_JUMP2 )
                {
                    color = vec4(83/255.0f,255/255.0f,26/255.0f, 1.0f);
                }
                else if( poly_area == SAMPLE_POLYAREA_JUMP3 )
                {
                    color = vec4(184/255.0f,138/255.0f,0/255.0f, 1.0f);
                }
                else if( poly_area == SAMPLE_POLYAREA_JUMP4 )
                {
                    color = vec4(0,61/255.0f,245/255.0f,1.0f);
                }
                else if( poly_area == SAMPLE_POLYAREA_JUMP5 )
                {
                    color = vec4(102/255.0f,51/255.0f, 255/255.0f, 1.0f);
                }

                DebugDraw::Instance()->AddLine(GetTranslation(), obj->GetTranslation(), color, _delete_on_draw, dd_flag);
            }
        }
    }
}

void NavmeshConnectionObject::FinalizeLoadedConnections() {
    Object::FinalizeLoadedConnections();
    // Remove all connections that aren't to other navmesh objects.
    for(size_t i=0; i<connections.size(); ++i){
        Object* obj = scenegraph_->GetObjectFromID(connections[i].other_object_id);
        if(obj){
            if( obj->GetType() != _navmesh_connection_object ) {
                LOGW << "We have an unexpected connection between a " << CStringFromEntityType(this->GetType()) << " and a " << CStringFromEntityType(obj->GetType()) << " removing."<< std::endl;
                connections.erase(connections.begin()+i);
                i--;
            }
        }
    }

    // Make sure all connections are symmetric
    for(auto & connection : connections){
        Object* obj = scenegraph_->GetObjectFromID(connection.other_object_id);
        if(obj){
            obj->ConnectTo(*this, true);
        }
    }
}

void NavmeshConnectionObject::RemapReferences(std::map<int,int> id_map) {
    for(auto & connection : connections){
        if( id_map.find(connection.other_object_id) != id_map.end() ) {
            connection.other_object_id = id_map[connection.other_object_id];
        } else {
            //The remapped id could belong to this, in which case it is valid
            bool is_this = false;
            for(auto & iter : id_map) {
                if(iter.second == this->GetID()) {
                    is_this = true;
                    break;
                }
            }
            if(!is_this)
                connection.other_object_id = -1;
        }
    }
}

void NavmeshConnectionObject::Moved(Object::MoveType type)
{
    Object::Moved(type); 
    //These beomce outdated if we are modified.
    //When pasting the object, it is moved before adding it to the scenegraph
    if(scenegraph_ != NULL) {
        ResetConnectionOffMeshReference();
        ResetPolyAreaAssignment();
    }
}

