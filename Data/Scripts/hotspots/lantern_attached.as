//-----------------------------------------------------------------------------
//           Name: lantern_attached.as
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

void SetParameters() {
    params.AddInt("lantern_id", -1);
    params.AddInt("light_id", -1);
}

void PreDraw(float curr_game_time) {
}

void Update() {
  int light_id = params.GetInt("light_id");
  int lantern_id = params.GetInt("lantern_id");
  if(light_id != -1 && lantern_id != -1){
      Object@ light_obj = ReadObjectFromID(light_id);
      Object@ lantern_obj = ReadObjectFromID(lantern_id);
      DebugText("lantern_obj", "lantern_obj: "+lantern_obj.GetTranslation(), 0.5f);
      light_obj.SetTranslation(lantern_obj.GetTranslation()+lantern_obj.GetRotation() * vec3(0,-0.05,0));
  }
}