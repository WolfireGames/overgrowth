//-----------------------------------------------------------------------------
//           Name: emitter.as
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

enum ParticleType {
    _smoke = 0, 
    _falling_water = 1,
    _foggy = 2
};

int particle_type;

void Init() {
    hotspot.SetCollisionEnabled(false);
}

void SetParameters() {
    params.AddString("Type", "Smoke");
    string type_string = params.GetString("Type");
    Log(info, "type_string: "+type_string);
    if(type_string == "Smoke"){
        particle_type = _smoke;
    } else if(type_string == "Foggy"){
        particle_type = _foggy;
    } else if(type_string == "Falling Water"){
        particle_type = _falling_water;
    }
}

float delay = 0.0;

float last_game_time = 0.0;
void PreDraw(float curr_game_time) {
    EnterTelemetryZone("Emitter Update");

    if(ReadObjectFromID(hotspot.GetID()).GetEnabled()){
        float delta_time = curr_game_time - last_game_time;

        Object@ obj = ReadObjectFromID(hotspot.GetID());
        vec3 pos = obj.GetTranslation();
        vec3 scale = obj.GetScale();
        vec4 v = obj.GetRotationVec4();
        quaternion rotation(v.x,v.y,v.z,v.a);
        delay -= delta_time;
        if(delay <= 0.0f){
            if(particle_type == _smoke){
                for(int i=0; i<1; ++i){
                    vec3 offset;
                    offset.x += RangedRandomFloat(-scale.x*2.0f,scale.x*2.0f);
                    offset.y += RangedRandomFloat(-scale.y*2.0f,scale.y*2.0f);
                    offset.z += RangedRandomFloat(-scale.z*2.0f,scale.z*2.0f);
                    uint32 id = MakeParticle("Data/Particles/ambientbloodcloud.xml", pos + Mult(rotation, offset), vec3(0.0f), vec3(1.0f));
                }
                delay += 5.1f;
            }
                    if(particle_type == _foggy){
                for(int i=0; i<1; ++i){
                    vec3 offset;
                    offset.x += RangedRandomFloat(-scale.x*2.0f,scale.x*2.0f);
                    offset.y += RangedRandomFloat(-scale.y*2.0f,scale.y*2.0f);
                    offset.z += RangedRandomFloat(-scale.z*2.0f,scale.z*2.0f);
                    uint32 id = MakeParticle("Data/Particles/smoke_foggy.xml", pos + Mult(rotation, offset), vec3(0.0f), vec3(1.0f));
                }
                delay += 0.4f;
            }
            if(particle_type == _falling_water){
                for(int i=0; i<1; ++i){
                    vec3 offset;
                    offset.x += RangedRandomFloat(-scale.x*2.0f,scale.x*2.0f);
                    offset.y += RangedRandomFloat(-scale.y*2.0f,scale.y*2.0f);
                    offset.z += RangedRandomFloat(-scale.z*2.0f,scale.z*2.0f);
                    vec3 vel = rotation * vec3(1,0,0);
                    uint32 id = MakeParticle("Data/Particles/falling_water.xml", pos + Mult(rotation, offset), vel * 3.0, vec3(1.0f));
                }
                for(int i=0; i<1; ++i){
                    vec3 offset;
                    offset.x += RangedRandomFloat(-scale.x*2.0f,scale.x*2.0f);
                    offset.y += RangedRandomFloat(-scale.y*2.0f,scale.y*2.0f);
                    offset.z += RangedRandomFloat(-scale.z*2.0f,scale.z*2.0f);
                    vec3 vel = rotation * vec3(1,0,0);
                    uint32 id = MakeParticle("Data/Particles/falling_water_drops.xml", pos + Mult(rotation, offset), vel * 2.0, vec3(1.0f));
                }
                delay += 0.2f;
            }
        }
        if(delay < -1.0){
            delay = -1.0;
        }
    }
    last_game_time = curr_game_time;
    LeaveTelemetryZone();
}
