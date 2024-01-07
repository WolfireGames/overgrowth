//-----------------------------------------------------------------------------
//           Name: fason_fire_hotspot.as
//      Developer:
//		   Author: Fason7
//    Script Type: Fire Hotspot
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

bool enabled = true;

float ribbon_interval = 0.1f;
bool ignite_characters = true;

float time_last_ribbon = 0.0f;

const array<string> ribbon_paths = {
	"Data/Particles/fason_large_fire/largefire1.xml",
	"Data/Particles/fason_large_fire/largefire2.xml",
	"Data/Particles/fason_large_fire/largefire3.xml",
	"Data/Particles/fason_large_fire/largefire4.xml",
	"Data/Particles/fason_large_fire/largefire5.xml"
};

string GetTypeString()
{
    return "FasonFireHotspot";
}

void SetParameters()
{
    params.AddFloatSlider("Ribbon Spawn Interval (ms)", ribbon_interval, "min:0,max:1,step:0.001,text_mult:1000");	
	ribbon_interval = params.GetFloat("Ribbon Spawn Interval (ms)");
	
	params.AddIntCheckbox("Ignite Characters", ignite_characters);
	ignite_characters = params.GetInt("Ignite Characters") == 1 ? true : false;
}

void SetEnabled(bool is_enabled)
{
    enabled = is_enabled;
}

void Update()
{
	if (!enabled) return;
	
	if (ImGui_GetTime() - time_last_ribbon >= ribbon_interval)
	{
		time_last_ribbon = ImGui_GetTime();
		
		int random_ribbon = int(RangedRandomFloat(0.0f, 1.0f) * ribbon_paths.length());
		
		MakeParticle(ribbon_paths[random_ribbon], ReadObjectFromID(hotspot.GetID()).GetTranslation(), vec3(0.0f));
	}
}

void HandleEvent(string event, MovementObject @mo)
{
    if (event == "enter" && ignite_characters == true)
		mo.ReceiveScriptMessage("ignite");
}
