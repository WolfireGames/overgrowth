//-----------------------------------------------------------------------------
//           Name: water.as
//      Developer: Wolfire Games LLC
//    Script Type: Hotspot
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

const int _ragdoll_state = 4;
const int head_bone = 30;
const float under_water_interval = 0.5f;
const float time_before_drowning = 10.0f;

array<Victim@> victims;
float time_elapsed = 0.0f;
float update_interval = 0.001f;
Object@ hotspot_obj = ReadObjectFromID(hotspot.GetID());

void SetParameters() {
    params.AddFloatSlider("XVel", 0.0f, "min:-10.0,max:10.0,step:0.1,text_mult:10");
    params.AddFloatSlider("ZVel", 0.0f, "min:-10.0,max:10.0,step:0.1,text_mult:10");
}

void Reset() {
    for (uint i = 0; i < victims.length(); ++i) {
        victims[i].Reset();
    }
    victims.resize(0);
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter") {
        OnEnter(mo);
    } else if (event == "exit") {
        OnExit(mo);
    }
}

void OnEnter(MovementObject@ mo) {
    if (FindVictimIndex(mo.GetID()) != -1) {
        return;
    }
    victims.insertLast(Victim(mo));
}

void OnExit(MovementObject@ mo) {
    int index = FindVictimIndex(mo.GetID());
    if (index == -1) {
        return;
    }
    victims[index].RestoreCharacterState();
    victims.removeAt(index);
}

void Update() {
    if (victims.length() == 0) {
        return;
    }

    time_elapsed += time_step;
    if (time_elapsed <= update_interval) {
        return;
    }

    for (uint i = 0; i < victims.length(); ++i) {
        Victim@ victim = victims[i];
        if (victim.character.GetIntVar("state") != _ragdoll_state) {
            HandleCharacterInWater(victim);
        } else {
            HandleRagdollInWater(victim);
        }
    }
    time_elapsed = 0.0f;
}

void HandleCharacterInWater(Victim@ victim) {
    vec3 char_pos = victim.character.position;
    float water_surface_y = hotspot_obj.GetTranslation().y + (2.0f * hotspot_obj.GetScale().y);
    vec3 hotspot_top = vec3(char_pos.x, water_surface_y, char_pos.z);
    vec3 ground = col.GetRayCollision(char_pos, vec3(char_pos.x, char_pos.y - 2.0f, char_pos.z));
    float water_depth = distance(hotspot_top, ground);

    CheckHeadUnderWater(victim, hotspot_top);

    if (victim.character.GetBoolVar("on_ground")) {
        AdjustCharacterSpeed(victim, water_depth);
    } else {
        ApplyWaterResistance(victim);
    }

    ToggleRollingAbility(victim, water_depth);
}

void HandleRagdollInWater(Victim@ victim) {
    if (victim.character.GetBoolVar("frozen")) {
        return;
    }
    vec3 char_pos = victim.character.position;
    float water_surface_y = hotspot_obj.GetTranslation().y + (2.0f * hotspot_obj.GetScale().y);
    vec3 hotspot_top = vec3(char_pos.x, water_surface_y, char_pos.z);

    Skeleton@ skeleton = victim.character.rigged_object().skeleton();
    for (int i = 0; i < skeleton.NumBones(); ++i) {
        if (!skeleton.HasPhysics(i)) {
            continue;
        }
        mat4 transform = skeleton.GetBoneTransform(i);
        if (transform.GetTranslationPart().y < hotspot_top.y) {
            ApplyBuoyancy(victim, i, char_pos, hotspot_top);
        }
    }
    victim.character.rigged_object().SetRagdollDamping(0.99f);
}

void CheckHeadUnderWater(Victim@ victim, const vec3& in hotspot_top) {
    Skeleton@ skeleton = victim.character.rigged_object().skeleton();
    mat4 head_transform = skeleton.GetBoneTransform(head_bone);
    if (head_transform.GetTranslationPart().y < hotspot_top.y) {
        victim.head_under_water = true;
        victim.head_under_water_timer += time_step;
        if (victim.head_under_water_timer > time_before_drowning) {
            victim.character.Execute("SetKnockedOut(_dead); Ragdoll(_RGDL_INJURED);");
            PlayUnderwaterSound(victim.character.position, 0.5f);
        }
    } else {
        victim.head_under_water = false;
        victim.head_under_water_timer = 0.0f;
    }
}

void AdjustCharacterSpeed(Victim@ victim, float water_depth) {
    float new_speed = max(0.1f, 1.0f - water_depth);
    victim.SetCharacterSpeed(new_speed);
}

void ApplyWaterResistance(Victim@ victim) {
    float in_water_resistance = 2.0f;
    victim.character.velocity.x *= pow(0.97f, in_water_resistance);
    victim.character.velocity.z *= pow(0.97f, in_water_resistance);
    if (victim.character.velocity.y > 0.0f) {
        victim.character.velocity.y *= pow(0.97f, in_water_resistance);
    }
    if (length_squared(victim.character.velocity) > 80.0f && victim.character.position.y < hotspot_obj.GetTranslation().y) {
        victim.character.Execute("GoLimp();");
    }
}

void ToggleRollingAbility(Victim@ victim, float water_depth) {
    float roll_threshold = 0.5f;
    if (water_depth < roll_threshold && !victim.allow_roll) {
        victim.character.Execute("allow_rolling = true;");
        victim.allow_roll = true;
    } else if (water_depth > roll_threshold && victim.allow_roll) {
        victim.character.Execute("allow_rolling = false;");
        victim.allow_roll = false;
    }
}

void ApplyBuoyancy(Victim@ victim, int bone_index, const vec3& in char_pos, const vec3& in hotspot_top) {
    float velocity_magnitude = length(victim.character.velocity);
    if (velocity_magnitude > 80.0f && char_pos.y < hotspot_top.y) {
        victim.character.Execute("GoLimp();");
    }
    vec3 local_velocity = hotspot_obj.GetRotation() * vec3(params.GetFloat("XVel"), 0.0f, params.GetFloat("ZVel"));
    victim.character.rigged_object().ApplyForceToBone(
        vec3(local_velocity.x, min(100.0f, 100.0f * distance(char_pos, hotspot_top)), local_velocity.z),
        bone_index
    );
}

void PlayUnderwaterSound(const vec3& in position, float pitch) {
    int sound_id = PlaySound("Data/Sounds/voice/animal2/voice_bunny_groan_3.wav", position);
    SetSoundPitch(sound_id, pitch);
}

int FindVictimIndex(int char_id) {
    for (uint i = 0; i < victims.length(); ++i) {
        if (victims[i].character.GetID() == char_id) {
            return int(i);
        }
    }
    return -1;
}

class Victim {
    MovementObject@ character;
    float original_speed;
    bool head_under_water = false;
    bool allow_roll = true;
    float head_under_water_timer = 0.0f;

    Victim(MovementObject@ char_ref) {
        @character = char_ref;
        original_speed = character.GetFloatVar("p_speed_mult");
    }

    void Reset() {
        character.rigged_object().ClearBoneConstraints();
        character.Execute("allow_rolling = true;");
    }

    void RestoreCharacterState() {
        SetCharacterSpeed(original_speed);
        character.Execute("allow_rolling = true;");
    }

    void SetCharacterSpeed(float speed) {
        character.Execute(
            "p_speed_mult = " + speed + "; " +
            "run_speed = _base_run_speed * p_speed_mult; " +
            "true_max_speed = _base_true_max_speed * p_speed_mult;"
        );
    }
}
