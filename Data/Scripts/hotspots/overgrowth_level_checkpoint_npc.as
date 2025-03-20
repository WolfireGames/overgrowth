//-----------------------------------------------------------------------------
//           Name: overgrowth_level_checkpoint_npc.as
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

void SetParameters() {
    params.AddInt("level_hotspot_id", -1);
    params.AddInt("checkpoint_id", -1);
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter") {
        OnEnter(mo);
    }
}

void OnEnter(MovementObject@ mo) {
    if (mo.controlled || !params.HasParam("level_hotspot_id") || !params.HasParam("checkpoint_id")) {
        return;
    }
    int level_hotspot_id = params.GetInt("level_hotspot_id");
    if (!ObjectExists(level_hotspot_id)) {
        return;
    }
    Object@ obj = ReadObjectFromID(level_hotspot_id);
    int checkpoint_id = params.GetInt("checkpoint_id");
    obj.ReceiveScriptMessage("player_entered_checkpoint " + checkpoint_id);
}
