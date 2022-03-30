//-----------------------------------------------------------------------------
//           Name: water.as
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

class victim{
    MovementObject@ char;
    Object@ charObj;
    float origSpeed;
    array<int> fixed_bone_ids;
    bool headUnderWater = false;
    bool allowRoll = true;
    float headUnderWaterTimer = 0.0f;
    victim(MovementObject@ _char){
        @char = @_char;
        //Get the current speed multiplier to be able to reset it when the character exits.
        origSpeed = char.GetFloatVar("p_speed_mult");
        @charObj = ReadObjectFromID(char.GetID());
    }
    void Reset(){
        interval = 0.001f;
    }
}

array<victim@> victims;
float time;
float interval = 0.001f;
Object@ thisHotspot = ReadObjectFromID(hotspot.GetID());
int currentIndex = 0;
const int _ragdoll_state = 4;
int headBone = 30;
float underWaterInterval = 0.5f;
float timeBeforeDrowning = 10.0f;

void SetParameters() {
    params.AddFloatSlider("XVel",0.0,"min:-10.0,max:10.0,step:0.1,text_mult:10");
    params.AddFloatSlider("ZVel",0.0,"min:-10.0,max:10.0,step:0.1,text_mult:10");
}

void Reset(){
    for(uint i = 0; i < victims.size(); i++){
        victims[i].char.rigged_object().ClearBoneConstraints();
        victims[i].Reset();
        victims[i].fixed_bone_ids.resize(0);
        victims[i].char.Execute("allow_rolling = true;");
    }
    victims.resize(0);
    currentIndex = 0;
}

void HandleEvent(string event, MovementObject @mo){
    //DebugText("wed", "Event: " + event, _fade);
    if(event == "enter"){
        OnEnter(mo);
    } else if(event == "exit"){
        OnExit(mo);
    }
}

void OnEnter(MovementObject @mo) {
    int index = InsideArray(mo.GetID());
    //If the character is already in the array it doesn't need to add it again.
    if(index == -1){
        victims.insertLast(victim(mo));
    }
}

void OnExit(MovementObject @mo) {
    //Find on which index the character is.
    int index = InsideArray(mo.GetID());
    //If the character is not in the array (somehow) don't do anything.
    if(index != -1){
        SetCharSpeed(mo, victims[index].origSpeed);
        mo.Execute("allow_rolling = true;");
        victims.removeAt(index);
    }
}

