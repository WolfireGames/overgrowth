//-----------------------------------------------------------------------------
//           Name: test_dear_imgui_low_level_drawing.as
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

TextureAssetRef test_texture1;
TextureAssetRef test_texture2;
string clipboard_input_text;

void Init() {
    // Initialization if needed
}

void Dispose() {
    // Cleanup if needed
}

void DrawEditor() {
    LoadTextures();
    ImGui_Begin("Test Draw Stuff");
    ImGui_Text("This is a test");
    HandleClipboard();
    DrawPrimitives();
    DrawStatefulPaths();
    ImGui_End();
}

void LoadTextures() {
    if (!test_texture1.IsValid()) {
        test_texture1 = LoadTexture("Data/Textures/ya.tga");
    }
    if (!test_texture2.IsValid()) {
        test_texture2 = LoadTexture("Data/Textures/water_foam.jpg");
    }
}

void HandleClipboard() {
    ImGui_Text(ImGui_GetClipboardText());
    ImGui_InputText("", clipboard_input_text, 64);
    ImGui_SameLine();
    if (ImGui_SmallButton("Put text in clipboard")) {
        ImGui_SetClipboardText(clipboard_input_text);
    }
}

void DrawPrimitives() {
    // Draw various primitive shapes and images
    ImDrawList_AddLine(vec2(0.0f, 0.0f), vec2(1000.0f, 1000.0f), ImGui_GetColorU32(ImGuiCol_TitleBg, 0.5f));
    ImDrawList_AddRect(vec2(200.0f, 200.0f), vec2(500.0f, 500.0f), ImGui_GetColorU32(ImGuiCol_TitleBg), 12.0f, ImDrawCornerFlags_TopLeft | ImDrawCornerFlags_BotRight, 10.0f);
    ImDrawList_AddRectFilled(vec2(200.0f, 200.0f), vec2(500.0f, 500.0f), ImGui_GetColorU32(ImGuiCol_TitleBg), 12.0f, ImDrawCornerFlags_TopLeft | ImDrawCornerFlags_BotRight);
    ImDrawList_AddRectFilledMultiColor(
        vec2(200.0f, 200.0f), vec2(500.0f, 500.0f),
        ImGui_GetColorU32(ImGuiCol_TitleBg),
        ImGui_GetColorU32(1.0f, 1.0f, 1.0f, 0.75f),
        ImGui_GetColorU32(0.0f, 1.0f, 0.0f, 0.25f),
        ImGui_GetColorU32(1.0f, 0.0f, 0.0f, 0.5f)
    );
    ImDrawList_AddQuad(vec2(150.0f, 200.0f), vec2(500.0f, 150.0f), vec2(400.0f, 450.0f), vec2(200.0f, 550.0f), ImGui_GetColorU32(ImGuiCol_TitleBg), 10.0f);
    ImDrawList_AddQuadFilled(vec2(150.0f, 200.0f), vec2(500.0f, 150.0f), vec2(400.0f, 450.0f), vec2(200.0f, 550.0f), ImGui_GetColorU32(1.0f, 0.0f, 1.0f));
    ImDrawList_AddTriangle(vec2(150.0f, 200.0f), vec2(500.0f, 150.0f), vec2(400.0f, 450.0f), ImGui_GetColorU32(ImGuiCol_TitleBg), 10.0f);
    ImDrawList_AddTriangleFilled(vec2(150.0f, 200.0f), vec2(500.0f, 150.0f), vec2(400.0f, 450.0f), ImGui_GetColorU32(ImGuiCol_TitleBg));
    ImDrawList_AddCircle(vec2(300.0f, 300.0f), 200.0f, ImGui_GetColorU32(ImGuiCol_TitleBg), 48, 10.0f);
    ImDrawList_AddCircleFilled(vec2(300.0f, 300.0f), 200.0f, ImGui_GetColorU32(ImGuiCol_TitleBg), 48);
    ImDrawList_AddText(vec2(300.0f, 300.0f), ImGui_GetColorU32(1.0f, 0.0f, 1.0f, 1.0f), "THIS IS A TEST!!!!");
    ImDrawList_AddImage(
        test_texture1,
        vec2(400.0f, 300.0f),
        vec2(500.0f, 400.0f),
        vec2(0.0f),
        vec2(1.0f),
        ImGui_GetColorU32(1.0f, 0.0f, 0.0f, 1.0f)
    );
    ImDrawList_AddImageQuad(
        test_texture1,
        vec2(150.0f, 200.0f),
        vec2(500.0f, 150.0f),
        vec2(400.0f, 450.0f),
        vec2(200.0f, 550.0f),
        vec2(0.0f, 0.0f),
        vec2(1.0f, 0.0f),
        vec2(1.0f, 1.0f),
        vec2(0.0f, 1.0f),
        ImGui_GetColorU32(0.0f, 1.0f, 0.0f, 1.0f)
    );
    ImDrawList_AddImageRounded(
        test_texture2,
        vec2(500.0f, 300.0f),
        vec2(600.0f, 400.0f),
        vec2(0.0f),
        vec2(1.0f),
        ImGui_GetColorU32(0.0f, 1.0f, 1.0f, 1.0f),
        32.0f,
        ImDrawCornerFlags_TopLeft | ImDrawCornerFlags_BotRight
    );
    ImDrawList_AddBezierCurve(
        vec2(300.0f, 300.0f),
        vec2(300.0f, 100.0f),
        vec2(500.0f, 700.0f),
        vec2(500.0f, 500.0f),
        ImGui_GetColorU32(ImGuiCol_TitleBg),
        10.0f,
        48
    );
}

