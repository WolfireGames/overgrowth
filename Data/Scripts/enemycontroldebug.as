//-----------------------------------------------------------------------------
//           Name: enemycontroldebug.as
//      Developer: Wolfire Games LLC
//    Script Type:
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

class DebugPath
{
    array<int> path_lines;

    DebugPath()
    {
    }

    ~DebugPath()
    {
        ClearPath();
    }

    void ClearPath()
    {
        for(int i=0; i<int(path_lines.length()); ++i){
            DebugDrawRemove(path_lines[i]);
        }
        path_lines.resize(0);
    }

    void UpdatePath()
    {
        ClearPath();
        int num_points = path.NumPoints();

        for(int i=1; i<num_points; i++){
            vec3 color(1.0f);
            uint32 flag = path.GetFlag(i-1);

            if( DT_STRAIGHTPATH_OFFMESH_CONNECTION & flag != 0 )
            {
                color = vec3(1.0f,0,0);
            }

            path_lines.insertLast(DebugDrawLine(path.GetPoint(i-1) + vec3(0.0, 0.1, 0.0), path.GetPoint(i) + vec3(0.0, 0.1, 0.0), color, _persistent));

            path_lines.insertLast(DebugDrawLine(path.GetPoint(i-1) + vec3(0.0, 0.1, 0.0), path.GetPoint(i-1) + vec3(0.0, 0.5, 0.0), vec3(1.0f,0,0),_persistent));
            path_lines.insertLast(DebugDrawLine(path.GetPoint(i) + vec3(0.0, 0.1, 0.0), path.GetPoint(i) + vec3(0.0, 0.5, 0.0), vec3(1.0f,0,0),_persistent));
        }
    }
}

class DebugInvestigatePoints
{
    array<int> path_lines;
    DebugInvestigatePoints()
    {
    }

    ~DebugInvestigatePoints()
    {
        ClearPath();
    }

    void ClearPath()
    {
        for(int i=0; i<int(path_lines.length()); ++i){
            DebugDrawRemove(path_lines[i]);
        }
        path_lines.resize(0);
    }

    void UpdatePath()
    {
        ClearPath();
        int num_points = investigate_points.size();

        if( num_points > 0 )
        {
            path_lines.insertLast(
                DebugDrawLine(
                    this_mo.position + vec3(0.0, 0.1, 0.0),
                    investigate_points[0].pos + vec3(0.0, 0.1, 0.0),
                    vec3(5.0f,5.0f,1.0f),
                    _persistent
                )
            );

        }

        for(int i=1; i<num_points; i++){
            path_lines.insertLast(DebugDrawLine(
                investigate_points[i-1].pos + vec3(0.0, 0.1, 0.0),
                investigate_points[i].pos + vec3(0.0, 0.1, 0.0),
                vec3(5.0f,5.0f,1.0f),
                _persistent
                )
            );

            path_lines.insertLast(DebugDrawLine(
                investigate_points[i-1].pos + vec3(0.0, 0.1, 0.0),
                investigate_points[i-1].pos + vec3(0.0, 0.5, 0.0),
                vec3(0.0f,0.0f,1.0f),
                _persistent
                )
            );

            path_lines.insertLast(DebugDrawLine(
                investigate_points[i].pos + vec3(0.0, 0.1, 0.0),
                investigate_points[i].pos + vec3(0.0, 0.5, 0.0),
                vec3(0.0f,0.0f,1.0f),
                _persistent
                )
            );
        }
    }
}

DebugPath debug_path;
DebugInvestigatePoints debug_investigate_points;
void DebugDrawAIPath()
{
    if( GetConfigValueBool( "debug_show_ai_path" ) )
    {
        debug_path.UpdatePath();
        debug_investigate_points.UpdatePath();
    }
    else
    {
        debug_path.ClearPath();
        debug_investigate_points.ClearPath();
    }
}
