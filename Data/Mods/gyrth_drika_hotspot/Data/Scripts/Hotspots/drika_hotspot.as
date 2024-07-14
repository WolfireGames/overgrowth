#include "drika_json_functions.as"
#include "drika_animation_group.as"
#include "drika_placeholder.as"
#include "hotspots/drika_element.as"
#include "drika_target_select.as"
#include "drika_go_to_line_select.as"
#include "hotspots/drika_slow_motion.as"
#include "hotspots/drika_on_input.as"
#include "hotspots/drika_set_morph_target.as"
#include "hotspots/drika_set_bone_inflate.as"
#include "hotspots/drika_send_character_message.as"
#include "hotspots/drika_animation.as"
#include "hotspots/drika_set_object_param.as"
#include "hotspots/drika_create_delete.as"
#include "hotspots/drika_check_character_state.as"
#include "hotspots/drika_create_particle.as"
#include "hotspots/drika_display_image.as"
#include "hotspots/drika_display_text.as"
#include "hotspots/drika_go_to_line.as"
#include "hotspots/drika_load_level.as"
#include "hotspots/drika_on_enter_exit.as"
#include "hotspots/drika_play_music.as"
#include "hotspots/drika_play_sound.as"
#include "hotspots/drika_send_level_message.as"
#include "hotspots/drika_set_camera_param.as"
#include "hotspots/drika_character_control.as"
#include "hotspots/drika_set_character.as"
#include "hotspots/drika_set_color.as"
#include "hotspots/drika_set_enabled.as"
#include "hotspots/drika_set_level_param.as"
#include "hotspots/drika_set_velocity.as"
#include "hotspots/drika_start_dialogue.as"
#include "hotspots/drika_transform_object.as"
#include "hotspots/drika_wait_level_message.as"
#include "hotspots/drika_wait.as"
#include "hotspots/drika_billboard.as"
#include "hotspots/drika_read_write_savefile.as"
#include "hotspots/drika_dialogue.as"
#include "hotspots/drika_comment.as"
#include "hotspots/drika_ai_control.as"
#include "hotspots/drika_user_interface.as"
#include "hotspots/drika_checkpoint.as"
#include "drika_quick_launch.as"
#include "drika_docs.as"

const float PI = 3.14159265359f;
double radToDeg = (180.0f / PI);
double degToRad = (PI / 180.0f);
bool show_editor = false;
bool run_in_editormode = true;
bool has_closed = true;
bool editing = false;
bool show_name = false;
string display_name = "Drika Hotspot";
bool script_finished = false;
int current_line;
array<DrikaElement@> drika_elements;
array<DrikaElement@> parallel_elements;
array<int> drika_indexes;
Object@ this_hotspot = ReadObjectFromID(hotspot.GetID());
string param_delimiter = "|";
bool is_selected = false;
array<Reference@> references;
array<DrikaElement@> continues_update_elements;
string default_preview_mesh = "Data/Objects/primitives/edged_cone.xml";
bool duplicating_hotspot = false;
bool duplicating_function = false;
bool exporting = false;
bool importing = false;
float image_scale;
vec4 image_tint;
string image_path;
bool show_image = false;
string text;
int font_size;
string font_path;
bool show_text = false;
float text_opacity = 1.0;
bool hotspot_enabled = true;
array<int> dialogue_actor_ids;
bool in_dialogue_mode = false;
float text_speed = 0.02;
array<DrikaElement@> post_init_queue;
bool element_added = false;
array<int> multi_select;
string file_content;
int ui_snap_scale = 20;
int unique_id_counter = 0;
array<DrikaElement@> imported_elements;
array<Object@> refresh_queue;
bool move_relative = false;
mat4 old_transform;
bool adding_function = false;
bool resetting = false;
string last_read_path = "";
bool reorded = false;
int display_index = 0;
bool update_scroll = false;
bool debug_current_line = false;
bool show_grid = true;
float left_over_drag_y = 0.0;
bool dragging = false;
bool steal_focus = false;
float fps_timer = 0.0f;
int fps_counter = 0;
int fps = 60;

string build_version_short = GetBuildVersionShort( );
string build_version_full = GetBuildVersionFull( );
string build_timestamp = GetBuildTimestamp( );
int monitor_count = GetMonitorCount();
bool workshop_available = IsWorkshopAvailable();
bool controller_connected = IsControllerConnected();
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
bool og_stable;

array<DrikaAnimationGroup@> all_animations;
array<DrikaAnimationGroup@> current_animations;
DrikaAnimationGroup@ custom_group;
array<string> active_mods;
array<int> refresh_queue_counter;
array<string> object_cache_names;
array<array<int>> object_cache;

const int _movement_state = 0;  // character is moving on the ground
const int _ground_state = 1;  // character has fallen down or is raising up, ATM ragdolls handle most of this
const int _attack_state = 2;  // character is performing an attack
const int _hit_reaction_state = 3;  // character was hit or dealt damage to and has to react to it in some manner
const int _ragdoll_state = 4;  // character is falling in ragdoll mode

// Coloring options
vec4 edit_outline_color = vec4(0.5, 0.5, 0.5, 1.0);
vec4 background_color();
vec4 titlebar_color();
vec4 item_hovered();
vec4 item_clicked();
vec4 text_color();

TextureAssetRef delete_icon = LoadTexture("Data/UI/ribbon/images/icons/color/Delete.png", TextureLoadFlags_NoMipmap | TextureLoadFlags_NoConvert |TextureLoadFlags_NoReduce);
TextureAssetRef duplicate_icon = LoadTexture("Data/UI/ribbon/images/icons/color/Copy.png", TextureLoadFlags_NoMipmap | TextureLoadFlags_NoConvert |TextureLoadFlags_NoReduce);

DrikaQuickLaunch quick_launch();

class Reference{
	array<DrikaElement@> elements;
	Reference(DrikaElement@ _element){
		elements.insertLast(_element);
	}
}

//Imposter class not used in hotspot script, but needed for shared class DialogueScriptEntry.
class IMText{}

void Init() {
	current_line = 0;
	show_name = (this_hotspot.GetName() != "");
	display_name = this_hotspot.GetName();
	level.ReceiveLevelEvents(hotspot.GetID());
	og_stable = GetBuildVersionShort() == "1.4.0";
	LoadPalette();
	SortFunctionsAlphabetical();
	//When the user duplicates a hotspot the editormode is active and the left alt is pressed.
	if(EditorModeActive() && GetInputDown(0, "lalt")){
		duplicating_hotspot = true;
	}else if(EditorModeActive() && GetInputDown(0, "lctrl") && GetInputDown(0, "v")){
		duplicating_hotspot = true;
	}
	InterpData();
	quick_launch.Init();
	level.SendMessage("drika_hardware_info " + this_hotspot.GetID());
}

string GetTypeString(){
	return "Drika Hotspot";
}

void LoadPalette(bool use_defaults = false){
	JSON data;
	JSONValue root;
	bool retrieve_default_palette = false;

	SavedLevel@ saved_level = save_file.GetSavedLevel("drika_data");
	string palette_data = saved_level.GetValue("drika_palette");

	//Check if the saved json is parseble, available or just use the defaults.
	if(palette_data == "" || !data.parseString(palette_data) || use_defaults){
		if(!data.parseString(palette_data)){
			Log(warning, "Unable to parse the saved JSON in the palette!");
		}
		retrieve_default_palette = true;
	}else{
		/* Log(warning, "Saved palette JSON loaded correctly."); */
	}

	//Check if the existing saved data has the relevant data.
	if(!retrieve_default_palette){
		root = data.getRoot();
		//Old palette savig method. Use defaults in stead.
		if(root.isArray()){
			retrieve_default_palette = true;
		}else if(!root.isMember("Function Palette")){
			Log(warning, "Could not find Function Palette in JSON.");
			retrieve_default_palette = true;
		}else if(!root.isMember("UI Palette")){
			Log(warning, "Could not find UI Palette in JSON.");
			retrieve_default_palette = true;
		}
	}

	//Get the defaults values.
	if(retrieve_default_palette){
		/* Log(warning, "Loading the default palette."); */
		if(!data.parseFile("Data/Scripts/drika_default_palette.json")){
			Log(warning, "Error loading the default palette.");
			return;
		}
		root = data.getRoot();
	}else{
		/* Log(warning, "Using the palette from the saved JSON."); */
	}

	JSONValue color_palette = root["Function Palette"];

	display_colors.resize(drika_element_names.size());
	for(uint i = 0; i < drika_element_names.size(); i++){
		if(i < color_palette.size()){
			JSONValue color = color_palette[i];
			vec4 palette_color = vec4(color[0].asFloat(), color[1].asFloat(), color[2].asFloat(), color[3].asFloat());
			display_colors[i] = palette_color;
		}else{
			display_colors[i] = vec4(1.0);
		}
	}

	JSONValue ui_palette = root["UI Palette"];

	JSONValue bg_color = ui_palette["Background Color"];
	background_color = vec4(bg_color[0].asFloat(), bg_color[1].asFloat(), bg_color[2].asFloat(), bg_color[3].asFloat());

	JSONValue tb_color = ui_palette["Titlebar Color"];
	titlebar_color = vec4(tb_color[0].asFloat(), tb_color[1].asFloat(), tb_color[2].asFloat(), tb_color[3].asFloat());

	JSONValue ih_color = ui_palette["Item Hovered"];
	item_hovered = vec4(ih_color[0].asFloat(), ih_color[1].asFloat(), ih_color[2].asFloat(), ih_color[3].asFloat());

	JSONValue ic_color = ui_palette["Item Clicked"];
	item_clicked = vec4(ic_color[0].asFloat(), ic_color[1].asFloat(), ic_color[2].asFloat(), ic_color[3].asFloat());

	JSONValue t_color = ui_palette["Text Color"];
	text_color = vec4(t_color[0].asFloat(), t_color[1].asFloat(), t_color[2].asFloat(), t_color[3].asFloat());
}

