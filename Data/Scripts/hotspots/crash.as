//-----------------------------------------------------------------------------
//           Name: crash.as
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

void Init() {
}

bool add_errors = false;

void SetParameters() {
}

void HandleEvent(string event, MovementObject @mo){
    if(event == "enter"){
        OnEnter(mo);
    } else if(event == "exit"){
        OnExit(mo);
    }
}

void OnEnter(MovementObject @mo) {
    if(!mo.controlled){
        add_errors = true;
        level.Execute("has_gui = true;");
    }
}

float error_timer = 0.0f;
float error_thresshold = 0.01f;
int counter = 0;
int number_of_errors = 250;
int font_size = 200;

void Update(){
    if(add_errors){
        error_timer += time_step;
        if(error_timer > error_thresshold){
            error_timer = 0.0f;
            counter++;
            if(rand() % 30 == 0){
                PlaySound("Data/Sounds/voice/kill_intent_1.wav");
            }else{
                PlaySound("Data/Sounds/FistImpact_1.wav");
            }
            level.Execute(  "FontSetup error_font(\"arial\", " + (font_size) + " , HexColor(\"#ff0000\"), true);" +
                            "IMText text(\"ERROR\", error_font);" +
                            /*"text.setRotation(" + RangedRandomFloat(0.0f, 360.0f) + ");" +*/
                            "imGUI.getMain().addFloatingElement(text, \"text" + counter + "\", vec2(" + RangedRandomFloat(-font_size, GetScreenWidth() + font_size) + ", " + RangedRandomFloat(-font_size, GetScreenHeight() + font_size) + "), 1);");
        }
        if(counter == number_of_errors){
            DisplayError("Therium-2", "F KEY DETECTED. CRASH INITIATED.");
            MovementObject@ mo = ReadCharacter(0);
            mo.GetBoolVar("doesn'texist");
        }
    }
}

void OnExit(MovementObject @mo) {
    if(mo.controlled){
    }
}
