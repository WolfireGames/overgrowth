//-----------------------------------------------------------------------------
//           Name: fire_test_cheap.as
//      Developer: Wolfire Games LLC
//    Script Type: Hotspot
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

float delay = 0.0f;

class Particle {
    vec3 pos;
    vec3 vel;
    float heat;
    float spawn_time;
}

int count = 0;
int num_ribbons;
float sound_start_time;

class Ribbon : C_ACCEL {
    array<Particle> particles;
    vec3 rel_pos;
    vec3 pos;
    float base_rand;
    float spawn_new_particle_delay;
    void Update(float delta_time, float curr_game_time) {
        EnterTelemetryZone("Ribbon Update");
        EnterTelemetryZone("a");
        spawn_new_particle_delay -= delta_time;
        if(spawn_new_particle_delay <= 0.0f){
            Particle particle;
            particle.pos = pos;
            particle.vel = vec3(0.0, 0.0, 0.0);
            particle.heat = RangedRandomFloat(0.5,1.5);
            particle.spawn_time = curr_game_time;
            particles.push_back(particle);
        
            while(spawn_new_particle_delay <= 0.0f){
                spawn_new_particle_delay += 0.1f;
            }
        }
        LeaveTelemetryZone();
        EnterTelemetryZone("b");
        Object@ obj = ReadObjectFromID(hotspot.GetID());
        vec3 fire_pos = obj.GetTranslation();
        int max_particles = 5;
        if(int(particles.size()) > max_particles){
            for(int i=0; i<max_particles; ++i){
                particles[i].pos = particles[particles.size()-max_particles+i].pos;
                particles[i].vel = particles[particles.size()-max_particles+i].vel;
                particles[i].heat = particles[particles.size()-max_particles+i].heat;
                particles[i].spawn_time = particles[particles.size()-max_particles+i].spawn_time;
            }
            particles.resize(max_particles);
        }
        LeaveTelemetryZone();
        EnterTelemetryZone("c");
        for(int i=0, len=particles.size(); i<len; ++i){
            particles[i].vel *= pow(0.2f, delta_time);
            particles[i].pos += particles[i].vel * delta_time;
            particles[i].vel += GetWind(particles[i].pos * 5.0f, curr_game_time, 10.0f) * delta_time * 1.0f;
            particles[i].vel += GetWind(particles[i].pos * 30.0f, curr_game_time, 10.0f) * delta_time * 2.0f;
            vec3 rel = particles[i].pos - fire_pos;
            rel[1] = 0.0;
            particles[i].heat -= delta_time * (2.0f + min(1.0f, pow(dot(rel,rel), 2.0)*64.0f)) * 2.0f;
            if(dot(rel,rel) > 1.0){
                rel = normalize(rel);
            }

            particles[i].vel += rel * delta_time * -3.0f * 6.0f;
            particles[i].vel[1] += delta_time * 12.0f;
        }
        LeaveTelemetryZone();
        /*for(int i=0, len=particles.size()-1; i<len; ++i){
            //DebugDrawLine(particles[i].pos, particles[i+1].pos, ColorFromHeat(particles[i].heat), ColorFromHeat(particles[i+1].heat), _delete_on_update);
            DebugDrawRibbon(particles[i].pos, particles[i+1].pos, ColorFromHeat(particles[i].heat), ColorFromHeat(particles[i+1].heat), flame_width * max(particles[i].heat, 0.0), flame_width * max(particles[i+1].heat, 0.0), _delete_on_update);
        }*/
        LeaveTelemetryZone();
    }
    void PreDraw(float curr_game_time) {
        EnterTelemetryZone("Ribbon PreDraw");
        int ribbon_id = DebugDrawRibbon(_delete_on_draw);
        const float flame_width = 0.12f;
        for(int i=0, len=particles.size(); i<len; ++i){
            AddDebugDrawRibbonPoint(ribbon_id, particles[i].pos, vec4(particles[i].heat, particles[i].spawn_time + base_rand, curr_game_time + base_rand, 0.0), flame_width);
        }
        LeaveTelemetryZone();
    }
}

array<Ribbon> ribbons;

vec3 GetWind(vec3 check_where, float curr_game_time, float change_rate) {
    vec3 wind_vel;
    check_where[0] += curr_game_time*0.7f*change_rate;
    check_where[1] += curr_game_time*0.3f*change_rate;
    check_where[2] += curr_game_time*0.5f*change_rate;
    wind_vel[0] = sin(check_where[0])+cos(check_where[1]*1.3f)+sin(check_where[2]*3.0f);
    wind_vel[1] = sin(check_where[0]*1.2f)+cos(check_where[1]*1.8f)+sin(check_where[2]*0.8f);
    wind_vel[2] = sin(check_where[0]*1.6f)+cos(check_where[1]*0.5f)+sin(check_where[2]*1.2f);

    return wind_vel;
}

int fire_object_id = -1;
int sound_handle = -1;

float last_game_time = 0.0f;

void Dispose() {
    if(fire_object_id != -1){
        QueueDeleteObjectID(fire_object_id);
    }
    if(sound_handle != -1){
        StopSound(sound_handle);
    }
}

void HandleEvent(string event, MovementObject @mo){
    if(event == "enter"){
        OnEnter(mo);
    } else if(event == "exit"){
        OnExit(mo);
    }
}

