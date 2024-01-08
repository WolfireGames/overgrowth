//-----------------------------------------------------------------------------
//           Name: start_dialogue.as
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

bool played;
float visible_amount = 0.0;
float last_game_time = 0.0f;

void Reset() {
    played = false;
    if(params.HasParam("Start Disabled")){
        played = true;
    }
}

void Init() {
    Reset();
}

void SetParameters() {
    params.AddString("Dialogue", "Default text");
    params.AddIntCheckbox("Automatic", true);
    params.AddIntCheckbox("Fade", true);
    params.AddIntCheckbox("Play only once", true);
    params.AddIntCheckbox("Visible in game", true);
    params.AddIntCheckbox("Force play", false);
    params.AddString("Color", "1.0 1.0 1.0");
}

void ReceiveMessage(string msg){
    if(msg == "player_pressed_attack"){
        TryToPlayDialogue();
    }
    if(msg == "reset"){
        Reset();
    }
    if(msg == "activate"){
        if(played || params.GetInt ("Play only once") == 1){
            played = false;

            array<int> collides_with;
            level.GetCollidingObjects(hotspot.GetID(), collides_with);
            for(int i=0, len=collides_with.size(); i<len; ++i){
                int id = collides_with[i];
                if(ObjectExists(id) && ReadObjectFromID(id).GetType() == _movement_object){
                    MovementObject@ mo = ReadCharacterID(id);
                    if(mo.controlled && params.GetInt("Automatic") == 1){
                        TryToPlayDialogue();
                    }
                }
            }
        }
    }
}

void HandleEvent(string event, MovementObject @mo){
    if(event == "enter"){
        OnEnter(mo);
    } else if(event == "exit"){
        OnExit(mo);
        if (params.GetInt ("Play only once") == 0) {
            played = false;
        }
    }
}

void TryToPlayDialogue() {
    if(!played){
        bool player_in_valid_state = false;
        for(int i=0, len=GetNumCharacters(); i<len; ++i){
            MovementObject@ mo = ReadCharacter(i);
            if(mo.controlled && mo.QueryIntFunction("int CanPlayDialogue()") == 1 || params.GetInt("Force play") == 1){
                player_in_valid_state = true;
            }
        }
        if(player_in_valid_state){
            if(!params.HasParam("Fade") || params.GetInt("Fade") == 1){
                level.SendMessage("start_dialogue_fade \""+params.GetString("Dialogue")+"\"");
            } else {
                level.SendMessage("start_dialogue \""+params.GetString("Dialogue")+"\"");                
            }
            played = true;
        }
    }
}

void OnEnter(MovementObject @mo) {
    if(mo.controlled && params.GetInt("Automatic") == 1){
        TryToPlayDialogue();
    }
}

void OnExit(MovementObject @mo) {
}