void QueryAnimation(string query){
	current_animations.resize(0);
	//If the query is empty then just return the whole list.
	if(query == ""){
		current_animations = all_animations;
	}else{
		//The query can be multiple words separated by spaces.
		array<string> split_query = query.split(" ");

		for(uint i = 0; i < all_animations.size(); i++){
			DrikaAnimationGroup@ current_group = all_animations[i];
			DrikaAnimationGroup new_group(current_group.name);

			for(uint j = 0; j < current_group.animations.size(); j++){
				bool found_result = true;
				for(uint k = 0; k < split_query.size(); k++){
					//Could not find part of query in the database.
					if(ToLowerCase(current_group.animations[j]).findFirst(ToLowerCase(split_query[k])) == -1){
						found_result = false;
						break;
					}
				}
				//Only if all parts of the query are found then add the result.
				if(found_result){
					new_group.AddAnimation(current_group.animations[j]);
				}
			}

			if(new_group.animations.size() > 0){
				current_animations.insertLast(@new_group);
			}
		}
	}
}

string ToLowerCase(string input){
	string output;
	for(uint i = 0; i < input.length(); i++){
		if(input[i] >= 65 &&  input[i] <= 90){
			string lower_case('0');
			lower_case[0] = input[i] + 32;
			output += lower_case;
		}else{
			string new_character('0');
			new_character[0] = input[i];
			output += new_character;
		}
	}
	return output;
}

void SortFunctionsAlphabetical(){
	sorted_element_names = drika_element_names;
	sorted_element_names.sortAsc();
	//Remove empty function names.
	for(uint i = 0; i < sorted_element_names.size(); i++){
		if(sorted_element_names[i] == ""){
			sorted_element_names.removeAt(i);
			i--;
		}
	}
}

void SetEnabled(bool val){
	hotspot_enabled = val;
}

bool AcceptConnectionsTo(Object @other){
	if(drika_elements.size() > 0){
		if(GetCurrentElement().placeholder.cube_id == other.GetID()){
			return false;
		}else if(GetCurrentElement().connection_types.find(other.GetType()) != -1){
			return true;
		}
	}
	return false;
}

bool ConnectTo(Object @other){
	if(drika_elements.size() > 0){
		bool return_value = GetCurrentElement().ConnectTo(other);
		Save();
		return return_value;
	}
	return false;
}

bool Disconnect(Object @other){
	if(drika_elements.size() > 0){
		return GetCurrentElement().Disconnect(other);
	}
	return false;
}

void Dispose() {
	level.StopReceivingLevelEvents(hotspot.GetID());
	if(GetInputDown(0, "delete")){
		if(editing && drika_elements.size() > 0){
			GetCurrentElement().EditDone();
		}
		for(uint i = 0; i < drika_elements.size(); i++){
			drika_elements[i].Delete();
			drika_elements[i].deleted = true;
		}
	}
}

void SetParameters(){
	params.AddIntCheckbox("Debug Current Line", debug_current_line);
	debug_current_line = (params.GetInt("Debug Current Line") == 1);
	params.AddIntCheckbox("Show UI Grid", show_grid);
	show_grid = (params.GetInt("Debug Current Line") == 1);
	params.AddInt("UI Snap Scale", ui_snap_scale);
	ui_snap_scale = params.GetInt("UI Snap Scale");
	params.AddIntCheckbox("Run in EditorMode", run_in_editormode);
	run_in_editormode = (params.GetInt("Run in EditorMode") == 1);
	params.AddIntCheckbox("Transform Relative", move_relative);
	move_relative = (params.GetInt("Transform Relative") == 1);
}

void InterpData(){
	if(params.HasParam("Script Data")){
		JSON data;
		if(!data.parseString(params.GetString("Script Data"))){
			Log(warning, "Unable to parse the JSON in the Script Data!");
		}else{
			for( uint i = 0; i < data.getRoot()["functions"].size(); ++i ) {
				DrikaElement@ new_element = InterpElement(none, data.getRoot()["functions"][i]);
				drika_elements.insertLast(@new_element);
				drika_indexes.insertLast(drika_elements.size() - 1);
				post_init_queue.insertLast(@new_element);
			}
		}
	}
	/* Log(info, "Interp of script done. Hotspot number: " + this_hotspot.GetID()); */
	ReorderElements();

	for(int i = 0; i < int(drika_elements.size()) - 1; i++){
		DrikaElement@ current_element = drika_elements[i];
		@current_element.nodes_slot_then_connected = drika_elements[current_element.index + 1];
	}
}

void LaunchCustomGUI(){
	show_editor = !show_editor;
	if(show_editor){
		update_scroll = true;
		multi_select = {current_line};
		level.SendMessage("drika_hotspot_editing " + this_hotspot.GetID());
		has_closed = false;
		if(drika_elements.size() > 0){
			GetCurrentElement().StartEdit();
		}
	}
	SendHotspotStateChange();
}

void SendHotspotStateChange(){
	for(uint i = 0; i < drika_elements.size(); i++){
		if(show_editor){
			drika_elements[i].HotspotStartEdit();
		}else{
			drika_elements[i].HotspotStopEdit();
		}
	}
}

void Update(){
	fps_timer += time_step;
	// Update the fps counter every half a second.
	if(fps_timer > 0.5){
		fps = int(fps_counter / fps_timer);
		fps_timer = 0.0f;
		fps_counter = 0;
	}

	//The post init queue is necessary so that Update is executing it, and not the Draw functions.
	//The Draw and DrawEditor sometimes can have issues such as spawning hotspots that crash the game.
	if(post_init_queue.size() > 0){
		for(uint i = 0; i < post_init_queue.size(); i++){
			post_init_queue[i].PostInit();
		}
		post_init_queue.resize(0);

		if(element_added || importing){
			element_added = false;
			ReorderElements();
			GetCurrentElement().StartEdit();
			Save();
		}

		duplicating_hotspot = false;
		duplicating_function = false;
		importing = false;
		adding_function = false;
		return;
	}

	if(show_editor){
		quick_launch.Update();
	}

	for(uint i = 0; i < refresh_queue_counter.size(); i++){
		refresh_queue_counter[i] += 1;
	}

	for(uint i = 0; i < refresh_queue_counter.size(); i++){
		if(refresh_queue_counter[i] > 2){
			Object@ child = refresh_queue[i];
			child.SetTranslation(child.GetTranslation());
			child.SetRotation(child.GetRotation());
			child.SetScale(child.GetScale());
			refresh_queue.removeAt(i);
			refresh_queue_counter.removeAt(i);
			i--;
		}
	}

	if(!show_editor && !has_closed){
		has_closed = true;
		Reset();
		Save();
		SendHotspotStateChange();
	}

	if(EditorModeActive() && editing == false){
		SwitchToEditing();
	}else if(!EditorModeActive() && editing == true){
		SwitchToPlaying();
	}

	if(move_relative && EditorModeActive()){
		if(this_hotspot.IsSelected() && !is_selected){
			//Switched from not selected to selected.
			old_transform = this_hotspot.GetTransform();
			is_selected = true;
		}else if(!this_hotspot.IsSelected() && is_selected){
			//Switched from being selected to not selected.
			is_selected = false;
		}else if(this_hotspot.IsSelected()){
			mat4 current_transform = this_hotspot.GetTransform();
			vec3 old_translation = old_transform.GetTranslationPart();
			vec3 current_translation = current_transform.GetTranslationPart();

			quaternion old_rotation = QuaternionFromMat4(old_transform.GetRotationPart());
			quaternion current_rotation = QuaternionFromMat4(current_transform.GetRotationPart());

			if(old_translation != current_translation || old_rotation != current_rotation){
				vec3 origin = this_hotspot.GetTranslation();
				vec3 translation_offset = current_translation - old_translation;
				quaternion rotaton_offset = quaternion(current_rotation.x - old_rotation.x, current_rotation.y - old_rotation.y, current_rotation.z - old_rotation.z, current_rotation.w - old_rotation.w);

				for(uint i = 0; i < drika_indexes.size(); i++){
					DrikaElement@ element = drika_elements[drika_indexes[i]];
					mat4 before_mat = old_transform.GetRotationPart();
					mat4 after_mat = current_transform.GetRotationPart();
					element.RelativeTransform(origin, translation_offset, before_mat, after_mat);
				}
				old_transform = this_hotspot.GetTransform();
			}
		}
	}

	// Do not allow the functions to be updated when the UI isn't shown and run in editormode is disabled.
	if(!run_in_editormode && EditorModeActive() && !show_editor){
		return;
	// DHS can still be edited when the hotspot is disabled, but when the UI isn't visible then stop updating.
	}else if(!show_editor && !this_hotspot.GetEnabled()){
		return;
	}

	if(drika_indexes.size() > 0){
		if(hotspot_enabled){
			UpdateContinuesElements();
		}

		if(!show_editor && hotspot_enabled){
			UpdateParallelOperations();

			if(!script_finished){
				DrikaElement@ current_element = GetCurrentElement();
				if(current_element.parallel_operation || current_element.Trigger()){
					if(current_line == int(drika_indexes.size() - 1)){
						script_finished = true;
					}else{
						current_line += 1;
						display_index = drika_indexes[current_line];
					}

					current_element.PostTrigger();
				}
			}
		}else if(!reorded){
			GetCurrentElement().Update();
		}
	}
}

void UpdateContinuesElements(){
	if(hotspot_enabled){
		for(uint i = 0; i < continues_update_elements.size(); i++){
			continues_update_elements[i].Update();
		}
	}
}

void UpdateParallelOperations(){
	for(uint i = 0; i < parallel_elements.size(); i++){
		if(parallel_elements[i].Trigger()){
			parallel_elements.removeAt(i);
			i--;
		}
	}
	if(!script_finished){
		if(GetCurrentElement().parallel_operation){
			//Check if the element is already added.
			for(uint i = 0; i < parallel_elements.size(); i++){
				if(parallel_elements[i].index == GetCurrentElement().index){
					return;
				}
			}
			parallel_elements.insertLast(GetCurrentElement());
		}
	}
}

void SwitchToEditing(){
	editing = true;
}

