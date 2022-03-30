//-----------------------------------------------------------------------------
//           Name: situationawareness.as
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

enum LookTargetType {
    _none,
    _character,
    _item
};

class LookTarget {
    int id;
    float interest;
    LookTargetType type;
    LookTarget() {
        id = 0;
        type = _none;
    }
}

class KnownChar {
    int id;
    bool friendly;
    float interest;
    int knocked_out;
    vec3 last_known_position;
    vec3 last_known_velocity;
    float last_seen_time;
};

class KnownItem {
    int id;
    vec3 last_known_position;
};

const float _interest_inertia = 0.96f;

class Situation {
    array<KnownChar> known_chars;
    array<KnownItem> known_items;
    array<LookTarget> look_targets;

    void clear() {
        known_chars.resize(0);
        known_items.resize(0);
        look_targets.resize(0);
    }

    void Notice(int id) {
        int already_known = -1;
        Object@ obj = ReadObjectFromID(id);
        EntityType type = obj.GetType();
        if(type == _movement_object){
            if(ReadCharacterID(id).static_char){
                return;
            }
            for(uint i=0; i<known_chars.size(); ++i){
                if(known_chars[i].id == id){
                    already_known = i;
                    break;
                }
            }
            if(already_known == -1){
                already_known = known_chars.size();
                KnownChar kc;
                kc.id = id;
                known_chars.push_back(kc);
                //Print("New char seen\n");
            } else {
                //Print("Char already seen\n");
            }
            known_chars[already_known].interest = 1.0f;
            MovementObject@ char = ReadCharacterID(known_chars[already_known].id);
            known_chars[already_known].friendly = this_mo.OnSameTeam(char);
            known_chars[already_known].knocked_out = char.GetIntVar("knocked_out");
            known_chars[already_known].last_seen_time = time;
            known_chars[already_known].last_known_position = char.position;
            known_chars[already_known].last_known_velocity = char.velocity;
        } else if(type == _item_object){
            for(uint i=0; i<known_items.size(); ++i){
                if(known_items[i].id == id){
                    already_known = i;
                    break;
                }
            }   
            if(already_known == -1){
                KnownItem ki;
                ki.id = id;
                ki.last_known_position = ReadItemID(id).GetPhysicsPosition();
                known_items.push_back(ki);
                //Print("New item seen\n");
            } else {
                known_items[already_known].last_known_position = ReadItemID(id).GetPhysicsPosition();
                //Print("Item already seen\n");
            }
        }
        
    }

    bool KnowsAbout(int id) {
        for(uint i=0; i<known_chars.size(); ++i){
            if(known_chars[i].id == id){
                return true;
            }
        }
        return false;
    }
    
    int KnownID(int id) {
        for(uint i=0; i<known_chars.size(); ++i){
            if(known_chars[i].id == id){
                return i;
            }
        }
        return -1;
    }

    void MovementObjectDeleted(int id) {
        for(uint i=0; i<known_chars.size(); ++i){
            if(known_chars[i].id == id){
                known_chars.removeAt(i);
                return;
            }
        }
    }

    void GetLookTarget(LookTarget& lt){
        for(uint i=0; i<known_chars.size(); ++i){
            if (ObjectExists(known_chars[i].id)) {
                vec3 char_pos = ReadCharacterID(known_chars[i].id).position;
                known_chars[i].interest = 1.0f/ distance(this_mo.position, char_pos);
                known_chars[i].interest = max(0.0f,min(1.0f,known_chars[i].interest));
            }
        }
        look_targets.resize(0);
        for(uint i=0; i<known_chars.size(); ++i){
            if (ObjectExists(known_chars[i].id)) {
                LookTarget new_lt;
                new_lt.id = known_chars[i].id;
                new_lt.interest = known_chars[i].interest;
                new_lt.type= _character;
                look_targets.push_back(new_lt);
            }
        }
        {
            LookTarget new_lt;
            new_lt.interest = 0.5f;
            new_lt.type= _none;
            look_targets.push_back(new_lt);
        }

        float pick_val = RangedRandomFloat(0.0f,1.0f);
        {
            float total_interest = 0.0;
            for(uint i=0; i<look_targets.size(); ++i){
                total_interest += look_targets[i].interest;
            }
            total_interest = max(total_interest, 1.0f);
            pick_val *= total_interest;
        }

        lt.type = _none;
        for(uint i=0; i<look_targets.size(); ++i){
            if(pick_val < look_targets[i].interest){
                lt = look_targets[i];
                //Print("Picked char: "+ look_targets[i].interest+"\n");
                return;
            } else {
                //Print("Picked none\n");
                pick_val -= look_targets[i].interest;
            }
        }
        return;
    }

    bool PlayCombatSong() {
        for(uint i=0; i<known_chars.size(); ++i){
            if(!known_chars[i].friendly){
                if( MovementObjectExists(known_chars[i].id) ) {
                    MovementObject@ char = ReadCharacterID(known_chars[i].id);
                    if(char.QueryIntFunction("int IsAggressive()") == 1){
                        return true;
                    }
                } else {
                    Log(warning, "Character " + known_chars[i].id + " appears to have disappeared.");
                }
            }
        }

        return false;
    }

    int GetForceLookTarget() {
        if(state == _attack_state){
            return target_id;
        }

        const float _target_look_threshold = this_mo.controlled?7.0f:30.0f; // How close target must be to look at it
        const float _target_look_threshold_sqrd = 
            _target_look_threshold * _target_look_threshold;

        int closest_id = -1;
        float closest_dist = 0.0f;

        for(uint i=0; i<known_chars.size(); ++i){
            if(!known_chars[i].friendly){
                if( MovementObjectExists(known_chars[i].id) ) {
                    MovementObject@ char = ReadCharacterID(known_chars[i].id);
                    float dist;
                    if(this_mo.controlled) {
                        vec3 cam_dir = normalize(char.position - camera.GetPos());
                        dist = -dot(cam_dir, camera.GetFacing());
                    } else {
                        dist = distance_squared(char.position, this_mo.position);
                    }
                    if(dist < _target_look_threshold_sqrd){
                        if(closest_id == -1 || dist < closest_dist){
                            closest_id = known_chars[i].id;
                            closest_dist = dist;
                        }
                    }
                } else {                    
                    Log(warning, "Character " + known_chars[i].id + " appears to have disappeared.");
                    MovementObjectDeleted(known_chars[i].id);
                    i--;
                }
            }
        }
        return closest_id;
    }
};
