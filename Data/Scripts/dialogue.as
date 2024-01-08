//-----------------------------------------------------------------------------
//           Name: dialogue.as
//      Developer: Wolfire Games LLC
//    Script Type:
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

int dialogue_text_billboard_id;
bool ready_to_skip = false;

const float MPI = 3.14159265359;

enum ProcessStringType { kInGame, kInEditor };


// Multiplayer sync variables
uint8 mp_next = 0;
uint8 mp_skip = 0;
uint8 mp_index = 0;
uint8 mp_sub_index = 0;
int start_state = -1;
int skip_state = -1;
int next_state = -1;

class NameInfo {
    NameInfo(){}

    NameInfo(string _name, vec3 _color, int _voice) {
        name = _name;
        color = _color;
        voice = _voice;
    }

    string name;
    vec3 color;
    int voice;
}

array<NameInfo> name_info;

bool NameInfoExists(string name) {
    for( uint i = 0; i < name_info.size(); i++ ) {
        if( name_info[i].name == name ) {
            return true;
        }
    }
    return false;
}

NameInfo GetNameInfo(string name) {
    for( uint i = 0; i < name_info.size(); i++ ) {
        if( name_info[i].name == name ) {
            return name_info[i];
        }
    }
    return NameInfo();
}

void RegisterMPCallBacks() {
    next_state = Online_RegisterState("dialogueNext");
    skip_state = Online_RegisterState("dialogueSkip");
    start_state = Online_RegisterState("dialogueStart");

    Online_RegisterStateCallback(next_state, "void OnDialogueNextState(array<uint8>& data)");
    Online_RegisterStateCallback(skip_state, "void OnDialogueSkipState(array<uint8>& data)");
    Online_RegisterStateCallback(start_state, "void OnDialogueStartState(array<uint8>& data)");
}

void SendSkipState() {
    array<uint8> data(1);
    data[0] = 1;

    Online_SendState(skip_state, data);
}

void SendNextState(int index, int sub_index) {
    array<uint8> data(3);
    data[0] = 1;
    data[1] = index;
    data[2] = sub_index;

    Online_SendState(next_state, data);
}

void OnDialogueNextState(array<uint8>& data) {
    mp_next = data[0];
    mp_index = data[1];
    mp_sub_index = data[2];
}

void OnDialogueSkipState(array<uint8>& data) {
    mp_skip = data[0];
}

void OnDialogueStartState(array<uint8>& data) {
    string temp;
    temp.resize(data.length());
    for (uint i = 0; i < data.length(); i++) {
        temp[i] = data[i];
    }

    Log(warning, "received start dialogue: " + temp);
    dialogue.StartDialogue(temp);
}

void SendStart(string dialogue_name) {
    Log(warning, "Sending: " + dialogue_name);

    array<uint8> data(dialogue_name.length());

    for (uint i = 0; i < dialogue_name.length(); i++) {
        data[i] = dialogue_name[i];
    }
    Log(info, "sending start dialogue: " + dialogue_name);
    Online_SendState(start_state, data);
}

void AddNameInfo(NameInfo ni)  {
    for( uint i = 0; i < name_info.size(); i++ ) {
        if( name_info[i].name == ni.name ) {
            name_info.removeAt(i);
            break;
        }
    }
    name_info.insertLast(ni);
}

// What actions can be triggered from a dialogue script line
enum ObjCommands {
    kUnknown,
    kCamera,
    kDialogueColor,
    kDialogueVoice,
    kHeadTarget,
    kEyeTarget,
    kChestTarget,
    kCharacter,
    kCharacterDialogueControl,
    kDialogueVisible,
    kDialogueName,
    kSetDialogueText,
    kAddDialogueText,
    kCharacterStartTalking,
    kCharacterStopTalking,
    kWait,
    kWaitForClick,
    kSetCamControl,
    kSetAnimation,
    kSay,
    kFadeToBlack,
    kSetMorphTarget
};

class ScriptElement {
    string str; // The raw string
    bool visible; // Display this line in editor?
    bool locked; // Allow changes?
    bool record_locked; 
    int spawned_id; // ID of the object associated with this script line
    int line_start_char; // index of the line in the full script
    // Pre-tokenized
    ObjCommands obj_command;
    array<string> params;
}

class CharRecord {
    int last_pos;
    int last_head;
    int last_eye;
    int last_torso;
    int last_anim;
    int last_morph;

    CharRecord() {
        last_pos = -1;
        last_head = -1;
        last_eye = -1;
        last_torso = -1;
        last_anim = -1;
        last_morph = -1;
    }
};

enum SayParseState {kStart, kInBracket, kContinue};

const int kMaxParticipants = 16;
const float kTextLeftMargin = 100;
const float kTextRightMargin = 100;

int voice_preview = 0;
float voice_preview_time = -100;

enum SavePopupValue { NONE, YES, NO };

class Dialogue {
    // Contains information from undo/redo
    string history_str;
    // This state is important for undo/redo
    int dialogue_obj_id = -1;
    array<ScriptElement> strings;
    array<ScriptElement> sub_strings;
    array<int> connected_char_ids;
    array<vec3> dialogue_colors;
    array<int> dialogue_voices;
    array<float> dof_params;

    int index; // which dialogue element is being executed
    int sub_index;
    bool has_cam_control;
    bool show_dialogue;
    bool waiting_for_dialogue;
    bool is_waiting_time;
    bool skip_dialogue; // This is dialogue we've already seen
    float wait_time;
    string dialogue_name;
    string dialogue_text;
    float dialogue_text_disp_chars;
    float line_start_time;
    int old_font_size;
    float speak_sound_time;
    int active_char;

    float fade_start;
    float fade_end;

    bool queue_skip;

    vec3 cam_pos;
    vec3 cam_rot;
    float cam_zoom;

    bool clear_on_complete;
    bool preview = false;

    bool show_editor_info;
    int selected_line; // Which line we are editing

    float init_time; // How long since level started
    float start_time = 0.0; // How long since dialogue started

    int old_cursor_pos = -1; // Detect if cursor pos has changed since last check

    string potential_display_name = "";
    int old_participant_count = -1;

    bool invalid_file = false;
    bool modified = false;

    string full_path;

    // Adds an extra '\' before '\' or '"' characters in string
    string EscapeString(const string &in str){
        int str_len = int(str.length());
        string new_str;
        for(int i=0; i<str_len; ++i){
            if(str[i] == 92 || str[i] == 34){
                new_str += "\\";
            }
            new_str.resize(new_str.length()+1);
            new_str[new_str.length()-1] = str[i];
        }
        return new_str;
    }

    // Play dialogue with given name
    void StartDialogue(const string &in name){
        if (Online_IsHosting()) {
            SendStart(name);
        }

        index = 0;
        sub_index = -1;
        start_time = the_time;
        array<int> @object_ids = GetObjectIDs();
        int num_objects = object_ids.length();
        int dialogue_id = -1;
        for(int i=0; i<num_objects; ++i){
            Object @obj = ReadObjectFromID(object_ids[i]);
            ScriptParams@ params = obj.GetScriptParams();
            if(obj.GetType() == _placeholder_object && params.HasParam("Dialogue") && params.HasParam("DisplayName") && params.GetString("DisplayName") == name){
                if(dialogue_id == -1) {
                    dialogue_id = object_ids[i];
                } else {
                    Log(warning, "Found more than one dialogue with the DisplayName: '" + name + "' - playing dialogue id: " + dialogue_id + " - skipping dialogue id: " + object_ids[i]);
                }
            }
        }
        if(dialogue_id != -1) {
            camera.SetDOF(0,0,0, 0,0,0);
            SetDialogueObjID(dialogue_id);
            Play();
            clear_on_complete = true;
        }
        speak_sound_time = 0.0;
    }

    void NotifyDeleted(int id){
        if(dialogue_obj_id == id){
            ClearEditor();
        }
        // Delete connectors if dialogue object is deleted
		if(ObjectExists(id)){
			Object @obj = ReadObjectFromID(id);
			ScriptParams@ params = obj.GetScriptParams();
			if(obj.GetType() == _placeholder_object && params.HasParam("Dialogue") && params.HasParam("NumParticipants")){
				int num_connectors = params.GetInt("NumParticipants");
				for(int j=1; j<=num_connectors; ++j){
					if(params.HasParam("obj_"+j)){
						int obj_id = params.GetInt("obj_"+j);
						if(ObjectExists(obj_id)){   
							DeleteObjectID(obj_id);
						}
					}
				}
			}
		}
    }

    void UpdateRecordLocked(bool recording) {
        if(dialogue_obj_id == -1 || !ObjectExists(dialogue_obj_id)){
            return;
        }
        int num_strings = int(strings.size());
        for(int i=0; i<num_strings; ++i){
            strings[i].record_locked = false;
        }
        if(recording){
            Object @obj = ReadObjectFromID(dialogue_obj_id);
            ScriptParams @params = obj.GetScriptParams();
            int num_participants = min(kMaxParticipants, params.GetInt("NumParticipants"));

            int num_lines = int(strings.size());
            int next_wait = num_lines-1;
            for(int i=num_lines-1; i>=selected_line; --i){
                if(strings[i].obj_command == kWaitForClick || strings[i].obj_command == kSay){
                    next_wait = i;
                }
            }

            int last_cam_update = -1;
            array<CharRecord> char_record;
            char_record.resize(num_participants);
            for(int i=0; i<next_wait; ++i){
                switch(strings[i].obj_command){
                case kCamera:
                    last_cam_update = i;
                    break;
                case kCharacter:{
                    int which_char = atoi(strings[i].params[0]);
                    char_record[which_char-1].last_pos = i;
                    break;}
                case kHeadTarget:{
                    int which_char = atoi(strings[i].params[0]);
                    char_record[which_char-1].last_head = i;
                    break;}
                case kEyeTarget:{
                    int which_char = atoi(strings[i].params[0]);
                    char_record[which_char-1].last_eye = i;
                    break;}
                case kChestTarget:{
                    int which_char = atoi(strings[i].params[0]);
                    char_record[which_char-1].last_torso = i;
                    break;}
                case kSetAnimation:{
                    int which_char = atoi(strings[i].params[0]);
                    char_record[which_char-1].last_anim = i;
                    break;}
                case kSetMorphTarget:{
                    int which_char = atoi(strings[i].params[0]);
                    char_record[which_char-1].last_morph = i;
                    break;}
                }
            }
            if(last_cam_update != -1){
                strings[last_cam_update].record_locked = true;
            }
            for(int i=0; i<num_participants; ++i){
                if(char_record[i].last_pos != -1){
                    strings[char_record[i].last_pos].record_locked = true;
                }
                if(char_record[i].last_anim != -1){
                    strings[char_record[i].last_anim].record_locked = true;
                }
                if(char_record[i].last_torso != -1){
                    strings[char_record[i].last_torso].record_locked = true;
                }
                if(char_record[i].last_head != -1){
                    strings[char_record[i].last_head].record_locked = true;
                }
                if(char_record[i].last_eye != -1){
                    strings[char_record[i].last_eye].record_locked = true;
                }
                if(char_record[i].last_morph != -1){
                    strings[char_record[i].last_morph].record_locked = true;
                }
            }
            for(int i=0; i<num_lines; ++i){
                if(strings[i].record_locked){
                    CreateEditorObj(strings[i]);
                }
            }
        }
    }

    void ClearSpawnedObjects() {
        int num_strings = int(strings.size());
        for(int i=0; i<num_strings; ++i){
            if(strings[i].spawned_id != -1){  
                DeleteObjectID(strings[i].spawned_id);
                strings[i].spawned_id = -1;
            }
        }
    }