void SwitchToPlaying(){
	if(this_hotspot.IsSelected()){
		this_hotspot.SetSelected(false);
	}
	show_editor = false;
	editing = false;
	Reset();
}

void DrawEditor(){
	if(camera.GetFlags() == kPreviewCamera){
		return;
	}

	if(show_name){
		DebugDrawText(this_hotspot.GetTranslation() + vec3(0, 0.5, 0), display_name, 1.0, false, _delete_on_draw);
	}

	if(show_editor){
		DebugDrawBillboard("Data/Textures/drika_hotspot.png", this_hotspot.GetTranslation(), 0.5, vec4(0.25, 1.0, 0.25, 1.0), _delete_on_draw);
	}else{
		DebugDrawBillboard("Data/Textures/drika_hotspot.png", this_hotspot.GetTranslation(), 0.5, vec4(0.5, 0.5, 0.5, 1.0), _delete_on_draw);
	}

	if(show_editor){
		ImGui_PushStyleColor(ImGuiCol_WindowBg, background_color);
		ImGui_PushStyleColor(ImGuiCol_PopupBg, background_color);
		ImGui_PushStyleColor(ImGuiCol_TitleBg, background_color);
		ImGui_PushStyleColor(ImGuiCol_TitleBgActive, titlebar_color);
		ImGui_PushStyleColor(ImGuiCol_TitleBgCollapsed, background_color);
		ImGui_PushStyleColor(ImGuiCol_MenuBarBg, titlebar_color);
		ImGui_PushStyleColor(ImGuiCol_Text, text_color);
		ImGui_PushStyleColor(ImGuiCol_Header, titlebar_color);
		ImGui_PushStyleColor(ImGuiCol_HeaderHovered, item_hovered);
		ImGui_PushStyleColor(ImGuiCol_HeaderActive, item_clicked);
		ImGui_PushStyleColor(ImGuiCol_ScrollbarBg, background_color);

		ImGui_PushStyleColor(ImGuiCol_ScrollbarGrab, titlebar_color);
		ImGui_PushStyleColor(ImGuiCol_ScrollbarGrabHovered, item_hovered);
		ImGui_PushStyleColor(ImGuiCol_ScrollbarGrabActive, item_clicked);

		ImGui_PushStyleColor(ImGuiCol_CloseButton, background_color);
		ImGui_PushStyleColor(ImGuiCol_Button, titlebar_color);
		ImGui_PushStyleColor(ImGuiCol_ButtonHovered, item_hovered);
		ImGui_PushStyleColor(ImGuiCol_ButtonActive, item_clicked);

		ImGui_PushStyleColor(ImGuiCol_CheckMark, text_color);
		ImGui_PushStyleColor(ImGuiCol_TextSelectedBg, titlebar_color);

		ImGui_PushStyleColor(ImGuiCol_SliderGrab, item_clicked);
		ImGui_PushStyleColor(ImGuiCol_SliderGrabActive, titlebar_color);

		ImGui_PushStyleColor(ImGuiCol_FrameBg, item_hovered);
		ImGui_PushStyleColor(ImGuiCol_FrameBgHovered, titlebar_color);
		ImGui_PushStyleColor(ImGuiCol_FrameBgActive, item_clicked);

		ImGui_PushStyleColor(ImGuiCol_ResizeGrip, item_hovered);
		ImGui_PushStyleColor(ImGuiCol_ResizeGripHovered, titlebar_color);
		ImGui_PushStyleColor(ImGuiCol_ResizeGripActive, item_clicked);

		ImGui_PushStyleVar(ImGuiStyleVar_WindowMinSize, vec2(350, 350));

		ImGui_SetNextWindowSize(vec2(600.0f, 400.0f), ImGuiSetCond_FirstUseEver);
		ImGui_SetNextWindowPos(vec2(100.0f, 100.0f), ImGuiSetCond_FirstUseEver);

		if(steal_focus){
			steal_focus = false;
			ImGui_SetNextWindowFocus();
		}

		ImGui_Begin("Drika Hotspot" + (show_name?" - " + display_name:" " + this_hotspot.GetID()) + "###Drika Hotspot", show_editor, ImGuiWindowFlags_MenuBar);
		ImGui_SetWindowFontScale(1.0);
		ImGui_PopStyleVar();

		ImGui_PushStyleVar(ImGuiStyleVar_WindowMinSize, vec2(350, 350));
		ImGui_SetNextWindowSize(vec2(900.0f, 450.0f), ImGuiSetCond_FirstUseEver);

		if(ImGui_BeginPopupModal("Settings###Edit", 0)){
			ImGui_PushItemWidth(-1);
			GetCurrentElement().DrawSettings();
			ImGui_PopItemWidth();

			vec4 window_info = GetWindowInfo();

			ImGui_EndPopup();

			if(CheckClosePopup(window_info)){
				GetCurrentElement().ApplySettings();
				Save();
			}
		}
		ImGui_PopStyleVar();

		quick_launch.Draw();

		if(ImGui_BeginMenuBar()){
			if(ImGui_BeginMenu("Add")){
				AddFunctionMenuItems();
				ImGui_EndMenu();
			}
			if(ImGui_BeginMenu("Settings")){
				if(ImGui_Checkbox("Enabled", hotspot_enabled)){
					this_hotspot.SetEnabled(hotspot_enabled);
				}
				if(ImGui_Checkbox("Show Name", show_name)){
					if(!show_name){
						this_hotspot.SetName("");
					}else{
						this_hotspot.SetName(display_name);
					}
				}
				if(show_name){
					if(ImGui_InputText("Name", display_name, 64)){
						this_hotspot.SetName(display_name);
					}
				}
				if(ImGui_Checkbox("Debug Current Line", debug_current_line)){
					params.SetInt("Debug Current Line", debug_current_line?1:0);
				}

				if(ImGui_Checkbox("Run in EditorMode", run_in_editormode)){
					params.SetInt("Run in EditorMode", run_in_editormode?1:0);
				}

				if(ImGui_Checkbox("Show UI Grid", show_grid)){
					params.SetInt("Show UI Grid", show_grid?1:0);
					level.SendMessage("drika_set_show_grid " + show_grid);
				}

				if(ImGui_DragInt("UI Snap Scale", ui_snap_scale, 1.0, 15, 150, "%.0f")){
					params.SetInt("UI Snap Scale", ui_snap_scale);
					level.SendMessage("drika_set_ui_snap_scale " + ui_snap_scale);
				}

				/* if(ImGui_Checkbox("Transform Relative", move_relative)){
					params.SetInt("Transform Relative", move_relative?1:0);
					//Reset the transform when turning this setting on or off.
					old_transform = this_hotspot.GetTransform();
				} */

				DrawPaletteModal();
				if(ImGui_Selectable("Configure Palette", false, ImGuiSelectableFlags_DontClosePopups)){
					ImGui_OpenPopup("Configure Palette");
				}

				ImGui_EndMenu();
			}
			if(ImGui_BeginMenu("Import/Export")){
				if(ImGui_Selectable("Export to file")){
					exporting = true;
					ExportToFile();
					exporting = false;
				}
				if(ImGui_Selectable("Import from file")){
					ImportFromFile();
				}
				if(ImGui_Selectable("Copy to clipboard")){
					exporting = true;
					CopyToClipBoard();
					exporting = false;
				}
				if(ImGui_Selectable("Paste from clipboard")){
					PasteFromClipboard();
				}
				ImGui_EndMenu();
			}

			if(ImGui_BeginMenu("About")){
				DrawHardwareInfoModal();
				if(ImGui_Selectable("Hardware Info", false, ImGuiSelectableFlags_DontClosePopups)){
					ImGui_OpenPopup("Hardware Info");
				}

				DrawCreditsModal();
				if(ImGui_Selectable("Credits", false, ImGuiSelectableFlags_DontClosePopups)){
					ImGui_OpenPopup("Credits");
				}

				DrawDocsModal();
				if(ImGui_Selectable("Docs", false, ImGuiSelectableFlags_DontClosePopups)){
					ImGui_OpenPopup("Docs");
				}

				ImGui_EndMenu();
			}

			if(ImGui_ImageButton(delete_icon, vec2(10), vec2(0), vec2(1), 5, vec4(0))){
				DeleteSelectedFunctions();
			}

			if(ImGui_IsItemHovered()){
				ImGui_PushStyleColor(ImGuiCol_PopupBg, titlebar_color);
				ImGui_SetTooltip("Delete (x)");
				ImGui_PopStyleColor();
			}
			if(ImGui_ImageButton(duplicate_icon, vec2(10), vec2(0), vec2(1), 5, vec4(0))){
				DuplicateSelectedFunctions();
			}
			if(ImGui_IsItemHovered()){
				ImGui_PushStyleColor(ImGuiCol_PopupBg, titlebar_color);
				ImGui_SetTooltip("Duplicate (c)");
				ImGui_PopStyleColor();
			}
			ImGui_EndMenuBar();
		}

		if(!ImGui_IsPopupOpen("###Edit") && !ImGui_IsPopupOpen("Palette") && !ImGui_IsPopupOpen("Quick Launch") && ImGui_IsWindowFocused()){
			if(ImGui_IsKeyPressed(ImGui_GetKeyIndex(ImGuiKey_UpArrow))){
				if(current_line > 0){
					multi_select = {current_line - 1};
					GetCurrentElement().EditDone();
					display_index = drika_indexes[current_line - 1];
					current_line -= 1;
					update_scroll = true;
					GetCurrentElement().StartEdit();
					steal_focus = true;
				}
			}else if(ImGui_IsKeyPressed(ImGui_GetKeyIndex(ImGuiKey_DownArrow))){
				if(current_line < int(drika_elements.size() - 1)){
					multi_select = {current_line + 1};
					GetCurrentElement().EditDone();
					display_index = drika_indexes[current_line + 1];
					current_line += 1;
					update_scroll = true;
					GetCurrentElement().StartEdit();
					steal_focus = true;
				}
			}else if(drika_elements.size() > 0 && !reorded && post_init_queue.size() == 0){
				if(!GetInputDown(0, "lctrl") && !GetInputDown(0, "lalt")){
					if(ImGui_IsKeyPressed(ImGui_GetKeyIndex(ImGuiKey_Enter))){
						if(GetCurrentElement().has_settings){
							GetCurrentElement().StartSettings();
							ImGui_OpenPopup("###Edit");
						}
					}else if(ImGui_IsWindowHovered() && ImGui_IsKeyPressed(ImGui_GetKeyIndex(ImGuiKey_X))){
						DeleteSelectedFunctions();
					}else if(ImGui_IsWindowHovered() && ImGui_IsKeyPressed(ImGui_GetKeyIndex(ImGuiKey_C))){
						DuplicateSelectedFunctions();
					}
				}
			}
		}

		// After duplicating with C, the function might not be ready since PostInit isn't triggered yet.
		// So stop rendering for one update while that finishes.
		if(post_init_queue.size() == 0){
			for(uint i = 0; i < drika_indexes.size(); i++){
				int item_no = drika_indexes[i];
				vec4 text_color = drika_elements[item_no].GetDisplayColor();
				ImGui_PushStyleColor(ImGuiCol_Text, text_color);
				bool line_selected = display_index == int(item_no) || multi_select.find(i) != -1;

				string display_string = drika_elements[item_no].line_number + drika_elements[item_no].GetDisplayString();
				// Remove any new lines so that every function stays as one selectable object.
				display_string = join(display_string.split("\n"), "");
				float space_for_characters = ImGui_CalcTextSize(display_string).x;

				if(space_for_characters > ImGui_GetWindowContentRegionWidth()){
					display_string = display_string.substr(0, int(display_string.length() * (ImGui_GetWindowContentRegionWidth() / space_for_characters)) - 3) + "...";
				}

				if(update_scroll && display_index == int(item_no)){
					ImGui_SetScrollHere(0.5);
					update_scroll = false;
				}

				if(ImGui_Selectable(display_string, line_selected, ImGuiSelectableFlags_AllowDoubleClick)){ 
					// This item has been selected that is inside multiselect, but no modifier key is pressed.
					if(multi_select.find(i) != -1 && !dragging && multi_select.size() > 1 && !GetInputDown(0, "lshift") && !GetInputDown(0, "lctrl")){
						multi_select = {i};
					}
				}

				if(ImGui_IsItemHovered() && ImGui_IsMouseClicked(0)){
					left_over_drag_y = 0.0;
					if(ImGui_IsMouseDoubleClicked(0)){
						if(drika_elements[drika_indexes[i]].has_settings){
							GetCurrentElement().StartSettings();
							ImGui_OpenPopup("###Edit");
						}
					}else{
						if(GetInputDown(0, "lshift")){
							int starting_point = multi_select[multi_select.size() - 1];
							int ending_point = i;
							int direction = starting_point < ending_point?1:-1;

							for(int j = starting_point; true; j += direction){
								if(multi_select.find(j) == -1){
									multi_select.insertLast(j);
								}
								if(j == ending_point){
									break;
								}
							}
						}else if(GetInputDown(0, "lctrl")){
							int find_selected = multi_select.find(i);
							if(multi_select.size() == 0){
								multi_select.insertLast(i);
							}else if(find_selected != -1 && multi_select.size() != 1){
								multi_select.removeAt(find_selected);
								GetCurrentElement().EditDone();
								display_index = int(drika_indexes[multi_select[multi_select.size() - 1]]);
								current_line = int(multi_select[multi_select.size() - 1]);
								GetCurrentElement().StartEdit();
								steal_focus = true;
								continue;
							}else{
								multi_select.insertLast(i);
							}
						}else if(multi_select.find(i) == -1){
							multi_select = {i};
						}
						GetCurrentElement().EditDone();
						display_index = int(item_no);
						current_line = int(i);
						GetCurrentElement().StartEdit();
						steal_focus = true;
					}
				}

				if(ImGui_IsItemHovered() && ImGui_IsMouseClicked(1)){
					GetCurrentElement().LeftClick();
				}

				ImGui_PopStyleColor();
				if(ImGui_IsItemActive()){
					float drag_dy = ImGui_GetMouseDragDelta(0).y + left_over_drag_y;
					bool can_drag_up = multi_select.find(0) == -1;
					bool can_drag_down = multi_select.find(drika_indexes.size() - 1) == -1;
					float drag_threshold = 17.0;

					if(drag_dy < -drag_threshold && can_drag_up){
						while(drag_dy < -drag_threshold && can_drag_up){
							// Dragging Up

							for(uint k = 0; k < drika_indexes.size(); k++){
								for(uint j = 0; j < multi_select.size(); j++){
									if(multi_select[j] == int(k)){
										SwapIndexes(k, k - 1);
										reorded = true;
									}
								}
							}

							for(uint j = 0; j < multi_select.size(); j++){
								multi_select[j] -= 1;
							}

							current_line -= 1;
							drag_dy += drag_threshold;
							can_drag_up = multi_select.find(0) == -1;
						}
						left_over_drag_y = drag_dy;
						ImGui_ResetMouseDragDelta();
					}else if(drag_dy > drag_threshold && can_drag_down){
						while(drag_dy > drag_threshold && can_drag_down){
							// Dragging Down

							for(int k = drika_indexes.size() - 1; k > -1; k--){
								for(uint j = 0; j < multi_select.size(); j++){
									if(multi_select[j] == int(k)){
										SwapIndexes(k, k + 1);
										reorded = true;
									}
								}
							}

							for(uint j = 0; j < multi_select.size(); j++){
								multi_select[j] += 1;
							}

							current_line += 1;
							drag_dy -= drag_threshold;
							can_drag_down = multi_select.find(drika_indexes.size() - 1) == -1;
						}
						left_over_drag_y = drag_dy;
						ImGui_ResetMouseDragDelta();
					}
				}
			}
		}

		ImGui_End();

		if(drika_elements.size() > 0 && !reorded && post_init_queue.size() == 0){
			GetCurrentElement().DrawEditing();
		}

		if(ImGui_IsMouseDragging(0)){
			dragging = true;
		}else{
			dragging = false;
		}

		ImGui_PopStyleColor(28);
	}

	if(reorded && !ImGui_IsMouseDragging(0)){
		reorded = false;
		ReorderElements();
		Save();
	}
}