void DrawStatefulPaths() {
    // Draw shapes using the path API
    ImDrawList_PathClear();
    ImDrawList_PathLineTo(vec2(300.0f, 300.0f));
    ImDrawList_PathLineTo(vec2(350.0f, 500.0f));
    ImDrawList_PathLineTo(vec2(500.0f, 700.0f));
    ImDrawList_PathFillConvex(ImGui_GetColorU32(ImGuiCol_TitleBg));

    ImDrawList_PathClear();
    ImDrawList_PathLineTo(vec2(500.0f, 300.0f));
    ImDrawList_PathLineTo(vec2(550.0f, 500.0f));
    ImDrawList_PathLineTo(vec2(700.0f, 700.0f));
    ImDrawList_PathStroke(ImGui_GetColorU32(ImGuiCol_TitleBg), true, 10.0f);

    ImDrawList_PathClear();
    ImDrawList_PathLineToMergeDuplicate(vec2(500.0f, 300.0f));
    ImDrawList_PathLineToMergeDuplicate(vec2(550.0f, 500.0f));
    ImDrawList_PathLineToMergeDuplicate(vec2(700.0f, 700.0f));
    ImDrawList_PathStroke(ImGui_GetColorU32(ImGuiCol_TitleBg), true, 10.0f);

    ImDrawList_PathClear();
    ImDrawList_PathLineTo(vec2(300.0f, 300.0f));
    ImDrawList_PathArcTo(vec2(500.0f, 500.0f), 50.0f, 20.0f, 70.0f, 20);
    ImDrawList_PathStroke(ImGui_GetColorU32(1.0f, 1.0f, 0.0f, 1.0f), true, 10.0f);

    ImDrawList_PathClear();
    ImDrawList_PathLineTo(vec2(300.0f, 300.0f));
    ImDrawList_PathArcToFast(vec2(500.0f, 500.0f), 50.0f, 8, 12);
    ImDrawList_PathStroke(ImGui_GetColorU32(0.0f, 1.0f, 1.0f, 1.0f), true, 10.0f);

    ImDrawList_PathClear();
    ImDrawList_PathLineTo(vec2(300.0f, 300.0f));
    ImDrawList_PathBezierCurveTo(vec2(300.0f, 100.0f), vec2(500.0f, 700.0f), vec2(500.0f, 500.0f), 48);
    ImDrawList_PathStroke(ImGui_GetColorU32(ImGuiCol_TitleBg), false, 10.0f);

    ImDrawList_PathClear();
    ImDrawList_PathLineTo(vec2(300.0f, 300.0f));
    ImDrawList_PathRect(vec2(200.0f, 200.0f), vec2(500.0f, 500.0f), 12.0f, ImDrawCornerFlags_TopLeft | ImDrawCornerFlags_BotRight);
    ImDrawList_PathStroke(ImGui_GetColorU32(ImGuiCol_TitleBg), true, 10.0f);
}