    void ClearEditor() {
        Log(info,"Clearing editor");        
        clear_on_complete = false;
        queue_skip = false;
        ready_to_skip = false;
        queued_id = -1;

        int num = GetNumCharacters();
        for(int i=0; i<num; ++i){
            MovementObject@ char = ReadCharacter(i);
            char.ReceiveScriptMessage("set_dialogue_control false");
        }

        if(preview) {
            preview = false;
        } else {
            if(dialogue_obj_id != -1){
                if(old_participant_count != -1) {
                    ReadObjectFromID(dialogue_obj_id).GetScriptParams().SetString("NumParticipants", "" + old_participant_count);
                    UpdateDialogueObjectConnectors(dialogue_obj_id);
                }
                ClearSpawnedObjects();
            }
            connected_char_ids.resize(0);
            selected_line = 0;
            dialogue_obj_id = -1;
            strings.resize(0);
            invalid_file = false;
            old_participant_count = -1;
            dialogue_colors.resize(0);
            dialogue_voices.resize(0);
            dof_params.resize(0);
            modified = false;
            active_char = 0;
            full_path = "";
        }
    }

    void ResizeUpdate( int w, int h ) {
    }

    void Init() {
        ClearEditor();
        skip_dialogue = false;
        is_waiting_time = false;
        old_font_size = -1;
        index = 0;
        sub_index = -1;
        init_time = the_time;
        fade_end = -1.0;

        ResizeUpdate(GetScreenWidth(),GetScreenHeight());

        has_cam_control = false;
        show_dialogue = false;
        show_editor_info = false;
        waiting_for_dialogue = false;
        if(level.GetScriptParams().HasParam("Dialogue Colors")){            
            LoadNameInfo(level.GetScriptParams().GetString("Dialogue Colors"));
        }
    }

    int GetActiveVoice() {
        int voice = 0;
        if(NameInfoExists(dialogue_name)){
            voice = GetNameInfo(dialogue_name).voice;
        } else if(active_char >= 0 && int(dialogue_voices.size()) > active_char){
            voice = dialogue_voices[active_char];
        } else {
            voice = 0;
        }
        if(voice_preview_time > the_time){
            voice = voice_preview;
        }
        return voice;
    }

    void PlayLineStartSound() {
        switch(GetActiveVoice()){
            case 0: PlaySoundGroup("Data/Sounds/concrete_foley/fs_light_concrete_run.xml"); break;
            case 1: PlaySoundGroup("Data/Sounds/drygrass_foley/fs_light_drygrass_walk.xml"); break;
            case 2: PlaySoundGroup("Data/Sounds/cloth_foley/cloth_fabric_choke_move.xml"); break;
            case 3: PlaySoundGroup("Data/Sounds/dirtyrock_foley/fs_light_dirtyrock_run.xml"); break;
            case 4: PlaySoundGroup("Data/Sounds/cloth_foley/cloth_leather_choke_move.xml"); break;
            case 5: PlaySoundGroup("Data/Sounds/grass_foley/bf_grass_medium.xml", 0.5); break;
            case 6: PlaySoundGroup("Data/Sounds/gravel_foley/fs_light_gravel_run.xml"); break;
            case 7: PlaySoundGroup("Data/Sounds/sand_foley/fs_light_sand_run.xml", 0.7); break;
            case 8: PlaySoundGroup("Data/Sounds/snow_foley/bf_snow_light.xml", 0.5); break;
            case 9: PlaySoundGroup("Data/Sounds/wood_foley/fs_light_wood_run.xml", 0.4); break;
            case 10: PlaySoundGroup("Data/Sounds/water_foley/mud_fs_run.xml", 0.4); break;
            case 11: PlaySoundGroup("Data/Sounds/concrete_foley/fs_heavy_concrete_run.xml", 0.5); break;
            case 12: PlaySoundGroup("Data/Sounds/drygrass_foley/fs_heavy_drygrass_run.xml", 0.4); break;
            case 13: PlaySoundGroup("Data/Sounds/dirtyrock_foley/fs_heavy_dirtyrock_run.xml", 0.5); break;
            case 14: PlaySoundGroup("Data/Sounds/grass_foley/fs_heavy_grass_run.xml", 0.3); break;
            case 15: PlaySoundGroup("Data/Sounds/gravel_foley/fs_heavy_gravel_run.xml", 0.3); break;
            case 16: PlaySoundGroup("Data/Sounds/sand_foley/fs_heavy_sand_jump.xml", 0.3); break;
            case 17: PlaySoundGroup("Data/Sounds/snow_foley/fs_heavy_snow_jump.xml", 0.3); break;
            case 18: PlaySoundGroup("Data/Sounds/wood_foley/fs_heavy_wood_run.xml", 0.3); break;
        }
    }

    void PlayLineContinueSound() {
        switch(GetActiveVoice()){
            case 0: PlaySoundGroup("Data/Sounds/concrete_foley/fs_light_concrete_edgecrawl.xml"); break;
            case 1: PlaySoundGroup("Data/Sounds/drygrass_foley/fs_light_drygrass_crouchwalk.xml"); break;
            case 2: PlaySoundGroup("Data/Sounds/cloth_foley/cloth_fabric_crouchwalk.xml"); break;
            case 3: PlaySoundGroup("Data/Sounds/dirtyrock_foley/fs_light_dirtyrock_crouchwalk.xml"); break;
            case 4: PlaySoundGroup("Data/Sounds/cloth_foley/cloth_leather_crouchwalk.xml"); break;
            case 5: PlaySoundGroup("Data/Sounds/grass_foley/fs_light_grass_run.xml", 0.5); break;
            case 6: PlaySoundGroup("Data/Sounds/gravel_foley/fs_light_gravel_crouchwalk.xml"); break;
            case 7: PlaySoundGroup("Data/Sounds/sand_foley/fs_light_sand_crouchwalk.xml", 0.7); break;
            case 8: PlaySoundGroup("Data/Sounds/snow_foley/fs_light_snow_run.xml", 0.5); break;
            case 9: PlaySoundGroup("Data/Sounds/wood_foley/fs_light_wood_crouchwalk.xml", 0.4); break;
            case 10: PlaySoundGroup("Data/Sounds/water_foley/mud_fs_walk.xml", 0.4); break;
            case 11: PlaySoundGroup("Data/Sounds/concrete_foley/fs_heavy_concrete_walk.xml", 0.5); break;
            case 12: PlaySoundGroup("Data/Sounds/drygrass_foley/fs_heavy_drygrass_walk.xml", 0.4); break;
            case 13: PlaySoundGroup("Data/Sounds/dirtyrock_foley/fs_heavy_dirtyrock_walk.xml", 0.5); break;
            case 14: PlaySoundGroup("Data/Sounds/grass_foley/fs_heavy_grass_walk.xml", 0.3); break;
            case 15: PlaySoundGroup("Data/Sounds/gravel_foley/fs_heavy_gravel_walk.xml", 0.3); break;
            case 16: PlaySoundGroup("Data/Sounds/sand_foley/fs_heavy_sand_run.xml", 0.3); break;
            case 17: PlaySoundGroup("Data/Sounds/snow_foley/fs_heavy_snow_crouchwalk.xml", 0.3); break;
            case 18: PlaySoundGroup("Data/Sounds/wood_foley/fs_heavy_wood_walk.xml", 0.3); break;
        }
    }

    string CreateStringFromParams(ObjCommands command, array<string> &in params){
        string str;
        int num_params = int(params.size());
        switch(command){
        case kCamera:
            str = "set_cam";
            for(int i=0; i<num_params; ++i){
                str += " " + params[i];
            }
            break;
        case kHeadTarget:
            str = "send_character_message "+params[0]+" \"set_head_target";
            for(int i=1; i<num_params; ++i){
                str += " " + params[i];
            }
            str += "\"";
            break;
        case kEyeTarget:
            str = "send_character_message "+params[0]+" \"set_eye_dir";
            for(int i=1; i<num_params; ++i){
                str += " " + params[i];
            }
            str += "\"";
            break;
        case kChestTarget:
            str = "send_character_message "+params[0]+" \"set_torso_target";
            for(int i=1; i<num_params; ++i){
                str += " " + params[i];
            }
            str += "\"";
            break;
        case kCharacterDialogueControl:
            str = "send_character_message "+params[0]+" \"set_dialogue_control "+params[1]+"\"";
            break;
        case kCharacter:
            str = "set_character_pos";
            for(int i=0; i<num_params; ++i){
                str += " " + params[i];
            }
            break;
        case kSetAnimation:
            str = "send_character_message "+params[0]+" \"set_animation \\\""+params[1]+"\\\"\"";
            break;
        case kSetMorphTarget:
            str = "send_character_message "+params[0]+" \"set_morph_target "+params[1]+" "+params[2]+" "+params[3]+" "+params[4]+"\"";
            break;
        case kFadeToBlack:
            str = "fade_to_black "+params[0];
            break;
        case kDialogueVisible:
            str = "set_dialogue_visible "+params[0];
            break;
        case kSetCamControl:
            str = "set_cam_control "+params[0];
            break;
        }
        return str;
    }

    void SaveScript() {
        if(dialogue_obj_id == -1){
            return;
        }

        Object @obj = ReadObjectFromID(dialogue_obj_id);
        ScriptParams @params = obj.GetScriptParams();

        if(!potential_display_name.isEmpty()) {
            params.SetString("DisplayName", potential_display_name);
            obj.SetEditorLabel(params.GetString("DisplayName"));
            cast<PlaceholderObject@>(obj).SetEditorDisplayName("Dialogue \""+params.GetString("DisplayName")+"\"");
            potential_display_name = "";
        }

        if(full_path != "") {
            SaveToFile(full_path);
        } else {
            string dialogue_script;
            int num_lines = int(strings.size());
            for(int i=0; i<num_lines; ++i){
                if(strings[i].visible){
                    dialogue_script += strings[i].str;
                    dialogue_script += "\n";
                }
            }
            params.SetString("Script", dialogue_script);
        }

        modified = false;
        cast<PlaceholderObject@>(obj).SetUnsavedChanges(true);
        old_participant_count = -1;
    }

    void SaveToFile(const string &in path) {
        Log(info,"Save to file: "+path);
        int num_strings = strings.size();
        StartWriteFile();
        for(int i=0; i<num_strings; ++i){
            if(!strings[i].visible){
                continue;
            }
            AddFileString(strings[i].str);
            if(i != num_strings - 1){
                AddFileString("\n");
            }
        }
        WriteFileKeepBackup(path);
        Object @obj = ReadObjectFromID(dialogue_obj_id);
        cast<PlaceholderObject@>(obj).SetUnsavedChanges(true);
        ScriptParams @params = obj.GetScriptParams();
        params.SetString("Dialogue", FindShortestPath(path));
        if(params.HasParam("Script")) {
            params.Remove("Script");
        }
        modified = false;
        old_participant_count = -1;
    }