void DrawPaletteModal(){
	ImGui_SetNextWindowSize(vec2(700.0f, 450.0f), ImGuiSetCond_FirstUseEver);
	if(ImGui_BeginPopupModal("Configure Palette", 0)){
		if(ImGui_Button("Reset to defaults")){
			LoadPalette(true);
			SavePalette();
		}
		ImGui_Separator();

		ImGui_PushItemWidth(-1);
		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, 200.0);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Function Colors");
		ImGui_NextColumn();
		ImGui_NextColumn();

		for(uint i = 0; i < sorted_element_names.size(); i++){
			drika_element_types current_element_type = drika_element_types(drika_element_names.find(sorted_element_names[i]));
			if(current_element_type == none){
				continue;
			}
			ImGui_PushStyleColor(ImGuiCol_Text, display_colors[current_element_type]);
			ImGui_AlignTextToFramePadding();
			ImGui_Text(drika_element_names[current_element_type] + " " + current_element_type);
			ImGui_PopStyleColor();

			ImGui_NextColumn();
			ImGui_PushItemWidth(-1);
			ImGui_ColorEdit4("##Palette Color" + i, display_colors[current_element_type]);
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}

		ImGui_AlignTextToFramePadding();
		ImGui_Text("UI Colors");
		ImGui_NextColumn();
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Background Color");
		ImGui_NextColumn();
		ImGui_PushItemWidth(-1);
		ImGui_ColorEdit4("##Background Color", background_color);
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Titlebar Color");
		ImGui_NextColumn();
		ImGui_PushItemWidth(-1);
		ImGui_ColorEdit4("##Titlebar Color", titlebar_color);
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Item Hovered");
		ImGui_NextColumn();
		ImGui_PushItemWidth(-1);
		ImGui_ColorEdit4("##Item Hovered", item_hovered);
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Item Clicked");
		ImGui_NextColumn();
		ImGui_PushItemWidth(-1);
		ImGui_ColorEdit4("##Item Clicked", item_clicked);
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Text Color");
		ImGui_NextColumn();
		ImGui_PushItemWidth(-1);
		ImGui_ColorEdit4("##Text Color", text_color);
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		ImGui_PopItemWidth();

		vec4 window_info = GetWindowInfo();

		ImGui_EndPopup();

		if(CheckClosePopup(window_info)){
			SavePalette();
		}
	}
}

