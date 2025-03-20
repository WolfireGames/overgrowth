//-----------------------------------------------------------------------------
//           Name: respawn_at_checkpoint.as
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

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter") {
        OnEnter(mo);
    }
}

void OnEnter(MovementObject@ mo) {
    int checkpoint_id = FindLatestCheckpoint();
    if (checkpoint_id == -1) {
        return;
    }
    RespawnAtCheckpoint(mo, checkpoint_id);
}

int FindLatestCheckpoint() {
    float latest_time = -1.0f;
    int best_checkpoint_id = -1;
    array<int>@ hotspot_ids = GetObjectIDsType(_hotspot_object);
    for (uint i = 0; i < hotspot_ids.length(); ++i) {
        Object@ obj = ReadObjectFromID(hotspot_ids[i]);
        ScriptParams@ obj_params = obj.GetScriptParams();
        if (obj_params.HasParam("LastEnteredTime")) {
            float curr_time = obj_params.GetFloat("LastEnteredTime");
            if (curr_time > latest_time) {
                latest_time = curr_time;
                best_checkpoint_id = hotspot_ids[i];
            }
        }
    }
    return best_checkpoint_id;
}

void RespawnAtCheckpoint(MovementObject@ mo, int checkpoint_id) {
    Object@ checkpoint_obj = ReadObjectFromID(checkpoint_id);
    mo.position = checkpoint_obj.GetTranslation();
    mo.velocity = vec3(0.0f);
}