    void AddInvisibleStrings() {   
        Object @obj = ReadObjectFromID(dialogue_obj_id);
        ScriptParams @params = obj.GetScriptParams();

        int num_participants = 0;
        if(params.HasParam("NumParticipants")){
            num_participants = min(kMaxParticipants, params.GetInt("NumParticipants"));
        }

        //Place the defaults at the top, but below the title, if there are no commands before then.
        int insert_index = 0;
        for( uint i = 0; i < strings.length(); i++ ) {
            if( strings[i].str.length() == 0 )   
            {

            }
            else if( strings[i].str.substr(0,1) == "#" )
            {

            }
            else
            {
                insert_index = i;
                break;
            }
        }

        array<string> str_params;
        str_params.resize(7);
        str_params[0] = obj.GetTranslation().x;
        str_params[1] = obj.GetTranslation().y;
        str_params[2] = obj.GetTranslation().z;
        str_params[3] = 0;
        str_params[4] = 0;
        str_params[5] = 0;
        str_params[6] = 90;
        AddLine(CreateStringFromParams(kCamera, str_params), insert_index);
        strings[insert_index].visible = false;
        for(int i=0; i<num_participants; ++i){
            int char_id = GetDialogueCharID(i+1);
            if(char_id == -1){
                continue;
            }

            if( MovementObjectExists(char_id) ) {
                str_params.resize(5);
                str_params[0] = i+1;
                                    
                MovementObject @char = ReadCharacterID(char_id);
                mat4 head_mat = char.rigged_object().GetAvgIKChainTransform("head");
                vec3 head_pos = head_mat * vec4(0,0,0,1) + head_mat * vec4(0,1,0,0);
                str_params[1] = head_pos.x;
                str_params[2] = head_pos.y;
                str_params[3] = head_pos.z;
                str_params[4] = 0.0f;
                AddLine(CreateStringFromParams(kHeadTarget, str_params), insert_index);
                strings[insert_index].visible = false;
                    
                str_params.resize(5);
                mat4 chest_mat = char.rigged_object().GetAvgIKChainTransform("torso");
                vec3 torso_pos = chest_mat * vec4(0,0,0,1) + chest_mat * vec4(0,1,0,0);
                str_params[1] = torso_pos.x;
                str_params[2] = torso_pos.y;
                str_params[3] = torso_pos.z;
                str_params[4] = 0.0f;
                AddLine(CreateStringFromParams(kChestTarget, str_params), insert_index);
                strings[insert_index].visible = false;
                    
                str_params.resize(2);
                str_params[1] = "front";
                AddLine(CreateStringFromParams(kEyeTarget, str_params), insert_index);
                strings[insert_index].visible = false;
                    
                str_params.resize(2);
                str_params[1] = "Data/Animations/r_actionidle.anm";
                AddLine(CreateStringFromParams(kSetAnimation, str_params), insert_index);
                strings[insert_index].visible = false;
                    
                str_params.resize(5);
                Object @char_spawn = ReadObjectFromID(char_id);
                str_params[1] = char_spawn.GetTranslation().x;
                str_params[2] = char_spawn.GetTranslation().y;
                str_params[3] = char_spawn.GetTranslation().z;
                str_params[4] = 0;
                AddLine(CreateStringFromParams(kCharacter, str_params), insert_index);
                strings[insert_index].visible = false;
                    
                str_params.resize(2);
                str_params[1] = "true";
                AddLine(CreateStringFromParams(kCharacterDialogueControl, str_params), insert_index);
                strings[insert_index].visible = false;
                    
                str_params.resize(2);
                str_params[1] = "false";
                int last_line = int(strings.size());
                AddLine(CreateStringFromParams(kCharacterDialogueControl, str_params), last_line);
                strings[last_line].visible = false;
            } else {
                Log( error, "No object with id " + char_id );
            }
        }
            
        str_params.resize(1);
        str_params[0] = "false";
        int last_line = int(strings.size());
        AddLine(CreateStringFromParams(kDialogueVisible, str_params), last_line);
        strings[last_line].visible = false;

        last_line = int(strings.size());
        AddLine(CreateStringFromParams(kSetCamControl, str_params), last_line);
        strings[last_line].visible = false;
    }

    void UpdateScriptFromStrings(){
        string full_script;
        for(int i=0, len=strings.size(); i<len; ++i){
            if(strings[i].visible && strings[i].str != ""){
                strings[i].line_start_char = full_script.length();
                full_script += strings[i].str + "\n";
            }
        }
        ImGui_SetTextBuf(full_script);
    }

    void UpdateStringsFromScript(const string &in script){
        strings.resize(0);
        string token = "\n";
        int script_len = int(script.length());
        int line_start = 0;
        for(int i=0; i<script_len; ++i){
            if(script[i] == token[0] || i==script_len-1){
                if(script[i] != token[0]){
                    ++i;
                }
                int index = int(strings.size());
                string str = script.substr(line_start, i-line_start);
                if(str != "" && str != "\n"){
                    AddLine(str,index);
                    strings[index].line_start_char = line_start;
                }
                line_start = i+1;
            }
        }
    }

    void ReloadScript() {
        array<ScriptElement> old_strings = strings;
        strings.resize(0);
        int index = 0;
        for(uint i = 0; i < old_strings.size(); ++i) {
            if(old_strings[i].visible) {
                AddLine(old_strings[i].str, index++);
            }
        }
        AddInvisibleStrings();
    }

    void UpdateConnectedChars() {
        bool changed = false;
        Object @obj = ReadObjectFromID(dialogue_obj_id);
        ScriptParams @params = obj.GetScriptParams();
        int num_participants = min(kMaxParticipants, params.GetInt("NumParticipants"));
        int old_size = int(connected_char_ids.size());
        if(num_participants < old_size){
            for(int i=num_participants; i<old_size; ++i){
                if(connected_char_ids[i] != -1 && ObjectExists(connected_char_ids[i])){
                    MovementObject@ char = ReadCharacterID(connected_char_ids[i]);
                    char.ReceiveScriptMessage("set_dialogue_control false");
                    changed = true;
                }
            }
            connected_char_ids.resize(num_participants);
        } else if(num_participants > old_size){
            connected_char_ids.resize(num_participants);
            for(int i=old_size; i<num_participants; ++i){
                connected_char_ids[i] = -1;
            }

        }
        for(int i=0; i<num_participants; ++i){
            int new_id = GetDialogueCharID(i+1);
            if(connected_char_ids[i] != new_id){
                if(connected_char_ids[i] != -1 && ObjectExists(connected_char_ids[i])){
                    MovementObject@ char = ReadCharacterID(connected_char_ids[i]);
                    char.ReceiveScriptMessage("set_dialogue_control false");
                    changed = true;
                }
                if(new_id != -1 && ObjectExists(new_id)){
                    //MovementObject@ char = ReadCharacterID(new_id);
                    //char.ReceiveScriptMessage("set_dialogue_control true");
                    changed = true;
                }
                connected_char_ids[i] = new_id;
            }
        }
        if(changed){
            ReloadScript();
            /*ClearSpawnedObjects();
            if(params.HasParam("Script")) {
                string script = params.GetString("Script");
                UpdateStringsFromScript(script);
                AddInvisibleStrings();
            } else {
                string path = params.GetString("Dialogue");
                if(path != "empty") {
                    if(LoadScriptFile(path)) {
                        AddInvisibleStrings();
                    }
                }
            }*/
        }
    }

    int queued_id = -1;
    void SetDialogueObjID(int id) {
        if(dialogue_obj_id != id) {
            if(!show_editor_info || !EditorModeActive()) {
                SetActiveDialogue(id);
            } else {
                queued_id = id;
            }
        }
    }

    void SetActiveDialogue(int id) {
        ClearEditor();
        dialogue_obj_id = id;
        if(dialogue_obj_id != -1){
            Object @obj = ReadObjectFromID(dialogue_obj_id);
            ScriptParams @params = obj.GetScriptParams();

            if(!params.HasParam("NumParticipants") || !params.HasParam("Dialogue")){
                Log(warning,"Selected dialogue object does not have the necessary parameters (id "+dialogue_obj_id+")");
            } else {
                bool script_exists = params.HasParam("Script");
                bool dialogue_exists = params.HasParam("Dialogue") && params.GetString("Dialogue") != "empty";
                // Always load from inline dialogue if there is an ambiguity
               if(script_exists) {
                    // Parse inline script
                    string script = params.GetString("Script");
                    UpdateStringsFromScript(script);
                    if(dialogue_exists) {
                        Log(warning, "Both inline dialogue script and external dialogue file exists, will use the inline script");
                    }
                } else if(dialogue_exists) {
                    string path = params.GetString("Dialogue");
                    if(!LoadScriptFile(path)) {
                        Log(warning, "Dialogue object tried to load non-existent file \"" + path + "\"");
                        if(show_dialogue && EditorModeActive()) {
                            // Only actually make this change if the dialogue is opened in the editor.
                            // Otherwise, for instance, an invalid campaign map would trigger the
                            // "Save changes?" popup on completion, even when played in "game" mode
                            params.SetString("Dialogue", "empty");
                            invalid_file = true;
                            cast<PlaceholderObject@>(obj).SetUnsavedChanges(true);
                        }
                        strings.resize(0);
                        AddLine("#name \"Unnamed\"", strings.size());
                        AddLine("#participants 1", strings.size());
                        AddLine("", strings.size());
                        AddLine("say 1 \"Name\" \"Type your dialogue here.\"", strings.size());
                    }
                } else {
                    strings.resize(0);
                    AddLine("#name \"Unnamed\"", strings.size());
                    AddLine("#participants 1", strings.size());
                    AddLine("", strings.size());
                    AddLine("say 1 \"Name\" \"Type your dialogue here.\"", strings.size());
                }

                UpdateConnectedChars();
                //AddInvisibleStrings();
                UpdateScriptFromStrings();
                selected_line = 1;
            }
        }
    }

    void AddLine(const string &in str, int index){
        strings.resize(strings.size() + 1);
        int num_lines = strings.size();
        for(int i=num_lines-1; i>index; --i){
            strings[i] = strings[i-1];
        }
        strings[index].str = str;
        strings[index].record_locked = false;
        strings[index].visible = true;
        ParseLine(strings[index]);
        if(index <= selected_line){
            ++selected_line;
        }
    }

    bool LoadScriptFile(const string &in path) {
        if(path.isEmpty()) {
            return false;
        }
        full_path = FindFilePath(path);
        if(!LoadFile(GetLocalizedDialoguePath(FindShortestPath(full_path)))){
            return false;
        } else {
            strings.resize(0);
            string new_str;
            while(true){
                new_str = GetFileLine();
                if(new_str == "end"){
                    break;
                }
                AddLine(new_str, strings.size());
            }
            return true;
        }
    }

    bool LoadNameInfo(const string &in path) {
        Log(info, "Attempting to load file: \""+path+"\" ...\n");
        if(!LoadFile(path)){
            Log(error, "Failed to load file: \""+path+"\" ...");
            return false;
        } else {
            TokenIterator token_iter;
            Log(info, "Success\n");
            strings.resize(0);
            string new_str;
            vec3 color;
            int voice;
            while(true){
                new_str = GetFileLine();
                Log(info, new_str+"\n");
                token_iter.Init();
                if(token_iter.FindNextToken(new_str)){
                    string name = token_iter.GetToken(new_str);
                    Log(info, "Name: "+name+"\n");
                    if(token_iter.FindNextToken(new_str) && 
                       token_iter.GetToken(new_str) == "color" && 
                       token_iter.FindNextToken(new_str))
                    {
                        color[0] = atof(token_iter.GetToken(new_str));
                        if(token_iter.FindNextToken(new_str)){
                            color[1] = atof(token_iter.GetToken(new_str));
                            if(token_iter.FindNextToken(new_str)){
                                color[2] = atof(token_iter.GetToken(new_str));
                                Log(info, "Color: "+color+"\n");
                                if(token_iter.FindNextToken(new_str) && 
                                   token_iter.GetToken(new_str) == "voice" && 
                                   token_iter.FindNextToken(new_str))
                                {
                                    voice = atoi(token_iter.GetToken(new_str));
                                    Log(info, "Voice: "+voice+"\n");
                                    AddNameInfo(NameInfo(name,color,voice));
                                }
                            }
                        }
                    }
                }
                if(new_str == "end"){
                    break;
                }
            }
            Log(info, "Done\n");
            return true;
        }
    }

    bool SkipKeyDown() {
        if(!ready_to_skip || queue_skip){
            return false;
        }
        if(GetInputDown(controller_id, "skip_dialogue") || GetInputDown(controller_id, "keypadenter")){
            return true;
        } else {
            return false;
        }
    }

    void Play() {
        bool stop = false;
        bool first = false;
  
        if(index == 0){

            Online_SetAvatarCameraAttachedMode(false);
            first = true;
        }

        int last_wait = -1;
        int prev_last_wait = -1;
        for(int i=0, len=strings.size(); i<len; ++i){
            if(strings[i].obj_command == kWaitForClick || strings[i].obj_command == kSay){
                prev_last_wait = last_wait;
                last_wait = i;
            }
        }

        if(the_time > init_time + 0.5){
            skip_dialogue = false; // Only skip dialogue that starts at the beginning of the level
        }

        while(!stop){
            if(index < int(strings.size())){
                stop = ExecuteScriptElement(strings[index], kInGame);
                if(index == prev_last_wait){
                    skip_dialogue = false;
                }
                if(skip_dialogue){
                    stop = false;
                }
                if(queue_skip){
                    stop = false;
                }
                if(sub_index == -1){
                    ++index;
                }
            } else {

                Online_SetAvatarCameraAttachedMode(true);
                stop = true;
                index = 0;
            }
        }

        if(first){
            for(int i=0, len=connected_char_ids.size(); i<len; ++i){
                if(connected_char_ids[i] != -1 && ObjectExists(connected_char_ids[i])){
                    MovementObject@ char = ReadCharacterID(connected_char_ids[i]);
                    char.Execute("FixDiscontinuity();");
                }
            }
            camera.FixDiscontinuity();
        }

        skip_dialogue = false;
    }


