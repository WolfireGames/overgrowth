//-----------------------------------------------------------------------------
//           Name: hide_in_shadow.as
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

// Variables used by the script, and not to be set by the user.
float vignette_amount = 0.0f;
float vignette_timer = 0.0f;
float vignette_start = 0.0f;
float vignette_end = 0.0f;
const float PI = 3.1415f;

// Settings that can be changed by the user.
float vignette_target_amount = 1.0f;
float vignette_change_speed = 2.0f;
bool show_editor_icon = true;

void Reset() {
	vignette_amount = 0.0f;
	vignette_timer = 0.0f;
	vignette_start = 0.0f;
	vignette_end = 0.0f;
}

void Init() {
	Reset();
}

void SetParameters() {
	params.AddIntCheckbox("Require Stationary Crouching", true);
}

void HandleEvent(string event, MovementObject @mo){
	if(event == "enter"){
		OnEnter(mo);
	} else if(event == "exit"){
		OnExit(mo);
	}
}

void OnEnter(MovementObject @mo) {
	if(mo.controlled){
		// Object@ char_obj = ReadObjectFromID(mo.GetID());
		// ScriptParams @char_params = char_obj.GetScriptParams();
		// char_params.SetInt("Invisible When Stationary", 1);
		if(params.GetInt("Require Stationary Crouching") == 1){
			mo.Execute("invisible_when_stationary = 1;");
		}
		else
		{
			mo.Execute("invisible_when_moving = 1;");
		}

		vignette_timer = 0.0f;
		vignette_start = vignette_amount;
		vignette_end = vignette_target_amount;
	}
}

void OnExit(MovementObject @mo) {
	if(mo.controlled){
		// Object@ char_obj = ReadObjectFromID(mo.GetID());
		// ScriptParams @char_params = char_obj.GetScriptParams();
		// char_params.SetInt("Invisible When Stationary", 0);

		if(params.GetInt("Require Stationary Crouching") == 1){
			mo.Execute("invisible_when_stationary = 0;");
		}
		else
		{
			mo.Execute("invisible_when_moving = 0;");
		}

		vignette_timer = 0.0f;
		vignette_start = vignette_amount;
		vignette_end = 0.0f;
	}
}

void DrawEditor(){
	if(show_editor_icon){
		Object@ obj = ReadObjectFromID(hotspot.GetID());
		DebugDrawBillboard("Data/Textures/ui/eye_widget.tga",
							obj.GetTranslation() + obj.GetScale()[1] * vec3(0.0f,0.5f,0.0f),
							2.0f,
							vec4(1.0f),
							_delete_on_draw);
	}
}

void Draw(){
	HUDImage @vignette_image = hud.AddImage();
	vignette_image.SetImageFromPath("Data/Textures/vignette.tga");
	vignette_image.position.y = 0.0f;
	vignette_image.position.x = 0.0f;
	vignette_image.position.z = -1.0f;
	vignette_image.scale = vec3(GetScreenWidth() / 256.0f, GetScreenHeight() / 256.0f, 1.0);
	vignette_image.color = vec4(1.0f, 1.0f, 1.0f, vignette_amount);
}

float easeInOutSine(float progress){
	return sin((progress * PI) / 2);
}

void Update() {
	if(vignette_timer < 1.0){
		vignette_timer += time_step * vignette_change_speed;
		vignette_amount = mix(vignette_start, vignette_end, easeInOutSine(vignette_timer));
	}
}