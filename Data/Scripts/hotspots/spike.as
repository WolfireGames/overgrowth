//-----------------------------------------------------------------------------
//           Name: spike.as
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

int spike_count = 5;
int spiked_character_id = -1;
int spike_tip_hotspot_id = -1;
int spike_collidable_id = -1;
int no_grab_id = -1;
int armed = 0;
bool is_short_spike = false;
const bool k_super_spike = false;

void SetParameters() {
    is_short_spike = params.HasParam("Short");
}

void Init() {
    // Initialization if needed
}

void Reset() {
    spike_count = 5;
    spiked_character_id = -1;
    armed = 0;
    UpdateObjects();
}

void Dispose() {
    DeleteObjectIfExists(spike_collidable_id);
    DeleteObjectIfExists(spike_tip_hotspot_id);
    DeleteObjectIfExists(no_grab_id);
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "reset") {
        spike_count = 20;
    }
}

void ReceiveMessage(string msg) {
    if (k_super_spike) {
        return;
    }
    if (msg == "arm_spike") {
        SetArmed(1);
    } else if (msg == "disarm_spike" && spiked_character_id == -1) {
        SetArmed(0);
    }
}

void Update() {
    if (armed != 1) {
        return;
    }
    CheckForSpikeCollision();
}

void PreDraw(float curr_game_time) {
    UpdateObjects();
}

void SetArmed(int value) {
    if (value == armed) {
        return;
    }
    armed = value;
    UpdateObjects();
}

void UpdateObjects() {
    CreateSpikeCollidable();
    CreateSpikeTipHotspot();
    CreateNoGrabHotspot();
    UpdateTransforms();
    UpdateCollisionState();
}

void CreateSpikeCollidable() {
    if (spike_collidable_id != -1) {
        return;
    }
    string spike_path = is_short_spike
        ? "Data/Objects/Environment/camp/sharp_stick_short.xml"
        : "Data/Objects/Environment/camp/sharp_stick_long.xml";
    spike_collidable_id = CreateObject(spike_path, true);
    ReadObjectFromID(spike_collidable_id).SetEnabled(true);
}

void CreateSpikeTipHotspot() {
    if (spike_tip_hotspot_id != -1) {
        return;
    }
    spike_tip_hotspot_id = CreateObject("Data/Objects/Hotspots/spike_tip.xml", true);
    Object@ obj = ReadObjectFromID(spike_tip_hotspot_id);
    obj.SetEnabled(true);
    obj.GetScriptParams().SetInt("Parent", hotspot.GetID());
}

void CreateNoGrabHotspot() {
    if (no_grab_id != -1) {
        return;
    }
    no_grab_id = CreateObject("Data/Objects/Hotspots/no_grab.xml", true);
}

void UpdateTransforms() {
    Object@ hotspot_obj = ReadObjectFromID(hotspot.GetID());
    vec3 position = hotspot_obj.GetTranslation();
    quaternion rotation = hotspot_obj.GetRotation();
    vec3 scale = hotspot_obj.GetScale();

    UpdateSpikeTipTransform(position, rotation, scale);
    UpdateSpikeCollidableTransform(position, rotation, scale);
    UpdateNoGrabTransform(position, rotation, scale);
}

void UpdateSpikeTipTransform(const vec3& in position, const quaternion& in rotation, const vec3& in scale) {
    if (spike_tip_hotspot_id == -1) {
        return;
    }
    Object@ obj = ReadObjectFromID(spike_tip_hotspot_id);
    obj.SetRotation(rotation);
    obj.SetTranslation(position + rotation * vec3(0, scale.y * 2 + 0.2f, 0));
    obj.SetScale(vec3(0.2f));
}

void UpdateSpikeCollidableTransform(const vec3& in position, const quaternion& in rotation, const vec3& in scale) {
    if (spike_collidable_id == -1 || armed == 1) {
        return;
    }
    Object@ obj = ReadObjectFromID(spike_collidable_id);
    float y_scale = scale.y * 2.0f * (is_short_spike ? 0.92f : 0.85f);
    obj.SetRotation(rotation);
    obj.SetTranslation(position + rotation * vec3(0.03f, 0, 0.0f));
    obj.SetScale(vec3(1, y_scale, 1));
}

void UpdateNoGrabTransform(const vec3& in position, const quaternion& in rotation, const vec3& in scale) {
    if (no_grab_id == -1) {
        return;
    }
    Object@ obj = ReadObjectFromID(no_grab_id);
    Object@ reference_obj = ReadObjectFromID(spike_collidable_id);
    obj.SetRotation(rotation);
    obj.SetTranslation(position);
    obj.SetScale(vec3(1, scale.y * 0.45f, 1) * reference_obj.GetBoundingBox());
}

void UpdateCollisionState() {
    if (spike_collidable_id == -1) {
        return;
    }
    ReadObjectFromID(spike_collidable_id).SetCollisionEnabled(armed == 0);
}

void CheckForSpikeCollision() {
    Object@ obj = ReadObjectFromID(hotspot.GetID());
    vec3 start = obj.GetTranslation() - obj.GetRotation() * vec3(0, obj.GetScale().y * 2, 0);
    vec3 end = obj.GetTranslation() + obj.GetRotation() * vec3(0, obj.GetScale().y * 2, 0);
    vec3 direction = normalize(end - start);

    vec3 check_start = k_super_spike ? start : end - direction * 0.1f;
    vec3 check_end = end + direction * 0.05f;

    col.CheckRayCollisionCharacters(check_start, check_end);
    if (sphere_col.NumContacts() == 0) {
        return;
    }

    MovementObject@ character = ReadCharacterID(sphere_col.GetContact(0).id);
    if (!k_super_spike && dot(character.velocity, direction) >= 0.0f) {
        return;
    }

    if (spike_count > 0) {
        character.rigged_object().Stab(
            sphere_col.GetContact(0).position,
            direction,
            (spike_count == 4) ? 1 : 0,
            0
        );
        --spike_count;
    }

    if (spiked_character_id != -1) {
        return;
    }

    SpikeCharacter(character, start, end);
}

void SpikeCharacter(MovementObject@ character, const vec3& in start, const vec3& in end) {
    vec3 dir = normalize(start - end);
    float extend = 0.4f;

    CheckAndSpikeRagdoll(character, start + dir * extend, end - dir * extend, dir);
    CheckAndSpikeRagdoll(character, end - dir * extend, start + dir * extend, -dir);

    PlaySoundGroup("Data/Sounds/weapon_foley/cut/flesh_hit.xml", character.position);

    spiked_character_id = character.GetID();
    if (character.GetIntVar("knocked_out") != _dead) {
        character.Execute(
            "TakeBloodDamage(1.0f);"
            "Ragdoll(_RGDL_INJURED);"
            "injured_ragdoll_time = RangedRandomFloat(0.0, 12.0);"
            "death_hint = _hint_avoid_spikes;"
        );
    }
}

void CheckAndSpikeRagdoll(MovementObject@ character, const vec3& in start, const vec3& in end, const vec3& in dir) {
    col.CheckRayCollisionCharacters(start, end);
    for (int i = 0; i < sphere_col.NumContacts(); ++i) {
        int bone = sphere_col.GetContact(i).tri;
        character.rigged_object().SpikeRagdollPart(bone, start, end, sphere_col.GetContact(i).position);
    }
}

void DeleteObjectIfExists(int& inout object_id) {
    if (object_id != -1) {
        QueueDeleteObjectID(object_id);
        object_id = -1;
    }
}