    void ClearUnselectedObjects() {
        int num_strings = int(strings.size());
        for(int i=0; i<num_strings; ++i){
            if(strings[i].spawned_id != -1 && i != selected_line && !strings[i].locked && !strings[i].record_locked){
                DeleteObjectID(strings[i].spawned_id);
                strings[i].spawned_id = -1;
            }
        }
    }

    void RecordInput(const string &in new_string, int line, int last_wait) {
        if(new_string != strings[line].str){
            cast<PlaceholderObject@>(ReadObjectFromID(dialogue_obj_id)).SetUnsavedChanges(true);
            modified = true;
            if(strings[line].record_locked && last_wait > line){
                int spawned_id = strings[line].spawned_id;
                strings[line].spawned_id = -1;
                AddLine(new_string, last_wait+1);
                strings[last_wait+1].spawned_id = spawned_id;
                UpdateRecordLocked(IsRecording());
            } else {
                strings[line].str = new_string;
                strings[line].visible = true;
            }
            ExecutePreviousCommands(selected_line);
        }
    }

    int GetLastWait(int line) {
        int last_wait = -1;
        if( line >= 0 && line < int(strings.size())){
            for(int j=0; j<line; ++j){
                if(strings[j].obj_command == kWaitForClick || strings[j].obj_command == kSay){
                    last_wait = j;
                }
            }
        }
        return last_wait;
    }

    int GetNextWait(int line) {
        int last_wait = -1;
        for(int j=line, len=strings.size(); j<len; ++j){
            if(strings[j].obj_command == kWaitForClick || strings[j].obj_command == kSay){
                last_wait = j;
            }
        }
        return last_wait;
    }

    bool IsRecording() {
        if(selected_line > 0 && selected_line < int(strings.size())){
            return strings[selected_line].obj_command == kSay;
        } else {
            return false;
        }
    }

    void Update() {
        if(fade_out_end != -1.0){
            return;
        }

        if (mp_next == 1) {
        	mp_next = 0;
        	Log(info, "played it");
        	Play();
        }

        EnterTelemetryZone("Dialogue Update");
        if(history_str != ""){
            LoadHistoryStr();
        }
        if(index == 0){
            camera.SetFlags(kEditorCamera);
            if(clear_on_complete){
                ClearEditor();
                UpdatedQueue();
            }
        }

        UpdatedQueue();

        // Apply camera transform if dialogue has control
        if(has_cam_control){
            camera.SetXRotation(cam_rot.x);
            camera.SetYRotation(cam_rot.y);
            camera.SetZRotation(cam_rot.z);
            camera.SetPos(cam_pos);
            camera.SetDistance(0.0f);
            camera.SetFOV(cam_zoom);
            UpdateListener(cam_pos,vec3(0.0f),camera.GetFacing(),camera.GetUpVector());
            if(EditorModeActive()){
                SetGrabMouse(false);
            } else {
                SetGrabMouse(true);                
            }
        }

        if(level.WaitingForInput()){
            dialogue_text_disp_chars = 0.0f;
            speak_sound_time = the_time + 0.1f;
            line_start_time = 0.0f;
            ready_to_skip = false; 
        } else if(the_time - start_time > 0.1f && !GetInputDown(controller_id, "skip_dialogue") && !GetInputDown(controller_id, "keypadenter")){
            ready_to_skip = true;
        }

        // Progress dialogue one character at a time
        if(waiting_for_dialogue && index != 0){
            float step = time_step * 40.0f / GetConfigValueFloat("global_time_scale_mult");
            dialogue_text_disp_chars += step;
            if(GetInputDown(controller_id, "attack")){
                dialogue_text_disp_chars += step;                
            }
            // Continue dialogue script if we have displayed all the text that we are waiting for
            if(uint32(dialogue_text_disp_chars) >= dialogue_text.length()){
                waiting_for_dialogue = false;
                Play();   
            }
            if(speak_sound_time < the_time && has_cam_control){
                PlayLineContinueSound();
                speak_sound_time = the_time + 0.1 * GetConfigValueFloat("global_time_scale_mult");
            }
        }

        if(voice_preview_time > the_time && speak_sound_time < the_time){
            PlayLineContinueSound();
            speak_sound_time = the_time + 0.1;
        }

        // Continue dialogue script if waiting time has completed
        if(is_waiting_time){
            wait_time -= time_step;
            if(GetInputDown(controller_id, "attack")){
                wait_time -= time_step;
            }
            if(wait_time <= 0.0f){
                is_waiting_time = false;
                Play();
            }
        }

        if(queue_skip && dialogue_obj_id != -1){
            Play();
        }
        if (!Online_IsClient()) {
            if(SkipKeyDown() && dialogue_obj_id != -1){
                fade_out_start = the_time;
                fade_out_end = the_time + 0.1f;
                fade_in_start = the_time + 0.1f;
                fade_in_end = the_time + 0.2f;
                queue_skip = true;
                SendSkipState();
                PlayLineStartSound();
            }
            if(GetInputPressed(controller_id, "attack") && start_time != the_time){
                if(index != 0){
                    if(waiting_for_dialogue || is_waiting_time){
                        while(waiting_for_dialogue || is_waiting_time){
                            dialogue_text_disp_chars = dialogue_text.length();
                            waiting_for_dialogue = false;
                            is_waiting_time = false;
                            wait_time = 0.0f;
                            SendNextState(index, sub_index);
                            Play();
                        }
                    } else if(line_start_time < the_time - 0.5){
                        if(GetNextWait(index+1) != -1){

                            SendNextState(index, sub_index);
                            Play();
                        } else {
                            fade_out_start = the_time;
                            fade_out_end = the_time + 0.1f;
                            fade_in_start = the_time + 0.1f;
                            fade_in_end = the_time + 0.2f;
                            queue_skip = true;

                            SendSkipState();
                            PlayLineStartSound();
                        }
                    }  else {
                        line_start_time = -1.0;
                    }
                    PlayLineStartSound();
                }
            }
        } else {
            // You're client
            if (mp_skip == 1) {
                fade_out_start = the_time;
                fade_out_end = the_time + 0.1f;
                fade_in_start = the_time + 0.1f;
                fade_in_end = the_time + 0.2f;
                queue_skip = true;
                waiting_for_dialogue = false;
                is_waiting_time = false;
                wait_time = 0.0f;
                PlayLineStartSound();
                Online_SetAvatarCameraAttachedMode(true);
                mp_skip = 0;
                Play();
            }
        }

        int last_wait = GetLastWait(selected_line);

        if(show_editor_info && dialogue_obj_id != -1){
            UpdateConnectedChars();
            EnterTelemetryZone("Apply editor object transforms");
            // Apply editor object transforms to scripts
            for(int i=0; i<int(strings.size()); ++i){
                switch(strings[i].obj_command){
                case kCamera:
                    if(strings[i].spawned_id != -1){
                        Object@ obj = ReadObjectFromID(strings[i].spawned_id);
                        vec3 pos = obj.GetTranslation();
                        vec4 v = obj.GetRotationVec4();
                        quaternion rot(v.x,v.y,v.z,v.a);
                        // Set camera euler angles from rotation matrix
                        vec3 front = Mult(rot, vec3(0,0,1));
                        float y_rot = atan2(front.x, front.z)*180.0f/MPI;
                        float x_rot = asin(front[1])*-180.0f/MPI;
                        vec3 up = Mult(rot, vec3(0,1,0));
                        vec3 expected_right = normalize(cross(front, vec3(0,1,0)));
                        vec3 expected_up = normalize(cross(expected_right, front));
                        float z_rot = atan2(dot(up,expected_right), dot(up, expected_up))*180.0f/MPI;            
                        const float zoom_sensitivity = 3.5f;
                        float zoom = min(150.0f, 90.0f / max(0.001f,(1.0f+(obj.GetScale().x-1.0f)*zoom_sensitivity)));
                        strings[i].params[0] = pos.x;
                        strings[i].params[1] = pos.y;
                        strings[i].params[2] = pos.z;
                        strings[i].params[3] = floor(x_rot*100.0f+0.5f)/100.0f;
                        strings[i].params[4] = floor(y_rot*100.0f+0.5f)/100.0f;
                        strings[i].params[5] = floor(z_rot*100.0f+0.5f)/100.0f;
                        strings[i].params[6] = zoom;
                        
                        string new_string = CreateStringFromParams(strings[i].obj_command, strings[i].params);
                        RecordInput(new_string, i, last_wait);
                    }
                    break;
                case kHeadTarget:
                    if(strings[i].spawned_id != -1){
                        Object@ obj = ReadObjectFromID(strings[i].spawned_id);
                        vec3 pos = obj.GetTranslation();
                
                        float scale = obj.GetScale().x;
                        if(scale < 0.1f){
                            obj.SetScale(vec3(0.1f));
                        }
                        if(scale > 0.35f){
                            obj.SetScale(vec3(0.35f));
                        }
                        float zoom = (obj.GetScale().x - 0.1f) * 4.0f;
                        
                        strings[i].params[1] = pos.x;
                        strings[i].params[2] = pos.y;
                        strings[i].params[3] = pos.z;
                        strings[i].params[4] = zoom;
                        
                        string new_string = CreateStringFromParams(strings[i].obj_command, strings[i].params);
                        RecordInput(new_string, i, last_wait);

                        int char_id = GetDialogueCharID(atoi(strings[i].params[0]));
                        if(char_id != -1){
                            if(MovementObjectExists(char_id)) {
                                MovementObject@ mo = ReadCharacterID(char_id);
                                DebugDrawLine(mo.position, pos, vec4(vec3(1.0), 0.1), vec4(vec3(1.0), 0.1), _delete_on_update);
                            }
                        }
                    }
                    break;
                case kChestTarget:
                    if(strings[i].spawned_id != -1){
                        Object@ obj = ReadObjectFromID(strings[i].spawned_id);
                        vec3 pos = obj.GetTranslation();
                
                        float scale = obj.GetScale().x;
                        if(scale < 0.1f){
                            obj.SetScale(vec3(0.1f));
                        }
                        if(scale > 0.35f){
                            obj.SetScale(vec3(0.35f));
                        }
                        float zoom = (obj.GetScale().x - 0.1f) * 4.0f;
                        
                        strings[i].params[1] = pos.x;
                        strings[i].params[2] = pos.y;
                        strings[i].params[3] = pos.z;
                        strings[i].params[4] = zoom;
                        
                        string new_string = CreateStringFromParams(strings[i].obj_command, strings[i].params);
                        
                        RecordInput(new_string, i, last_wait);

                        int char_id = GetDialogueCharID(atoi(strings[i].params[0]));
                        if(char_id != -1){
                            if(MovementObjectExists(char_id)) {
                                MovementObject@ mo = ReadCharacterID(char_id);
                                DebugDrawLine(mo.position, pos, vec4(vec3(1.0), 0.1), vec4(vec3(1.0), 0.1), _delete_on_update);
                            }
                        }
                    }
                    break;
                case kEyeTarget:
                    if(strings[i].spawned_id != -1){
                        Object@ obj = ReadObjectFromID(strings[i].spawned_id);
                        vec3 pos = obj.GetTranslation();

                        float scale = obj.GetScale().x;
                        if(scale < 0.05f){
                            obj.SetScale(vec3(0.05f));
                        }
                        if(scale > 0.1f){
                            obj.SetScale(vec3(0.1f));
                        }

                        if(obj.IsSelected()) {
                            if(strings[i].params.size() != 5){
                                strings[i].params.resize(5);
                            }
                            float blink_mult = (obj.GetScale().x-0.05f)/0.05f;

                            strings[i].params[1] = pos.x;
                            strings[i].params[2] = pos.y;
                            strings[i].params[3] = pos.z;
                            strings[i].params[4] = blink_mult;

                            string new_string = CreateStringFromParams(strings[i].obj_command, strings[i].params);

                            RecordInput(new_string, i, last_wait);
                        }

                        int char_id = GetDialogueCharID(atoi(strings[i].params[0]));
                        if(char_id != -1){
                            if(MovementObjectExists(char_id) ) {
                                MovementObject@ mo = ReadCharacterID(char_id);
                                DebugDrawLine(mo.position, pos, vec4(vec3(1.0), 0.1), vec4(vec3(1.0), 0.1), _delete_on_update);
                            }                         
                        }
                    }
                    break;
                case kCharacter:
                    if(strings[i].spawned_id != -1){
                        Object@ obj = ReadObjectFromID(strings[i].spawned_id);
                        vec3 pos = obj.GetTranslation();
                        vec4 v = obj.GetRotationVec4();
                        quaternion quat(v.x,v.y,v.z,v.a);
                        vec3 facing = Mult(quat, vec3(0,0,1));
                        float rot = atan2(facing.x, facing.z)*180.0f/MPI;
                        obj.SetRotation(quaternion(vec4(0,1,0,rot*MPI/180.0f)));
                        
                        strings[i].params[1] = pos.x;
                        strings[i].params[2] = pos.y;
                        strings[i].params[3] = pos.z;
                        strings[i].params[4] = floor(rot+0.5f);
                        
                        string new_string = CreateStringFromParams(strings[i].obj_command, strings[i].params);
                        
                        RecordInput(new_string, i, last_wait);
                    }
                    break;
                }
            }
            //SaveScriptToParams();
            LeaveTelemetryZone(); // editor object transforms
        }
        LeaveTelemetryZone(); // dialogue update
    }

