//-----------------------------------------------------------------------------
//           Name: fog.as
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

void Init() {
}

const float frequency = 2.0f;
float delay = frequency;

void Update() {
    Object@ obj = ReadObjectFromID(hotspot.GetID());
    vec3 pos = obj.GetTranslation();
    vec3 scale = obj.GetScale();
    vec4 v = obj.GetRotationVec4();
    quaternion rotation(v.x,v.y,v.z,v.a);
    delay -= time_step;
    if(delay <= 0.0f){
        for(int i=0; i<1; ++i){
            vec3 offset;
            offset.x += RangedRandomFloat(-scale.x*2.0f,scale.x*2.0f);
            offset.y += RangedRandomFloat(-scale.y*2.0f,scale.y*2.0f);
            offset.z += RangedRandomFloat(-scale.z*2.0f,scale.z*2.0f);
            uint32 id = MakeParticle("Data/Particles/fog.xml",pos + Mult(rotation, offset),vec3(0.0f),vec3(1.0f));
        }
        delay += frequency;
    }
}