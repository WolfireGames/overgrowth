//-----------------------------------------------------------------------------
//           Name: collectable_target.as
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

int collectables_needed;
int collectables_contained = 0;
bool condition_satisfied = false;

void Init() {
    // No initialization needed
}

void SetParameters() {
    params.AddString("Collectables needed", "1");
    collectables_needed = max(1, params.GetInt("Collectables needed"));
}

void HandleEventItem(string event, ItemObject@ obj) {
    if (event == "enter") {
        OnEnterItem(obj);
    } else if (event == "exit") {
        OnExitItem(obj);
    }
}

void OnEnterItem(ItemObject@ obj) {
    if (obj.GetType() != _collectable) {
        return;
    }
    ++collectables_contained;
    condition_satisfied = IsConditionSatisfied();
}

void OnExitItem(ItemObject@ obj) {
    if (obj.GetType() != _collectable) {
        return;
    }
    collectables_contained = max(0, collectables_contained - 1);
    condition_satisfied = IsConditionSatisfied();
}

bool IsConditionSatisfied() {
    return collectables_contained >= collectables_needed;
}
