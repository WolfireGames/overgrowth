//-----------------------------------------------------------------------------
//           Name: sound_hotspot.as
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

void Init() {
    level.ReceiveLevelEvents(hotspot.GetID());
}

void Dispose() {
    level.StopReceivingLevelEvents(hotspot.GetID());
}

void ReceiveMessage(string message) {
    TokenIterator token_iter;
    token_iter.Init();

    if (!token_iter.FindNextToken(message)) {
        return;
    }

    string command = token_iter.GetToken(message);
    if (command != "sound_hotspot_play_sound") {
        return;
    }

    HandlePlaySoundCommand(token_iter, message);
}

void HandlePlaySoundCommand(TokenIterator@ token_iter, const string& in message) {
    const string usage_message = "Usage: sound_hotspot_play_sound \"filename\" [x y z]";

    if (!token_iter.FindNextToken(message)) {
        Log(error, "Invalid parameters");
        Log(error, usage_message);
        return;
    }

    string sound_filename = token_iter.GetToken(message);
    if (!FileExists(sound_filename)) {
        Log(error, "File not found: " + sound_filename);
        return;
    }

    vec3 sound_pos = camera.GetPos();
    if (ParsePosition(token_iter, message, sound_pos)) {
        PlaySound(sound_filename, sound_pos);
    } else {
        Log(warning, "Invalid position specified. Using default camera position.");
        Log(warning, usage_message);
        PlaySound(sound_filename, sound_pos);
    }
}

bool ParsePosition(TokenIterator@ token_iter, const string& in message, vec3& out position) {
    if (!token_iter.FindNextToken(message)) {
        return false;
    }
    string pos_x = token_iter.GetToken(message);

    if (!token_iter.FindNextToken(message)) {
        return false;
    }
    string pos_y = token_iter.GetToken(message);

    if (!token_iter.FindNextToken(message)) {
        return false;
    }
    string pos_z = token_iter.GetToken(message);

    position = vec3(atof(pos_x), atof(pos_y), atof(pos_z));
    return true;
}