    void ExecutePreviousCommands(int id) {
        show_dialogue = false;
        if(id >= int(strings.size())) {
            return;
        }
        for(int i=0; i<=id; ++i){
            ExecuteScriptElement(strings[i], kInEditor);
        }
    }
    
    
    void ParseSelectedCharMessage(ScriptElement &se, const string &in msg){    
        TokenIterator token_iter;
        token_iter.Init();
        if(!token_iter.FindNextToken(msg)){
            return;
        }
        string token = token_iter.GetToken(msg);
        if(token == "set_head_target"){
            se.obj_command = kHeadTarget;
            const int kNumParams = 4;
            int old_param_size = se.params.size();
            int new_param_size = old_param_size+kNumParams;
            se.params.resize(new_param_size);
            for(int i=old_param_size; i<new_param_size; ++i){
                token_iter.FindNextToken(msg);
                se.params[i] = token_iter.GetToken(msg);
            }
        } else if(token == "set_torso_target"){
            se.obj_command = kChestTarget;
            const int kNumParams = 4;
            int old_param_size = se.params.size();
            int new_param_size = old_param_size+kNumParams;
            se.params.resize(new_param_size);
            for(int i=old_param_size; i<new_param_size; ++i){
                token_iter.FindNextToken(msg);
                se.params[i] = token_iter.GetToken(msg);
            }
        } else if(token == "set_eye_dir"){
            se.obj_command = kEyeTarget;
            int old_param_size = se.params.size();
            while(token_iter.FindNextToken(msg)){
                se.params.push_back(token_iter.GetToken(msg));
            }
        } else if(token == "set_animation"){
            se.obj_command = kSetAnimation;
            const int kNumParams = 1;
            int old_param_size = se.params.size();
            int new_param_size = old_param_size+kNumParams;
            se.params.resize(new_param_size);
            for(int i=old_param_size; i<new_param_size; ++i){
                token_iter.FindNextToken(msg);
                se.params[i] = token_iter.GetToken(msg);
            }
        } else if(token == "set_morph_target"){
            se.obj_command = kSetMorphTarget;
            const int kNumParams = 4;
            int old_param_size = se.params.size();
            int new_param_size = old_param_size+kNumParams;
            se.params.resize(new_param_size);
            for(int i=old_param_size; i<new_param_size; ++i){
                token_iter.FindNextToken(msg);
                se.params[i] = token_iter.GetToken(msg);
            }
        }
    }

    // Fill script element from selected line string
    void ParseLine(ScriptElement &se){
        se.locked = false;
        se.obj_command = kUnknown;
        se.spawned_id = -1;
        string msg = se.str;        
        TokenIterator token_iter;
        token_iter.Init();
        if(!token_iter.FindNextToken(msg)){
            return;
        }
        string token = token_iter.GetToken(msg);
        if(token == "set_cam"){
            se.obj_command = kCamera;
            const int kNumParams = 7;
            se.params.resize(kNumParams);
            for(int i=0; i<kNumParams; ++i){
                token_iter.FindNextToken(msg);
                se.params[i] = token_iter.GetToken(msg);
            }
        } else if(token == "fade_to_black"){
            se.obj_command = kFadeToBlack;
            const int kNumParams = 1;
            se.params.resize(kNumParams);            
            for(int i=0; i<kNumParams; ++i){
                token_iter.FindNextToken(msg);
                se.params[i] = token_iter.GetToken(msg);
            }
        } else if(token == "set_character_pos"){
            se.obj_command = kCharacter;
            const int kNumParams = 5;
            se.params.resize(kNumParams);
            for(int i=0; i<kNumParams; ++i){
                token_iter.FindNextToken(msg);
                se.params[i] = token_iter.GetToken(msg);
            }
        } else if(token == "send_character_message"){
            se.params.resize(1);
            token_iter.FindNextToken(msg);
            se.params[0] = token_iter.GetToken(msg);
            token_iter.FindNextToken(msg);
            token = token_iter.GetToken(msg);
            ParseSelectedCharMessage(se, token);
        } else if(token == "wait"){
            se.obj_command = kWait;
            se.params.resize(1);
            token_iter.FindNextToken(msg);
            se.params[0] = token_iter.GetToken(msg);
        } else if(token == "wait_for_click"){
            se.obj_command = kWaitForClick;
            ParseSelectedCharMessage(se, token);
        } else if(token == "set_dialogue_visible"){
            se.obj_command = kDialogueVisible;
            se.params.resize(1);
            token_iter.FindNextToken(msg);
            se.params[0] = token_iter.GetToken(msg);            
        } else if(token == "set_dialogue_name"){
            se.obj_command = kDialogueName;
            se.params.resize(1);
            token_iter.FindNextToken(msg);
            se.params[0] = token_iter.GetToken(msg);            
        } else if(token == "set_dialogue_color"){
            se.obj_command = kDialogueColor;
            const int kNumParams = 4;
            se.params.resize(kNumParams);
            for(int i=0; i<kNumParams; ++i){
                token_iter.FindNextToken(msg);
                se.params[i] = token_iter.GetToken(msg);
            }                 
        } else if(token == "set_dialogue_voice"){
            se.obj_command = kDialogueVoice;
            se.params.resize(2);
            token_iter.FindNextToken(msg);
            se.params[0] = token_iter.GetToken(msg);        
            token_iter.FindNextToken(msg);
            se.params[1] = token_iter.GetToken(msg);         
        } else if(token == "set_dialogue_text"){
            se.obj_command = kSetDialogueText;
            se.params.resize(1);
            token_iter.FindNextToken(msg);
            se.params[0] = token_iter.GetToken(msg);            
        } else if(token == "say"){
            se.obj_command = kSay;
            const int kNumParams = 3;
            se.params.resize(kNumParams);
            for(int i=0; i<kNumParams; ++i){
                token_iter.FindNextToken(msg);
                se.params[i] = token_iter.GetToken(msg);
            }      
        } else if(token == "add_dialogue_text"){
            se.obj_command = kAddDialogueText;
            se.params.resize(1);
            token_iter.FindNextToken(msg);
            se.params[0] = token_iter.GetToken(msg);            
        } else if(token == "set_cam_control"){
            se.obj_command = kSetCamControl;
            se.params.resize(1);
            token_iter.FindNextToken(msg);
            se.params[0] = token_iter.GetToken(msg);            
        } else if(token == "#name"){
            if(token_iter.FindNextToken(msg)){
                potential_display_name = token_iter.GetToken(msg);
            }
        } else if(token == "#participants"){
            if(token_iter.FindNextToken(msg)) {
                int participant_count = atoi(token_iter.GetToken(msg));
                if(token_iter.GetToken(msg) == "0" || (participant_count > 0 && participant_count < 32)) {
                    Object @obj = ReadObjectFromID(dialogue_obj_id);
                    ScriptParams@ params = obj.GetScriptParams();
                    int previous_participant_count = params.GetInt("NumParticipants");
                    if(old_participant_count == -1) {
                        old_participant_count = previous_participant_count;
                    }
                    params.SetInt("NumParticipants", participant_count);
                    UpdateDialogueObjectConnectors(dialogue_obj_id);
                    if(previous_participant_count != participant_count) {
                        cast<PlaceholderObject@>(obj).SetUnsavedChanges(true);
                    }
                } 
            }
        }
    }

    int GetDialogueCharID(int id){
        Object@ obj = ReadObjectFromID(dialogue_obj_id);
        ScriptParams@ params = obj.GetScriptParams();
        if(!params.HasParam("obj_"+id)){
            Log(info, "Error: Dialogue object "+dialogue_obj_id+" does not have parameter \""+"obj_"+id+"\"");
            return -1;
        }
        int connector_id = params.GetInt("obj_"+id);
        if(!ObjectExists(connector_id)){
            Log(info,"Error: Connector does not exist");
            return -1;
        }
        Object@ connector_obj = ReadObjectFromID(connector_id);
        PlaceholderObject@ placeholder_object = cast<PlaceholderObject@>(connector_obj);
        int connect_id = placeholder_object.GetConnectID();
       
        if( connect_id == -1 ) {
            //Log(warning, "Connection id is -1 for placeholder object " + connector_id);
            return -1;
        }

        if(IsGroupDerived(connect_id)) {
            connect_id = FindFirstCharacterInGroup(connect_id);
        }

        if( MovementObjectExists( connect_id ) ) {
            return connect_id;
        } else {
            Log(warning, "There is no character with id " + connect_id + " to tie dialog to.");
            return -1; 
        }
    }

