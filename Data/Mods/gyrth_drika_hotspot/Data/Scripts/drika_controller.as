#include "drika_json_functions.as"
#include "drika_animation_group.as"
#include "drika_shared.as"
#include "drika_ui_element.as"
#include "drika_ui_fullscreen_image.as"
#include "drika_ui_grabber.as"
#include "drika_ui_image.as"
#include "drika_ui_text.as"
#include "drika_ui_font.as"
#include "drika_ui_button.as"
#include "drika_ui_input.as"
#include "drika_dialogue_functions.as"

bool resetting = false;
bool animating_camera = false;
bool has_camera_control = false;
bool showing_interactive_ui = false;
bool show_dialogue = false;
array<string> hotspot_ids;
IMGUI@ imGUI;
FontSetup name_font_arial("arial", 70 , HexColor("#CCCCCC"), true);
FontSetup name_font("edosz", 70 , HexColor("#CCCCCC"), true);
FontSetup dialogue_font("arial", 50 , HexColor("#CCCCCC"), true);
FontSetup controls_font("arial", 45 , HexColor("#616161"), true);
FontSetup red_dialogue_font("arial", 50 , HexColor("#990000"), true);
FontSetup green_dialogue_font("arial", 50 , HexColor("#009900"), true);
FontSetup blue_dialogue_font("arial", 50 , HexColor("#000099"), true);
FontSetup death_font_arial("Lato-Regular", 70 , HexColor("#CCCCCC"), true);
array<DrikaAnimationGroup@> all_animations;
DrikaAnimationGroup @custom_group;
vec3 camera_position;
vec3 camera_rotation;
float camera_zoom = 90.0f;
bool fading = false;
float blackout_amount = 0.0;
float starting_fade_amount = 0.0;
float fade_direction = 1.0;
float fade_duration = 0.25;
float fade_timer = 0.0;
float target_fade_to_black = 1.0;
float fade_to_black_duration = 1.0;
bool fade_to_black = false;
array<int> waiting_hotspot_ids;
int dialogue_layout = 0;
bool use_voice_sounds = true;
bool show_names = true;
int ui_hotspot_id = -1;
float camera_near_blur = 0.0;
float camera_near_dist = 0.0;
float camera_near_transition = 0.0;
float camera_far_blur = 0.0;
float camera_far_dist = 0.0;
float camera_far_transition = 0.0;
bool update_dof = false;
bool enable_look_at_target = false;
bool enable_move_with_target = false;
int look_at_target_id = -1;
int move_with_target_id = -1;
vec3 target_positional_difference = vec3();
vec3 current_camera_position = vec3();
bool camera_settings_changed = false;
vec2 dialogue_size = vec2(2560, 450);
IMText@ lmb_continue;
IMText@ rtn_skip;
vec3 old_camera_translation;
vec3 old_camera_rotation;
bool show_avatar;
bool add_camera_shake = false;
float position_shake_max_distance;
float position_shake_slerp_speed;
float position_shake_interval;
float rotation_shake_max_distance;
float rotation_shake_slerp_speed;
float rotation_shake_interval;

vec3 camera_shake_position;
vec3 camera_shake_rotation;
float rotation_shake_timer;
vec3 new_camera_shake_rotation;
float position_shake_timer;
vec3 new_camera_shake_position;

FontSetup default_font("Cella", 70 , HexColor("#CCCCCC"), true);
IMContainer@ dialogue_container;
IMContainer@ image_container;
IMContainer@ text_container;
IMContainer@ grabber_container;
IMContainer@ death_container;
IMContainer@ death_ui_container;
bool editing_ui = false;
array<IMText@> text_elements;
array<DrikaUIElement@> ui_elements;
bool grabber_dragging = false;
DrikaUIGrabber@ current_grabber = null;
DrikaUIElement@ current_ui_element = null;
vec2 click_position;
bool show_grid = true;
bool post_init_done = false;
bool read_animation_list = false;
bool added_death_screen = false;
string defeat_sting = "Data/Music/slaver_loop/the_slavers_defeat.wav";
float ko_time;
bool checkpoint_fading = false;
float checkpoint_blackout_amount = 0.0f;
float checkpoint_fade_timer = 0.0f;
float checkpoint_fade_duration = 0.25f;
float checkpoint_fade_direction = 1.0f;

string in_combat_song = "";
bool in_combat_from_beginning_no_fade = false;
string player_died_song = "";
bool player_died_from_beginning_no_fade = false;
string enemies_defeated_song = "";
bool enemies_defeated_from_beginning_no_fade = false;
string ambient_song = "";
bool ambient_from_beginning_no_fade = false;
string current_song = "None";

string report_version;
string os;
string arch;
string gpu_vendor;
string gl_renderer;
string gl_version;
string gl_driver_version;
string vram;
string glsl_version;
string shader_model;

CheckpointData@ checkpoint = null;
float PI = 3.14159265359f;

const int _movement_state = 0;  // character is moving on the ground
const int _ground_state = 1;  // character has fallen down or is raising up, ATM ragdolls handle most of this
const int _attack_state = 2;  // character is performing an attack
const int _hit_reaction_state = 3;  // character was hit or dealt damage to and has to react to it in some manner
const int _ragdoll_state = 4;  // character is falling in ragdoll mode

const int _RGDL_NO_TYPE = 0;
const int _RGDL_FALL = 1;
const int _RGDL_LIMP = 2;
const int _RGDL_INJURED = 3;
const int _RGDL_ANIMATION = 4;

class CheckpointData{
	string data;

	CheckpointData(){

	}
}

class ActorSettings{
	string name = "Default";
	vec4 color = vec4(1.0);
	int voice = 0;
	string avatar_path = "None";

	ActorSettings(){

	}
}

class ReadFileProcess{
	int hotspot_id = -1;
	string data = "";
	string file_path;
	string param_1;
	int function_index;

	ReadFileProcess(int _hotspot_id, int _function_index, string _file_path, string _param_1){
		hotspot_id = _hotspot_id;
		file_path = _file_path;
		param_1 = _param_1;
		function_index = _function_index;
	}
}

class WriteFileProcess{
	int hotspot_id = -1;
	string data;
	string file_path;
	int function_index;

	WriteFileProcess(int _hotspot_id, int _function_index, string _file_path, string _data){
		hotspot_id = _hotspot_id;
		file_path = _file_path;
		data = _data;
		function_index = _function_index;
	}
}

bool ui_created = false;
string current_actor_name = "Default";
array<ActorSettings@> actor_settings;
ActorSettings@ current_actor_settings = ActorSettings();
array<ReadFileProcess@> read_file_processes;
array<WriteFileProcess@> write_file_processes;

void Init(string str){
	@imGUI = CreateIMGUI();
	@dialogue_container = IMContainer(2560, 1440);
	@image_container = IMContainer(2560, 1440);
	@text_container = IMContainer(2560, 1440);
	@grabber_container = IMContainer(2560, 1440);
	@death_container = IMContainer(2560, 1440);
	CreateIMGUIContainers();
	ReadCustomAnimationList();
	ReadHardwareReport();
}

void PostInit(){
	AddMusic("Data/Music/drika_silence.xml");
	level.SendMessage("level_start");
	post_init_done = true;
}

void AddDeathScreen(){
	if(added_death_screen){
		int player_id = GetPlayerCharacterID();

		if(player_id != -1 && ObjectExists(player_id)){
			float player_blackout_amount = 0.2 + 0.6 * (1.0 - pow(0.5, (the_time - ko_time)));
			// TODO replace this with the next function once IT is released.
			ReadCharacter(player_id).Execute("level_blackout = "+player_blackout_amount+";");
			/* ReadCharacter(player_id).SetFloatVar("level_blackout", player_blackout_amount); */
		}
		return;
	}

	bool use_keyboard = (max(last_mouse_event_time, last_keyboard_event_time) > last_controller_event_time);
	death_container.clear();
	@death_ui_container = IMContainer(2560, 800);
	death_container.setAlignment(CACenter, CATop);
	death_container.setElement(death_ui_container);
	death_container.setSize(vec2(2560, 1440));

	IMContainer text_container(0.0, 125.0);

	IMDivider text_divider("text_divider", DOHorizontal);
	text_divider.setZOrdering(2);
	text_divider.setAlignment(CACenter, CACenter);
	text_container.setElement(text_divider);

	vec2 offset(0.0, 15.0);
	IMText death_text("Press " + GetStringDescriptionForBinding(use_keyboard?"key":"gamepad_0", "attack") + " to load checkpoint.", death_font_arial);
	death_text.addUpdateBehavior(IMFadeIn(1000, inSineTween ), "");
	text_divider.appendSpacer(150.0);
	text_divider.append(death_text);
	text_divider.appendSpacer(130.0);
	death_text.setVisible(false);

	IMImage text_background("Textures/ui/menus/main/brushStroke.png");
	text_background.addUpdateBehavior(IMFadeIn(1000, inSineTween ), "");
	text_background.setClip(false);
	text_background.setColor(vec4(1.0, 1.0, 1.0, 0.5));
	death_ui_container.setElement(text_container);
	text_background.setVisible(false);

	imGUI.update();
	float added_height = 50.0f;
	text_background.setSize(text_container.getSize() + vec2(0.0, added_height));
	text_container.addFloatingElement(text_background, "text_background", vec2(0, -(added_height / 2.0)), 1);
	text_background.setZOrdering(1);

	imGUI.update();
	text_background.setVisible(true);
	death_text.setVisible(true);

	PlayDeathSting();
	ko_time = the_time;
	added_death_screen = true;
}

void PlayDeathSting(){
	int sting_handle = PlaySound(defeat_sting);
	SetSoundGain(sting_handle, GetConfigValueFloat("music_volume"));
}

void CreateIMGUIContainers(){
	imGUI.setup();
	imGUI.setBackgroundLayers(1);

	/* imGUI.getMain().showBorder(); */
	imGUI.getMain().setZOrdering(-1);

	imGUI.getMain().addFloatingElement(dialogue_container, "dialogue_container", vec2(0));
	imGUI.getMain().addFloatingElement(image_container, "image_container", vec2(0));
	imGUI.getMain().addFloatingElement(text_container, "text_container", vec2(0));
	imGUI.getMain().addFloatingElement(grabber_container, "grabber_container", vec2(0));
	imGUI.getMain().addFloatingElement(death_container, "death_container", vec2(0));
}

void SetWindowDimensions(int width, int height){
	imGUI.doScreenResize();
}

void PostScriptReload(){

}