void Update(){
    if(victims.size() != 0){
        //This time is used to update the script every interval. For better performance.
        time += time_step;
        if(time > interval){
            //Get the victim object from the list and use it for the rest of the loop.
            victim@ curVictim = victims[currentIndex];
            //If the character is still controllable.
            if(curVictim.char.GetIntVar("state") != _ragdoll_state){
                vec3 charPos = curVictim.char.position;
                vec3 hotspotTop = vec3(charPos.x, thisHotspot.GetTranslation().y + (2.0f * thisHotspot.GetScale().y) ,charPos.z);
                vec3 ground = col.GetRayCollision(charPos, vec3(charPos.x, charPos.y - 2.0f, charPos.z));
                float waterDepth = distance(hotspotTop, ground);
                //Apply damage when the head it under water.
                if(curVictim.headUnderWater){
                    curVictim.headUnderWaterTimer += time_step;
                    if(rand()%100 == 0){
                        int soundID = PlaySoundGroup("Data/Sounds/voice/animal2/voice_bunny_light_block.xml", curVictim.char.position);
                        SetSoundPitch(soundID, 0.3f);
                    }else if(curVictim.headUnderWaterTimer > timeBeforeDrowning){
                        curVictim.char.Execute("SetKnockedOut(_dead);"+
                                                "Ragdoll(_RGDL_INJURED);");
                        int soundID = PlaySound("Data/Sounds/voice/animal2/voice_bunny_groan_3.wav", curVictim.char.position);
                        SetSoundPitch(soundID, 0.5f);
                    }
                }
                float charDepth = distance(hotspotTop, charPos);
                float roll_thresshold = 0.5f;
                if(waterDepth < roll_thresshold && !curVictim.allowRoll){
                    curVictim.char.Execute("allow_rolling = true;");
                    curVictim.allowRoll = true;
                }else if(waterDepth > roll_thresshold && curVictim.allowRoll){
                    curVictim.char.Execute("allow_rolling = false;");
                    curVictim.allowRoll = false;
                }
                if(curVictim.char.GetBoolVar("on_ground")){
                    //The character is standing on the ground.
                    float newSpeed = max(0.1f, 1.0f - waterDepth);
                    SetCharSpeed(curVictim.char, newSpeed);
                    float animSpeed = max(0.0f,((0.8f - newSpeed)/2));

                }else{
                    //The character is jumping or falling.
                    float speed = length_squared(curVictim.char.velocity);
                    float in_water = 2.0f;
                    curVictim.char.velocity.x *= pow(0.97f, in_water);
                    curVictim.char.velocity.z *= pow(0.97f, in_water);
                    if(curVictim.char.velocity.y > 0.0f){
                        curVictim.char.velocity.y *= pow(0.97f, in_water);
                    }

                    Log(info, "depth " + charDepth);
                    if(speed > 80.0f && charPos.y < hotspotTop.y && charDepth > 0.25f){
                        curVictim.char.Execute("GoLimp();");
                    }
                }



                Skeleton @skeleton = curVictim.char.rigged_object().skeleton();
                for(int i = 0; i < curVictim.char.rigged_object().skeleton().NumBones(); i++){
                    if(skeleton.HasPhysics(i)){
                        mat4 transform = skeleton.GetBoneTransform(i);
                        //Check if the bone is under the water.
                        if(transform.GetTranslationPart().y < hotspotTop.y){
                            if(i == headBone){
                                curVictim.headUnderWater = true;
                            }
                        }else{
                            if(i == headBone){
                                curVictim.headUnderWater = false;
                                curVictim.headUnderWaterTimer = 0.0f;
                            }
                        }

                    }
                }
            }else{
                //Handle ragdolls in water, does not mean it's dead though.
                if(curVictim.char.GetBoolVar("frozen") == false){
                    vec3 charPos = curVictim.char.position;
                    vec3 hotspotTop = vec3(charPos.x, thisHotspot.GetTranslation().y + (2.0f * thisHotspot.GetScale().y) ,charPos.z);
                    //DebugDrawLine(hotspotTop, charPos, vec3(1), _delete_on_update);
                    Skeleton @skeleton = curVictim.char.rigged_object().skeleton();
                    for(int i = 0; i < curVictim.char.rigged_object().skeleton().NumBones(); i++){
                        if(skeleton.HasPhysics(i)){
                            mat4 transform = skeleton.GetBoneTransform(i);
                            if(transform.GetTranslationPart().y < hotspotTop.y){
                               float vel = length(curVictim.char.velocity);
                               for(int p = 0; p < floor(vel); p++){
                                    if(floor(vel) % 1.0f == 0 && floor(the_time * 100) % 4.0f == 0){
                                        //MakeParticle("Data/Particles/water_bubble.xml", transform.GetTranslationPart(), vec3(0, 10 * distance(charPos, hotspotTop), 0), vec3(0.3));
                                    }
                               }
                               vec3 localVel = thisHotspot.GetRotation() * vec3(params.GetFloat("XVel"), 0, params.GetFloat("ZVel"));
                               curVictim.char.rigged_object().ApplyForceToBone(vec3(localVel.x, min(100.0f, 100 * distance(charPos, hotspotTop)),localVel.z), i);
                            }
                        }
                    }
                    curVictim.char.rigged_object().SetRagdollDamping(0.99f);
                }
            }
            time = 0.0f;
        }
    }
}

int InsideArray(int id){
    //Because the victims array contains victim objects, we can't use the array.find() function.
    //This is the reason for a function that loops through all of them and tries to find the char id.
    int inside = -1;
    for(uint i = 0; i < victims.size(); i++){
        if(victims[i].char.GetID() == id){
            inside = i;
        }
    }
    return inside;
}

void SetCharSpeed(MovementObject@ char, float speed){
    //Not only the speed multiplier needs to be set. Also the speeds need to be recalculated, for it to take effect.
    char.Execute("p_speed_mult = " + speed + ";" +
                    "run_speed = _base_run_speed * p_speed_mult;" +
                    "true_max_speed = _base_true_max_speed * p_speed_mult;");
}
