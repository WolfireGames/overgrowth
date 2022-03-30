//-----------------------------------------------------------------------------
//           Name: interpdirection.as
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

vec3 InterpDirections(vec3 dir1, vec3 dir2, float amount){
    vec3 new_dir = normalize(dir1 * (1.0f-amount) +
                             dir2 * amount);
    
    // Add perpendicular offset to ease transitions between opposite facings
    if(dot(dir1, dir2) < -0.8f){
        vec3 break_axis = cross(vec3(0.0f,1.0f,0.0f),dir1);
        if(dot(break_axis,dir2)<0.0f){
            break_axis *= -1.0f;
        }
        new_dir = normalize(dir1 * (1.0f-amount) +
                            break_axis * amount);
    
    }

    return new_dir;
}