//-----------------------------------------------------------------------------
//           Name: spike.as
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

int spike_num = 5;

int spiked = -1;

int spike_tip_hotspot_id = -1;
int spike_collidable_id = -1;
int no_grab_id = -1;

int armed = 0;
bool g_short = false;

const bool super_spike = false;

void HandleEvent(string event, MovementObject @mo){
    //DebugText("wed", "Event: " + event, _fade);
    if(event == "enter"){
        OnEnter(mo);
    } else if(event == "exit"){
        OnExit(mo);
    }else if(event == "reset"){
        spike_num = 20;
        DebugText("spike_num", "spike_num: "+spike_num, 10.0);
    }
}

void OnEnter(MovementObject @mo) {
}

void OnExit(MovementObject @mo) {
}

void Dispose() {
    if(spike_collidable_id != -1){
        QueueDeleteObjectID(spike_collidable_id);
        spike_collidable_id = -1;
    }
    if(spike_tip_hotspot_id != -1){
        QueueDeleteObjectID(spike_tip_hotspot_id);
        spike_tip_hotspot_id = -1;
    }
    if(no_grab_id != -1) {
        QueueDeleteObjectID(no_grab_id);
        no_grab_id = -1;
    }
}

void Init() {
}

void Reset(){
    spike_num = 5;
    spiked = -1;
    armed = 0;
    UpdateObjects();
}

void SetArmed(int val){
    if(val != armed){
        armed = val;
        UpdateObjects();
    }
}

void ReceiveMessage(string msg) {
    if(!super_spike){
        if(msg == "arm_spike"){
            SetArmed(1);
        }
        if(msg == "disarm_spike"){
            if(spiked == -1){
                SetArmed(0);
            }
        }
    }
}

bool g_is_transform_dirty = true;
vec3 g_prev_translation;
quaternion g_prev_rotation;
vec3 g_prev_scale;
bool g_has_set_spike_tip_transform = false;
bool g_has_set_spike_collidable_transform = false;
bool g_has_set_spike_no_grab_transform = false;
int g_prev_armed = -1;

void UpdateObjects() {
    if(spike_collidable_id == -1){
        if(g_short){
            spike_collidable_id = CreateObject("Data/Objects/Environment/camp/sharp_stick_short.xml", true);
            ReadObjectFromID(spike_collidable_id).SetEnabled(true);
        } else {
            spike_collidable_id = CreateObject("Data/Objects/Environment/camp/sharp_stick_long.xml", true);
            ReadObjectFromID(spike_collidable_id).SetEnabled(true);
        }
        g_has_set_spike_collidable_transform = false;
    }

    if(spike_tip_hotspot_id == -1){
        spike_tip_hotspot_id = CreateObject("Data/Objects/Hotspots/spike_tip.xml", true);
        Object@ obj = ReadObjectFromID(spike_tip_hotspot_id);
        obj.SetEnabled(true);
        obj.GetScriptParams().SetInt("Parent", hotspot.GetID());
        g_has_set_spike_tip_transform = false;
    }

    if(no_grab_id == -1){
        no_grab_id = CreateObject("Data/Objects/Hotspots/no_grab.xml", true);
        g_has_set_spike_no_grab_transform = false;
    }

    // See if current transform changed, so we can avoid costly updates if it hasn't
    Object@ hotspot_obj = ReadObjectFromID(hotspot.GetID());
    vec3 current_translation = hotspot_obj.GetTranslation();
    quaternion current_rotation = hotspot_obj.GetRotation();
    vec3 current_scale = hotspot_obj.GetScale();

    if(g_prev_translation != current_translation || g_prev_rotation != current_rotation || g_prev_scale != current_scale){
        g_is_transform_dirty = true;
    }

    // spike tip
    if(spike_tip_hotspot_id != -1){
        if(g_is_transform_dirty || !g_has_set_spike_tip_transform){
            Object@ obj = ReadObjectFromID(spike_tip_hotspot_id);
            obj.SetRotation(current_rotation);
            obj.SetTranslation(current_translation+current_rotation * vec3(0,current_scale[1]*2+0.2,0));
            obj.SetScale(vec3(0.2));
            g_has_set_spike_tip_transform = true;
        }
    } else {
        g_has_set_spike_tip_transform = false;
    }

    // spike collidable
    if(spike_collidable_id != -1){
        if(armed == 0) {
            if(g_is_transform_dirty || !g_has_set_spike_collidable_transform){
                Object@ obj = ReadObjectFromID(spike_collidable_id);
                obj.SetRotation(current_rotation);
                obj.SetTranslation(current_translation+current_rotation * vec3(0.03,0,0.0));
                obj.SetScale(vec3(1,current_scale.y*2.0*(g_short?0.92:0.85),1));
                g_has_set_spike_collidable_transform = true;
            }
        }
    } else {
        g_has_set_spike_collidable_transform = false;
    }

    // spike no-grab
    if(no_grab_id != -1){
        if(g_is_transform_dirty || !g_has_set_spike_no_grab_transform){
            Object@ obj = ReadObjectFromID(no_grab_id);
            Object@ reference_obj = ReadObjectFromID(spike_collidable_id);
            obj.SetRotation(current_rotation);
            obj.SetTranslation(current_translation);
            obj.SetScale(vec3(1,current_scale.y*0.45f,1) * reference_obj.GetBoundingBox());
            g_has_set_spike_no_grab_transform = true;
        }
    } else {
        g_has_set_spike_no_grab_transform = false;
    }

    if(armed != g_prev_armed){
        if(armed == 1){
            ReadObjectFromID(spike_collidable_id).SetCollisionEnabled(false);
        }
        if(armed == 0){
            ReadObjectFromID(spike_collidable_id).SetCollisionEnabled(true);
        }
        g_prev_armed = armed;
    }

    g_prev_translation = current_translation;
    g_prev_rotation = current_rotation;
    g_prev_scale = current_scale;
    g_is_transform_dirty = false;
}