void DrawHardwareInfoModal(){
	ImGui_SetNextWindowSize(vec2(450.0f, 450.0f), ImGuiSetCond_FirstUseEver);
	if(ImGui_BeginPopupModal("Hardware Info", ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)){

		vec2 resolution = vec2(GetScreenWidth(), GetScreenHeight());

		ImGui_BeginChild("Hardware Info", vec2(-1, -1));
		ImGui_PushItemWidth(-1);
		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, 200.0);

		ImGui_Text("Hardware Info");
		ImGui_NextColumn();
		ImGui_NextColumn();

		ImGui_Text("Build Version");
		ImGui_NextColumn();
		ImGui_Text(build_version_full);
		ImGui_NextColumn();

		ImGui_Text("Build Timestamp");
		ImGui_NextColumn();
		ImGui_Text(build_timestamp);
		ImGui_NextColumn();

		ImGui_Text("Monitor Count");
		ImGui_NextColumn();
		ImGui_Text("" + monitor_count);
		ImGui_NextColumn();

		ImGui_Text("Workshop Available");
		ImGui_NextColumn();
		ImGui_Text("" + workshop_available);
		ImGui_NextColumn();

		ImGui_Text("Controller Connected");
		ImGui_NextColumn();
		ImGui_Text("" + controller_connected);
		ImGui_NextColumn();

		ImGui_Text("Resolution");
		ImGui_NextColumn();
		ImGui_Text(resolution.x + "x" + resolution.y);
		ImGui_NextColumn();

		ImGui_Text("Report Version");
		ImGui_NextColumn();
		ImGui_Text(report_version);
		ImGui_NextColumn();

		ImGui_Text("OS");
		ImGui_NextColumn();
		ImGui_Text(os);
		ImGui_NextColumn();

		ImGui_Text("Arch");
		ImGui_NextColumn();
		ImGui_Text(arch);
		ImGui_NextColumn();

		ImGui_Text("GPU Vendor");
		ImGui_NextColumn();
		ImGui_Text(gpu_vendor);
		ImGui_NextColumn();

		ImGui_Text("GL Renderer");
		ImGui_NextColumn();
		ImGui_Text(gl_renderer);
		ImGui_NextColumn();

		ImGui_Text("GL Version");
		ImGui_NextColumn();
		ImGui_Text(gl_version);
		ImGui_NextColumn();

		ImGui_Text("GL Driver Version");
		ImGui_NextColumn();
		ImGui_Text(gl_driver_version);
		ImGui_NextColumn();

		ImGui_Text("VRAM");
		ImGui_NextColumn();
		ImGui_Text(vram);
		ImGui_NextColumn();

		ImGui_Text("GLSL Version");
		ImGui_NextColumn();
		ImGui_Text(glsl_version);
		ImGui_NextColumn();

		ImGui_Text("Shader Model");
		ImGui_NextColumn();
		ImGui_Text(shader_model);
		ImGui_NextColumn();

		ImGui_PopItemWidth();
		ImGui_EndChild();

		if((!ImGui_IsMouseHoveringAnyWindow() && ImGui_IsMouseClicked(0)) || ImGui_IsKeyPressed(ImGui_GetKeyIndex(ImGuiKey_Escape))){
			steal_focus = true;
			ImGui_CloseCurrentPopup();
		}

		ImGui_EndPopup();
	}
}

void DrawCreditsModal(){
	ImGui_SetNextWindowSize(vec2(450.0f, 450.0f), ImGuiSetCond_FirstUseEver);
	if(ImGui_BeginPopupModal("Credits", ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)){

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, 180.0);

		ImGui_NextColumn();
		ImGui_Text("Created by");
		ImGui_Spacing();
		ImGui_NextColumn();

		ImGui_NextColumn();
		ImGui_Text("Gyrth");
		ImGui_NextColumn();

		ImGui_NextColumn();
		ImGui_Text("Fason7");
		ImGui_NextColumn();

		ImGui_NextColumn();
		ImGui_Text("Alpines");
		ImGui_NextColumn();

		ImGui_NextColumn();
		ImGui_Text("Merlyn");
		ImGui_NextColumn();

		vec4 window_info = GetWindowInfo();

		ImGui_EndPopup();

		CheckClosePopup(window_info);
	}
}

void DeleteSelectedFunctions(){
	if(drika_elements.size() > 0){
		GetCurrentElement().EditDone();
		array<int> sorted_selected = multi_select;
		sorted_selected.sortAsc();
		for(uint i = 0; i < sorted_selected.size(); i++){
			DeleteDrikaElement(sorted_selected[i] - i);
		}
		multi_select.resize(0);

		ReorderElements();
		Save();
		if(drika_elements.size() > 0){
			multi_select = {current_line};
			display_index = drika_indexes[current_line];
		}
		if(drika_elements.size() > 0){
			GetCurrentElement().StartEdit();
		}
	}
}

void DuplicateSelectedFunctions(){
	if(drika_elements.size() > 0){
		duplicating_function = true;
		int last_selected = multi_select[multi_select.size() - 1];
		array<int> sorted_selected = multi_select;
		multi_select.resize(0);
		sorted_selected.sortDesc();
		int insert_at = sorted_selected[0];

		GetCurrentElement().EditDone();

		for(uint i = 0; i < sorted_selected.size(); i++){
			DrikaElement@ target = drika_elements[drika_indexes[sorted_selected[i]]];
			DrikaElement@ new_element = InterpElement(target.drika_element_type, target.GetSaveData());
			// The elements are added in reverse order so insertLast can be used.
			int index_new_element = insert_at + (sorted_selected.size() - i);
			new_element.SetIndex(index_new_element);
			post_init_queue.insertLast(@new_element);

			multi_select.insertLast(insert_at + 1 + i);
			drika_elements.insertLast(new_element);
			drika_indexes.insertAt(insert_at + 1, drika_elements.size() - 1);
			display_index = drika_indexes[insert_at + 1];

			if(target.index == last_selected){
				current_line = insert_at + 1;
			}
		}

		element_added = true;
		update_scroll = true;
	}
}

void SavePalette(){
	JSON data;
	JSONValue root;

	JSONValue function_palette;

	for(uint i = 0; i < display_colors.size(); i++){
		JSONValue color = JSONValue(JSONarrayValue);
		color.append(display_colors[i].x);
		color.append(display_colors[i].y);
		color.append(display_colors[i].z);
		color.append(display_colors[i].a);
		function_palette.append(color);
	}
	root["Function Palette"] = function_palette;

	JSONValue ui_palette;

	JSONValue bg_color = JSONValue(JSONarrayValue);
	bg_color.append(background_color.x);
	bg_color.append(background_color.y);
	bg_color.append(background_color.z);
	bg_color.append(background_color.a);
	ui_palette["Background Color"] = bg_color;

	JSONValue tb_color = JSONValue(JSONarrayValue);
	tb_color.append(titlebar_color.x);
	tb_color.append(titlebar_color.y);
	tb_color.append(titlebar_color.z);
	tb_color.append(titlebar_color.a);
	ui_palette["Titlebar Color"] = tb_color;

	JSONValue ih_color = JSONValue(JSONarrayValue);
	ih_color.append(item_hovered.x);
	ih_color.append(item_hovered.y);
	ih_color.append(item_hovered.z);
	ih_color.append(item_hovered.a);
	ui_palette["Item Hovered"] = ih_color;

	JSONValue ic_color = JSONValue(JSONarrayValue);
	ic_color.append(item_clicked.x);
	ic_color.append(item_clicked.y);
	ic_color.append(item_clicked.z);
	ic_color.append(item_clicked.a);
	ui_palette["Item Clicked"] = ic_color;

	JSONValue t_color = JSONValue(JSONarrayValue);
	t_color.append(text_color.x);
	t_color.append(text_color.y);
	t_color.append(text_color.z);
	t_color.append(text_color.a);
	ui_palette["Text Color"] = t_color;

	root["UI Palette"] = ui_palette;

	data.getRoot() = root;
	SavedLevel@ saved_level = save_file.GetSavedLevel("drika_data");
	saved_level.SetValue("drika_palette", data.writeString(false));
	save_file.WriteInPlace();
}

void ExportToFile(){
	string write_path = GetUserPickedWritePath("txt", "Data/Dialogues");
	if(write_path != ""){
		/* Log(info,"Save to file: " + write_path); */

		JSON data;
		JSONValue functions;
		array<int> target_indexes = drika_indexes;

		//Only export the selected functions.
		if(multi_select.size() > 1){
			array<int> sorted_selected = multi_select;
			sorted_selected.sortAsc();
			target_indexes = sorted_selected;
		}

		for(uint i = 0; i < target_indexes.size(); i++){
			drika_elements[target_indexes[i]].SetExportIndex(i);
		}

		for(uint i = 0; i < target_indexes.size(); i++){
			JSONValue function_data = drika_elements[target_indexes[i]].GetSaveData();
			function_data["function"] = JSONValue(drika_elements[target_indexes[i]].drika_element_type);
			functions.append(function_data);
		}

		for(uint i = 0; i < target_indexes.size(); i++){
			drika_elements[target_indexes[i]].ClearExportIndex();
		}

		data.getRoot()["functions"] = functions;
		string send_data = join(data.writeString(false).split("\""), "\\\"");
		level.SendMessage("drika_export_to_file " + write_path + " " + "\"" + send_data + "\"");
	}
}

void CopyToClipBoard(){
	/* Log(info,"Copy To Clipboard"); */

	JSON data;
	JSONValue functions;
	array<int> target_indexes = drika_indexes;

	//Only export the selected functions.
	if(multi_select.size() > 1){
		array<int> sorted_selected = multi_select;
		sorted_selected.sortAsc();
		target_indexes = sorted_selected;
	}

	for(uint i = 0; i < target_indexes.size(); i++){
		drika_elements[target_indexes[i]].SetExportIndex(i);
	}

	for(uint i = 0; i < target_indexes.size(); i++){
		JSONValue function_data = drika_elements[target_indexes[i]].GetSaveData();
		function_data["function"] = JSONValue(drika_elements[target_indexes[i]].drika_element_type);
		functions.append(function_data);
	}

	for(uint i = 0; i < target_indexes.size(); i++){
		drika_elements[target_indexes[i]].ClearExportIndex();
	}

	data.getRoot()["functions"] = functions;
	ImGui_SetClipboardText(data.writeString(false));
}

void PasteFromClipboard(){
	string clipboard_content = ImGui_GetClipboardText();
	InterpImportData(clipboard_content);
}

void ImportFromFile(){
	string read_path = GetUserPickedReadPath("txt", "Data/Dialogues");
	if(read_path != ""){
		read_path = read_path;
		level.SendMessage("drika_read_file " + hotspot.GetID() + " " + 0 + " " + read_path + " " + "drika_import_from_file");
	}
}

void DeleteDrikaElement(int index){
	DrikaElement@ target = drika_elements[drika_indexes[index]];

	target.Delete();
	target.deleted = true;

	for(uint i = 0; i < drika_indexes.size(); i++){
		if(drika_indexes[i] > drika_indexes[index]){
			drika_indexes[i] -= 1;
		}
	}

	drika_elements.removeAt(drika_indexes[index]);
	drika_indexes.removeAt(index);

	// If the last element is deleted then the target needs to be the previous element.
	if(current_line > 0 && current_line == int(drika_elements.size())){
		display_index = drika_indexes[current_line - 1];
		current_line -= 1;
	}else if(drika_elements.size() > 0){
		display_index = drika_indexes[current_line];
	}
}