    void CreateEditorObj(ScriptElement@ se){
        switch(se.obj_command){
        case kCamera: {
            if(se.spawned_id == -1){
                se.spawned_id = CreateObject("Data/Objects/placeholder/camera_placeholder.xml", true);
            }
            vec3 pos(atof(se.params[0]), atof(se.params[1]), atof(se.params[2]));
            vec3 rot(atof(se.params[3]), atof(se.params[4]), atof(se.params[5]));
            float zoom = atof(se.params[6]);
            Object@ obj = ReadObjectFromID(se.spawned_id);
            obj.SetTranslation(pos);            
            float deg2rad = 3.14159265359/180.0f;
            quaternion rot_y(vec4(0,1,0,rot.y*deg2rad));
            quaternion rot_x(vec4(1,0,0,rot.x*deg2rad));
            quaternion rot_z(vec4(0,0,1,rot.z*deg2rad));
            obj.SetRotation(rot_y*rot_x*rot_z);            
            const float zoom_sensitivity = 3.5f;
            float scale = (90.0f / zoom - 1.0f) / zoom_sensitivity + 1.0f;
            obj.SetScale(vec3(scale));
            ScriptParams @params = obj.GetScriptParams();
            params.AddIntCheckbox("No Save", true);
            obj.SetCopyable(false);
            obj.SetDeletable(false);
            obj.SetSelectable(true);
            obj.SetTranslatable(true);
            obj.SetScalable(true);
            obj.SetRotatable(true);
            break;}
        case kCharacter: {
            if(se.spawned_id == -1){
                se.spawned_id = CreateObject("Data/Objects/placeholder/empty_placeholder.xml", true);
            }
            int char_id = atoi(se.params[0]);
            vec3 pos(atof(se.params[1]), atof(se.params[2]), atof(se.params[3]));
            float rot = floor(atof(se.params[4])+0.5f);
            Object@ obj = ReadObjectFromID(se.spawned_id);
            obj.SetTranslation(pos);
            obj.SetRotation(quaternion(vec4(0,1,0,rot*3.1415/180.0f)));
            ScriptParams @params = obj.GetScriptParams();
            params.AddIntCheckbox("No Save", true);
            obj.SetCopyable(false);
            obj.SetDeletable(false);
            obj.SetScalable(false);
            obj.SetSelectable(true);
            obj.SetTranslatable(true);
            obj.SetRotatable(true);
            break; }
        case kHeadTarget: {
            if(se.spawned_id == -1){
                se.spawned_id = CreateObject("Data/Objects/placeholder/empty_placeholder.xml", true);
            }
            int id = atoi(se.params[0]);
            vec3 pos(atof(se.params[1]), atof(se.params[2]), atof(se.params[3]));
            float zoom = atof(se.params[4]);
            Object@ obj = ReadObjectFromID(se.spawned_id);
            obj.SetTranslation(pos);
            obj.SetScale(zoom / 4.0f + 0.1f);
            PlaceholderObject@ placeholder_object = cast<PlaceholderObject@>(obj);
            placeholder_object.SetBillboard("Data/Textures/ui/head_widget.tga");
            ScriptParams @params = obj.GetScriptParams();
            params.AddIntCheckbox("No Save", true);
            obj.SetCopyable(false);
            obj.SetDeletable(false);
            obj.SetRotatable(false);
            obj.SetSelectable(true);
            obj.SetTranslatable(true);
            obj.SetScalable(true);
            break; }
        case kChestTarget: {
            if(se.spawned_id == -1){
                se.spawned_id = CreateObject("Data/Objects/placeholder/empty_placeholder.xml", true);
            }
            int id = atoi(se.params[0]);
            vec3 pos(atof(se.params[1]), atof(se.params[2]), atof(se.params[3]));
            float zoom = atof(se.params[4]);
            Object@ obj = ReadObjectFromID(se.spawned_id);
            obj.SetTranslation(pos);
            obj.SetScale(zoom / 4.0f + 0.1f);
            PlaceholderObject@ placeholder_object = cast<PlaceholderObject@>(obj);
            placeholder_object.SetBillboard("Data/Textures/ui/torso_widget.tga");
            ScriptParams @params = obj.GetScriptParams();
            params.AddIntCheckbox("No Save", true);
            obj.SetCopyable(false);
            obj.SetDeletable(false);
            obj.SetRotatable(false);
            obj.SetSelectable(true);
            obj.SetTranslatable(true);
            obj.SetScalable(true);
            break; }
        case kEyeTarget: {           
            if(se.spawned_id == -1){
                se.spawned_id = CreateObject("Data/Objects/placeholder/empty_placeholder.xml", true);
            }
            int id = atoi(se.params[0]);
            float blink_mult = 1.0f;    
            vec3 pos;
            if(se.params.size() == 5){
                pos = vec3(atof(se.params[1]), atof(se.params[2]), atof(se.params[3]));
                blink_mult = atof(se.params[4]);   
            } else {
                MovementObject @char = ReadCharacterID(GetDialogueCharID(id));
                mat4 head_mat = char.rigged_object().GetAvgIKChainTransform("head");
                pos = head_mat * vec4(0,0,0,1) - head_mat * vec4(0,1,0,0);
            }
            Object@ obj = ReadObjectFromID(se.spawned_id);        
            obj.SetTranslation(pos);
            obj.SetScale(0.05f+0.05f*blink_mult);
            PlaceholderObject@ placeholder_object = cast<PlaceholderObject@>(obj);
            placeholder_object.SetBillboard("Data/Textures/ui/eye_widget.tga");
            ScriptParams @params = obj.GetScriptParams();
            params.AddIntCheckbox("No Save", true);
            obj.SetCopyable(false);
            obj.SetDeletable(false);
            obj.SetRotatable(false);
            obj.SetSelectable(true);
            obj.SetTranslatable(true);
            obj.SetScalable(true);
            break; }
        }
    }

    void HandleSelectedString(int line){
        ExecutePreviousCommands(line);
        if(line < 0 || line >= int(strings.size())){
            return;
        }
        switch(strings[line].obj_command){
        case kCamera:
        case kCharacter:
        case kHeadTarget:
        case kChestTarget:
        case kEyeTarget: 
            CreateEditorObj(strings[line]);
            break;
        }
    }

    void AnalyzeForLineBreaks(string &inout str){
        TextMetrics metrics = GetTextAtlasMetrics(font_path, GetFontSize(), 0, dialogue_text);
        int font_size = GetFontSize();
        float threshold = GetScreenWidth() - kTextLeftMargin - font_size - kTextRightMargin;
        string final;
        string first_line = dialogue_text;
        string second_line;
        while(first_line.length() > 0){
            while(metrics.bounds_x > threshold){
                int last_space = first_line.findLastOf(" ");
                second_line.insert(0, first_line.substr(last_space));
                first_line.resize(last_space);
                metrics = GetTextAtlasMetrics(font_path, font_size, 0, first_line);
            }
            final += first_line + "\n";
            if(second_line.length() > 0){
                first_line = second_line.substr(1);
                second_line = "";
            } else {
                first_line = "";
            }
            metrics = GetTextAtlasMetrics(font_path, font_size, 0, first_line);
        }
        dialogue_text = final.substr(0, final.length()-1);
    }

    void ParseSayText(int player_id, const string &in str, ProcessStringType type){
        sub_strings.resize(0);
        SayParseState state = kContinue;
        bool done = false;
        string tokens = "[]";
        int str_len = int(str.length());
        int i = 0;
        int token_start = 0;
        while(!done){
            switch(state){
                case kInBracket:
                    if(i == str_len){
                        done = true;                        
                    } else if(str[i] == tokens[1]){  
                        int sub_str_id = int(sub_strings.size());
                        sub_strings.resize(sub_str_id+1);
                        sub_strings[sub_str_id].str = str.substr(token_start,i-token_start);
                        ParseLine(sub_strings[sub_str_id]);
                        state = kContinue;
                        token_start = i+1;
                    }
                    break;
                case kContinue: {
                    bool token_end = false;
                    if(i == str_len){
                        token_end = true;
                        done = true;
                    } else if(str[i] == tokens[0]){
                        token_end = true;  
                        state = kInBracket;
                    }
                    if(token_end){
                        {
                            int sub_str_id = int(sub_strings.size());
                            sub_strings.resize(sub_str_id+1);
                            sub_strings[sub_str_id].str = "send_character_message \""+player_id+" \"start_talking\"";
                            ParseLine(sub_strings[sub_str_id]);
                        }
                        {
                            int sub_str_id = int(sub_strings.size());
                            sub_strings.resize(sub_str_id+1);
                            sub_strings[sub_str_id].str = "add_dialogue_text \""+str.substr(token_start,i-token_start)+"\"";
                            ParseLine(sub_strings[sub_str_id]);
                        }
                        {
                            int sub_str_id = int(sub_strings.size());
                            sub_strings.resize(sub_str_id+1);
                            sub_strings[sub_str_id].str = "send_character_message \""+player_id+" \"stop_talking\"";
                            ParseLine(sub_strings[sub_str_id]);
                        }
                        token_start = i+1;
                    }
                    break; }
            }
            ++i;
        }
        
        int sub_str_id = int(sub_strings.size());
        sub_strings.resize(sub_str_id+1);
        sub_strings[sub_str_id].str = "wait_for_click";
        ParseLine(sub_strings[sub_str_id]);

        dialogue_text = "";
        dialogue_text_disp_chars = 0;
        line_start_time = the_time;
    }

    bool ExecuteScriptElement(const ScriptElement &in script_element, ProcessStringType type) { 
        switch(script_element.obj_command){
        case kSetCamControl:
            if(type == kInGame){
                if(script_element.params[0] == "true"){
                    has_cam_control = true;
                } else if(script_element.params[0] == "false"){
                    has_cam_control = false;
                }
            }
            break;       
        case kCamera:
            if(type == kInGame){
                // Set camera control
                has_cam_control = true;
            }
            // Set camera position
            cam_pos.x = atof(script_element.params[0]);
            cam_pos.y = atof(script_element.params[1]);
            cam_pos.z = atof(script_element.params[2]);
            // Set camera rotation
            cam_rot.x = atof(script_element.params[3]);
            cam_rot.y = atof(script_element.params[4]);
            cam_rot.z = atof(script_element.params[5]);
            // Set camera zoom
            cam_zoom = atof(script_element.params[6]);
            break;
        case kWait:
            if(type == kInGame){
                is_waiting_time = true;
                wait_time = atof(script_element.params[0]);
            }
            return true;
        case kFadeToBlack:
            fade_start = the_time;
            fade_end = the_time + atof(script_element.params[0]);
            break;
        case kDialogueVisible:
            if(script_element.params[0] == "true"){
                show_dialogue = true;
            } else if(script_element.params[0] == "false"){
                show_dialogue = false;
            }
            break;
        case kSetDialogueText:
            dialogue_text = script_element.params[0];
            dialogue_text_disp_chars = 0;
            line_start_time = the_time;
            if(type == kInGame){
                waiting_for_dialogue = true;
            } else { 
                dialogue_text_disp_chars = dialogue_text.length();
            }
            return true;
        case kDialogueColor: {
            int id = atoi(script_element.params[0])-1;
            if(id >= 0){
                if(id >= int(dialogue_colors.size())){
                    dialogue_colors.resize(id+1);
                }
                dialogue_colors[id] = vec3(
                    atof(script_element.params[1]),
                    atof(script_element.params[2]),
                    atof(script_element.params[3]));
            } }
            break;
        case kDialogueVoice: {
            int id = atoi(script_element.params[0])-1;
            if(id >= 0){
                if(id >= int(dialogue_voices.size())){
                    dialogue_voices.resize(id+1);
                }
                dialogue_voices[id] = atoi(script_element.params[1]);
            } }
            break;
        case kSay:
            if(sub_index == -1 || type == kInEditor){
                active_char = atoi(script_element.params[0]) - 1;
                show_dialogue = true;
                dialogue_name = script_element.params[1];
                ParseSayText(atoi(script_element.params[0]), script_element.params[2], type);
                sub_index = 0;
                if(type == kInEditor){
                    while(sub_index < int(sub_strings.size())){
                        ExecuteScriptElement(sub_strings[sub_index++], type);
                    }
                    sub_index = -1;
                }
            } else {
                if(sub_index >= int(sub_strings.size())){
                    sub_index = -1;
                } else {
                    return ExecuteScriptElement(sub_strings[sub_index++], type);
                }
            }
            break;
        case kAddDialogueText:
            dialogue_text += script_element.params[0];
            AnalyzeForLineBreaks(dialogue_text);
            if(type == kInGame){
                waiting_for_dialogue = true;
            } else { 
                dialogue_text_disp_chars = dialogue_text.length();
            }
            return true;
        case kDialogueName:
            dialogue_name = script_element.params[0];
            break;
        case kCharacter: {
            vec3 pos;
            pos.x = atof(script_element.params[1]);
            pos.y = atof(script_element.params[2]);
            pos.z = atof(script_element.params[3]);
            float rot = atof(script_element.params[4]);
            int char_id = GetDialogueCharID(atoi(script_element.params[0]));
            if(char_id != -1){
                if(MovementObjectExists(char_id)) {
                    MovementObject@ mo = ReadCharacterID(char_id);

                    // When Online, move every player movementobject if one of them is moved inside dialogue
                    if (Online_IsHosting() && mo.is_player) {
                        array<int> player_ids = GetControlledCharacterIDs();
                        for(uint i = 0; i < player_ids.length(); i++) {
                            MovementObject@ player_mo = ReadCharacter(player_ids[i]);
                            player_mo.ReceiveScriptMessage("set_rotation "+rot);
                            player_mo.ReceiveScriptMessage("set_dialogue_position "+pos.x+" "+pos.y+" "+pos.z);
                        }
                    }

                    mo.ReceiveScriptMessage("set_rotation "+rot);
                    mo.ReceiveScriptMessage("set_dialogue_position "+pos.x+" "+pos.y+" "+pos.z);
                } else {
                    Log(error, "No movement object for " + char_id);
                }
            }
            break; }
        case kWaitForClick:
            return true;
        default: { 
            TokenIterator token_iter;
            token_iter.Init();
            if(!token_iter.FindNextToken(script_element.str)){
                return false;
            }
            string token = token_iter.GetToken(script_element.str);
            if(token == "send_character_message"){
                token_iter.FindNextToken(script_element.str);
                token = token_iter.GetToken(script_element.str);
                int id = atoi(token);
                int char_id = GetDialogueCharID(id);
                if(char_id != -1){
                    if(MovementObjectExists(char_id)) {
                        MovementObject@ mo = ReadCharacterID(char_id);
                        token_iter.FindNextToken(script_element.str);
                        token = token_iter.GetToken(script_element.str);
                        mo.ReceiveScriptMessage(token);
                    } else {
                        Log(error, "No movement object exists with id: " + char_id);
                    }
                }
            }
            if(!EditorModeActive()){
                if(token == "send_level_message"){
                    token_iter.FindNextToken(script_element.str);
                    token = token_iter.GetToken(script_element.str);
                    level.SendMessage(token);
                }
                if(token == "send_global_message") {
                    token_iter.FindNextToken(script_element.str);
                    token = token_iter.GetToken(script_element.str);
                    SendGlobalMessage(token);
                }
            } else {
                if(token == "send_level_message"){
                    token_iter.FindNextToken(script_element.str);
                    token = token_iter.GetToken(script_element.str);

                    TokenIterator level_token_iter;
                    level_token_iter.Init();
                    if(level_token_iter.FindNextToken(token)){
                        string sub_msg = level_token_iter.GetToken(token);
                        if(sub_msg == "set_camera_dof"){
                            dof_params.resize(0);
                            while(level_token_iter.FindNextToken(token)){
                                dof_params.push_back(atof(level_token_iter.GetToken(token)));
                            }
                        }
                    }
                }
            }
            if(token == "make_participants_aware"){
                Object @obj = ReadObjectFromID(dialogue_obj_id);
                ScriptParams@ params = obj.GetScriptParams();
                int num_participants = min(kMaxParticipants, params.GetInt("NumParticipants"));
                Log(info,"make_participants_aware");
                for(int i=0; i<num_participants; ++i){
                    int id_a = GetDialogueCharID(i+1);
                    if(id_a != -1){
                        Log(info,"id_a: "+id_a);
                        if( MovementObjectExists(id_a) ){
                            MovementObject@ mo_a = ReadCharacterID(id_a);
                            if(!mo_a.controlled){
                                mo_a.ReceiveScriptMessage("set_omniscient true");
                            }
                        } else {
                            Log(error, "Unable to handle " + script_element.str + " object id: " + id_a + " is invalid" );
                        }
                    }
                }
            }
            break;
            }
        }
        return false;
    }
    