void OnEnter(MovementObject @mo) {
    bool ignite = true;
    if(params.HasParam("Ignite Characters") && params.GetInt("Ignite Characters") == 0){
        ignite = false;
    }
    if(ignite){
        mo.ReceiveScriptMessage("entered_fire");
    }
}

void SetEnabled(bool val){
    if(!val){
        Dispose();
    }
}

void OnExit(MovementObject @mo) {
}

void PreDraw(float curr_game_time) {
    int obj_id = hotspot.GetID();
    if(ObjectExists(obj_id) && ReadObjectFromID(hotspot.GetID()).GetEnabled()){
        EnterTelemetryZone("Fire Hotspot Predraw");
        float delta_time = curr_game_time - last_game_time;

        Object@ obj = ReadObjectFromID(hotspot.GetID());
        vec3 pos = obj.GetTranslation();
        vec3 scale = obj.GetScale();
        quaternion rot = obj.GetRotation();
        if(int(ribbons.size()) != num_ribbons){
            ribbons.resize(num_ribbons);
            for(int i=0; i<num_ribbons; ++i){
                ribbons[i].rel_pos = vec3(RangedRandomFloat(-1.0f, 1.0f), 0.0f, RangedRandomFloat(-1.0f,1.0f));
                ribbons[i].base_rand += RangedRandomFloat(0.0f, 100.0f);
                ribbons[i].spawn_new_particle_delay = RangedRandomFloat(0.0f, 0.1f);
            }
        }
        --count;
        for(int ribbon_index=0; 
            ribbon_index < num_ribbons;
            ++ribbon_index)
        {
            ribbons[ribbon_index].pos = pos + rot*vec3(ribbons[ribbon_index].rel_pos[0]*scale[0],scale[1]*2.0,ribbons[ribbon_index].rel_pos[2]*scale[2]);
        }
        if(count <= 0){
            count = 10;
        }
        delay -= delta_time;

        if(delay <= 0.0f){
            for(int i=0; i<1; ++i){
//                uint32 id = MakeParticle("Data/Particles/firespark.xml", ribbons[int(RangedRandomFloat(0, num_ribbons-0.01))].pos, vec3(RangedRandomFloat(-2.0f, 2.0f), RangedRandomFloat(5.0f, 10.0f), RangedRandomFloat(-2.0f, 2.0f)), vec3(1.0f));
            }
            delay = RangedRandomFloat(0.0f, 0.6f);
        }
        if(fire_object_id == -1){
//            fire_object_id = CreateObject("Data/Objects/default_light.xml", true);
        }
        if(sound_handle == -1){
            if(!level.WaitingForInput()){
//                sound_handle = PlaySoundLoopAtLocation("Data/Sounds/fire/campfire_loop.wav",pos,0.0f);
//                sound_start_time = curr_game_time;
            }
        } else {
//            SetSoundPosition(sound_handle, pos);
//            SetSoundGain(sound_handle, min(1.0, curr_game_time - sound_start_time));
        }

        if(int(ribbons.size()) == num_ribbons){
            for(int ribbon_index=0; 
                ribbon_index < num_ribbons;
                ++ribbon_index)
            {   
                //ribbons[ribbon_index].Update(delta_time, curr_game_time);
                CFireRibbonUpdate(ribbons[ribbon_index], delta_time, curr_game_time, ReadObjectFromID(hotspot.GetID()).GetTranslation()); 
                //ribbons[ribbon_index].PreDraw(curr_game_time);
                CFireRibbonPreDraw(ribbons[ribbon_index], curr_game_time); 
            }
/*            float amplify = 1.0f;
            if(params.HasParam("Light Amplify")){
                amplify *= params.GetFloat("Light Amplify");
            }*/
/*            float distance = 10.0f;
            if(params.HasParam("Light Distance")){
                distance *= params.GetFloat("Light Distance");
            }*/
/*            if(ribbons[0].particles.size()>3){
                Object@ fire_obj = ReadObjectFromID(fire_object_id);
                fire_obj.SetTranslation(mix(ribbons[0].particles[3].pos, ribbons[0].particles[2].pos, ribbons[0].spawn_new_particle_delay / 0.1f));
                fire_obj.SetTint(amplify * 0.2 * vec3(2.0,1.0,0.0)*(2.0 + mix(ribbons[0].particles[3].heat, ribbons[0].particles[2].heat, ribbons[0].spawn_new_particle_delay / 0.1f)));
                fire_obj.SetScale(vec3(distance));
            }*/
        }
        last_game_time = curr_game_time;
        LeaveTelemetryZone();
    }
}

vec4 ColorFromHeat(float heat){
    if(heat < 0.0){
        return vec4(0.0);
    } else {
        if(heat > 0.5){
            return mix(vec4(2.0, 1.0, 0.0, 1.0), vec4(4.0, 4.0, 0.0, 1.0), (heat-0.5)/0.5);
        } else {
            return mix(vec4(0.0, 0.0, 0.0, 0.0), vec4(2.0, 1.0, 0.0, 1.0), (heat)/0.5);            
        }
    }
}

void SetParameters() {
//    params.AddFloatSlider("Light Amplify",1.0f,"min:0,max:20,step:0.1");
//    params.AddFloatSlider("Light Distance",2.0f,"min:0,max:100,step:0.1");
    params.AddInt("Fire Ribbons",4);
    params.AddIntCheckbox("Ignite Characters", true);
    num_ribbons = max(0, min(500, params.GetInt("Fire Ribbons")));
    hotspot.SetCollisionEnabled(params.GetInt("Ignite Characters") == 1);
}