void SwapIndexes(int index_1, int index_2){
	int value_1 = drika_indexes[index_1];
	drika_indexes[index_1] = drika_indexes[index_2];
	drika_indexes[index_2] = value_1;
}

DrikaElement@ GetCurrentElement(){
	if(drika_elements.size() == 0){
		return DrikaElement();
	}
	return drika_elements[drika_indexes[current_line]];
}

void ReorderElements(){
	for(uint index = 0; index < drika_indexes.size(); index++){
		DrikaElement@ current_element = drika_elements[drika_indexes[index]];
		current_element.SetIndex(index);

		int item_no = drika_indexes[index];
		current_element.line_number = drika_elements[item_no].index + ".";
		int initial_length = max(1, (5 - current_element.line_number.length()));
		for(int j = 0; j < initial_length; j++){
			current_element.line_number += " ";
		}
	}

	for(uint index = 0; index < drika_indexes.size(); index++){
		DrikaElement@ current_element = drika_elements[drika_indexes[index]];
		current_element.ReorderDone();
	}
}

void InsertElement(DrikaElement@ new_element){
	if(drika_elements.size() > 0){
		GetCurrentElement().EditDone();
	}
	drika_elements.insertLast(new_element);
	//There are no functions in the list yet.
	if(drika_indexes.size() < 1){
		drika_indexes.insertLast(drika_elements.size() - 1);
		display_index = drika_indexes[0];
	//Add a the new function to the next line and make that line the current one.
	}else{
		drika_indexes.insertAt(current_line + 1, drika_elements.size() - 1);
		display_index = drika_indexes[current_line + 1];
		current_line += 1;
	}
	ReorderElements();
}

void ReceiveMessage(string msg){
	TokenIterator token_iter;
	token_iter.Init();
	token_iter.FindNextToken(msg);
	string token = token_iter.GetToken(msg);

	if(token == "level_event"){
		if(editing){
			array<string> editor_messages;
			while(token_iter.FindNextToken(msg)){
				editor_messages.insertLast(token_iter.GetToken(msg));
			}

			//This message is send when ctrl + s is pressed.
			if(editor_messages[0] == "save_selected_dialogue"){
				Save();
			}else if(editor_messages[0] == "drika_hotspot_editing" && atoi(editor_messages[1]) != this_hotspot.GetID()){
				show_editor = false;
			}else if(editor_messages[0] == "drika_dialogue_add_new_custom_animation"){
				//Only add the custom animation if there is existing data.
				if(all_animations.size() > 0){
					string custom_animation_path = editor_messages[1];

					for(uint i = 0; i < custom_group.animations.size(); i++){
						if(custom_group.animations[i] == custom_animation_path){
							return;
						}
					}
					custom_group.animations.insertLast(custom_animation_path);
				}
			}
			GetCurrentElement().ReceiveEditorMessage(editor_messages);
		}else{
			// Discard the messages when this hotspot is disabled.
			if(!script_finished && drika_elements.size() > 0 && hotspot_enabled){
				token_iter.FindNextToken(msg);
				string first_message = token_iter.GetToken(msg);
				array<string> all_messages = {first_message};

				while(token_iter.FindNextToken(msg)){
					all_messages.insertLast(token_iter.GetToken(msg));
				}

				GetCurrentElement().ReceiveMessage(first_message);
				GetCurrentElement().ReceiveMessage(all_messages);
			}
		}
	}else if(token == "drika_dialogue_add_animation_group"){
		token_iter.FindNextToken(msg);
		string group_name = token_iter.GetToken(msg);

		DrikaAnimationGroup new_group(group_name);
		all_animations.insertLast(@new_group);
		if(group_name == "Custom"){
			@custom_group = @new_group;
		}
	}else if(token == "drika_dialogue_add_animation"){
		token_iter.FindNextToken(msg);
		string new_animation = token_iter.GetToken(msg);

		all_animations[all_animations.size() -1].AddAnimation(new_animation);
	}else if(token == "drika_read_file_done"){
		token_iter.FindNextToken(msg);
		int function_index = atoi(token_iter.GetToken(msg));

		token_iter.FindNextToken(msg);
		string param_1 = token_iter.GetToken(msg);

		string file_content = "";
		while(token_iter.FindNextToken(msg)){
			file_content += token_iter.GetToken(msg);
		}

		if(param_1 == "drika_import_from_file"){
			InterpImportData(file_content);
		}else if(param_1 == "doc"){
			SetDocData(file_content, function_index);
		}else{
			drika_elements[drika_indexes[function_index]].ReceiveMessage(file_content, param_1);
		}
	}else if(token == "drika_external_hotspot"){
		token_iter.FindNextToken(msg);
		string event = token_iter.GetToken(msg);

		token_iter.FindNextToken(msg);
		int char_id = atoi(token_iter.GetToken(msg));

		token_iter.FindNextToken(msg);
		int source_hotspot_id = atoi(token_iter.GetToken(msg));

		GetCurrentElement().ReceiveMessage(event, char_id, source_hotspot_id);
	}else if(token == "drika_ui_event"){

		token_iter.FindNextToken(msg);
		string event = token_iter.GetToken(msg);

		token_iter.FindNextToken(msg);
		int param_1 = atoi(token_iter.GetToken(msg));

		GetCurrentElement().ReceiveMessage(event, param_1);
	}else if(token == "drika_ui_instruction"){
		array<string> instruction;

		while(token_iter.FindNextToken(msg)){
			instruction.insertLast(token_iter.GetToken(msg));
		}

		GetCurrentElement().ReadUIInstruction(instruction);
	}else if(token == "drika_ui_function_event"){
		token_iter.FindNextToken(msg);
		string ui_element_identifier = token_iter.GetToken(msg);
		array<string> parameters;

		while(token_iter.FindNextToken(msg)){
			parameters.insertLast(token_iter.GetToken(msg));
		}

		for(uint i = 0; i < drika_elements.size(); i++){
			if(drika_elements[i].drika_element_type == drika_user_interface){
				DrikaUserInterface@ ui_function = cast<DrikaUserInterface@>(drika_elements[i]);
				if(ui_function.ui_element_identifier == ui_element_identifier){
					ui_function.ReadUIFunctionEvent(parameters);
					break;
				}
			}
		}
	}else if(token == "drika_message"){
		array<string> drika_messages;

		while(token_iter.FindNextToken(msg)){
			drika_messages.insertLast(token_iter.GetToken(msg));
		}

		GetCurrentElement().ReceiveMessage(drika_messages);
	}else if(token == "drika_load_checkpoint_data"){
		string checkpoint_data;

		while(token_iter.FindNextToken(msg)){
			checkpoint_data += token_iter.GetToken(msg);
		}
		SetCheckpointData(checkpoint_data);
	}else if(token == "drika_function_message"){
		token_iter.FindNextToken(msg);
		int function_index = atoi(token_iter.GetToken(msg));

		token_iter.FindNextToken(msg);
		string message = token_iter.GetToken(msg);

		drika_elements[drika_indexes[function_index]].ReceiveMessage(message);
	}else if(token == "drika_hardware_info"){
		token_iter.FindNextToken(msg);
		report_version = token_iter.GetToken(msg);

		token_iter.FindNextToken(msg);
		os = token_iter.GetToken(msg);

		token_iter.FindNextToken(msg);
		arch = token_iter.GetToken(msg);

		token_iter.FindNextToken(msg);
		gpu_vendor = token_iter.GetToken(msg);

		token_iter.FindNextToken(msg);
		gl_renderer = token_iter.GetToken(msg);

		token_iter.FindNextToken(msg);
		gl_version = token_iter.GetToken(msg);

		token_iter.FindNextToken(msg);
		gl_driver_version = token_iter.GetToken(msg);

		token_iter.FindNextToken(msg);
		vram = token_iter.GetToken(msg);

		token_iter.FindNextToken(msg);
		glsl_version = token_iter.GetToken(msg);

		token_iter.FindNextToken(msg);
		shader_model = token_iter.GetToken(msg);
	}
}

void InterpImportData(string import_data){
	JSON data;
	duplicating_hotspot = true;
	array<int> created_indexes;
	importing = true;
	imported_elements.resize(0);

	if(!data.parseString(import_data)){
		Log(warning, "Unable to parse the JSON in the Script Data!");
		duplicating_hotspot = false;
		return;
	}else{
		int start_index = (drika_indexes.size() > 0)?current_line + 1:current_line;
		for( uint i = 0; i < data.getRoot()["functions"].size(); ++i ) {
			DrikaElement@ new_element = InterpElement(none, data.getRoot()["functions"][i]);
			drika_elements.insertLast(@new_element);
			drika_indexes.insertAt(start_index + i, drika_elements.size() - 1);
			created_indexes.insertLast(start_index + i);
			post_init_queue.insertLast(@new_element);
			imported_elements.insertLast(@new_element);
		}
	}

	if(drika_indexes.size() == 0){
		duplicating_hotspot = false;
		return;
	}

	multi_select = created_indexes;
	current_line = multi_select[multi_select.size() - 1];
	display_index = drika_indexes[current_line];
}

DrikaElement@ GetImportElement(int _index){
	if(int(imported_elements.size()) > _index){
		return imported_elements[_index];
	}else{
		return null;
	}
}

void HandleEvent(string event, MovementObject @mo){
	if(event == "enter" || event == "exit"){
		if(!script_finished && drika_indexes.size() > 0 && hotspot_enabled){
			GetCurrentElement().ReceiveMessage(event, mo.GetID());
		}
	}
}

void HandleEventItem(string event, ItemObject @obj){
	Log(info, "on item works!");
	if(event == "enter"){
		if(!script_finished && drika_indexes.size() > 0 && hotspot_enabled){
			GetCurrentElement().ReceiveMessage("ItemEnter", obj.GetID());
			GetCurrentElement().ReceiveMessage("ItemEnter", obj.GetLabel(), obj.GetID());
		}
	}
}

