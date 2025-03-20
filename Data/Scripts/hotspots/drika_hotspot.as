//-----------------------------------------------------------------------------
//           Name: drika_hotspot.as
//      Developer:
//         Author: Gyrth, Fason7
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

#include "hotspots/drika_element.as"
#include "hotspots/drika_wait.as"
#include "hotspots/drika_wait_level_message.as"
#include "hotspots/drika_set_enabled.as"
#include "hotspots/drika_set_character.as"
#include "hotspots/drika_create_particle.as"
#include "hotspots/drika_play_sound.as"

bool editor_open = false;
bool script_finished = false;
int current_line = 0;
array<DrikaElement@> drika_elements;
array<int> drika_indexes;
bool post_init_done = false;

vec4 edit_outline_color = vec4(0.5, 0.5, 0.5, 1.0);
vec4 background_color = vec4(0.25, 0.25, 0.25, 0.98);
vec4 titlebar_color = vec4(0.15, 0.15, 0.15, 0.98);
vec4 item_hovered = vec4(0.2, 0.2, 0.2, 0.98);
vec4 item_clicked = vec4(0.1, 0.1, 0.1, 0.98);
vec4 text_color = vec4(0.7, 0.7, 0.7, 1.0);

TextureAssetRef delete_icon = LoadTexture("Data/UI/ribbon/images/icons/color/Delete.png", TextureLoadFlags_NoMipmap | TextureLoadFlags_NoConvert | TextureLoadFlags_NoReduce);

void Init() {
    level.ReceiveLevelEvents(hotspot.GetID());
}

void Dispose() {
    level.StopReceivingLevelEvents(hotspot.GetID());
}

void SetParameters() {
    params.AddString("Script Data", "");
}

void Update() {
    if (!post_init_done) {
        InterpretData();
        post_init_done = true;
        return;
    }
    if (!script_finished && !EditorModeActive()) {
        if (GetCurrentElement().Trigger()) {
            if (current_line == int(drika_indexes.size() - 1)) {
                script_finished = true;
            }
            current_line += 1;
        }
    }
}

void InterpretData() {
    array<string> lines = params.GetString("Script Data").split("\n");
    for (uint index = 0; index < lines.size(); index++) {
        array<string> line_elements = lines[index].split(" ");
        DrikaElement@ element = CreateElementFromData(index, line_elements);
        if (element !is null) {
            drika_elements.insertLast(element);
            drika_indexes.insertLast(index);
        }
    }
    ReorderElements();
}

DrikaElement@ CreateElementFromData(int index, array<string>@ line_elements) {
    string command = line_elements[0];
    if (command == "set_character") {
        int character_id = atoi(line_elements[1]);
        return DrikaSetCharacter(index, character_id, line_elements[2]);
    } else if (command == "set_enabled") {
        int object_id = atoi(line_elements[1]);
        bool enabled = line_elements[2] == "true";
        return DrikaSetEnabled(index, object_id, enabled);
    } else if (command == "wait") {
        int duration = atoi(line_elements[1]);
        return DrikaWait(index, duration);
    } else if (command == "wait_level_message") {
        return DrikaWaitLevelMessage(index, line_elements[1]);
    } else if (command == "create_particle") {
        int object_id = atoi(line_elements[1]);
        int amount = atoi(line_elements[2]);
        return DrikaCreateParticle(index, object_id, amount, line_elements[3]);
    } else if (command == "play_sound") {
        int object_id = atoi(line_elements[1]);
        return DrikaPlaySound(index, object_id, line_elements[2]);
    }
    return null;
}

