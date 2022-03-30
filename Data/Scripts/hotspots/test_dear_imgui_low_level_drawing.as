//-----------------------------------------------------------------------------
//           Name: test_dear_imgui_low_level_drawing.as
//      Developer: Wolfire Games LLC
//    Script Type: Hotspot
//    Description:
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

TextureAssetRef testTexture1;
TextureAssetRef testTexture2;
string inputTextForClipboard;

void DrawEditor() {
    if(!testTexture1.IsValid()) {
        testTexture1 = LoadTexture("Data/Textures/ya.tga");
    }

    if(!testTexture2.IsValid()) {
        testTexture2 = LoadTexture("Data/Textures/water_foam.jpg");
    }

    ImGui_Begin("TEST DRAW STUFF");

    ImGui_Text("THIS IS A TEST");

    // --- Clipboard stuff. Unrelated to low level drawing, just happened to do it at the same time
    ImGui_Text(ImGui_GetClipboardText());
    ImGui_InputText("", inputTextForClipboard, 64);

    ImGui_SameLine();

    if(ImGui_SmallButton("Put text in clipboard")) {
        ImGui_SetClipboardText(inputTextForClipboard);
    }

    // --- Primitive drawing
    ImDrawList_AddLine(vec2(0.0, 0.0), vec2(1000.0, 1000.0), ImGui_GetColorU32(ImGuiCol_TitleBg, 0.5));
    ImDrawList_AddRect(vec2(200.0, 200.0), vec2(500.0, 500.0), ImGui_GetColorU32(ImGuiCol_TitleBg), 12.0, ImDrawCornerFlags_TopLeft | ImDrawCornerFlags_BotRight, 10.0);
    ImDrawList_AddRectFilled(vec2(200.0, 200.0), vec2(500.0, 500.0), ImGui_GetColorU32(ImGuiCol_TitleBg), 12.0, ImDrawCornerFlags_TopLeft | ImDrawCornerFlags_BotRight);
    ImDrawList_AddRectFilledMultiColor(
        vec2(200.0, 200.0), vec2(500.0, 500.0),
        ImGui_GetColorU32(ImGuiCol_TitleBg), ImGui_GetColorU32(1.0, 1.0, 1.0, 0.75), ImGui_GetColorU32(0.0, 1.0, 0.0, 0.25), ImGui_GetColorU32(1.0, 0.0, 0.0, 0.5));
    ImDrawList_AddQuad(vec2(150.0, 200.0), vec2(500.0, 150.0), vec2(400.0, 450.0), vec2(200.0, 550.0), ImGui_GetColorU32(ImGuiCol_TitleBg), 10.0);
    ImDrawList_AddQuadFilled(vec2(150.0, 200.0), vec2(500.0, 150.0), vec2(400.0, 450.0), vec2(200.0, 550.0), ImGui_GetColorU32(1.0, 0.0, 1.0));
    ImDrawList_AddTriangle(vec2(150.0, 200.0), vec2(500.0, 150.0), vec2(400.0, 450.0), ImGui_GetColorU32(ImGuiCol_TitleBg), 10.0);
    ImDrawList_AddTriangleFilled(vec2(150.0, 200.0), vec2(500.0, 150.0), vec2(400.0, 450.0), ImGui_GetColorU32(ImGuiCol_TitleBg));
    ImDrawList_AddCircle(vec2(300.0, 300.0), 200.0, ImGui_GetColorU32(ImGuiCol_TitleBg), 48, 10.0);
    ImDrawList_AddCircleFilled(vec2(300.0, 300.0), 200.0, ImGui_GetColorU32(ImGuiCol_TitleBg), 48);
    ImDrawList_AddText(vec2(300.0, 300.0), ImGui_GetColorU32(1.0, 0.0, 1.0, 1.0), "THIS IS A TEST!!!!");
    ImDrawList_AddImage(
        testTexture1,
        vec2(400.0, 300.0), vec2(500.0, 400.0), vec2(0.0), vec2(1.0),
        ImGui_GetColorU32(1.0, 0.0, 0.0, 1.0));
    ImDrawList_AddImageQuad(
        testTexture1,
        vec2(150.0, 200.0), vec2(500.0, 150.0), vec2(400.0, 450.0), vec2(200.0, 550.0),
        vec2(0, 0), vec2(1, 0), vec2(1, 1), vec2(0, 1),
        ImGui_GetColorU32(0.0, 1.0, 0.0, 1.0));
    ImDrawList_AddImageRounded(
        testTexture2,
        vec2(500.0, 300.0), vec2(600.0, 400.0), vec2(0.0), vec2(1.0),
        ImGui_GetColorU32(0.0, 1.0, 1.0, 1.0),
        32.0,
        ImDrawCornerFlags_TopLeft | ImDrawCornerFlags_BotRight);
    ImDrawList_AddBezierCurve(vec2(300.0, 300.0), vec2(300.0, 100.0), vec2(500.0, 700.0), vec2(500.0, 500.0), ImGui_GetColorU32(ImGuiCol_TitleBg), 10.0, 48);

    // --- Stateful path drawing
    ImDrawList_PathClear();
    ImDrawList_PathLineTo(vec2(300.0, 300.0));
    ImDrawList_PathLineTo(vec2(350.0, 500.0));
    ImDrawList_PathLineTo(vec2(500.0, 700.0));
    ImDrawList_PathFillConvex(ImGui_GetColorU32(ImGuiCol_TitleBg));

    ImDrawList_PathClear();
    ImDrawList_PathLineTo(vec2(500.0, 300.0));
    ImDrawList_PathLineTo(vec2(550.0, 500.0));
    ImDrawList_PathLineTo(vec2(700.0, 700.0));
    ImDrawList_PathStroke(ImGui_GetColorU32(ImGuiCol_TitleBg), true, 10.0);

    ImDrawList_PathClear();
    ImDrawList_PathLineToMergeDuplicate(vec2(500.0, 300.0));
    ImDrawList_PathLineToMergeDuplicate(vec2(550.0, 500.0));
    ImDrawList_PathLineToMergeDuplicate(vec2(550.0, 500.0));
    ImDrawList_PathLineToMergeDuplicate(vec2(550.0, 500.0));
    ImDrawList_PathLineToMergeDuplicate(vec2(700.0, 700.0));
    ImDrawList_PathStroke(ImGui_GetColorU32(ImGuiCol_TitleBg), true, 10.0);

    ImDrawList_PathClear();
    ImDrawList_PathLineTo(vec2(300.0, 300.0));
    ImDrawList_PathArcTo(vec2(500.0, 500.0), 50.0, 20.0, 70.0, 20);
    ImDrawList_PathStroke(ImGui_GetColorU32(1.0, 1.0, 0.0, 1.0), true, 10.0);

    ImDrawList_PathClear();
    ImDrawList_PathLineTo(vec2(300.0, 300.0));
    ImDrawList_PathArcToFast(vec2(500.0, 500.0), 50.0, 8, 12);
    ImDrawList_PathStroke(ImGui_GetColorU32(0.0, 1.0, 1.0, 1.0), true, 10.0);

    ImDrawList_PathClear();
    ImDrawList_PathLineTo(vec2(300.0, 300.0));
    ImDrawList_PathBezierCurveTo(vec2(300.0, 100.0), vec2(500.0, 700.0), vec2(500.0, 500.0), 48);
    ImDrawList_PathStroke(ImGui_GetColorU32(ImGuiCol_TitleBg), false, 10.0);

    ImDrawList_PathClear();
    ImDrawList_PathLineTo(vec2(300.0, 300.0));
    ImDrawList_PathRect(vec2(200.0, 200.0), vec2(500.0, 500.0), 12.0, ImDrawCornerFlags_TopLeft | ImDrawCornerFlags_BotRight);
    ImDrawList_PathStroke(ImGui_GetColorU32(ImGuiCol_TitleBg), true, 10.0);

    ImGui_End();
}
