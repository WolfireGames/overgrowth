//-----------------------------------------------------------------------------
//           Name: paths.cpp
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
#include "paths.h"

#include <Math/enginemath.h>
#include <Math/vec3math.h>

#include <Graphics/pxdebugdraw.h>
#include <Scripting/angelscript/ascontext.h>

#include <sstream>

void AIPaths::Draw() {
    for(PointMap::const_iterator iter = points.begin(); iter != points.end(); ++iter){
        const vec3& point = iter->second;
        DebugDraw::Instance()->AddWireSphere(point, 
                                             0.5f, 
                                             vec4(0.0f,0.5f,0.5f,0.5f),
                                             _delete_on_draw);
    }
    for(auto & connection : connections){
        DebugDraw::Instance()->AddLine(GetPointPosition(connection.point_ids[0]),
                                       GetPointPosition(connection.point_ids[1]),
                                       vec4(0.0f,0.5f,0.5f,0.5f), 
                                       _delete_on_draw);
    }
}

int AIPaths::GetNearestPoint( const vec3 &pos )
{
    float closest_distance;
    float distance;
    int closest_point = -1;
    for(PointMap::const_iterator iter = points.begin(); iter != points.end(); ++iter){
        const vec3 &point = iter->second;
        int id = iter->first;
        distance = distance_squared(pos, point);
        if(closest_point == -1 || distance < closest_distance){
            closest_point = id;
            closest_distance = distance;
        }
    }
    return closest_point;
}

vec3 AIPaths::GetPointPosition( int point_id ) {
    PointMap::const_iterator iter = points.find(point_id);
    if(iter != points.end()){
        return iter->second;
    } else {
        std::ostringstream oss;
        oss << "Could not find waypoint " << point_id;
        DisplayError("Error",oss.str().c_str());
        return vec3(0.0f);
    }
}

int AIPaths::GetConnectedPoint( int point_id )
{
    for(auto & connection : connections){
        if(connection.point_ids[0] == point_id){
            return connection.point_ids[1];
        }
        if(connection.point_ids[1] == point_id){
            return connection.point_ids[0];
        }
    }
    return -1;
}

int AIPaths::GetOtherConnectedPoint( int point_id, int other_id )
{
    for(auto & connection : connections){
        if(connection.point_ids[0] == point_id &&
           connection.point_ids[1] != other_id){
            return connection.point_ids[1];
        }
        if(connection.point_ids[1] == point_id &&
           connection.point_ids[0] != other_id){
            return connection.point_ids[0];
        }
    }
    return -1;
}

void AIPaths::AddPoint( int id, const vec3 &pos ) {
    points[id] = pos;
}

void AIPaths::AddConnection( int a, int b ) {
    connections.push_back( PathConnection(a, b) );
}

void AIPaths::RemovePoint( int id ) {
    PointMap::iterator iter = points.find(id);
    if(iter != points.end()){
        points.erase(iter);
    }
    for(std::vector<PathConnection>::iterator iter = connections.begin(); iter != connections.end();){
        PathConnection &connection = (*iter);
        if(connection.point_ids[0] == id || connection.point_ids[1] == id){
            iter = connections.erase(iter);
        } else {
            ++iter;
        }
    }
}

void AIPaths::RemoveConnection( int a, int b ) {
    for(std::vector<PathConnection>::iterator iter = connections.begin(); iter != connections.end();){
        PathConnection &connection = (*iter);
        if(connection.point_ids[0] == a && connection.point_ids[1] == b){
            iter = connections.erase(iter);
        } else {
            ++iter;
        }
    }
}

void AIPaths::SetPoint( int id, const vec3 &pos ) {
    PointMap::iterator iter = points.find(id);
    if(iter != points.end()){
        iter->second = pos;
    }/* else {
        std::ostringstream oss;
        oss << "Could not find waypoint " << id;
        DisplayError("Error", oss.str());
    }*/
}

void PathScriptReader::AttachToScript( ASContext *as_context, const std::string& as_name )
{
    as_context->RegisterObjectType("PathScriptReader", 0, asOBJ_REF | asOBJ_NOHANDLE);
    as_context->RegisterObjectMethod("PathScriptReader",
        "int GetNearestPoint(vec3)",
        asMETHOD(PathScriptReader, GetNearestPoint), asCALL_THISCALL);
    as_context->RegisterObjectMethod("PathScriptReader",
        "vec3 GetPointPosition(int point_id)",
        asMETHOD(PathScriptReader, GetPointPosition), asCALL_THISCALL);
    as_context->RegisterObjectMethod("PathScriptReader",
        "int GetConnectedPoint(int point_id)",
        asMETHOD(PathScriptReader, GetConnectedPoint), asCALL_THISCALL);
    as_context->RegisterObjectMethod("PathScriptReader",
        "int GetOtherConnectedPoint(int point_id, int other_id)",
        asMETHOD(PathScriptReader, GetOtherConnectedPoint), asCALL_THISCALL);
    as_context->DocsCloseBrace();

    as_context->RegisterGlobalProperty(("PathScriptReader "+as_name).c_str(), this);
}

int PathScriptReader::GetNearestPoint( vec3 pos ) {
    return AIPaths::Instance()->GetNearestPoint(pos);
}

vec3 PathScriptReader::GetPointPosition( int point_id ) {
    return AIPaths::Instance()->GetPointPosition(point_id);
}

int PathScriptReader::GetConnectedPoint( int point_id ) {
    return AIPaths::Instance()->GetConnectedPoint(point_id);
}

int PathScriptReader::GetOtherConnectedPoint( int point_id, int other_id ) {
    return AIPaths::Instance()->GetOtherConnectedPoint(point_id, other_id);
}