void DrawEditor() {
    if (!editor_open) {
        return;
    }

    SetupImGuiStyles();
    ImGui_SetNextWindowSize(vec2(600.0f, 400.0f), ImGuiSetCond_FirstUseEver);
    ImGui_SetNextWindowPos(vec2(100.0f, 100.0f), ImGuiSetCond_FirstUseEver);
    ImGui_Begin("Drika Hotspot", editor_open, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

    if (ImGui_BeginMenuBar()) {
        DrawMenuBar();
        ImGui_EndMenuBar();
    }

    HandleKeyboardNavigation();

    DrawElementsList();

    ImGui_End();
    ImGui_PopStyleVar(); // End ImGuiStyleVar_WindowMinSize
    ImGui_PopStyleColor(17);

    if (reorded && !ImGui_IsMouseDragging(0)) {
        reorded = false;
        ReorderElements();
    }
}

void SetupImGuiStyles() {
    ImGui_PushStyleColor(ImGuiCol_WindowBg, background_color);
    ImGui_PushStyleColor(ImGuiCol_PopupBg, background_color);
    ImGui_PushStyleColor(ImGuiCol_TitleBgActive, titlebar_color);
    ImGui_PushStyleColor(ImGuiCol_TitleBg, item_hovered);
    ImGui_PushStyleColor(ImGuiCol_MenuBarBg, titlebar_color);
    ImGui_PushStyleColor(ImGuiCol_Text, text_color);
    ImGui_PushStyleColor(ImGuiCol_Header, titlebar_color);
    ImGui_PushStyleColor(ImGuiCol_HeaderHovered, item_hovered);
    ImGui_PushStyleColor(ImGuiCol_HeaderActive, item_clicked);
    ImGui_PushStyleColor(ImGuiCol_ScrollbarBg, background_color);
    ImGui_PushStyleColor(ImGuiCol_ScrollbarGrab, titlebar_color);
    ImGui_PushStyleColor(ImGuiCol_ScrollbarGrabHovered, item_hovered);
    ImGui_PushStyleColor(ImGuiCol_ScrollbarGrabActive, item_clicked);
    ImGui_PushStyleColor(ImGuiCol_CloseButton, background_color);
    ImGui_PushStyleColor(ImGuiCol_Button, titlebar_color);
    ImGui_PushStyleColor(ImGuiCol_ButtonHovered, item_hovered);
    ImGui_PushStyleColor(ImGuiCol_ButtonActive, item_clicked);
    ImGui_PushStyleVar(ImGuiStyleVar_WindowMinSize, vec2(300, 300));
}

void DrawMenuBar() {
    if (ImGui_BeginMenu("Add")) {
        if (ImGui_MenuItem("Set Character")) {
            InsertElement(DrikaSetCharacter(current_line, 0, "Data/Characters/guard.xml"));
        }
        if (ImGui_MenuItem("Set Enabled")) {
            InsertElement(DrikaSetEnabled(current_line, 0, true));
        }
        if (ImGui_MenuItem("Wait LevelMessage")) {
            InsertElement(DrikaWaitLevelMessage(current_line, "continue_drika_hotspot"));
        }
        if (ImGui_MenuItem("Wait")) {
            InsertElement(DrikaWait(current_line, 1000));
        }
        if (ImGui_MenuItem("CreateParticle")) {
            InsertElement(DrikaCreateParticle(current_line, 0, 5, "Data/Particles/bloodmist.xml"));
        }
        if (ImGui_MenuItem("PlaySound")) {
            InsertElement(DrikaPlaySound(current_line, 0, "Data/Sounds/weapon_foley/impact/weapon_knife_hit_neck_2.wav"));
        }
        ImGui_EndMenu();
    }
    if (ImGui_ImageButton(delete_icon, vec2(10), vec2(0), vec2(1), 5, vec4(0))) {
        DeleteCurrentElement();
    }
}

void HandleKeyboardNavigation() {
    if (ImGui_IsKeyPressed(ImGui_GetKeyIndex(ImGuiKey_UpArrow))) {
        if (current_line > 0) {
            current_line -= 1;
            update_scroll = true;
        }
    } else if (ImGui_IsKeyPressed(ImGui_GetKeyIndex(ImGuiKey_DownArrow))) {
        if (current_line < int(drika_elements.size() - 1)) {
            current_line += 1;
            update_scroll = true;
        }
    }
}

void DrawElementsList() {
    for (uint i = 0; i < drika_indexes.size(); ++i) {
        int item_no = drika_indexes[i];
        string line_number = drika_elements[item_no].index + ". ";
        vec4 text_color = drika_elements[item_no].display_color / 255.0;

        ImGui_PushStyleColor(ImGuiCol_Text, text_color);
        if (ImGui_Selectable(line_number + drika_elements[item_no].GetDisplayString(), current_line == int(i), ImGuiSelectableFlags_AllowDoubleClick)) {
            if (ImGui_IsMouseDoubleClicked(0) && drika_elements[item_no].has_settings) {
                ImGui_OpenPopup("Edit");
            } else {
                current_line = int(i);
            }
        }
        HandleDragAndDrop(i, item_no);
        ImGui_PopStyleColor();
    }
}

void HandleDragAndDrop(uint i, int item_no) {
    if (ImGui_IsItemActive() && !ImGui_IsItemHovered()) {
        float drag_dy = ImGui_GetMouseDragDelta(0).y;
        if (drag_dy < 0.0 && i > 0) {
            SwapElements(i, i - 1);
            reorded = true;
            ImGui_ResetMouseDragDelta();
        } else if (drag_dy > 0.0 && i < drika_elements.size() - 1) {
            SwapElements(i, i + 1);
            reorded = true;
            ImGui_ResetMouseDragDelta();
        }
    }
}

void SwapElements(uint a, uint b) {
    int temp = drika_indexes[a];
    drika_indexes[a] = drika_indexes[b];
    drika_indexes[b] = temp;
}

void InsertElement(DrikaElement @element) {
    drika_elements.insertLast(element);
    drika_indexes.insertAt(current_line + 1, drika_elements.size() - 1);
    current_line += 1;
    ReorderElements();
}

void DeleteCurrentElement() {
    if (drika_elements.size() == 0) {
        return;
    }
    drika_elements.removeAt(drika_indexes[current_line]);
    drika_indexes.removeAt(current_line);
    if (current_line >= drika_indexes.size() && current_line > 0) {
        current_line -= 1;
    }
    ReorderElements();
}

DrikaElement @GetCurrentElement() {
    return drika_elements[drika_indexes[current_line]];
}

void ReorderElements() {
    for (uint i = 0; i < drika_indexes.size(); ++i) {
        drika_elements[drika_indexes[i]].SetIndex(i);
    }
    Save();
}

void ReceiveMessage(string msg) {
    TokenIterator token_iter;
    token_iter.Init();
    if (!token_iter.FindNextToken(msg)) {
        return;
    }
    string token = token_iter.GetToken(msg);
    if (token == "level_event" && !script_finished && drika_indexes.size() > 0) {
        token_iter.FindNextToken(msg);
        string message = token_iter.GetToken(msg);
        GetCurrentElement().ReceiveMessage(message);
    }
}

void Reset() {
    current_line = 0;
    script_finished = false;
    for (uint i = 0; i < drika_elements.size(); ++i) {
        drika_elements[i].Reset();
    }
}

void Save() {
    string data;
    for (uint i = 0; i < drika_indexes.size(); ++i) {
        data += drika_elements[drika_indexes[i]].GetSaveString();
        if (i != drika_indexes.size() - 1) {
            data += "\n";
        }
    }
    params.SetString("Script Data", data);
}