void WriteMusicXML(string music_path, string song_name, string song_path){
	StartWriteFile();
	AddFileString("<?xml version=\"2.0\" ?>\n");
	AddFileString("<Music version=\"1\">\n");

	AddFileString("<Song name=\"" + song_name + "\" type=\"single\" file_path=\"" + song_path + "\" />\n");

	AddFileString("</Music>\n");
	WriteFileToWriteDir(music_path);
}

void DrawGUI(){
	imGUI.render();

	HUDImage @blackout_image = hud.AddImage();
	blackout_image.SetImageFromPath("Data/Textures/diffuse.tga");
	blackout_image.position.y = (GetScreenWidth() + GetScreenHeight()) * -1.0f;
	blackout_image.position.x = (GetScreenWidth() + GetScreenHeight()) * -1.0f;
	blackout_image.position.z = -2.0f;
	blackout_image.scale = vec3(GetScreenWidth() + GetScreenHeight()) * 2.0f;
	blackout_image.color = vec4(0.0f, 0.0f, 0.0f, blackout_amount);

	HUDImage @checkpoint_blackout_image = hud.AddImage();
	checkpoint_blackout_image.SetImageFromPath("Data/Textures/diffuse.tga");
	checkpoint_blackout_image.position.y = (GetScreenWidth() + GetScreenHeight()) * -1.0f;
	checkpoint_blackout_image.position.x = (GetScreenWidth() + GetScreenHeight()) * -1.0f;
	checkpoint_blackout_image.position.z = -2.0f;
	checkpoint_blackout_image.scale = vec3(GetScreenWidth() + GetScreenHeight()) * 2.0f;
	checkpoint_blackout_image.color = vec4(0.0f, 0.0f, 0.0f, checkpoint_blackout_amount);

	// Log(warning, " nr " + ui_elements.size());
	for(uint i = 0; i < ui_elements.size(); i++){
		ui_elements[i].Draw();
	}

	if(editing_ui){
		DrawMouseBlockContainer();
		DrawGrid();
	}
}

