//-----------------------------------------------------------------------------
//           Name: sound_hotspot.as
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
    level.ReceiveLevelEvents(hotspot.GetID());
}

void Dispose() {
    level.StopReceivingLevelEvents(hotspot.GetID());
}

void ReceiveMessage(string message) {
    TokenIterator token_iter;
    token_iter.Init();

    if(!token_iter.FindNextToken(message)) {
        return;
    }

    string token = token_iter.GetToken(message);
    const string usage_message = "sound_hotspot_play_sound: usage - `sound_hotspot_play_sound \\\"filename\\\" x y z` - x y z (position) is optional";

    if(token == "sound_hotspot_play_sound") {
        if(!token_iter.FindNextToken(message)) {
            Log(error, "sound_hotspot_play_sound: Invalid parameters");
            Log(error, usage_message);
            return;
        }

        string sound_filename = token_iter.GetToken(message);

        if(!FileExists(sound_filename)) {
            Log(error, "sound_hotspot_play_sound: No file found with the given filename");
            return;
        }

        vec3 sound_pos = camera.GetPos();
        bool specified_position = false;

        if(token_iter.FindNextToken(message)) {
            string pos_x = token_iter.GetToken(message);

            if(token_iter.FindNextToken(message)) {
                string pos_y = token_iter.GetToken(message);

                if(token_iter.FindNextToken(message)) {
                    string pos_z = token_iter.GetToken(message);

                    sound_pos = vec3(atof(pos_x), atof(pos_y), atof(pos_z));
                } else {
                    Log(warning, "sound_hotspot_play_sound: Specified x andy position, but not y and z. Ignoring pos passed in");
                    Log(warning, usage_message);
                }
            } else {
                Log(warning, "sound_hotspot_play_sound: Specified x position, but not y and z. Ignoring pos passed in");
                Log(warning, usage_message);
            }
        }

        PlaySound(sound_filename, sound_pos);
    }
}