    void UpdateDialogueObjectConnectors(int id){
        Object @obj = ReadObjectFromID(id);
        ScriptParams@ params = obj.GetScriptParams();
        int num_connectors = params.GetInt("NumParticipants");

        for(int j=1; j<=num_connectors; ++j){
            if(!params.HasParam("obj_"+j)){
                int obj_id = CreateObject("Data/Objects/placeholder/empty_placeholder.xml", false);
                params.AddInt("obj_"+j, obj_id);
                Object@ object = ReadObjectFromID(obj_id);
                object.SetSelectable(true);
                PlaceholderObject@ inner_placeholder_object = cast<PlaceholderObject@>(object);
                inner_placeholder_object.SetSpecialType(kPlayerConnect);
            }
            int obj_id = params.GetInt("obj_"+j);
            if(ObjectExists(obj_id)){
                Object @new_obj = ReadObjectFromID(obj_id);
                vec4 v = obj.GetRotationVec4();
                quaternion quat(v.x,v.y,v.z,v.a);
                new_obj.SetTranslation(obj.GetTranslation() + Mult(quat,vec3((num_connectors*0.5f+0.5f-j)*obj.GetScale().x*0.35f,obj.GetScale().y*(0.5f+0.2f),0)));
                new_obj.SetRotation(quat);
                new_obj.SetScale(obj.GetScale()*0.3f);
                new_obj.SetEditorLabel(""+j);
                new_obj.SetEditorLabelScale(6);
                new_obj.SetEditorLabelOffset(vec3(0));

                new_obj.SetCopyable(false);
                new_obj.SetDeletable(false);

                PlaceholderObject@ placeholder_object = cast<PlaceholderObject@>(new_obj);
                if( placeholder_object !is null ) {
                    placeholder_object.SetEditorDisplayName("Dialogue \""+params.GetString("DisplayName")+"\" Connector "+j);
                } else {
                    Log(error, "dialogue place holder object  id " + obj_id + " isn't a placeholder object");
                }
            } else {
                params.Remove("obj_"+j);
            }
        }
        for(int j=num_connectors+1; j<=kMaxParticipants; ++j){
            if(params.HasParam("obj_"+j)){
                int obj_id = params.GetInt("obj_"+j);
                if(ObjectExists(obj_id)){
                    DeleteObjectID(obj_id);
                }
                params.Remove("obj_"+j);
            }
        }
    }

    void MovedObject(int id){
        if(id == -1){
            return;
        }
        Object @obj = ReadObjectFromID(id);
        if(obj.GetType() != _placeholder_object){
            return;
        }
        UpdateScriptFromStrings();
        ScriptParams@ params = obj.GetScriptParams();
        if(!params.HasParam("Dialogue")){
            return;
        }
        UpdateDialogueObjectConnectors(id);
    }

    void AddedObject(int id){
        if(id == -1){
            return;
        }
        if(!ObjectExists(id)) {  // Mostly happens during undo/redo spam
            return;
        }
        Object @obj = ReadObjectFromID(id);
        if(obj.GetType() != _placeholder_object){
            return;
        }
        ScriptParams@ params = obj.GetScriptParams();
        if(!params.HasParam("Dialogue")){
            return;
        }
        // Object @obj is a dialogue object
        obj.SetCopyable(false);
        PlaceholderObject@ placeholder_object = cast<PlaceholderObject@>(obj);
        placeholder_object.SetBillboard("Data/Textures/ui/dialogue_widget.tga");
        if(!params.HasParam("DisplayName") || !params.HasParam("NumParticipants")){
			params.SetString("DisplayName", "Unnamed");
			params.SetInt("NumParticipants", 1);
        }
        obj.SetEditorLabelOffset(vec3(0,1.25,0)); 
        obj.SetEditorLabelScale(10);
        obj.SetEditorLabel(params.GetString("DisplayName"));
        cast<PlaceholderObject@>(placeholder_object).SetEditorDisplayName("Dialogue \""+params.GetString("DisplayName")+"\"");
        UpdateDialogueObjectConnectors(id);
    }

    void UpdatedQueue(){
        if(dialogue_queue.size() > 0 && dialogue_obj_id == -1){
            string str = dialogue_queue[0];
            dialogue_queue.removeAt(0);
            StartDialogue(str);

            Update();
            camera.FixDiscontinuity();
        }
    }

    void ReceiveMessage(const string &in msg) {
        TokenIterator token_iter;
        token_iter.Init();
        if(!token_iter.FindNextToken(msg)){
            return;
        }
        string token = token_iter.GetToken(msg);
        if(token == "notify_deleted"){
		    token_iter.FindNextToken(msg);
            NotifyDeleted(atoi(token_iter.GetToken(msg)));
        } else if(token == "skip_dialogue"){
            skip_dialogue = true;
        } else if(token == "added_object"){
            token_iter.FindNextToken(msg);
            int obj_id = atoi(token_iter.GetToken(msg));
            AddedObject(obj_id);
        } else if(token == "moved_objects"){
            while(token_iter.FindNextToken(msg)){
                int obj_id = atoi(token_iter.GetToken(msg));
                MovedObject(obj_id);
            }
        } else if(token == "stop_editing_dialogue") {
            DeactivateKeyboardEvents();
            ClearEditor();
        } else if(token == "edit_selected_dialogue"){
            ActivateKeyboardEvents();
            array<int> @object_ids = GetObjectIDs();
            int num_objects = object_ids.length();
            for(int i=0; i<num_objects; ++i){
                if(!ObjectExists(object_ids[i])){ // This is needed because SetDialogueObjID can delete some objects
                    continue;
                }
                Object @obj = ReadObjectFromID(object_ids[i]);
                ScriptParams@ params = obj.GetScriptParams();
                if(obj.IsSelected() && obj.GetType() == _placeholder_object && params.HasParam("Dialogue")){
                    dialogue.SetDialogueObjID(object_ids[i]);
                    show_editor_info = true;
                }
            }
        } else if(token.findFirst("edit_dialogue_id") != -1) {
            Log(info, "Got edit_dialogue_id");
            int id = parseInt(token.substr(16));
            Log(info, "id is " + id);
            Object @obj = ReadObjectFromID(id);
            if(obj !is null) {
                ScriptParams@ params = obj.GetScriptParams();
                if(obj.GetType() == _placeholder_object && params.HasParam("Dialogue")){
                    dialogue.SetDialogueObjID(id);
                    show_editor_info = true;
                }
            }
        } else if(token == "preview_dialogue"){
            DebugText("preview_dialogue", "preview_dialogue", 0.5f);
            if(index == 0 && dialogue_obj_id != -1){
                //camera.SetFlags(kPreviewCamera);
                //SetGUIEnabled(false);
                preview = true;
                clear_on_complete = true;
                Play();
            }
        } else if(token == "request_preview_dof"){
            if(dof_params.size() == 6){
                camera.SetDOF(dof_params[0], dof_params[1], dof_params[2], dof_params[3], dof_params[4], dof_params[5]);
            } else {
                camera.SetDOF(0,0,0,0,0,0);
            }
        }  else if(token == "load_dialogue_pose"){
		    token_iter.FindNextToken(msg);
            string path = token_iter.GetToken(msg);
            
            int id = -1;
            int num_strings = int(strings.size());
            for(int i=0; i<num_strings; ++i){
                if(strings[i].obj_command == kCharacter && strings[i].spawned_id != -1 && ReadObjectFromID(strings[i].spawned_id).IsSelected()){
                    id = atoi(strings[i].params[0]);
                    break;
                }
            }

            if(id != -1){
                array<string> str_params;
                str_params.resize(2);
                str_params[0] = id;
                str_params[1] = path;

                int last_wait = GetLastWait(selected_line);
                int last_set_animation = -1;
                for(int i=selected_line; i>=0; --i){
                    if(strings[i].obj_command == kSetAnimation && strings[i].params[0] == str_params[0]){
                        last_set_animation = i;
                        break;
                    }
                }
                if(last_set_animation == -1 || last_set_animation < last_wait){
                    AddLine(CreateStringFromParams(kSetAnimation, str_params), last_wait+1);
                } else {
                    strings[last_set_animation].str = CreateStringFromParams(kSetAnimation, str_params);
                    ParseLine(strings[last_set_animation]);
                    strings[last_set_animation].visible = true;
                }
                UpdateScriptFromStrings();
            
                ExecutePreviousCommands(selected_line);
            }
        } else if(token == "save_selected_dialogue") {
            SaveScript();
        }
    }

    bool HasCameraControl() {
        return has_cam_control;
    }

