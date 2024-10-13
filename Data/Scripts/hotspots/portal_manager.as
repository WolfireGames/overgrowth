//-----------------------------------------------------------------------------
//           Name: portal_manager.as
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

array<int> portal_ids;

void SetParameters() {
    params.AddString("Portals", "");
}

void Update() {
    UpdatePortalIDs();
    if (GetInputPressed(0, "k") && portal_ids.length() > 0) {
        TeleportToRandomPortal();
    }
}

void UpdatePortalIDs() {
    portal_ids.resize(0);
    if (!params.HasParam("Portals")) {
        return;
    }
    string portals_str = params.GetString("Portals");
    TokenIterator token_iter;
    token_iter.Init();
    while (token_iter.FindNextToken(portals_str)) {
        int target_id = atoi(token_iter.GetToken(portals_str));
        if (ObjectExists(target_id) && ReadObjectFromID(target_id).GetType() == _hotspot_object) {
            portal_ids.push_back(target_id);
        }
    }
}

void TeleportToRandomPortal() {
    int which = rand() % portal_ids.length();
    Object@ portal_obj = ReadObjectFromID(portal_ids[which]);
    ScriptParams@ portal_params = portal_obj.GetScriptParams();
    if (!portal_params.HasParam("spawn_point")) {
        return;
    }
    int spawn_point_id = portal_params.GetInt("spawn_point");
    if (!ObjectExists(spawn_point_id)) {
        return;
    }
    Object@ spawn_point_obj = ReadObjectFromID(spawn_point_id);
    TeleportCharactersToSpawnPoint(spawn_point_obj);
}

void TeleportCharactersToSpawnPoint(Object@ spawn_point_obj) {
    int num_chars = GetNumCharacters();
    for (int i = 0; i < num_chars; ++i) {
        MovementObject@ char = ReadCharacter(i);
        char.position = spawn_point_obj.GetTranslation();
        char.SetRotationFromFacing(spawn_point_obj.GetRotation() * vec3(0, 0, 1));
        char.velocity = vec3(0.0f);
        char.Execute("Reset(); PostReset(); ResetSecondaryAnimation();");
    }
}
