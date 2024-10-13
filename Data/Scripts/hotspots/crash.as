//-----------------------------------------------------------------------------
//           Name: crash.as
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

bool add_errors = false;
float error_timer = 0.0f;
const float error_threshold = 0.01f;
int counter = 0;
const int number_of_errors = 250;
const int font_size = 200;

void Init() {
    // No initialization needed
}

void SetParameters() {
    // No parameters to set
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter") {
        OnEnter(mo);
    }
}

void OnEnter(MovementObject@ mo) {
    if (mo.controlled) {
        return;
    }
    add_errors = true;
    level.Execute("has_gui = true;");
}

void Update() {
    if (!add_errors) {
        return;
    }
    error_timer += time_step;
    if (error_timer <= error_threshold) {
        return;
    }
    error_timer = 0.0f;
    counter++;
    PlayRandomErrorSound();
    DisplayErrorMessage();

    if (counter == number_of_errors) {
        TriggerCrash();
    }
}

void PlayRandomErrorSound() {
    if (rand() % 30 == 0) {
        PlaySound("Data/Sounds/voice/kill_intent_1.wav");
    } else {
        PlaySound("Data/Sounds/FistImpact_1.wav");
    }
}

void DisplayErrorMessage() {
    string command = "FontSetup error_font(\"arial\", " + font_size + ", HexColor(\"#ff0000\"), true);" +
                     "IMText text(\"ERROR\", error_font);" +
                     "imGUI.getMain().addFloatingElement(text, \"text" + counter + "\", vec2(" +
                     RangedRandomFloat(-font_size, GetScreenWidth() + font_size) + ", " +
                     RangedRandomFloat(-font_size, GetScreenHeight() + font_size) + "), 1);";
    level.Execute(command);
}

void TriggerCrash() {
    DisplayError("Therium-2", "F KEY DETECTED. CRASH INITIATED.");
    MovementObject@ mo = ReadCharacter(0);
    mo.GetBoolVar("doesn'texist"); // Intentional crash
}