void Draw() {
    if(params.GetInt("Visible in game") == 1 || EditorModeActive()){
        Object@ obj = ReadObjectFromID(hotspot.GetID());
        DebugDrawBillboard("Data/UI/spawner/thumbs/Hotspot/sign_icon.png",
                           obj.GetTranslation() + obj.GetScale()[1] * vec3(0.0f,0.5f,0.0f),
                           2.0f,
                           vec4(1.0f),
                           _delete_on_draw);
    }
    if(visible_amount > 0.0){
        vec3 color(1.0);
        if(params.HasParam("Color")){
            TokenIterator token_iter;
            token_iter.Init();
            string str = params.GetString("Color");
            token_iter.FindNextToken(str);
            color[0] = atof(token_iter.GetToken(str));
            if(token_iter.FindNextToken(str)){
                color[1] = atof(token_iter.GetToken(str));
                if(token_iter.FindNextToken(str)){
                    color[2] = atof(token_iter.GetToken(str));
                }
            }
        }
        vec3 offset;
        if(params.HasParam("Offset")){
            offset = vec3(0.4, 0.0, -0.4);
        }
        if(params.HasParam("Exclamation Character")){
            TokenIterator token_iter;
            token_iter.Init();
            string str = params.GetString("Exclamation Character");

            while(token_iter.FindNextToken(str)){
                int id = atoi(token_iter.GetToken(str));
                if(ObjectExists(id) && ReadObjectFromID(id).GetType() == _movement_object){
                    DebugDrawBillboard("Data/Textures/ui/stealth_debug/exclamation_themed.png",
                                ReadCharacterID(id).position + vec3(0, 1.6 +sin(the_time*3.0)*0.03, 0) + offset,
                                    1.0f+sin(the_time*3.0)*0.05,
                                    vec4(color, visible_amount),
                                  _delete_on_draw);
                }
            }
        }
        if(params.HasParam("Question Character")){
            TokenIterator token_iter;
            token_iter.Init();
            string str = params.GetString("Question Character");

            while(token_iter.FindNextToken(str)){
                int id = atoi(token_iter.GetToken(str));
                if(ObjectExists(id) && ReadObjectFromID(id).GetType() == _movement_object){
                    DebugDrawBillboard("Data/Textures/ui/stealth_debug/question_themed.png",
                                ReadCharacterID(id).position + vec3(0, 1.6 +sin(the_time*3.0)*0.03, 0) + offset,
                                    1.0f+sin(the_time*3.0)*0.05,
                                    vec4(color, visible_amount),
                                  _delete_on_draw);
                }
            }
        }
//        Log(warning, "Check");
        if(params.HasParam("Exclamation Object")){
            TokenIterator token_iter;
            token_iter.Init();
            string str = params.GetString("Exclamation Object");

            while(token_iter.FindNextToken(str)){
                int id = atoi(token_iter.GetToken(str));
                if(ObjectExists(id) && ReadObjectFromID(id).GetType() == _env_object){
                    DebugDrawBillboard("Data/Textures/ui/stealth_debug/exclamation_themed.png",
                                ReadObjectFromID(id).GetTranslation() + vec3(0, 1.6 +sin(the_time*3.0)*0.03, 0) + offset,
                                    1.0f+sin(the_time*3.0)*0.05,
                                    vec4(color, visible_amount),
                                  _delete_on_draw);
                }
            }
        }
        if(params.HasParam("Question Object")){
            TokenIterator token_iter;
            token_iter.Init();
            string str = params.GetString("Question Object");

            while(token_iter.FindNextToken(str)){
                int id = atoi(token_iter.GetToken(str));
                if(ObjectExists(id) && ReadObjectFromID(id).GetType() == _env_object){
                    DebugDrawBillboard("Data/Textures/ui/stealth_debug/question_themed.png",
                                ReadObjectFromID(id).GetTranslation() + vec3(0, 1.6 +sin(the_time*3.0)*0.03, 0) + offset,
                                    1.0f+sin(the_time*3.0)*0.05,
                                    vec4(color, visible_amount),
                                  _delete_on_draw);
                }
            }
        }
        if(params.HasParam("Exclamation Item")){
            TokenIterator token_iter;
            token_iter.Init();
            string str = params.GetString("Exclamation Item");

            while(token_iter.FindNextToken(str)){
                int id = atoi(token_iter.GetToken(str));
                if(ObjectExists(id) && ReadObjectFromID(id).GetType() == _item_object){
                    DebugDrawBillboard("Data/Textures/ui/stealth_debug/exclamation_themed.png",
                                ReadItemID(id).GetPhysicsPosition() + vec3(0, 1.6 +sin(the_time*3.0)*0.03, 0) + offset,
                                    1.0f+sin(the_time*3.0)*0.05,
                                    vec4(color, visible_amount),
                                  _delete_on_draw);
                }
            }
        }
        if(params.HasParam("Question Item")){
            TokenIterator token_iter;
            token_iter.Init();
            string str = params.GetString("Question Item");

            while(token_iter.FindNextToken(str)){
                int id = atoi(token_iter.GetToken(str));
                if(ObjectExists(id) && ReadObjectFromID(id).GetType() == _item_object){
                    DebugDrawBillboard("Data/Textures/ui/stealth_debug/question_themed.png",
                                ReadItemID(id).GetPhysicsPosition() + vec3(0, 1.6 +sin(the_time*3.0)*0.03, 0) + offset,
                                    1.0f+sin(the_time*3.0)*0.05,
                                    vec4(color, visible_amount),
                                  _delete_on_draw);
                }
            }
        }
    }
}


void PreDraw(float curr_game_time) {
    EnterTelemetryZone("Start_Dialogue hotspot update");
    if(!played || params.GetInt ("Play only once") == 0){
        array<int> collides_with;
        level.GetCollidingObjects(hotspot.GetID(), collides_with);
        for(int i=0, len=collides_with.size(); i<len; ++i){
            int id = collides_with[i];
            if(ObjectExists(id) && ReadObjectFromID(id).GetType() == _movement_object){
                MovementObject@ mo = ReadCharacterID(id);
                if(mo.controlled){
                    mo.Execute("dialogue_request_time = time;");
                }
            }
        }        
    }

    if(params.HasParam("Exclamation Character") || params.HasParam("Question Character") || params.HasParam("Exclamation Object") || params.HasParam("Question Object") || params.HasParam("Exclamation Item") || params.HasParam("Question Item")){
        const float kFadeSpeed = 2.0;
        float offset = (curr_game_time-last_game_time) * kFadeSpeed;
        if(!played && level.QueryIntFunction("int HasCameraControl()") == 0 || params.GetInt ("Play only once") == 0){
            visible_amount = min(visible_amount+offset, 1.0);
        } else {
            visible_amount = max(visible_amount-offset, 0.0);
        }
    }

    last_game_time = curr_game_time;

    LeaveTelemetryZone();
}