void Reset(){
	if(drika_elements.size() == 0){
		return;
	}

	object_cache.resize(0);
	object_cache_names.resize(0);

	GetCurrentElement().EditDone();
	//If the user is editing the script then stay with the current line to edit.
	current_line = 0;
	multi_select = {current_line};
	display_index = drika_indexes[current_line];
	parallel_elements.resize(0);

	script_finished = false;
	in_dialogue_mode = false;
	resetting = true;
	for(int i = int(drika_indexes.size() - 1); i > -1; i--){
		drika_elements[drika_indexes[i]].Reset();
	}
	resetting = false;
	if(editing && show_editor){
		GetCurrentElement().StartEdit();
	}
	ClearDialogueActors();
}

void AddDialogueActor(MovementObject@ char){
	if(char is null){
		return;
	}
	int character_id = char.GetID();

	if(dialogue_actor_ids.find(character_id) == -1){
		dialogue_actor_ids.insertLast(character_id);
		vec3 char_position;
		float char_rotation;

		//Use the spawn cube if the character has just been Reset();
		if(char.GetIntVar("updated") != 0){
			char_position = char.position;
			RiggedObject@ rigged_object = char.rigged_object();
			Skeleton@ skeleton = rigged_object.skeleton();
			// Get relative chest transformation
			int chest_bone = skeleton.IKBoneStart("torso");
			BoneTransform chest_frame_matrix = rigged_object.GetFrameMatrix(chest_bone);
			quaternion quat = chest_frame_matrix.rotation;
			vec3 facing = Mult(quat, vec3(1,0,0));
			float rot = atan2(facing.x, facing.z) * 180.0f / PI;
			char_rotation = floor(rot + 0.5f);
		}else{
			Object@ char_obj = ReadObjectFromID(char.GetID());
			char_position = char_obj.GetTranslation();
			vec3 facing = Mult(char_obj.GetRotation(), vec3(-1,0,0));
			float rot = atan2(facing.x, facing.z) * 180.0f / PI;
			char_rotation = floor(rot + 0.5f);
		}

		char.ReceiveScriptMessage("set_dialogue_control true");
		char.ReceiveScriptMessage("set_dialogue_position " + char_position.x + " " + char_position.y + " " + char_position.z);
		char.ReceiveScriptMessage("set_rotation " + char_rotation);
		char.Execute("FixDiscontinuity();");
		if(!char.static_char){
			string no_character_collision = "reset_no_collide = " + (the_time + 1000.0f) + ";";
			char.Execute(no_character_collision);
		}
		/* char.rigged_object().anim_client().Reset(); */
	}
}

void RemoveDialogueActor(MovementObject@ char){
	if(char is null){
		return;
	}

	int character_id = char.GetID();

	int index = dialogue_actor_ids.find(character_id);
	if(index != -1){
		char.ReceiveScriptMessage("set_dialogue_control false");
		string no_character_collision = "reset_no_collide = " + the_time + ";";
		char.Execute(no_character_collision);
		dialogue_actor_ids.removeAt(index);
	}
}

void ClearDialogueActors(){
	for(uint i = 0; i < dialogue_actor_ids.size(); i++){
		if(MovementObjectExists(dialogue_actor_ids[i])){
			MovementObject@ char = ReadCharacterID(dialogue_actor_ids[i]);
			char.Execute("roll_ik_fade = 0.0f;");
			char.ReceiveScriptMessage("set_dialogue_control false");
			string no_character_collision = "reset_no_collide = " + the_time + ";";
			char.Execute(no_character_collision);
			/* char.rigged_object().anim_client().Reset(); */
		}
	}
	dialogue_actor_ids.resize(0);
}

void Draw(){
	if(camera.GetFlags() == kPreviewCamera){
		return;
	}

	fps_counter += 1;

	if(debug_current_line && drika_elements.size() > 0 && post_init_queue.size() == 0){
		if(!hotspot_enabled){
			DebugDrawText(this_hotspot.GetTranslation() + vec3(0, 0.75, 0), "Disabled", 1.0, false, _delete_on_draw);
		}else{
			string trimmed_display_string = GetCurrentElement().GetDisplayString();
			trimmed_display_string = join(trimmed_display_string.split("\n"), "");

			if(trimmed_display_string.length() > 35){
				trimmed_display_string = trimmed_display_string.substr(0, 35) + "...";
			}
			
			DebugDrawText(this_hotspot.GetTranslation() + vec3(0, 0.75, 0), trimmed_display_string, 1.0, false, _delete_on_draw);
		}
	}

	if(show_text){
		vec2 pos(GetScreenWidth() *0.5, GetScreenHeight() *0.2);
		TextMetrics metrics = GetTextAtlasMetrics(font_path, font_size, 0, text);
		pos.x -= metrics.bounds_x * 0.5;
		DrawTextAtlas(font_path, font_size, 0, text,
					  int(pos.x+2), int(pos.y+2), vec4(vec3(0.0f), text_opacity * 0.5));
		DrawTextAtlas(font_path, font_size, 0, text,
					  int(pos.x), int(pos.y), vec4(vec3(1.0f), text_opacity));
	}
	
	if(show_image){
		HUDImage@ image = hud.AddImage();
		image.SetImageFromPath(image_path);

		vec2 screen_dims = vec2(GetScreenWidth(), GetScreenHeight());
		float screen_aspect_ratio = screen_dims.x / screen_dims.y;

		vec2 image_dims = vec2(image.GetWidth(), image.GetHeight());
		float image_aspect_ratio = image_dims.x / image_dims.y;

		float fill_scale = screen_aspect_ratio <= image_aspect_ratio ?
			screen_dims.x / image_dims.x :
			screen_dims.y / image_dims.y;

		float new_image_scale = image_scale * fill_scale;
		vec2 image_pos = vec2(
			screen_dims.x - (image_dims.x * new_image_scale),
			screen_dims.y - (image_dims.y * new_image_scale)) * 0.5f;

		image.scale = vec3(new_image_scale, new_image_scale, 0.0f);
		image.position = vec3(image_pos.x, image_pos.y, 5.0f);
		image.color = image_tint;
	}
}

void ShowImage(string _image_path, vec4 _image_tint, float _image_scale){
	if(_image_path == ""){
		show_image = false;
	}else{
		image_scale = _image_scale;
		image_tint = _image_tint;
		image_path = _image_path;
		show_image = true;
	}
}

void ShowText(string _text, int _font_size, string _font_path){
	DisposeTextAtlases();
	if(_text == ""){
		show_text = false;
	}else{
		text = _text;
		font_size = _font_size;
		font_path = _font_path;
		show_text = true;
	}
}

void Save(){
	JSON data;
	JSONValue functions;

	for(uint i = 0; i < drika_indexes.size(); i++){
		JSONValue function_data = drika_elements[drika_indexes[i]].GetSaveData();
		function_data["function"] = JSONValue(drika_elements[drika_indexes[i]].drika_element_type);
		functions.append(function_data);
	}
	data.getRoot()["functions"] = functions;
	params.SetString("Script Data", data.writeString(false));
}

string GetCheckpointData(){
	JSON json;
	JSONValue function_data;

	json.getRoot()["current_line"] = JSONValue(current_line);
	json.getRoot()["display_index"] = JSONValue(display_index);
	json.getRoot()["script_finished"] = JSONValue(script_finished);
	json.getRoot()["in_dialogue_mode"] = JSONValue(in_dialogue_mode);

	for(uint i = 0; i < drika_indexes.size(); i++){
		function_data.append(drika_elements[drika_indexes[i]].GetCheckpointData());
	}

	json.getRoot()["function_data"] = function_data;
	return json.writeString(false);
}

void SetCheckpointData(string checkpoint_data){
	JSON json;
	if(!json.parseString(checkpoint_data)){
		Log(error, "Could not parse checkpoint JSON!");
		return;
	}
	JSONValue root = json.getRoot();
	JSONValue function_data = root["function_data"];

	current_line = root["current_line"].asInt();
	display_index = root["display_index"].asInt();
	script_finished = root["script_finished"].asBool();
	in_dialogue_mode = root["in_dialogue_mode"].asBool();

	for(uint i = 0; i < function_data.size(); i++){
		drika_elements[drika_indexes[i]].SetCheckpointData(function_data[i]);
	}
}

string GetUniqueID(){
	unique_id_counter += 1;
	return hotspot.GetID() + "" + unique_id_counter;
}

void AddFunctionMenuItems(){
	for(uint i = 0; i < sorted_element_names.size(); i++){
		drika_element_types current_element_type = drika_element_types(drika_element_names.find(sorted_element_names[i]));
		if(current_element_type == none){
			continue;
		}
		ImGui_PushStyleColor(ImGuiCol_Text, display_colors[current_element_type]);
		if(ImGui_Selectable(sorted_element_names[i])){
			adding_function = true;
			DrikaElement@ new_element = InterpElement(current_element_type, JSONValue());
			post_init_queue.insertLast(@new_element);
			InsertElement(new_element);
			element_added = true;
			update_scroll = true;
			multi_select = {current_line};
		}
		ImGui_PopStyleColor();
	}
}

