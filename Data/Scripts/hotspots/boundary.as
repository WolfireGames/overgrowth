//-----------------------------------------------------------------------------
//           Name: boundary.as
//      Developer: Wolfire Games LLC
//    Script Type: Hotspot
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

const int _ragdoll_state = 4;
const float _push_force_mult = 0.5f;

void Init() {
    // No initialization needed
}

void SetParameters() {
    // No parameters to set
}

void Reset() {
    // No reset needed
}

void Update() {
    if (EditorModeActive()) {
        ShowPlaceholder();
    } else if (ReadObjectFromID(hotspot.GetID()).GetEnabled()) {
        PushCollidingCharacters();
    }
}

void ShowPlaceholder() {
    // Placeholder implementation if needed
}

void PushCollidingCharacters() {
    array<int> charIDs;
    level.GetCollidingObjects(hotspot.GetID(), charIDs);

    for (uint i = 0; i < charIDs.size(); ++i) {
        if (ReadObjectFromID(charIDs[i]).GetType() == _movement_object) {
            MovementObject@ mo = ReadCharacterID(charIDs[i]);
            if (!mo.static_char) {
                ApplyPushForce(mo);
            }
        }
    }
}

void ApplyPushForce(MovementObject@ mo) {
    vec3 direction = ReadObjectFromID(hotspot.GetID()).GetRotation() * vec3(0, 0, -1);
    vec3 push_force = -direction * _push_force_mult;

    if (length_squared(push_force) > 0.0f) {
        mo.velocity += push_force;
        if (mo.GetIntVar("state") == _ragdoll_state) {
            mo.rigged_object().ApplyForceToRagdoll(push_force * 500.0f, mo.rigged_object().skeleton().GetCenterOfMass());
        }
    }
}
