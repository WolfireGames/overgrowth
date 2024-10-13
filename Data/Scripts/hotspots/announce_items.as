//-----------------------------------------------------------------------------
//           Name: announce_items.as
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

array<int> contained_items;

void Init() {
    // No initialization needed
}

void SetParameters() {
    params.AddString("Hotspot ID", "ID for the announcing hotspot");
}

void HandleEventItem(string event, ItemObject@ obj) {
    if (event == "enter") {
        OnItemEnter(obj);
    } else if (event == "exit") {
        OnItemExit(obj);
    }
}

void OnItemEnter(ItemObject@ obj) {
    if (!IsItemContained(obj.GetID())) {
        contained_items.insertLast(obj.GetID());
    }
    AnnounceItemEvent("enter", obj);
    AnnounceContainedItems();
}

void OnItemExit(ItemObject@ obj) {
    RemoveItem(obj.GetID());
    AnnounceItemEvent("exit", obj);
    AnnounceContainedItems();
}

bool IsItemContained(int id) {
    for (uint i = 0; i < contained_items.length(); ++i) {
        if (contained_items[i] == id) {
            return true;
        }
    }
    return false;
}

void RemoveItem(int id) {
    for (uint i = 0; i < contained_items.length(); ++i) {
        if (contained_items[i] == id) {
            contained_items.removeAt(i);
            break;
        }
    }
}

void AnnounceItemEvent(string event, ItemObject@ obj) {
    level.SendMessage("hotspot_announce_items " + params.GetString("Hotspot ID") + " " + event + " " + obj.GetID());
}

void AnnounceContainedItems() {
    string message = "hotspot_announce_items " + params.GetString("Hotspot ID") + " inside_list";
    for (uint i = 0; i < contained_items.length(); ++i) {
        message += " " + contained_items[i];
    }
    level.SendMessage(message);
}