void DrawMouseBlockContainer(){
	// Only block mouse input when the mouse is hovering over a grabber, image or text.
	if(current_grabber !is null){
		ImGui_PushStyleColor(ImGuiCol_WindowBg, vec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui_Begin("MouseBlockContainer", editing_ui, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoFocusOnAppearing);
		UpdateGrabber();
		ImGui_PopStyleColor(1);
		ImGui_SetWindowPos("MouseBlockContainer", vec2(0,0));
		ImGui_SetWindowSize("MouseBlockContainer", vec2(GetScreenWidth(), GetScreenHeight()));
		ImGui_End();
	}

}

int ui_snap_scale = 20;

void DrawGrid(){
	if(!show_grid){
		return;
	}
	vec2 vertical_position = vec2(0.0, 0.0);
	vec2 horizontal_position = vec2(0.0, 0.0);
	int nr_horizontal_lines = int(ceil(screenMetrics.screenSize.y / (ui_snap_scale * screenMetrics.GUItoScreenYScale)));
	int nr_vertical_lines = int(ceil(screenMetrics.screenSize.x / (ui_snap_scale * screenMetrics.GUItoScreenXScale))) + 1;
	vec4 line_color = vec4(0.25, 0.25, 0.25, 1.0);
	float line_width = 1.0;
	float thick_line_width = 3.0;

	for(int i = 0; i < nr_vertical_lines; i++){
		bool thick_line = i % 10 == 0;

		imGUI.drawBox(vertical_position, vec2(thick_line?thick_line_width:line_width, screenMetrics.screenSize.y), line_color, 0, false);
		vertical_position += vec2((ui_snap_scale * screenMetrics.GUItoScreenXScale), 0.0);
	}

	for(int i = 0; i < nr_horizontal_lines; i++){
		bool thick_line = i % 10 == 0;
		imGUI.drawBox(horizontal_position, vec2(screenMetrics.screenSize.x, thick_line?thick_line_width:line_width), line_color, 0, false);
		horizontal_position += vec2(0.0, (ui_snap_scale * screenMetrics.GUItoScreenYScale));
	}
}

void ReceiveMessage(string msg){
	TokenIterator token_iter;
	token_iter.Init();
	if(!token_iter.FindNextToken(msg)){
		return;
	}
	string token = token_iter.GetToken(msg);

	/* Log(warning, token); */
	if(token == "animating_camera"){
		token_iter.FindNextToken(msg);
		string enable = token_iter.GetToken(msg);
		token_iter.FindNextToken(msg);
		string hotspot_id = token_iter.GetToken(msg);
		if(enable == "true"){
			hotspot_ids.insertLast(hotspot_id);
			animating_camera = true;
		}else{
			for(uint i = 0; i < hotspot_ids.size(); i++){
				if(hotspot_ids[i] == hotspot_id){
					hotspot_ids.removeAt(i);
					i--;
				}
			}
			if(hotspot_ids.size() == 0){
				animating_camera = false;
			}
		}
	}else if(token == "reset"){
		has_camera_control = false;
		fade_timer = 0.0;
		camera_near_blur = 0.0;
		camera_near_dist = 0.0;
		camera_near_transition = 0.0;
		camera_far_blur = 0.0;
		camera_far_dist = 0.0;
		camera_far_transition = 0.0;
		allow_dialogue_move_in = true;
		showing_choice = false;
		showing_interactive_ui = false;
		blackout_amount = 0.0f;
		death_container.clear();
		checkpoint_blackout_amount = 0.0f;
		resetting = true;
	}else if(token == "write_music_xml"){
		array<string> lines;
		string xml_content;

		token_iter.FindNextToken(msg);
		string music_path = token_iter.GetToken(msg);

		token_iter.FindNextToken(msg);
		string song_name = token_iter.GetToken(msg);

		token_iter.FindNextToken(msg);
		string song_path = token_iter.GetToken(msg);

		WriteMusicXML(music_path, song_name, song_path);
	}else if(token == "drika_dialogue_clear"){
		show_dialogue = false;
		showing_choice = false;
		dialogue_move_in = false;
		ui_hotspot_id = -1;
		dialogue_container.clear();
	}else if(token == "allow_dialogue_move_in"){
		allow_dialogue_move_in = true;
	}else if(token == "drika_dialogue_add_say"){
		token_iter.FindNextToken(msg);
		string actor_name = token_iter.GetToken(msg);

		string text = "";
		while(token_iter.FindNextToken(msg)){
			text += token_iter.GetToken(msg);
		}

		DialogueAddSay(actor_name, text);
	}else if(token == "drika_dialogue_next"){
		DialogueNext();
	}else if(token == "drika_dialogue_clear_say"){
		choices.resize(0);
		line_counter = 0;

		if(show_dialogue){
			dialogue_holder.clear();
			@dialogue_line = IMDivider("dialogue_line" + line_counter, DOHorizontal);
			dialogue_holder.append(dialogue_line);
			dialogue_line.setZOrdering(2);
			dialogue_holder.setSize(dialogue_holder_size);
		}
	}else if(token == "drika_dialogue_set_actor_settings"){
		token_iter.FindNextToken(msg);
		string actor_name = token_iter.GetToken(msg);

		vec4 color;
		token_iter.FindNextToken(msg);
		color.x = atof(token_iter.GetToken(msg));
		token_iter.FindNextToken(msg);
		color.y = atof(token_iter.GetToken(msg));
		token_iter.FindNextToken(msg);
		color.z = atof(token_iter.GetToken(msg));
		token_iter.FindNextToken(msg);
		color.a = atof(token_iter.GetToken(msg));

		token_iter.FindNextToken(msg);
		int voice = atoi(token_iter.GetToken(msg));

		token_iter.FindNextToken(msg);
		string avatar_path = token_iter.GetToken(msg);

		if(avatar_path != "None"){
			array<string> split_path = avatar_path.split("/");
			//Remove the Data/ in the beginning of the path.
			split_path.removeAt(0);
			avatar_path = join(split_path, "/");
		}

		bool settings_found = false;
		for(uint i = 0; i < actor_settings.size(); i++){
			if(actor_settings[i].name == actor_name){
				settings_found = true;
				actor_settings[i].voice = voice;
				actor_settings[i].color = color;
				actor_settings[i].avatar_path = avatar_path;
				break;
			}
		}

		if(!settings_found){
			ActorSettings new_settings();
			new_settings.name = actor_name;
			new_settings.voice = voice;
			new_settings.color = color;
			new_settings.avatar_path = avatar_path;
			actor_settings.insertLast(@new_settings);
		}
	}else if(token == "drika_dialogue_test_voice"){
		token_iter.FindNextToken(msg);
		int test_voice = atoi(token_iter.GetToken(msg));
		PlayLineContinueSound(test_voice);
	}else if(token == "drika_dialogue_skip"){
		smooth_dialogue_displacement = false;
		PlayLineStartSound();
	}else if(token == "drika_dialogue_get_animations"){
		token_iter.FindNextToken(msg);
		int hotspot_id = atoi(token_iter.GetToken(msg));

		if(!read_animation_list){
			ReadAnimationList();
		}

		Object@ hotspot_obj = ReadObjectFromID(hotspot_id);
		for(uint i = 0; i < all_animations.size(); i++){
			hotspot_obj.ReceiveScriptMessage("drika_dialogue_add_animation_group " + "\"" + join(all_animations[i].name.split("\""), "\\\"") + "\"");

			for(uint j = 0; j < all_animations[i].animations.size(); j++){
				hotspot_obj.ReceiveScriptMessage("drika_dialogue_add_animation " + all_animations[i].animations[j]);
			}
		}
		hotspot_obj.ReceiveScriptMessage("drika_dialogue_send_done");
	}else if(token == "drika_dialogue_set_camera_position"){
		token_iter.FindNextToken(msg);
		camera_rotation.x = atof(token_iter.GetToken(msg));
		token_iter.FindNextToken(msg);
		camera_rotation.y = atof(token_iter.GetToken(msg));
		token_iter.FindNextToken(msg);
		camera_rotation.z = atof(token_iter.GetToken(msg));

		token_iter.FindNextToken(msg);
		camera_position.x = atof(token_iter.GetToken(msg));
		token_iter.FindNextToken(msg);
		camera_position.y = atof(token_iter.GetToken(msg));
		token_iter.FindNextToken(msg);
		camera_position.z = atof(token_iter.GetToken(msg));

		token_iter.FindNextToken(msg);
		camera_zoom = atof(token_iter.GetToken(msg));

		token_iter.FindNextToken(msg);
		add_camera_shake = token_iter.GetToken(msg) == "true";

		if(add_camera_shake){
			token_iter.FindNextToken(msg);
			position_shake_max_distance = atof(token_iter.GetToken(msg));

			token_iter.FindNextToken(msg);
			position_shake_slerp_speed = atof(token_iter.GetToken(msg));

			token_iter.FindNextToken(msg);
			position_shake_interval = atof(token_iter.GetToken(msg));

			token_iter.FindNextToken(msg);
			rotation_shake_max_distance = atof(token_iter.GetToken(msg));

			token_iter.FindNextToken(msg);
			rotation_shake_slerp_speed = atof(token_iter.GetToken(msg));

			token_iter.FindNextToken(msg);
			rotation_shake_interval = atof(token_iter.GetToken(msg));
		}

		token_iter.FindNextToken(msg);
		bool new_enable_look_at_target = token_iter.GetToken(msg) == "true";

		// If the camera wasn't using lookat before, but now is then set the start transform from the placeholder.
		if(!enable_look_at_target && new_enable_look_at_target || !enable_look_at_target && !new_enable_look_at_target){
			level.Execute("dialogue.cam_pos = vec3(" + camera_position.x + ", " + camera_position.y + ", " + camera_position.z + ");");
			level.Execute("dialogue.cam_rot = vec3(" + camera_rotation.x + "," + camera_rotation.y + "," + camera_rotation.z + ");");
			level.Execute("dialogue.cam_zoom = " + camera_zoom + ");");

			current_camera_position = camera_position;
			old_camera_translation = camera_position;
			old_camera_rotation = camera_rotation;
			camera_settings_changed = true;
		}

		enable_look_at_target = new_enable_look_at_target;

		if(enable_look_at_target){
			token_iter.FindNextToken(msg);
			look_at_target_id = atoi(token_iter.GetToken(msg));
		}

		token_iter.FindNextToken(msg);
		enable_move_with_target = token_iter.GetToken(msg) == "true";

		if(enable_move_with_target){
			token_iter.FindNextToken(msg);
			move_with_target_id = atoi(token_iter.GetToken(msg));

			if(move_with_target_id != -1 && ObjectExists(move_with_target_id)){
				Object@ track_target = ReadObjectFromID(move_with_target_id);
				vec3 target_location;
				if(track_target.GetType() == _movement_object){
					MovementObject@ char = ReadCharacterID(move_with_target_id);
					target_location = char.rigged_object().GetAvgIKChainPos("torso");
				}else if(track_target.GetType() == _item_object){
					ItemObject@ item = ReadItemID(move_with_target_id);
					target_location = item.GetPhysicsPosition();
				}
				target_positional_difference = camera_position - target_location;
			}
		}

		if(!has_camera_control){
			for(int i = 0; i < GetNumCharacters(); i++){
				MovementObject@ char = ReadCharacter(i);
				if(char.controlled){
					char.Execute(	"level_cam_rotation.x = " + camera_rotation.x + ";" +
									"level_cam_rotation.y = " + camera_rotation.y + ";" +
									"level_cam_rotation.z = " + camera_rotation.z + ";" +
									"cam_rotation = level_cam_rotation.y;" +
									"target_rotation = level_cam_rotation.y;" +
									"level_cam_pos = vec3(" + camera_position.x + "," + camera_position.y + "," + camera_position.z +");");
				}
			}
		}

	}else if(token == "drika_dialogue_end"){
		show_dialogue = false;
		fade_to_black = false;
		showing_choice = false;
		has_camera_control = false;
		ui_hotspot_id = -1;
		dialogue_container.clear();
	}else if(token == "drika_dialogue_start"){
		camera_position = camera.GetPos();
		camera_rotation = vec3(camera.GetXRotation(), camera.GetYRotation(), camera.GetZRotation());
		old_camera_translation = camera_position;
		old_camera_rotation = camera_rotation;
		has_camera_control = true;
		allow_dialogue_move_in = true;
	}else if(token == "drika_dialogue_fade_out_in"){
		token_iter.FindNextToken(msg);
		int hotspot_id = atoi(token_iter.GetToken(msg));
		waiting_hotspot_ids.insertLast(hotspot_id);

		fading = true;
		fade_direction = 1.0;
	}else if(token == "drika_dialogue_fade_to_black"){
		token_iter.FindNextToken(msg);
		target_fade_to_black = atof(token_iter.GetToken(msg));
		starting_fade_amount = blackout_amount;

		token_iter.FindNextToken(msg);
		fade_to_black_duration = atof(token_iter.GetToken(msg));

		fade_to_black = true;
	}else if(token == "drika_dialogue_clear_fade_to_black"){
		fade_timer = 0.0;
		blackout_amount = 0.0;
		fade_to_black = false;
	}else if(token == "drika_dialogue_set_settings"){
		token_iter.FindNextToken(msg);
		dialogue_layout = atoi(token_iter.GetToken(msg));

		token_iter.FindNextToken(msg);
		string new_font_path = token_iter.GetToken(msg);
		array<string> new_font_path_split = new_font_path.split("/");
		string new_font_file = new_font_path_split[new_font_path_split.size() - 1];
		string dialogue_text_font = new_font_file.substr(0, new_font_file.length() - 4);

		token_iter.FindNextToken(msg);
		int dialogue_text_size = atoi(token_iter.GetToken(msg));

		vec4 dialogue_text_color;
		token_iter.FindNextToken(msg);
		dialogue_text_color.x = atof(token_iter.GetToken(msg));
		token_iter.FindNextToken(msg);
		dialogue_text_color.y = atof(token_iter.GetToken(msg));
		token_iter.FindNextToken(msg);
		dialogue_text_color.z = atof(token_iter.GetToken(msg));
		token_iter.FindNextToken(msg);
		dialogue_text_color.a = atof(token_iter.GetToken(msg));

		token_iter.FindNextToken(msg);
		bool dialogue_text_shadow = token_iter.GetToken(msg) == "true";

		token_iter.FindNextToken(msg);
		use_voice_sounds = token_iter.GetToken(msg) == "true";

		token_iter.FindNextToken(msg);
		show_names = token_iter.GetToken(msg) == "true";

		token_iter.FindNextToken(msg);
		show_avatar = token_iter.GetToken(msg) == "true";

		token_iter.FindNextToken(msg);
		dialogue_location = atoi(token_iter.GetToken(msg));

		dialogue_font = FontSetup(dialogue_text_font, dialogue_text_size, dialogue_text_color, dialogue_text_shadow);
		red_dialogue_font.fontName = dialogue_text_font;
		red_dialogue_font.size = dialogue_text_size;
		red_dialogue_font.shadowed = dialogue_text_shadow;
		green_dialogue_font.fontName = dialogue_text_font;
		green_dialogue_font.size = dialogue_text_size;
		green_dialogue_font.shadowed = dialogue_text_shadow;
		blue_dialogue_font.fontName = dialogue_text_font;
		blue_dialogue_font.size = dialogue_text_size;
		blue_dialogue_font.shadowed = dialogue_text_shadow;
	}else if(token == "drika_read_file"){
		token_iter.FindNextToken(msg);
		int hotspot_id = atoi(token_iter.GetToken(msg));

		token_iter.FindNextToken(msg);
		int function_index = atoi(token_iter.GetToken(msg));

		token_iter.FindNextToken(msg);
		string file_path = token_iter.GetToken(msg);

		token_iter.FindNextToken(msg);
		string param_1 = token_iter.GetToken(msg);

		read_file_processes.insertLast(ReadFileProcess(hotspot_id, function_index, file_path, param_1));
	}else if(token == "drika_dialogue_choice"){
		token_iter.FindNextToken(msg);
		int hotspot_id = atoi(token_iter.GetToken(msg));

		array<string> lines;

		while(token_iter.FindNextToken(msg)){
			lines.insertLast(token_iter.GetToken(msg));
		}

		selected_choice = 0;
		choices = lines;

		if(!show_dialogue){

			if(!EditorModeActive()){
				SetGrabMouse(false);
			}

			show_dialogue = true;
			showing_choice = true;
			ui_hotspot_id = hotspot_id;
			BuildDialogueUI();
		}

	}else if(token == "drika_dialogue_choice_select"){
		token_iter.FindNextToken(msg);
		int new_selected_choice = atoi(token_iter.GetToken(msg));

		if(new_selected_choice != selected_choice){
			SelectChoice(new_selected_choice);
		}
	}else if(token == "drika_dialogue_show_skip_message"){
		if(lmb_continue !is null){
			lmb_continue.setVisible(true);
			lmb_continue.addUpdateBehavior(IMFadeIn(100, inSineTween ), "");
		}
		if(rtn_skip !is null){
			rtn_skip.setVisible(true);
			rtn_skip.addUpdateBehavior(IMFadeIn(100, inSineTween ), "");
		}
	}else if(token == "drika_set_dof"){
		token_iter.FindNextToken(msg);
		camera_near_blur = atof(token_iter.GetToken(msg));
		token_iter.FindNextToken(msg);
		camera_near_dist = atof(token_iter.GetToken(msg));
		token_iter.FindNextToken(msg);
		camera_near_transition = atof(token_iter.GetToken(msg));
		token_iter.FindNextToken(msg);
		camera_far_blur = atof(token_iter.GetToken(msg));
		token_iter.FindNextToken(msg);
		camera_far_dist = atof(token_iter.GetToken(msg));
		token_iter.FindNextToken(msg);
		camera_far_transition = atof(token_iter.GetToken(msg));
		update_dof = true;
	}else if(token == "drika_set_fov"){
		token_iter.FindNextToken(msg);
		camera_zoom = atof(token_iter.GetToken(msg));
	}else if(token == "request_preview_dof"){
		if(update_dof){
			camera.SetDOF(camera_near_blur, camera_near_dist, camera_near_transition, camera_far_blur, camera_far_dist, camera_far_transition);
		}
	}else if(token == "drika_export_to_file"){
		token_iter.FindNextToken(msg);
		string path = token_iter.GetToken(msg);
		string content = "";

		while(token_iter.FindNextToken(msg)){
			content += token_iter.GetToken(msg);
		}
		StartWriteFile();
		AddFileString(content);
		WriteFileKeepBackup(path);
	}else if(token == "drika_edit_ui"){
		token_iter.FindNextToken(msg);
		string ui_element_identifier = token_iter.GetToken(msg);

		token_iter.FindNextToken(msg);
		editing_ui = token_iter.GetToken(msg) == "true";

		if(editing_ui){
			token_iter.FindNextToken(msg);
			ui_hotspot_id = atoi(token_iter.GetToken(msg));

			token_iter.FindNextToken(msg);
			show_grid = token_iter.GetToken(msg) == "true";

			token_iter.FindNextToken(msg);
			ui_snap_scale = max(5, atoi(token_iter.GetToken(msg)));
		}else{
			ui_hotspot_id = -1;
		}
	}else if(token == "drika_ui_add_element"){
		token_iter.FindNextToken(msg);
		string json_string = token_iter.GetToken(msg);
		// Get all the JSON from the message as a string. It is later converted into a JSONValue.
		AddUIElement(json_string);
	}else if(token == "drika_ui_remove_element"){
		token_iter.FindNextToken(msg);
		string ui_element_identifier = token_iter.GetToken(msg);

		int index = GetUIElementIndex(ui_element_identifier);
		if(index != -1){
			ui_elements[index].Delete();
			ui_elements.removeAt(index);
		}
		@current_ui_element = null;
		@current_grabber = null;
	}else if(token == "drika_ui_set_editing"){
		token_iter.FindNextToken(msg);
		string ui_element_identifier = token_iter.GetToken(msg);

		token_iter.FindNextToken(msg);
		bool editing = token_iter.GetToken(msg) == "true";

		@current_ui_element = GetUIElement(ui_element_identifier);
		current_ui_element.SetEditing(editing);
	}else if(token == "drika_ui_instruction"){
		token_iter.FindNextToken(msg);
		string ui_element_identifier = token_iter.GetToken(msg);
		array<string> instruction;

		while(token_iter.FindNextToken(msg)){
			instruction.insertLast(token_iter.GetToken(msg));
		}

		GetUIElement(ui_element_identifier).ReadUIInstruction(instruction);
	}else if(token == "drika_set_show_grid"){
		token_iter.FindNextToken(msg);
		show_grid = token_iter.GetToken(msg) == "true";
	}else if(token == "drika_set_ui_snap_scale"){
		token_iter.FindNextToken(msg);
		ui_snap_scale = max(5, atoi(token_iter.GetToken(msg)));
	}else if(token == "drika_music_event"){
		token_iter.FindNextToken(msg);
		int event_type = atoi(token_iter.GetToken(msg));
		token_iter.FindNextToken(msg);
		string song_name = token_iter.GetToken(msg);
		token_iter.FindNextToken(msg);
		bool from_beginning_no_fade = token_iter.GetToken(msg) == "true";

		if(event_type == music_song_in_combat){
			in_combat_song = song_name;
			in_combat_from_beginning_no_fade = from_beginning_no_fade;
		}else if(event_type == music_song_player_died){
			player_died_song = song_name;
			player_died_from_beginning_no_fade = from_beginning_no_fade;
		}else if(event_type == music_song_enemies_defeated){
			enemies_defeated_song = song_name;
			enemies_defeated_from_beginning_no_fade = from_beginning_no_fade;
		}else if(event_type == music_song_ambient){
			ambient_song = song_name;
			ambient_from_beginning_no_fade = from_beginning_no_fade;
		}

		if(EditorModeActive()){
			if(from_beginning_no_fade){
				SetSong(song_name);
			}else{
				PlaySong(song_name);
			}
		}
	}else if(token == "drika_save_checkpoint"){
		SaveCheckpoint();
	}else if(token == "drika_load_checkpoint"){
		LoadCheckpoint();
	}else if(token == "drika_write_file"){
		token_iter.FindNextToken(msg);
		int hotspot_id = atoi(token_iter.GetToken(msg));

		token_iter.FindNextToken(msg);
		int function_index = atoi(token_iter.GetToken(msg));

		token_iter.FindNextToken(msg);
		string file_path = token_iter.GetToken(msg);

		token_iter.FindNextToken(msg);
		string data = token_iter.GetToken(msg);

		while(token_iter.FindNextToken(msg)){
			data += token_iter.GetToken(msg) + " ";
		}

		write_file_processes.insertLast(WriteFileProcess(hotspot_id, function_index, file_path, data));
	}else if(token == "drika_get_old_camera_transform"){
		token_iter.FindNextToken(msg);
		int hotspot_id = atoi(token_iter.GetToken(msg));

		Object@ hotspot_obj = ReadObjectFromID(hotspot_id);
		string return_msg;
		return_msg += "drika_message ";
		return_msg += "old_camera_transform ";
		return_msg += old_camera_translation.x + " ";
		return_msg += old_camera_translation.y + " ";
		return_msg += old_camera_translation.z + " ";
		return_msg += old_camera_rotation.x + " ";
		return_msg += old_camera_rotation.y + " ";
		return_msg += old_camera_rotation.z + " ";
		hotspot_obj.ReceiveScriptMessage(return_msg);
	}else if(token == "drika_dialogue_add_custom_animation" || token == "drika_dialogue_add_new_custom_animation"){
		token_iter.FindNextToken(msg);
		string custom_animation_path = token_iter.GetToken(msg);

		AddCustomAnimation(custom_animation_path);
	}else if(token == "drika_hardware_info"){
		token_iter.FindNextToken(msg);
		int hotspot_id = atoi(token_iter.GetToken(msg));
		Object@ hotspot_obj = ReadObjectFromID(hotspot_id);
		array<string> save_names;

		string return_msg = "drika_hardware_info";

		return_msg += "\"" + report_version + "\"" + " ";
		return_msg += "\"" + os + "\"" + " ";
		return_msg += "\"" + arch + "\"" + " ";
		return_msg += "\"" + gpu_vendor + "\"" + " ";
		return_msg += "\"" + gl_renderer + "\"" + " ";
		return_msg += "\"" + gl_version + "\"" + " ";
		return_msg += "\"" + gl_driver_version + "\"" + " ";
		return_msg += "\"" + vram + "\"" + " ";
		return_msg += "\"" + glsl_version + "\"" + " ";
		return_msg += "\"" + shader_model + "\"" + " ";

		hotspot_obj.ReceiveScriptMessage(return_msg);
	}else if(token == "drika_variable_changed"){
		// Tell all the ui elements that a variable has changed.
		// The UI Element can choose what to do next, like update it's text for example.
		for(uint i = 0; i < ui_elements.size(); i++){
			ui_elements[i].ReadUIInstruction({"variable_changed"});
		}
	}
}

void AddUIElement(string json_string){
	JSON data;

	// Convert the string from the message into JSON.
	if(!data.parseString(json_string)){
		Log(warning, "Unable to parse the JSON in the UI JSON Data!");
	}else{
		JSONValue json_data = data.getRoot();
		DrikaUIElement@ new_element;

		switch(json_data["type"].asInt()) {
			case ui_clear:
				if(!EditorModeActive()){
					SetGrabMouse(true);
				}
				showing_interactive_ui = false;
				// Don't add a new ui_element to the array, just grab the mouse and return.
				return;
			case ui_image:
				@new_element = DrikaUIImage(json_data);
				break;
			case ui_text:
				@new_element = DrikaUIText(json_data);
				break;
			case ui_font:
				@new_element = DrikaUIFont(json_data);
				break;
			case ui_button:
				SetGrabMouse(false);
				showing_interactive_ui = true;
				@new_element = DrikaUIButton(json_data);
				break;
			case ui_input:
				SetGrabMouse(false);
				showing_interactive_ui = true;
				@new_element = DrikaUIInput(json_data);
				break;
			case ui_fullscreen_image:
				@new_element = DrikaUIFullscreenImage(json_data);
				break;
			default:
				Log(warning, "Unknown ui element type: " + json_data["type"].asInt());
				break;
		}

		// Store the original json string in the class so that it can be used by the checkpoint system.
		new_element.json_string = json_string;
		@current_ui_element = @new_element;
		ui_elements.insertAt(0, new_element);
	}
}

void SaveCheckpoint(){
	@checkpoint = CheckpointData();

	JSON json;
	JSONValue object_data;

	for(int i = 0; i < GetNumCharacters(); i++){
		JSONValue character_data;
		MovementObject@ char = ReadCharacter(i);
		character_data["id"] = JSONValue(char.GetID());

		character_data["translation"] = JSONValue(JSONarrayValue);
		vec3 translation = char.position;
		character_data["translation"].append(translation.x);
		character_data["translation"].append(translation.y);
		character_data["translation"].append(translation.z);

		RiggedObject@ rigged_object = char.rigged_object();
		Skeleton@ skeleton = rigged_object.skeleton();
		// Get relative chest transformation
		int chest_bone = skeleton.IKBoneStart("torso");
		BoneTransform chest_frame_matrix = rigged_object.GetFrameMatrix(chest_bone);
		quaternion quat = chest_frame_matrix.rotation;
		vec3 facing = Mult(quat, vec3(1,0,0));
		float rot = atan2(facing.x, facing.z) * 180.0f / PI;
		float rotation = floor(rot + 0.5f);
		character_data["rotation"] = JSONValue(rotation);

		character_data["velocity"] = JSONValue(JSONarrayValue);
		vec3 velocity = char.velocity;
		character_data["velocity"].append(velocity.x);
		character_data["velocity"].append(velocity.y);
		character_data["velocity"].append(velocity.z);

		int state = char.GetIntVar("state");
		character_data["state"] = JSONValue(state);
		character_data["ragdoll_type"] = JSONValue(char.GetIntVar("ragdoll_type"));
		character_data["knocked_out"] = JSONValue(char.GetIntVar("knocked_out"));

		character_data["blood_health"] = JSONValue(char.GetFloatVar("blood_health"));
		character_data["block_health"] = JSONValue(char.GetFloatVar("block_health"));
		character_data["blood_damage"] = JSONValue(char.GetFloatVar("blood_damage"));
		character_data["temp_health"] = JSONValue(char.GetFloatVar("temp_health"));
		character_data["permanent_health"] = JSONValue(char.GetFloatVar("permanent_health"));
		character_data["on_fire"] = JSONValue(char.GetBoolVar("on_fire"));
		character_data["fire_sleep"] = JSONValue(char.GetBoolVar("fire_sleep"));
		character_data["injured_mouth_open"] = JSONValue(char.GetFloatVar("injured_mouth_open"));
		character_data["blood_amount"] = JSONValue(char.GetFloatVar("blood_amount"));
		character_data["recovery_time"] = JSONValue(char.GetFloatVar("recovery_time"));
		character_data["roll_recovery_time"] = JSONValue(char.GetFloatVar("roll_recovery_time"));
		character_data["zone_killed"] = JSONValue(char.GetIntVar("zone_killed"));
		character_data["ko_shield"] = JSONValue(char.GetIntVar("ko_shield"));
		character_data["got_hit_by_leg_cannon_count"] = JSONValue(char.GetIntVar("got_hit_by_leg_cannon_count"));
		character_data["ragdoll_static_time"] = JSONValue(char.GetFloatVar("ragdoll_static_time"));
		character_data["frozen"] = JSONValue(char.GetBoolVar("frozen"));
		character_data["no_freeze"] = JSONValue(char.GetBoolVar("no_freeze"));
		character_data["cut_throat"] = JSONValue(char.GetBoolVar("cut_throat"));
		character_data["tethered"] = JSONValue(char.GetIntVar("tethered"));
		character_data["water_id"] = JSONValue(char.GetIntVar("water_id"));
		character_data["water_depth"] = JSONValue(char.GetFloatVar("water_depth"));
		character_data["breath"] = JSONValue(char.GetFloatVar("breath"));
		character_data["breath_bars"] = JSONValue(char.GetIntVar("breath_bars"));
		character_data["red_tint"] = JSONValue(char.GetFloatVar("red_tint"));
		character_data["black_tint"] = JSONValue(char.GetFloatVar("black_tint"));
		character_data["level_blackout"] = JSONValue(char.GetFloatVar("level_blackout"));
		character_data["blood_flash_time"] = JSONValue(char.GetFloatVar("blood_flash_time"));
		character_data["hit_flash_time"] = JSONValue(char.GetFloatVar("hit_flash_time"));
		character_data["dark_hit_flash_time"] = JSONValue(char.GetFloatVar("dark_hit_flash_time"));
		character_data["idle_stance"] = JSONValue(char.GetBoolVar("idle_stance"));
		character_data["idle_stance_amount"] = JSONValue(char.GetFloatVar("idle_stance_amount"));
		character_data["target_rotation"] = JSONValue(char.GetFloatVar("target_rotation"));
		character_data["target_rotation2"] = JSONValue(char.GetFloatVar("target_rotation2"));
		character_data["cam_rotation"] = JSONValue(char.GetFloatVar("cam_rotation"));
		character_data["cam_rotation2"] = JSONValue(char.GetFloatVar("cam_rotation2"));
		character_data["cam_distance"] = JSONValue(char.GetFloatVar("cam_distance"));
		character_data["auto_cam_override"] = JSONValue(char.GetFloatVar("auto_cam_override"));
		character_data["on_ground"] = JSONValue(char.GetBoolVar("on_ground"));
		character_data["air_time"] = JSONValue(char.GetFloatVar("air_time"));
		character_data["on_ground_time"] = JSONValue(char.GetFloatVar("on_ground_time"));
		character_data["duck_amount"] = JSONValue(char.GetFloatVar("duck_amount"));
		character_data["target_duck_amount"] = JSONValue(char.GetFloatVar("target_duck_amount"));
		character_data["duck_vel"] = JSONValue(char.GetFloatVar("duck_vel"));

		if(state == _ragdoll_state){
			string bone_data;

			for(int j = 0, len = skeleton.NumBones(); j < len; ++j) {
				if(skeleton.HasPhysics(j)) {
					vec3 bone_pos = skeleton.GetBoneTransform(j).GetTranslationPart();
					quaternion bone_quat = QuaternionFromMat4(skeleton.GetBoneTransform(j));
					vec3 bone_vel = skeleton.GetBoneLinearVelocity(j);
					bone_data +=	"" + j +
									" " + bone_pos[0] +
									" " + bone_pos[1] +
									" " + bone_pos[2] +
									" " + bone_quat.x +
									" " + bone_quat.y +
									" " + bone_quat.z +
									" " + bone_quat.w +
									" " + bone_vel.x +
									" " + bone_vel.y +
									" " + bone_vel.z + " ";
				}
			}
			character_data["bone_data"] = JSONValue(bone_data);

			character_data["avg_bone_velocity"] = JSONValue(JSONarrayValue);
			vec3 avg_bone_velocity = char.rigged_object().GetAvgVelocity();
			character_data["avg_bone_velocity"].append(avg_bone_velocity.x);
			character_data["avg_bone_velocity"].append(avg_bone_velocity.y);
			character_data["avg_bone_velocity"].append(avg_bone_velocity.z);

		}

		object_data.append(character_data);
	}

	for(int i = 0; i < GetNumItems(); i++){
		ItemObject@ item = ReadItem(i);
		JSONValue item_data;

		int item_id = item.GetID();
		item_data["id"] = JSONValue(item_id);

		int stuck_in_whom = item.StuckInWhom();
		item_data["stuck_in_whom"] = JSONValue(stuck_in_whom);

		bool is_held = item.IsHeld();
		item_data["is_held"] = JSONValue(is_held);
		if(is_held){
			item_data["held_by_whom"] = JSONValue(item.HeldByWhom());
			MovementObject@ char = ReadCharacterID(item.HeldByWhom());

			if(item_id == char.GetArrayIntVar("weapon_slots", _held_left)){
				item_data["item_slot"] = JSONValue(_held_left);
			}else if(item_id == char.GetArrayIntVar("weapon_slots", _held_right)){
				item_data["item_slot"] = JSONValue(_held_right);
			}else if(item_id == char.GetArrayIntVar("weapon_slots", _sheathed_left)){
				item_data["item_slot"] = JSONValue(_sheathed_left);
			}else if(item_id == char.GetArrayIntVar("weapon_slots", _sheathed_right)){
				item_data["item_slot"] = JSONValue(_sheathed_right);
			}else if(item_id == char.GetArrayIntVar("weapon_slots", _sheathed_left_sheathe)){
				item_data["item_slot"] = JSONValue(_sheathed_left_sheathe);
			}else if(item_id == char.GetArrayIntVar("weapon_slots", _sheathed_right_sheathe)){
				item_data["item_slot"] = JSONValue(_sheathed_right_sheathe);
			}else{
				Log(warning, "Unknown slot used for item! " + item_id);
				item_data["item_slot"] = JSONValue(-1);
			}
		}else{
			item_data["translation"] = JSONValue(JSONarrayValue);
			vec3 translation = item.GetPhysicsPosition();
			item_data["translation"].append(translation.x);
			item_data["translation"].append(translation.y);
			item_data["translation"].append(translation.z);

			item_data["rotation"] = JSONValue(JSONarrayValue);
			quaternion rotation = QuaternionFromMat4(item.GetPhysicsTransform().GetRotationPart());
			item_data["rotation"].append(rotation.x);
			item_data["rotation"].append(rotation.y);
			item_data["rotation"].append(rotation.z);
			item_data["rotation"].append(rotation.w);

			item_data["linear_velocity"] = JSONValue(JSONarrayValue);
			vec3 linear_velocity = item.GetLinearVelocity();
			item_data["linear_velocity"].append(linear_velocity.x);
			item_data["linear_velocity"].append(linear_velocity.y);
			item_data["linear_velocity"].append(linear_velocity.z);

			item_data["angular_velocity"] = JSONValue(JSONarrayValue);
			vec3 angular_velocity = item.GetAngularVelocity();
			item_data["angular_velocity"].append(angular_velocity.x);
			item_data["angular_velocity"].append(angular_velocity.y);
			item_data["angular_velocity"].append(angular_velocity.z);
		}

		object_data.append(item_data);
	}

	for(int i = 0; i < GetNumHotspots(); i++){
		//Only save DHS hotspots.
		Hotspot@ hotspot = ReadHotspot(i);
		if(hotspot.GetTypeString() != "Drika Hotspot"){
			continue;
		}
		JSONValue hotspot_data;
		hotspot_data["id"] = JSONValue(hotspot.GetID());
		hotspot_data["dhs_data"] = JSONValue(hotspot.QueryStringFunction("string GetCheckpointData()"));

		object_data.append(hotspot_data);
	}

	//Save the current ui elements.
	JSONValue ui_element_data;

	for(uint i = 0; i < ui_elements.size(); i++){
		ui_element_data.append(JSONValue(ui_elements[i].json_string));
	}
	json.getRoot()["ui_element_data"] = ui_element_data;

	json.getRoot()["checkpoint_data"] = object_data;
	checkpoint.data = json.writeString(false);
}

void LoadCheckpoint(){
	if(checkpoint is null){
		Log(error, "Could not find the checkpoint data!");
		return;
	}

	added_death_screen = false;
	death_container.clear();

	JSON file;
	file.parseString(checkpoint.data);
	JSONValue object_data = file.getRoot()["checkpoint_data"];

	//Since we can't add or remove specific decals, just remove all of them when loading.
	ClearTemporaryDecals();

	for(uint i = 0; i < object_data.size(); i++){
		JSONValue obj_data = object_data[i];
		int id = obj_data["id"].asInt();

		if(!ObjectExists(id)){
			continue;
		}

		Object@ obj = ReadObjectFromID(id);

		if(obj.GetType() == _movement_object){
			vec3 position = vec3(obj_data["translation"][0].asFloat(), obj_data["translation"][1].asFloat(), obj_data["translation"][2].asFloat());
			float rotation = obj_data["rotation"].asFloat();
			vec3 velocity = vec3(obj_data["velocity"][0].asFloat(), obj_data["velocity"][1].asFloat(), obj_data["velocity"][2].asFloat());
			int state = obj_data["state"].asInt();

			MovementObject@ char = ReadCharacterID(id);
			char.Execute("Reset();");
			char.Execute("SetState(_movement_state);");
			char.position = position;
			char.ReceiveScriptMessage("set_rotation " + rotation);
			char.velocity = velocity;

			char.Execute("knocked_out = " + obj_data["knocked_out"].asInt() + ";");

			if(state == _ragdoll_state){
				int ragdoll_type = obj_data["ragdoll_type"].asInt();
				char.Execute("Ragdoll(" + ragdoll_type + ");");

				//Set the transform of all the bones back.
				Skeleton@ skeleton = char.rigged_object().skeleton();

				char.rigged_object().SetRagdollDamping(0.0f);
				char.rigged_object().RefreshRagdoll();

				/* char.rigged_object().SetRagdollDamping(1.0f); */
				string bone_data = obj_data["bone_data"].asString();
				TokenIterator token_iter;
				token_iter.Init();
				int index = 0;
				int bone = -1;
				vec3 bone_pos;
				vec3 bone_vel;
				quaternion bone_quat;
				int num = 0;

				while(token_iter.FindNextToken(bone_data)) {
					switch(index) {
						case 0: bone = atoi(token_iter.GetToken(bone_data)); break;
						case 1: bone_pos[0] = atof(token_iter.GetToken(bone_data)); break;
						case 2: bone_pos[1] = atof(token_iter.GetToken(bone_data)); break;
						case 3: bone_pos[2] = atof(token_iter.GetToken(bone_data)); break;
						case 4: bone_quat.x = atof(token_iter.GetToken(bone_data)); break;
						case 5: bone_quat.y = atof(token_iter.GetToken(bone_data)); break;
						case 6: bone_quat.z = atof(token_iter.GetToken(bone_data)); break;
						case 7: bone_quat.w = atof(token_iter.GetToken(bone_data)); break;
						case 8: bone_vel.x = atof(token_iter.GetToken(bone_data)); break;
						case 9: bone_vel.y = atof(token_iter.GetToken(bone_data)); break;
						case 10: bone_vel.z = atof(token_iter.GetToken(bone_data));
							{
								mat4 translate_mat;
								translate_mat.SetTranslationPart(bone_pos);
								mat4 rotation_mat;
								rotation_mat = Mat4FromQuaternion(bone_quat);
								mat4 mat = translate_mat * rotation_mat;
								/* DebugDrawLine(mat * skeleton.GetBindMatrix(bone) * skeleton.GetPointPos(skeleton.GetBonePoint(bone, 0)),
											  mat * skeleton.GetBindMatrix(bone) * skeleton.GetPointPos(skeleton.GetBonePoint(bone, 1)),
											  vec4(1.0f), vec4(1.0f), _persistent); */
								skeleton.SetBoneTransform(bone, mat);
								char.rigged_object().ApplyForceToBone(bone_vel, bone);
							}

							break;
					}

					++index;

					if(index == 11) {
						index = 0;
					}
				}

				vec3 avg_bone_velocity = vec3(obj_data["avg_bone_velocity"][0].asFloat(), obj_data["avg_bone_velocity"][1].asFloat(), obj_data["avg_bone_velocity"][2].asFloat());
				skeleton.AddVelocity(avg_bone_velocity);
			}

			if(obj_data["frozen"].asBool()){

			}

			char.Execute("blood_health = " + obj_data["blood_health"].asFloat() + ";");
			char.Execute("block_health = " + obj_data["block_health"].asFloat() + ";");
			char.Execute("blood_damage = " + obj_data["blood_damage"].asFloat() + ";");
			char.Execute("temp_health = " + obj_data["temp_health"].asFloat() + ";");
			char.Execute("permanent_health = " + obj_data["permanent_health"].asFloat() + ";");
			char.Execute("on_fire = " + obj_data["on_fire"].asBool() + ";");
			char.Execute("fire_sleep = " + obj_data["fire_sleep"].asBool() + ";");
			char.Execute("injured_mouth_open = " + obj_data["injured_mouth_open"].asFloat() + ";");
			char.Execute("blood_amount = " + obj_data["blood_amount"].asFloat() + ";");
			char.Execute("recovery_time = " + obj_data["recovery_time"].asFloat() + ";");
			char.Execute("roll_recovery_time = " + obj_data["roll_recovery_time"].asFloat() + ";");
			char.Execute("zone_killed = " + obj_data["zone_killed"].asInt() + ";");
			char.Execute("ko_shield = " + obj_data["ko_shield"].asInt() + ";");
			char.Execute("got_hit_by_leg_cannon_count = " + obj_data["got_hit_by_leg_cannon_count"].asInt() + ";");
			char.Execute("ragdoll_static_time = " + obj_data["ragdoll_static_time"].asFloat() + ";");
			char.Execute("frozen = " + obj_data["frozen"].asBool() + ";");
			char.Execute("no_freeze = " + obj_data["no_freeze"].asBool() + ";");
			char.Execute("cut_throat = " + obj_data["cut_throat"].asBool() + ";");
			char.Execute("tethered = " + obj_data["tethered"].asInt() + ";");
			char.Execute("water_id = " + obj_data["water_id"].asInt() + ";");
			char.Execute("water_depth = " + obj_data["water_depth"].asFloat() + ";");
			char.Execute("breath_bars = " + obj_data["breath_bars"].asInt() + ";");
			char.Execute("breath = " + obj_data["breath"].asFloat() + ";");
			char.Execute("red_tint = " + obj_data["red_tint"].asFloat() + ";");
			char.Execute("black_tint = " + obj_data["black_tint"].asFloat() + ";");
			char.Execute("level_blackout = " + obj_data["level_blackout"].asFloat() + ";");
			char.Execute("blood_flash_time = " + obj_data["blood_flash_time"].asFloat() + ";");
			char.Execute("hit_flash_time = " + obj_data["hit_flash_time"].asFloat() + ";");
			char.Execute("dark_hit_flash_time = " + obj_data["dark_hit_flash_time"].asFloat() + ";");
			char.Execute("idle_stance = " + obj_data["idle_stance"].asBool() + ";");
			char.Execute("idle_stance_amount = " + obj_data["idle_stance_amount"].asFloat() + ";");
			char.Execute("target_rotation = " + obj_data["target_rotation"].asFloat() + ";");
			char.Execute("target_rotation2 = " + obj_data["target_rotation2"].asFloat() + ";");
			char.Execute("cam_rotation = " + obj_data["cam_rotation"].asFloat() + ";");
			char.Execute("cam_rotation2 = " + obj_data["cam_rotation2"].asFloat() + ";");
			char.Execute("cam_distance = " + obj_data["cam_distance"].asFloat() + ";");
			char.Execute("auto_cam_override = " + obj_data["auto_cam_override"].asFloat() + ";");
			char.Execute("on_ground = " + obj_data["on_ground"].asBool() + ";");
			char.Execute("air_time = " + obj_data["air_time"].asFloat() + ";");
			char.Execute("on_ground_time = " + obj_data["on_ground_time"].asFloat() + ";");
			char.Execute("duck_amount = " + obj_data["duck_amount"].asFloat() + ";");
			char.Execute("target_duck_amount = " + obj_data["target_duck_amount"].asFloat() + ";");
			char.Execute("duck_vel = " + obj_data["duck_vel"].asFloat() + ";");

			char.FixDiscontinuity();
			char.Execute("FixDiscontinuity();");

			/* char.Execute("ResetMind();"); */
			/* char.Execute("SetState(" + state + ");"); */
		}else if(obj.GetType() == _hotspot_object){
			string msg = "drika_load_checkpoint_data " + join(obj_data["dhs_data"].asString().split("\""), "\\\"");
			obj.ReceiveScriptMessage(msg);
		}else if(obj.GetType() == _item_object){
			bool is_held = obj_data["is_held"].asBool();
			ItemObject@ item = ReadItemID(id);
			item.CleanBlood();

			bool currently_held = item.IsHeld();
			//Detach the item from any character first.
			if(currently_held){
				int char_id = item.HeldByWhom();
				MovementObject@ holder = ReadCharacterID(char_id);
				holder.Execute("this_mo.DetachItem(" + id + ");NotifyItemDetach(" + id + ");");
			}

			int stuck_id = item.StuckInWhom();
			if(stuck_id != -1){
				MovementObject@ holder = ReadCharacterID(stuck_id);
				holder.Execute("this_mo.DetachItem(" + id + ");NotifyItemDetach(" + id + ");");
			}

			int stuck_in_whom = obj_data["stuck_in_whom"].asInt();
			if(stuck_in_whom != -1){
				MovementObject@ holder = ReadCharacterID(stuck_in_whom);
			}

			if(is_held){
				int item_slot = obj_data["item_slot"].asInt();
				//If the slot is not found then just skip attaching it.
				if(item_slot != -1){
					int char_id = obj_data["held_by_whom"].asInt();
					MovementObject@ holder = ReadCharacterID(char_id);
					bool is_left = (item_slot == _held_left || item_slot == _sheathed_left || item_slot == _sheathed_left_sheathe);
					string attachement_type = (item_slot == _held_left || item_slot == _held_right)?"_at_grip":"_at_sheathe";
					string command = "this_mo.AttachItemToSlot(" + id + ", " + attachement_type + ", " + is_left + ");HandleEditorAttachment(" + id + ", " + attachement_type + ", " + is_left + ");";
					holder.Execute(command);
				}
			}else{
				vec3 position = vec3(obj_data["translation"][0].asFloat(), obj_data["translation"][1].asFloat(), obj_data["translation"][2].asFloat());
				mat4 rotation = Mat4FromQuaternion(quaternion(obj_data["rotation"][0].asFloat(), obj_data["rotation"][1].asFloat(), obj_data["rotation"][2].asFloat(), obj_data["rotation"][3].asFloat()));
				vec3 linear_velocity = vec3(obj_data["linear_velocity"][0].asFloat(), obj_data["linear_velocity"][1].asFloat(), obj_data["linear_velocity"][2].asFloat());
				vec3 angular_velocity = vec3(obj_data["angular_velocity"][0].asFloat(), obj_data["angular_velocity"][1].asFloat(), obj_data["angular_velocity"][2].asFloat());

				mat4 physics_transform = item.GetPhysicsTransform();
				physics_transform.SetTranslationPart(position);
				physics_transform.SetRotationPart(rotation);
				item.SetPhysicsTransform(physics_transform);
				item.SetAngularVelocity(angular_velocity);
				item.SetLinearVelocity(linear_velocity);
				/* item.ActivatePhysics(); */
				item.SetThrown();
			}
		}
	}

	// Restore all the ui elements that were on the screen when the checkpoint was saved.
	@current_ui_element = null;
	for(uint i = 0; i < ui_elements.size(); i++){
		ui_elements[i].Delete();
	}
	ui_elements.resize(0);
	@current_ui_element = null;
	@current_grabber = null;

	JSONValue ui_element_data = file.getRoot()["ui_element_data"];
	for(uint i = 0; i < ui_element_data.size(); i++){
		string json_string = ui_element_data[i].asString();
		AddUIElement(json_string);
	}

	level.SendMessage("drika_checkpoint_loaded");
}

DrikaUIElement@ GetUIElement(string identifier){
	for(uint i = 0; i < ui_elements.size(); i++){
		if(ui_elements[i].ui_element_identifier == identifier){
			return ui_elements[i];
		}
	}
	return @DrikaUIElement();
}

int GetUIElementIndex(string identifier){
	for(uint i = 0; i < ui_elements.size(); i++){
		if(ui_elements[i].ui_element_identifier == identifier){
			return i;
		}
	}
	return -1;
}

void ReadCustomAnimationList(){
	JSON data;
	JSONValue root;

	SavedLevel@ saved_level = save_file.GetSavedLevel("drika_data");
	string custom_animations = saved_level.GetValue("custom_animations");
	//Always add a custom group even if it's empty. DHS decides to not render it.
	@custom_group = DrikaAnimationGroup("Custom");
	all_animations.insertLast(@custom_group);

	//Check if the saved json is parseble.
	if(custom_animations == "" || !data.parseString(custom_animations)){
		/* Log(warning, "Couldn't find custom animation list."); */
		return;
	}else{
		/* Log(warning, "Found custom animation list."); */
	}

	//Check if the existing saved data has the relevant data.
	root = data.getRoot();
	if(!root.isMember("Custom Animations")){
		/* Log(warning, "Could not find Custom Animations in JSON."); */
		return;
	}

	JSONValue custom_animation_list = root["Custom Animations"];

	for(uint i = 0; i < custom_animation_list.size(); i++){
		string animation_path = custom_animation_list[i].asString();
		if(FileExists(animation_path)){
			custom_group.AddAnimation(animation_path);
		}
	}
}

void SaveCustomAnimationList(){
	//If there are no custom animations, then just skip saving.
	if(custom_group.Size() == 0 ){
		return;
	}

	JSON data;
	JSONValue root;

	JSONValue custom_animation_list;

	for(uint i = 0; i < custom_group.animations.size(); i++){
		custom_animation_list.append(JSONValue(custom_group.animations[i]));
	}
	root["Custom Animations"] = custom_animation_list;

	data.getRoot() = root;
	SavedLevel@ saved_level = save_file.GetSavedLevel("drika_data");
	saved_level.SetValue("custom_animations", data.writeString(false));
	save_file.WriteInPlace();
}

bool AddCustomAnimation(string animation_path){
	//Check if it's already in the custom animation group.
	for(uint i = 0; i < custom_group.animations.size(); i++){
		if(custom_group.animations[i] == animation_path){
			return false;
		}
	}

	custom_group.animations.insertLast(animation_path);
	SaveCustomAnimationList();
	return true;
}

void ReadAnimationList(){
	JSON file;
	file.parseFile("Data/Scripts/drika_dialogue_animation_list.json");
	JSONValue root = file.getRoot();
	array<string> list_groups = root.getMemberNames();
	array<string> active_mods;

	array<ModID> mod_ids = GetActiveModSids();
	for(uint i = 0; i < mod_ids.size(); i++){
		active_mods.insertLast(ModGetID(mod_ids[i]));
	}

	for(uint i = 0; i < list_groups.size(); i++){
		//Skip this mod if it's not active
		if(active_mods.find(root[list_groups[i]]["Mod ID"].asString()) == -1){
			continue;
		}
		DrikaAnimationGroup new_group(list_groups[i]);
		JSONValue animation_list = root[list_groups[i]]["Animations"];
		for(uint j = 0; j < animation_list.size(); j++){
			string new_animation = animation_list[j].asString();
			if(FileExists(new_animation)){
				//This animation exists in the game fils so add it to the animation group.
				new_group.AddAnimation(new_animation);
			}
		}
		new_group.SortAlphabetically();
		all_animations.insertLast(@new_group);
	}

	read_animation_list = true;
}

void Update(){
	if(!post_init_done){
		PostInit();
	}

	while(imGUI.getMessageQueueSize() > 0 ){
		IMMessage@ message = imGUI.getNextMessage();

		if(ui_hotspot_id != -1){
			Object@ hotspot_obj = ReadObjectFromID(ui_hotspot_id);
			if(message.name == "drika_dialogue_choice_select"){
				hotspot_obj.ReceiveScriptMessage("drika_ui_event drika_dialogue_choice_select " + message.getInt(0));
			}else if(message.name == "drika_dialogue_choice_pick"){
				hotspot_obj.ReceiveScriptMessage("drika_ui_event drika_dialogue_choice_pick " + message.getInt(0));
			}else if(message.name == "grabber_activate"){
				if(!grabber_dragging){
					@current_grabber = current_ui_element.GetGrabber(message.getString(0));
				}
			}else if(message.name == "grabber_deactivate"){
				if(!grabber_dragging){
					@current_grabber = null;
				}
			}
		}

		if(message.name == "drika_button_go_to_line"){
			GetUIElement(message.getString(0)).ReadUIInstruction({"button_clicked"});
		}else if(message.name == "drika_button_hover_enter"){
			GetUIElement(message.getString(0)).ReadUIInstruction({"button_hovered"});
		}else if(message.name == "drika_input_clicked"){
			GetUIElement(message.getString(0)).ReadUIInstruction({"input_clicked"});
		}
	}

	imGUI.update();
	SetCameraPosition();
	UpdateReadFileProcesses();
	UpdateWriteFileProcesses();
	UpdateMusic();
	UpdateUIElements();
	UpdateDialogueFade();
	UpdateCheckpointFade();
	CheckPlayerDeath();

	if(show_dialogue){
		if(dialogue_move_in){
			UpdateDialogueMoveIn();
		}
		UpdateDialogueDisplacement();
	}

	if(resetting){
		PostReset();
		resetting = false;
	}
}

void PostReset(){
	added_death_screen = false;
}

void CheckPlayerDeath(){
	// Only check for player death if there is a checkpoint data available.
	if(checkpoint is null || checkpoint_fading){
		return;
	}

	int player_id = GetPlayerCharacterID();

	if(player_id != -1 && ObjectExists(player_id)){
		MovementObject@ char = ReadCharacter(player_id);

		// Allow clicking to load checkpoint after one second of showing the death screen.
		if(added_death_screen && ko_time < the_time - 1.0){
			if(GetInputPressed(char.controller_id, "attack")){
				added_death_screen = false;
				death_container.clear();
				blackout_amount = 0.0f;
				checkpoint_fading = true;
				checkpoint_fade_timer = 0.0f;
				checkpoint_fade_direction = 1.0f;
			}
		}else if(char.GetIntVar("knocked_out") != _awake) {
			AddDeathScreen();
		}
	}
}

void UpdateDialogueFade(){
	if(fading){
		blackout_amount = fade_timer / fade_duration;
		if(fade_direction == 1.0){
			if(fade_timer >= fade_duration){
				//Screen has faded all the way to black.
				fade_direction = -1.0;
				MessageWaitingForFadeOut();
			}
			fade_timer += time_step;
		}else{
			if(fade_timer <= 0.0){
				//Screen has faded all the way from black to clear.
				fading = false;
			}
			fade_timer -= time_step;
		}
	}else if(fade_to_black){
		blackout_amount = mix(starting_fade_amount, target_fade_to_black, fade_timer / max(0.0001, fade_to_black_duration));
		if(fade_timer >= fade_to_black_duration){
			fade_to_black = false;
			fade_timer = fade_duration;
			return;
		}
		fade_timer += time_step;
	}
}

void UpdateCheckpointFade(){
	if(checkpoint_fading){
		checkpoint_blackout_amount = checkpoint_fade_timer / checkpoint_fade_duration;
		if(checkpoint_fade_direction == 1.0){
			if(checkpoint_fade_timer >= checkpoint_fade_duration){
				//Screen has faded all the way to black.
				checkpoint_fade_direction = -1.0;
				LoadCheckpoint();
			}
			checkpoint_fade_timer += time_step;
		}else{
			if(checkpoint_fade_timer <= 0.0){
				//Screen has faded all the way from black to clear.
				checkpoint_fading = false;
			}
			checkpoint_fade_timer -= time_step;
		}
	}
}

void UpdateMusic(){
	if(EditorModeActive()){
		return;
	}

	if(player_died_song != ""){
		int player_id = GetPlayerCharacterID();
		if(player_id != -1 && ReadCharacter(player_id).GetIntVar("knocked_out") != _awake){
			if(current_song != player_died_song){
				current_song = player_died_song;
				if(in_combat_from_beginning_no_fade){
					SetSong(player_died_song);
				}else{
					PlaySong(player_died_song);
				}
			}
			return;
		}else if(current_song == player_died_song){
			current_song = "None";
		}
	}

	if(in_combat_song != ""){
		int player_id = GetPlayerCharacterID();
		if(player_id != -1 && ReadCharacter(player_id).QueryIntFunction("int CombatSong()") == 1){
			if(current_song != in_combat_song){
				current_song = in_combat_song;
				if(in_combat_from_beginning_no_fade){
					SetSong(in_combat_song);
				}else{
					PlaySong(in_combat_song);
				}
			}
			return;
		}else if(current_song == in_combat_song){
			current_song = "None";
		}
	}

	if(enemies_defeated_song != ""){
		int threats_remaining = ThreatsRemaining();
		if(threats_remaining == 0){
			if(current_song != enemies_defeated_song){
				current_song = enemies_defeated_song;
				if(enemies_defeated_from_beginning_no_fade){
					SetSong(enemies_defeated_song);
				}else{
					PlaySong(enemies_defeated_song);
				}
			}
			return;
		}else if(current_song == enemies_defeated_song){
			current_song = "None";
		}
	}

	if(ambient_song != ""){
		if(ambient_from_beginning_no_fade){
			SetSong(ambient_song);
		}else{
			PlaySong(ambient_song);
		}
	}
}

void UpdateUIElements(){
	for(uint i = 0; i < ui_elements.size(); i++){
		ui_elements[i].Update();
	}
}

int GetPlayerCharacterID() {
	int num = GetNumCharacters();
	for(int i=0; i<num; ++i){
		MovementObject@ char = ReadCharacter(i);
		if(char.controlled){
			return i;
		}
	}
	return -1;
}

int ThreatsRemaining() {
	int player_id = GetPlayerCharacterID();
	if(player_id == -1){
		return -1;
	}
	MovementObject@ player_char = ReadCharacter(player_id);

	int num = GetNumCharacters();
	int num_threats = 0;
	for(int i=0; i<num; ++i){
		MovementObject@ char = ReadCharacter(i);
		if(char.GetIntVar("knocked_out") == _awake && !player_char.OnSameTeam(char))
		{
			++num_threats;
		}
	}
	return num_threats;
}

void UpdateGrabber(){
	if(grabber_dragging){
		if(!ImGui_IsMouseDown(0)){
			grabber_dragging = false;
		}else{
			vec2 current_grabber_position = current_grabber.GetPosition();
			vec2 new_position = current_grabber_position + (imGUI.guistate.mousePosition - click_position);

			bool round_x_direction = (new_position.x % ui_snap_scale > (ui_snap_scale / 2.0))?true:false;
			bool round_y_direction = (new_position.y % ui_snap_scale > (ui_snap_scale / 2.0))?true:false;
			vec2 new_snap_position;
			new_snap_position.x = round_x_direction?ceil(new_position.x / ui_snap_scale):floor(new_position.x / ui_snap_scale);
			new_snap_position.y = round_y_direction?ceil(new_position.y / ui_snap_scale):floor(new_position.y / ui_snap_scale);
			new_snap_position *= ui_snap_scale;

			if(current_grabber_position != new_snap_position){
				vec2 difference = (new_snap_position - current_grabber_position);
				int direction_x = (difference.x > 0.0) ? 1 : -1;
				int direction_y = (difference.y > 0.0) ? 1 : -1;

				if(current_grabber.grabber_type == scaler){
					if(current_grabber_position.x != new_snap_position.x){
						current_ui_element.AddSize(ivec2(int(difference.x), 0), current_grabber.direction_x, current_grabber.direction_y);
						click_position.x += int(difference.x);
					}
					if(current_grabber_position.y != new_snap_position.y){
						current_ui_element.AddSize(ivec2(0, int(difference.y)), current_grabber.direction_x, current_grabber.direction_y);
						click_position.y += int(difference.y);
					}
				}else if(current_grabber.grabber_type == mover){
					if(current_grabber_position.x != new_snap_position.x){
						current_ui_element.AddPosition(ivec2(int(difference.x), 0));
						click_position.x += int(difference.x);
					}
					if(current_grabber_position.y != new_snap_position.y){
						current_ui_element.AddPosition(ivec2(0, int(difference.y)));
						click_position.y += int(difference.y);
					}
				}
			}
		}
	}else{
		if(ImGui_IsMouseDown(0) && @current_grabber != null && ImGui_IsWindowHovered()){
			click_position = imGUI.guistate.mousePosition;
			grabber_dragging = true;
		}
	}
}

void UpdateReadFileProcesses(){
	if(read_file_processes.size() > 0){
		if(LoadFile(read_file_processes[0].file_path)){
			while(true){
				string line = GetFileLine();
				if(line == "end"){
					break;
				}else{
					read_file_processes[0].data += line + "\n";
				}
			}
			Object@ hotspot_obj = ReadObjectFromID(read_file_processes[0].hotspot_id);
			read_file_processes[0].data = join(read_file_processes[0].data.split("\""), "\\\"");
			hotspot_obj.ReceiveScriptMessage("drika_read_file_done " + " " + read_file_processes[0].function_index + " " + read_file_processes[0].param_1 + "\"" + read_file_processes[0].data + "\"");
		}else{
			Log(error, "Error loading file: " + read_file_processes[0].file_path);
		}

		read_file_processes.removeAt(0);
	}
}

int update_counter = 0;

void UpdateWriteFileProcesses(){
	if(write_file_processes.size() > 0){
		StartWriteFile();
		AddFileString(write_file_processes[0].data);
		bool success = WriteFileKeepBackup(write_file_processes[0].file_path);

		/* Log(warning, "Success : " + success + ". Written placeholder for hotspot id : " + write_file_processes[0].hotspot_id); */

		Object@ hotspot_obj = ReadObjectFromID(write_file_processes[0].hotspot_id);
		hotspot_obj.ReceiveScriptMessage("drika_function_message " + " " + write_file_processes[0].function_index + " " + "drika_write_placeholder_done");

		write_file_processes.removeAt(0);
	}
}


void SetCameraPosition(){
	if((animating_camera || has_camera_control) && !EditorModeActive()){
		UpdateCameraShake();

		if(camera_settings_changed){
			camera.SetDistance(0.0f);
			camera.SetFOV(camera_zoom);
			camera.SetDOF(camera_near_blur, camera_near_dist, camera_near_transition, camera_far_blur, camera_far_dist, camera_far_transition);
			camera.SetPos(camera_position + camera_shake_position);
			camera.SetXRotation(camera_rotation.x + camera_shake_rotation.x);
			camera.SetYRotation(camera_rotation.y + camera_shake_rotation.y);
			camera.SetZRotation(camera_rotation.z + camera_shake_rotation.z);

			camera.CalcFacing();
			camera.FixDiscontinuity();
			camera_settings_changed = false;

			return;
		}

		if(enable_look_at_target){
			if(look_at_target_id != -1 && ObjectExists(look_at_target_id)){
				Object@ track_target = ReadObjectFromID(look_at_target_id);
				if(track_target.GetType() == _movement_object){
					MovementObject@ char = ReadCharacterID(look_at_target_id);
					SmoothCameraLookAt(char.rigged_object().GetAvgIKChainPos("head"));
				}else if(track_target.GetType() == _item_object){
					ItemObject@ item = ReadItemID(look_at_target_id);
					SmoothCameraLookAt(item.GetPhysicsPosition());
				}else{
					Object@ obj = ReadObjectFromID(look_at_target_id);
					SmoothCameraLookAt(obj.GetTranslation());
				}
			}
			SmoothCameraFOV(camera_zoom);
		}else{
			camera.SetXRotation(camera_rotation.x + camera_shake_rotation.x);
			camera.SetYRotation(camera_rotation.y + camera_shake_rotation.y);
			camera.SetZRotation(camera_rotation.z + camera_shake_rotation.z);
			camera.SetFOV(camera_zoom);
		}

		if(enable_move_with_target){
			if(move_with_target_id != -1 && ObjectExists(move_with_target_id)){
				Object@ track_target = ReadObjectFromID(move_with_target_id);
				if(track_target.GetType() == _movement_object){
					MovementObject@ char = ReadCharacterID(move_with_target_id);
					SmoothCameraMoveWith(char.rigged_object().GetAvgIKChainPos("torso"));
				}else if(track_target.GetType() == _item_object){
					ItemObject@ item = ReadItemID(move_with_target_id);
					SmoothCameraMoveWith(item.GetPhysicsPosition());
				}else{
					Object@ obj = ReadObjectFromID(move_with_target_id);
					SmoothCameraMoveWith(obj.GetTranslation());
				}
			}
		}else{
			camera.SetPos(camera_position + camera_shake_position);
			current_camera_position = camera_position;
		}

		camera.SetDistance(0.0f);
		UpdateListener(camera_position, vec3(0.0f), camera.GetFacing(), camera.GetUpVector());
		if(!showing_choice && !showing_interactive_ui){
			SetGrabMouse(true);
		}
	}

}

void UpdateCameraShake(){
	if(!add_camera_shake){
		camera_shake_rotation = vec3();
		camera_shake_position = vec3();
		return;
	}

	rotation_shake_timer += time_step;
	position_shake_timer += time_step;

	if(position_shake_timer > position_shake_interval){
		position_shake_timer = 0.0f;
		new_camera_shake_position = vec3(	RangedRandomFloat(-position_shake_max_distance, position_shake_max_distance),
											RangedRandomFloat(-position_shake_max_distance, position_shake_max_distance),
											RangedRandomFloat(-position_shake_max_distance, position_shake_max_distance));
	}
	camera_shake_position = mix(camera_shake_position, new_camera_shake_position, time_step * position_shake_slerp_speed);

	camera_shake_position.x = min(position_shake_max_distance, max(-position_shake_max_distance, camera_shake_position.x));
	camera_shake_position.y = min(position_shake_max_distance, max(-position_shake_max_distance, camera_shake_position.y));
	camera_shake_position.z = min(position_shake_max_distance, max(-position_shake_max_distance, camera_shake_position.z));

	if(rotation_shake_timer > rotation_shake_interval){
		rotation_shake_timer = 0.0f;
		new_camera_shake_rotation = vec3(	RangedRandomFloat(-rotation_shake_max_distance, rotation_shake_max_distance),
											RangedRandomFloat(-rotation_shake_max_distance, rotation_shake_max_distance),
											RangedRandomFloat(-rotation_shake_max_distance, rotation_shake_max_distance));
	}
	camera_shake_rotation = mix(camera_shake_rotation, new_camera_shake_rotation, time_step * rotation_shake_slerp_speed);

	camera_shake_rotation.x = min(rotation_shake_max_distance, max(-rotation_shake_max_distance, camera_shake_rotation.x));
	camera_shake_rotation.y = min(rotation_shake_max_distance, max(-rotation_shake_max_distance, camera_shake_rotation.y));
	/* camera_shake_rotation.z = min(rotation_shake_max_distance, max(-rotation_shake_max_distance, camera_shake_rotation.z)); */
	camera_shake_rotation.z = 0.0f;
}

void SmoothCameraMoveWith(vec3 target_location){
	vec3 adjusted_target_location = target_location + target_positional_difference;
	current_camera_position = mix(current_camera_position, adjusted_target_location, time_step * 10.0);
	old_camera_translation = current_camera_position;
	camera.SetPos(current_camera_position + camera_shake_position);
}

void SmoothCameraLookAt(vec3 target_location){
	float camera_distance = distance(target_location, current_camera_position);
	vec3 current_look_location = current_camera_position + (camera.GetFacing() * camera_distance);
	/* DebugDrawWireSphere(current_look_location, 0.5, vec3(1.0), _delete_on_draw); */
	camera.LookAt(mix(current_look_location, target_location, time_step * 10.0));
	old_camera_rotation = vec3(camera.GetXRotation(), camera.GetYRotation(), camera.GetZRotation());
}

void SmoothCameraFOV(float target_fov){
	float current_fov = camera.GetFOV();
	camera.SetFOV(mix(current_fov, target_fov, time_step * 5.0f));
}

void MessageWaitingForFadeOut(){
	for(uint i = 0; i < waiting_hotspot_ids.size(); i++){
		Object@ hotspot_obj = ReadObjectFromID(waiting_hotspot_ids[i]);
		hotspot_obj.ReceiveScriptMessage("drika_message fade_out_done");
	}
	waiting_hotspot_ids.resize(0);
}

bool HasFocus(){
	return ((showing_choice || showing_interactive_ui) && !EditorModeActive())?true:false;
}

bool DialogueCameraControl() {
	if((animating_camera || has_camera_control) && !EditorModeActive()){
		return true;
	}else{
		return false;
	}
}

void ReadHardwareReport(){
	string path = "Data/hwreport.txt";

	if(!LoadFile(path)){
		Print("Couldn't load " + path + "\n");
	}else{
		string new_str;
		while(true){
			new_str = GetFileLine();
			if(new_str == "end"){
				break;
			}
			if(new_str.findFirst("Report Version: ") != -1){
				report_version = join(new_str.split("Report Version: "), "");
			}else if(new_str.findFirst("OS: ") != -1){
				os = join(new_str.split("OS: "), "");
			}else if(new_str.findFirst("Arch: ") != -1){
				arch = join(new_str.split("Arch: "), "");
			}else if(new_str.findFirst("GPU Vendor: ") != -1){
				gpu_vendor += join(new_str.split("GPU Vendor: "), "");
			}else if(new_str.findFirst("GL Renderer: ") != -1){
				gl_renderer += join(new_str.split("GL Renderer: "), "");
			}else if(new_str.findFirst("GL Version: ") != -1){
				gl_version = join(new_str.split("GL Version: "), "");
			}else if(new_str.findFirst("GL Driver Version: ") != -1){
				gl_driver_version = join(new_str.split("GL Driver Version: "), "");
			}else if(new_str.findFirst("VRAM: ") != -1){
				vram = join(new_str.split("VRAM: "), "");
			}else if(new_str.findFirst("GLSL Version: ") != -1){
				glsl_version = join(new_str.split("GLSL Version: "), "");
			}else if(new_str.findFirst("Shader Model: ") != -1){
				shader_model = join(new_str.split("Shader Model: "), "");
			}
		}
	}
}