    void Display() {
        if(MediaMode()){
            return;
        }

        // This needs to be here in case the "Save dialogue?" popup needs to be
        // drawn
        if(queued_id != -1) {
            bool skip_dialogue = false;
            if(dialogue_obj_id != -1) {
                skip_dialogue = !modified;
            } else {
                skip_dialogue = true;
            }

            if(!skip_dialogue && !ImGui_IsPopupOpen("savedialoguepopup")) {
                ImGui_OpenPopup("savedialoguepopup");
            }
            SavePopupValue save_popup_value = SavePopup();

            if(skip_dialogue || save_popup_value != NONE) {
                SetActiveDialogue(queued_id);
                queued_id = -1;
            }
        }

        if(show_dialogue && (camera.GetFlags() == kPreviewCamera || has_cam_control)){
            int font_size = GetFontSize();
            EnterTelemetryZone("Draw text background");
            vec3 color = vec3(1.0);
            if(NameInfoExists(dialogue_name)){
                color = GetNameInfo(dialogue_name).color;
            } else if(active_char >= 0 && int(dialogue_colors.size()) > active_char){
                color = dialogue_colors[active_char];
            } else {
                color = vec3(1.0f);
            }
            if(fade_end != -1.0){        
                float blackout_amount = 1.0 - ((fade_end - the_time) / (fade_end - fade_start));
                HUDImage @blackout_image = hud.AddImage();
                blackout_image.SetImageFromPath("Data/Textures/diffuse.tga");
                blackout_image.position.y = (GetScreenWidth() + GetScreenHeight())*-1.0f;
                blackout_image.position.x = (GetScreenWidth() + GetScreenHeight())*-1.0f;
                blackout_image.position.z = -2.0f;
                blackout_image.scale = vec3(GetScreenWidth() + GetScreenHeight())*2.0f;
                blackout_image.color = vec4(0.0f,0.0f,0.0f,blackout_amount);
            } else {
                float height_scale = 1.0/75.0;
                float vert_size = (font_size * 6.8) / 512.0;
                {
                    HUDImage @blackout_image = hud.AddImage();
                    blackout_image.SetImageFromPath("Data/Textures/ui/dialogue/dialogue_bg.png");
                    blackout_image.position.y = GetScreenHeight() * 0.25 - font_size * height_scale * 510.0;
                    blackout_image.position.x = GetScreenWidth()*0.2;
                    blackout_image.position.z = -2.0f;
                    blackout_image.scale = vec3(GetScreenWidth()/32.0f*0.6, vert_size, 1.0f);
                    blackout_image.color = vec4(color,0.7f);
                }

                {
                    HUDImage @blackout_image = hud.AddImage();
                    blackout_image.SetImageFromPath("Data/Textures/ui/dialogue/dialogue_bg-fade.png");
                    blackout_image.position.y = GetScreenHeight() * 0.25 - font_size * height_scale * 510.0;
                    blackout_image.position.z = -2.0f;
                    float width_scale = GetScreenWidth()/2500.0;
                    blackout_image.position.x = GetScreenWidth()*0.2-512*width_scale;
                    blackout_image.scale = vec3(width_scale, vert_size, 1.0f);
                    blackout_image.color = vec4(color,0.7f);
                }

                {
                    HUDImage @blackout_image = hud.AddImage();
                    blackout_image.SetImageFromPath("Data/Textures/ui/dialogue/dialogue_bg-fade_reverse.png");
                    blackout_image.position.y = GetScreenHeight() * 0.25 - font_size * height_scale * 510.0;
                    blackout_image.position.z = -2.0f;
                    float width_scale = GetScreenWidth()/2500.0;
                    blackout_image.position.x = GetScreenWidth()*0.8;
                    blackout_image.scale = vec3(width_scale, vert_size, 1.0f);
                    blackout_image.color = vec4(color,0.7f);
                }

                {
                    HUDImage @blackout_image = hud.AddImage();
                    TextMetrics metrics = GetTextAtlasMetrics(name_font_path, int(GetFontSize()*1.8), kSmallLowercase, dialogue_name);
            
                    blackout_image.SetImageFromPath("Data/Textures/ui/menus/main/brushStroke.png");
                    blackout_image.position.y = GetScreenHeight() * 0.25 - font_size * 1.5;
                    blackout_image.position.x = kTextLeftMargin - font_size * 2;
                    blackout_image.position.z = -2.0f;
                    blackout_image.scale = vec3((metrics.bounds_x+font_size*4)/768.0, font_size/40.0, 1.0f);
                    blackout_image.color = vec4(vec3(0.15),1.0f);
                }
            }
            LeaveTelemetryZone();
        }

        // Draw editor text
        if(dialogue_obj_id != -1 && show_editor_info && !has_cam_control && EditorModeActive()){
            Object @obj = ReadObjectFromID(dialogue_obj_id);
            ScriptParams @params = obj.GetScriptParams();
            PlaceholderObject@ placeholder_obj = cast<PlaceholderObject@>(obj);

            ImGui_SetNextWindowSize(vec2(600.0f, 300.0f), ImGuiSetCond_FirstUseEver);
            ImGui_Begin("Dialogue Editor - " + params.GetString("DisplayName") + (modified ? "*" : "") + "###Dialogue Editor", show_editor_info, ImGuiWindowFlags_MenuBar);
            if(ImGui_BeginMenuBar()){
                if(ImGui_BeginMenu("File")){
                    if(ImGui_MenuItem("Save")){
                        SaveScript();
                    }
                    if(ImGui_MenuItem("Save to file")){
                        full_path = GetUserPickedWritePath("txt", "Data/Dialogues");
                        if(full_path != ""){
                            SaveScript();
                        }
                    }
                    if(ImGui_MenuItem("Save inline")){
                        params.SetString("Dialogue", "empty");
                        SaveScript();
                    }
                    ImGui_EndMenu();
                }
                if(ImGui_BeginMenu("Edit")){
                    if(ImGui_MenuItem("Preview")){
                        ReceiveMessage("preview_dialogue");
                    }
                    if(ImGui_MenuItem("Load Pose")){
                        string path = GetUserPickedReadPath("anm", "Data/Animations");
                        if(path != ""){
                            ReceiveMessage("load_dialogue_pose \""+path+"\"");
                        }
                    }
                    if(ImGui_BeginMenu("Preview Voice")){
                        if(ImGui_DragInt("Voice", voice_preview, 0.1, 0, 18)){
                            voice_preview_time = the_time + 1.0;
                            PlayLineStartSound();
                        }
                        ImGui_EndMenu();
                    }
                    ImGui_EndMenu();
                }
                ImGui_EndMenuBar();
            }

            SavePopupValue popup_value = SavePopup();
            if(popup_value != NONE) {
                modified = false;
                show_editor_info = false;
            }
            if(!show_editor_info){ // User closed dialogue editor window
                if(modified) {
                    ImGui_OpenPopup("savedialoguepopup");
                    show_editor_info = true;
                } else {
                    ClearEditor();
                }
            } else {
                if(invalid_file) {
                    ImGui_Text("Level xml contained invalid file, dialogue will be saved inline");
                } else {
                    if(params.GetString("Dialogue") == "empty") {
                        ImGui_Text("Not saving to file");
                        if(ImGui_IsItemHovered()) {
                            ImGui_SetTooltip("Consider saving to file to allow translations to be made");
                        }
                    }
                }
                //UpdateScriptFromStrings();
                int new_cursor_pos = imgui_text_input_CursorPos;

                ImGui_Columns(2);
                if(ImGui_InputTextMultiline("##TEST", vec2(-1.0, -1.0))){
                    ClearSpawnedObjects();
                    UpdateStringsFromScript(ImGui_GetTextBuf());
                    AddInvisibleStrings();
                    modified = true;
                }
                bool is_active = ImGui_IsItemActive();
                ImGui_NextColumn();
                for(int i=0, len=strings.size(); i<len; ++i){
                    vec4 color = vec4(1.0);
                    if(!strings[i].visible){
                        color = vec4(1,1,1,0.5);
                    }
                    if(i == selected_line){
                        color = vec4(0,1,0,1);
                    }
                    ImGui_TextColored(color, strings[i].str);
                }
                if(new_cursor_pos != old_cursor_pos && is_active){
                    int line = 0;
                    for(int i=0, len=strings.size(); i<len; ++i){
                        if(strings[i].visible && new_cursor_pos >= strings[i].line_start_char){
                            line = i;
                        }
                    }
                    if(line != selected_line){
                        selected_line = line;
                        HandleSelectedString(selected_line);
                        UpdateRecordLocked(IsRecording());
                        ClearUnselectedObjects();
                    }
                    DebugText("line", "Line: "+line, 0.5);
                    old_cursor_pos = new_cursor_pos;
                }
                ImGui_Columns(1);
            }
            ImGui_End();
        }
    }

    int GetFontSize() {
        return int(max(18, min(GetScreenHeight() / 30, GetScreenWidth() / 50)));
    }

    SavePopupValue SavePopup() {
        SavePopupValue return_value = NONE;
        ImGui_SetNextWindowSize(vec2(350.0f, 100.0f), ImGuiCond_Always);
        if(ImGui_BeginPopupModal("savedialoguepopup", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
            ImGui_SetCursorPos(vec2(35.0f, 30.0f));
            ImGui_Text("Dialogue has unsaved changes, save them?");
            ImGui_SetCursorPosX(70.0f);
            if(ImGui_Button("Yes", vec2(100.0f, 0.0f))) {
                SaveScript();
                ImGui_CloseCurrentPopup();
                return_value = YES;
            }
            ImGui_SameLine();
            if(ImGui_Button("No", vec2(100.0f, 0.0f))) {
                ImGui_CloseCurrentPopup();
                return_value = NO;
            }
            ImGui_EndPopup();
        }
        return return_value;
    }

    void Display2() {
        if(MediaMode()){
            return;
        }

        // Draw actual dialogue text
        if(show_dialogue && (camera.GetFlags() == kPreviewCamera || has_cam_control)){
            EnterTelemetryZone("Draw actual dialogue text");
            bool use_keyboard = (max(last_mouse_event_time, last_keyboard_event_time) > last_controller_event_time);
            string continue_string = GetStringDescriptionForBinding(use_keyboard?"key":"gamepad_0", "attack")+" to continue"+
                        "\n"+GetStringDescriptionForBinding(use_keyboard?"key":"gamepad_0", "skip_dialogue")+" to skip";

            if(Online_IsClient()) {
                continue_string = "Waiting for host\nto progress dialogue";
            }

            int font_size = GetFontSize();
            if(font_size != old_font_size){
                DisposeTextAtlases();
                old_font_size = font_size;
            }

            vec2 pos(kTextLeftMargin, GetScreenHeight() *0.75 + font_size * 1.2);
            vec3 color = vec3(1.0);
            if(NameInfoExists(dialogue_name)){
                color = GetNameInfo(dialogue_name).color;
            } else if(active_char >= 0 && int(dialogue_colors.size()) > active_char){
                color = dialogue_colors[active_char];
            } else {
                color = vec3(1.0f);
            }
            DrawTextAtlas(name_font_path, int(font_size*1.8), kSmallLowercase, dialogue_name, 
                          int(pos.x), int(pos.y)-int(font_size*0.8), vec4(color, 1.0f));
            //string display_text = dialogue_text.substr(0, int(dialogue_text_disp_chars));
            DrawTextAtlas2(font_path, font_size, 0, dialogue_text, 
                          int(pos.x)+font_size, int(pos.y)+font_size, vec4(vec3(1.0f), 1.0f), int(dialogue_text_disp_chars));
            TextMetrics test_metrics = GetTextAtlasMetrics2(font_path, GetFontSize(), 0, dialogue_text, int(dialogue_text_disp_chars));
            if(!waiting_for_dialogue && !is_waiting_time && test_metrics.bounds_y < GetFontSize() * 3){
                TextMetrics metrics = GetTextAtlasMetrics(font_path, GetFontSize(), 0, continue_string);
                DrawTextAtlas(font_path, font_size, 0, continue_string, 
                               GetScreenWidth() - int(kTextRightMargin) - metrics.bounds_x, int(pos.y)+font_size*4, vec4(vec3(1.0f), 0.5f));
            }
            
            LeaveTelemetryZone();
        }
    }

    void SaveHistoryState(SavedChunk@ chunk) {
        Log(info,"Called Dialogue::SaveHistoryState");
        string str = dialogue_obj_id + " ";
        str += selected_line + " ";
        str += show_editor_info + " ";
        chunk.WriteString(str);
    }

    void ReadChunk(SavedChunk@ chunk) {
        Log(info,"Called Dialogue::ReadChunk");
        history_str = chunk.ReadString();
        Log(info,"Read "+history_str);
    }

    void LoadHistoryStr(){
        ClearEditor();
        Log(info,"Loading history str");
        TokenIterator token_iter;
        token_iter.Init();
        token_iter.FindNextToken(history_str);
        SetDialogueObjID(atoi(token_iter.GetToken(history_str)));
        token_iter.FindNextToken(history_str);
        selected_line = atoi(token_iter.GetToken(history_str));
        token_iter.FindNextToken(history_str);
        show_editor_info = (token_iter.GetToken(history_str)=="true");
        HandleSelectedString(selected_line);
        history_str = "";
    }
};