DrikaElement@ InterpElement(drika_element_types element_type, JSONValue &in function_json){
	drika_element_types target_element_type;
	if(element_type == none){
		if(function_json.isMember("function")){
			target_element_type = drika_element_types(function_json["function"].asInt());
		}else{
			Log(warning, "Found a function without a function identifier.");
		}
	}else{
		target_element_type = element_type;
	}

	switch(target_element_type){
		case drika_wait_level_message:
			return DrikaWaitLevelMessage(function_json);
		case drika_wait:
			return DrikaWait(function_json);
		case drika_set_enabled:
			return DrikaSetEnabled(function_json);
		case drika_set_character:
			return DrikaSetCharacter(function_json);
		case drika_create_particle:
			return DrikaCreateParticle(function_json);
		case drika_play_sound:
			return DrikaPlaySound(function_json);
		case drika_go_to_line:
			return DrikaGoToLine(function_json);
		case drika_on_enter_exit:
			return DrikaOnEnterExit(function_json);
		case drika_on_item_enter_exit:
			return DrikaOnEnterExit(function_json);
		case drika_send_level_message:
			return DrikaSendLevelMessage(function_json);
		case drika_start_dialogue:
			return DrikaStartDialogue(function_json);
		case drika_set_object_param:
			return DrikaSetObjectParam(function_json);
		case drika_set_level_param:
			return DrikaSetLevelParam(function_json);
		case drika_set_camera_param:
			return DrikaSetCameraParam(function_json);
		case drika_create_delete:
			return DrikaCreateDelete(function_json);
		case drika_transform_object:
			return DrikaTransformObject(function_json);
		case drika_set_color:
			return DrikaSetColor(function_json);
		case drika_play_music:
			return DrikaPlayMusic(function_json);
		case drika_character_control:
			return DrikaCharacterControl(function_json);
		case drika_display_text:
			return DrikaDisplayText(function_json);
		case drika_display_image:
			return DrikaDisplayImage(function_json);
		case drika_load_level:
			return DrikaLoadLevel(function_json);
		case drika_check_character_state:
			return DrikaCheckCharacterState(function_json);
		case drika_set_velocity:
			return DrikaSetVelocity(function_json);
		case drika_slow_motion:
			return DrikaSlowMotion(function_json);
		case drika_on_input:
			return DrikaOnInput(function_json);
		case drika_set_morph_target:
			return DrikaSetMorphTarget(function_json);
		case drika_set_bone_inflate:
			return DrikaSetBoneInflate(function_json);
		case drika_send_character_message:
			return DrikaSendCharacterMessage(function_json);
		case drika_animation:
			return DrikaAnimation(function_json);
		case drika_billboard:
			return DrikaBillboard(function_json);
		case drika_read_write_savefile:
			return DrikaReadWriteSaveFile(function_json);
		case drika_comment:
			return DrikaComment(function_json);
		case drika_dialogue:
			return DrikaDialogue(function_json);
		case drika_ai_control:
			return DrikaAIControl(function_json);
		case drika_user_interface:
			return DrikaUserInterface(function_json);
		case drika_checkpoint:
			return DrikaCheckpoint(function_json);
	}
	return DrikaElement();
}

string GetStringBetween(string source, string first, string second){
	array<string> first_cut = source.split(first);
	if(first_cut.size() <= 1){
		return "";
	}
	array<string> second_cut = first_cut[1].split(second);

	if(second_cut.size() <= 1){
		return "";
	}
	return second_cut[0];
}

void RefreshChildren(Object@ obj){
	if(obj.GetType() == _group){
		array<int> children = obj.GetChildren();
		for(uint i = 0; i < children.size(); i++){
			Object@ child = ReadObjectFromID(children[i]);
			refresh_queue.insertLast(child);
			refresh_queue_counter.insertLast(0);
			RefreshChildren(child);
		}
	}
}

void RefreshActors(Object@ obj){
	if(obj.GetType() == _movement_object){
		MovementObject@ char = ReadCharacterID(obj.GetID());
		vec3 translation = obj.GetTranslation();
		quaternion rotation = obj.GetRotation();

		float extra_y_rot = (-90.0 / 180.0f * PI);
		rotation = rotation.opMul(quaternion(vec4(0.0, 1.0, 0.0, extra_y_rot)));

		vec3 facing = Mult(rotation, vec3(0,0,1));
		float rot = atan2(facing.x, facing.z) * 180.0f / PI;
		float new_rotation = floor(rot + 0.5f);
		vec3 new_facing = Mult(quaternion(vec4(0, 1, 0, new_rotation * PI / 180.0f)), vec3(1, 0, 0));

		if(char.GetBoolVar("dialogue_control")){
			vec3 direction = SingularityFix(rotation);
			char.ReceiveScriptMessage("set_rotation " + direction.y);
			char.ReceiveScriptMessage("set_dialogue_position " + translation.x + " " + translation.y + " " + translation.z);
			char.Execute("this_mo.velocity = vec3(0.0, 0.0, 0.0);");
			char.Execute("FixDiscontinuity();");
		}else{
			char.SetRotationFromFacing(new_facing);
			char.position = translation;
			char.velocity = vec3(0.0, 0.0, 0.0);
		}

	}else if(obj.GetType() == _group){
		array<int> children = obj.GetChildren();
		for(uint i = 0; i < children.size(); i++){
			Object@ child = ReadObjectFromID(children[i]);
			RefreshActors(child);
		}
	}
}

array<string> GetReferences(){
	array<string> reference_strings;

	for(uint i = 0; i < references.size(); i++){
		reference_strings.insertLast(references[i].elements[0].reference_string);
	}
	return reference_strings;
}

DrikaElement@ GetReferenceElement(string reference){
	for(uint i = 0; i < references.size(); i++){
		//Use the first element in the array as it's the last element to be registered.
		if(references[i].elements[0].reference_string == reference){
			return references[i].elements[0];
		}
	}
	return null;
}

void SetLastReadPath(string new_path){
	array<string> split_path = new_path.split("/");
	split_path.removeLast();
	last_read_path = join(split_path, "/");
}

string GetLastReadPath(string default_path){
	if(last_read_path == ""){
		return default_path;
	}else{
		return ShortenPath(last_read_path);
	}
}

string ShortenPath(string path){
	array<string> split_path = path.split("Data/");

	for(int i = split_path.size() - 1; i > -1; i--){
		if(split_path[i] == ""){continue;}

		array<string> possible_path_array;
		for(uint j = i; j < split_path.size(); j++){
			possible_path_array.insertLast(split_path[j]);
		}

		string possible_path = "Data/" + join(possible_path_array, "Data/");
		if(FileExists(possible_path)){
			return possible_path;
		}
	}

	return "";
}

vec4 GetWindowInfo(){
	vec4 info;

	info.x = ImGui_GetWindowPos().x;
	info.y = ImGui_GetWindowPos().y;
	info.z = info.x + ImGui_GetWindowSize().x;
	info.a = info.y + ImGui_GetWindowSize().y;

	return info;
}

bool CheckClosePopup(vec4 window_info){
	vec2 mouse_pos = ImGui_GetMousePos();
	bool hovering_window = (mouse_pos.x > window_info.x && mouse_pos.x < window_info.z && mouse_pos.y > window_info.y && mouse_pos.y < window_info.a);

	if(ImGui_IsRootWindowOrAnyChildHovered() || ImGui_IsAnyItemHovered()){
		hovering_window = true;
	}

	if((!hovering_window && ImGui_IsMouseClicked(0)) || ImGui_IsKeyPressed(ImGui_GetKeyIndex(ImGuiKey_Escape))){
		steal_focus = true;
		ImGui_CloseCurrentPopup();
		return true;
	}

	return false;
}

void AddContinuesUpdateElement(DrikaElement@ element){
	// Check if this element has already been added.
	for(uint i = 0; i < continues_update_elements.size(); i++){
		if(element is continues_update_elements[i]){
			return;
		}
	}

	continues_update_elements.insertLast(element);
}

void RemoveContinuesUpdateElement(DrikaElement@ element){
	for(uint i = 0; i < continues_update_elements.size(); i++){
		if(element is continues_update_elements[i]){
			continues_update_elements.removeAt(i);
			return;
		}
	}
}

void RemoveContinuesUpdateElementByName(string name){
	for(uint i = 0; i < continues_update_elements.size(); i++){
		if(continues_update_elements[i].reference_string == name){
			continues_update_elements.removeAt(i);
			i--;
		}
	}
}

vec3 SingularityFix(quaternion rotation){
	// Set camera euler angles from rotation matrix
	vec3 front = Mult(rotation, vec3(0,0,1));
	float x_rot;
	float y_rot;
	float z_rot;

	double sqw = rotation.w*rotation.w;
	double sqx = rotation.x*rotation.x;
	double sqy = rotation.y*rotation.y;
	double sqz = rotation.z*rotation.z;

	double unit = sqx + sqy + sqz + sqw; // if normalised is one, otherwise is correction factor
	double test = rotation.x * rotation.w - rotation.y * rotation.z;
	if(test > 0.49999*unit) { // singularity at north pole
		x_rot = PI / 2.0f;
		y_rot = 2.0f * atan2(rotation.y, rotation.x);
		z_rot = 0.0f;

		vec3 v = vec3(x_rot, y_rot, z_rot);
		vec3 norm = NormalizeAngles(radToDeg * v);
		x_rot = norm.x;
		y_rot = norm.y;
		z_rot = norm.z;
	}else if(test < -0.49999*unit) { // singularity at south pole
		y_rot = -2.0f * atan2(rotation.y, rotation.x);
		x_rot = -PI / 2.0f;
		z_rot = 0.0f;

		vec3 v = vec3(x_rot, y_rot, z_rot);
		vec3 norm = NormalizeAngles(radToDeg * v);
		x_rot = norm.x;
		y_rot = norm.y;
		z_rot = norm.z;
	}else{
		y_rot = atan2(front.x, front.z)*180.0f/PI;
		x_rot = asin(front[1])*-180.0f/PI;
		vec3 up = Mult(rotation, vec3(0,1,0));
		vec3 expected_right = normalize(cross(front, vec3(0,1,0)));
		vec3 expected_up = normalize(cross(expected_right, front));
		z_rot = atan2(dot(up,expected_right), dot(up, expected_up))*180.0f/PI;
	}

	return vec3(floor(x_rot * 100.0f + 0.5f) / 100.0f, floor(y_rot * 100.0f + 0.5f) / 100.0f, floor(z_rot * 100.0f + 0.5f) / 100.0f);
}

vec3 NormalizeAngles(vec3 angles){
	double X = NormalizeAngle(angles.x);
	double Y = NormalizeAngle(angles.y);
	double Z = NormalizeAngle(angles.z);
	return vec3(X, Y, Z);
}

double NormalizeAngle(double angle){
	while (angle > 360)
	{
		angle -= 360;
	}
	while (angle < 0)
	{
		angle += 360;
	}
	return angle;
}