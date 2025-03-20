//-----------------------------------------------------------------------------
//           Name: particle_occluder.as
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

int occluder_decal_id = -1;

void Update() {
    if (occluder_decal_id == -1) {
        occluder_decal_id = CreateObject("Data/Objects/Decals/water_fog.xml", true);
    }
    UpdateOccluderDecal();
}

void UpdateOccluderDecal() {
    if (!ObjectExists(occluder_decal_id)) {
        return;
    }
    Object@ occluder_decal = ReadObjectFromID(occluder_decal_id);
    Object@ obj = ReadObjectFromID(hotspot.GetID());
    occluder_decal.SetTranslation(obj.GetTranslation());
    occluder_decal.SetRotation(obj.GetRotation());
    occluder_decal.SetScale(obj.GetScale() * 4.0f);
}

void Dispose() {
    if (occluder_decal_id != -1) {
        QueueDeleteObjectID(occluder_decal_id);
        occluder_decal_id = -1;
    }
}

void Draw() {
    if (EditorModeActive()) {
        Object@ obj = ReadObjectFromID(hotspot.GetID());
        DebugDrawBillboard(
            "Data/UI/spawner/thumbs/Hotspot/emitter_icon.png",
            obj.GetTranslation() + obj.GetScale().y * vec3(0.0f, 0.5f, 0.0f),
            2.0f,
            vec4(1.0f),
            _delete_on_draw);
    }
}
