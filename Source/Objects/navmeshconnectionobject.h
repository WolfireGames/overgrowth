//-----------------------------------------------------------------------------
//           Name: navmeshconnectionobject.h
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

#include <Objects/object.h>
#include <Game/EntityDescription.h>

#include <vector>
#include <string>
#include <list>

//-----------------------------------------------------------------------------
// Class Definition
//-----------------------------------------------------------------------------

class NavmeshConnectionObject : public Object {
   public:
    EntityType GetType() const override { return _navmesh_connection_object; }
    std::vector<NavMeshConnectionData> connections;

    NavmeshConnectionObject();

    bool ConnectTo(Object& other, bool checking_other = false) override;
    bool AcceptConnectionsFrom(ConnectionType type, Object& object) override;
    virtual bool Disconnect(Object& other, bool checking_other = false);
    void GetConnectionIDs(std::vector<int>* cons) override;

    void ResetConnectionOffMeshReference();
    void ResetPolyAreaAssignment();
    std::vector<NavMeshConnectionData>::iterator GetConnectionTo(int other_object_id);
    void UpdatePolyAreas();

    int GetModelID();
    void NotifyDeleted(Object* o) override;
    void GetDesc(EntityDescription& desc) const override;

    bool SetFromDesc(const EntityDescription& desc) override;
    static void RegisterToScript(ASContext* as_context);
    void Draw() override;
    void Update(float timestep) override;
    void FinalizeLoadedConnections() override;
    bool Initialize() override;

    void Moved(Object::MoveType type) override;

    void RemapReferences(std::map<int, int> id_map) override;
};