void PreDraw(float curr_game_time) {
    UpdateObjects();
}

void Draw() {
}

void Update() {
    if(armed == 1){
        Object@ obj = ReadObjectFromID(hotspot.GetID());
        /*for(int i=0; i<200; ++i){
            DebugDrawWireSphere(obj.GetTranslation() + obj.GetRotation() * (vec3(0,obj.GetScale()[1]*2,0)) * mix(-1, 1, (i)/199.0), mix(0.01,0.1,(i)/199.0), vec3(1.0), _delete_on_draw);        
        }*/

        vec3 start = obj.GetTranslation() + obj.GetRotation() * (vec3(0,obj.GetScale()[1]*2,0)) * -1.0f;
        vec3 end = obj.GetTranslation() + obj.GetRotation() * (vec3(0,obj.GetScale()[1]*2,0)) * 1.0f;

        vec3 start_to_end = normalize(end-start);
        if(!super_spike){
            col.CheckRayCollisionCharacters(end-start_to_end*0.1, end+start_to_end*0.05);
        } else {
            col.CheckRayCollisionCharacters(start, end+start_to_end*0.05);            
        }
        if(sphere_col.NumContacts() != 0){
            MovementObject@ char = ReadCharacterID(sphere_col.GetContact(0).id);
            if(super_spike || dot(char.velocity, start_to_end) < 0.0){
                if(spike_num > 0){
                    char.rigged_object().Stab(sphere_col.GetContact(0).position, normalize(end-start), (spike_num==4)?1:0, 0);
                    --spike_num;
                }
                if(spiked == -1){
                    vec3 dir = normalize(start-end);
                    float extend = 0.4;
                    col.CheckRayCollisionCharacters(start+dir*extend, end+dir*-extend);
                    for(int i=0; i<sphere_col.NumContacts(); ++i){
                        int bone = sphere_col.GetContact(i).tri;
                        char.rigged_object().SpikeRagdollPart(bone,start,end,sphere_col.GetContact(i).position);   
                    }
                    col.CheckRayCollisionCharacters(end+dir*-extend,start+dir*extend);
                    for(int i=0; i<sphere_col.NumContacts(); ++i){
                        int bone = sphere_col.GetContact(i).tri;
                        char.rigged_object().SpikeRagdollPart(bone,end,start,sphere_col.GetContact(i).position);   
                    }
                    //string sound = "Data/Sounds/hit/hit_medium_juicy.xml";
                    string sound = "Data/Sounds/weapon_foley/cut/flesh_hit.xml";
                    PlaySoundGroup(sound, char.position);
                    spiked = char.GetID();
                    if(char.GetIntVar("knocked_out") != _dead){
                        char.Execute("TakeBloodDamage(1.0f);Ragdoll(_RGDL_INJURED);injured_ragdoll_time = RangedRandomFloat(0.0, 12.0);death_hint=_hint_avoid_spikes;");
                    }
                }
            }
            /*vec3 force = camera.GetFacing() * 5000.0f;
            vec3 hit_pos = sphere_col.GetContact(0).position;
            char.Execute("vec3 impulse = vec3("+force.x+", "+force.y+", "+force.z+");" +
                         "vec3 pos = vec3("+hit_pos.x+", "+hit_pos.y+", "+hit_pos.z+");" +
                         "HandleRagdollImpactImpulse(impulse, pos, 5.0f);");*/
        } 
    }
}

void SetParameters() {
    g_short = params.HasParam("Short");
}
