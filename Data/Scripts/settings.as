//-----------------------------------------------------------------------------
//           Name: settings.as
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

const int kInvalidControllerButton = -1234; // Banking on controllers never having 1234 axes
const int kInvalidKeyboardButton = -1;
const int kInvalidMouseButton = -1;
const int kMaxShownKeybinds = 10;

IMDivider@ settings_content;
string current_screen;
int active_rebind = -1;
int controller_sequence_id;
int keyboard_sequence_id;
int mouse_sequence_id;

IMText@ apply_label;
IMText@ cancel_label;
IMText@ rebind_text;
IMText@ hint_text;
string rebind_key = "";
bool allow_rebind = true; // User must release menu_return before allowing another keybind

int potential_rebind_scancode;
int potential_rebind_mouse_button;
int potential_rebind_controller_button;
ScrollDir potential_rebind_scroll_dir;
enum ScrollDir { SCROLL_NONE, SCROLL_UP, SCROLL_DOWN, SCROLL_LEFT, SCROLL_RIGHT };

int show_keybinds_offset = 0;
IMContainer@ key_rebind_container;

const float background_height = 1300;
const float background_width = 1600;
IMContainer@ main_container;

bool controller_connected = false;

bool popup_open = false;
const string popup_name = "popup";
IMDivider@ OpenPopup(int width, int height) {
    if(popup_open) {
        main_container.removeElement(popup_name);
        main_container.setPauseBehaviors(false);
    }

    IMContainer popup_container(width, height);

    IMDivider popup(popup_name + "_main", DOVertical);
    popup.setZOrdering(10);
    popup_container.setElement(popup);

    IMImage background(white_background);
    background.setSizeX(width);
    background.setSizeY(height);
    background.setColor(vec4(0.0f, 0.0f, 0.0f, 1.0f));

    popup_container.addFloatingElement(background, "background", vec2(0.0f, 0.0f), 9);

    main_container.setPauseBehaviors(true);
    main_container.addFloatingElement(popup_container, popup_name, vec2(background_width * 0.5f - width * 0.5f, background_height * 0.5f - height * 0.5f), 10);

    popup_open = true;
    return @popup;
}

void ClosePopup() {
    if(popup_open) {
        main_container.removeElement(popup_name);
        main_container.setPauseBehaviors(false);
        popup_open = false;
    }
}

string landing_screen_override;
void BuildUI(string landing_screen){
    float header_width = 550;
    float header_height = 128;

    ExitRebindMode(false);

    const bool kAnimate = false;

    gui_elements.resize(0);
    category_elements.resize(0);

    IMContainer main_container_local(background_width, background_height);
    @main_container = @main_container_local;

    main_container.setBorderColor(vec4(0,1,0,1));
    float middle_x = main_container.getSizeX() / 2.0f;
    float middle_y = main_container.getSizeY() / 2.0f;
    main_container.setAlignment(CACenter, CACenter);
    IMImage menu_background(settings_sidebar);

    IMImage settings_background(white_background);
    settings_background.setSizeX(1200);
    settings_background.setSizeY(1200);
    settings_background.setZOrdering(0);
    settings_background.setColor(vec4(0,0,0,0.85f));

    if(kAnimate){
        menu_background.addUpdateBehavior(IMMoveIn ( move_in_time, vec2(0, move_in_distance * -1), inQuartTween ), "");
    }
    IMDivider mainDiv( "mainDiv", DOHorizontal );

    menu_background.setSizeY(background_height);
    menu_background.setSizeX(350);
    menu_background.setZOrdering(0);

    mainDiv.setAlignment(CACenter, CACenter);
    mainDiv.setSize(vec2(main_container.getSizeX(), main_container.getSizeY()));

    IMDivider buttons_holder(DOVertical);
    buttons_holder.setSize(vec2(600, mainDiv.getSizeY()));
    buttons_holder.setBorderColor(vec4(1,0,0,1));

    IMImage header_background( brushstroke_background );
    if(kAnimate){
        header_background.addUpdateBehavior(IMMoveIn ( move_in_time, vec2(0, move_in_distance * -1), inQuartTween ), "");
    }
    header_background.scaleToSizeX(header_width);
    header_background.setColor(button_background_color);
    IMDivider header_holder("header_holder", DOHorizontal);
    IMText header_text("Settings", button_font);
    if(kAnimate){
        header_text.addUpdateBehavior(IMMoveIn ( move_in_time, vec2(0, move_in_distance * -1), inQuartTween ), "");
    }
    IMContainer header_container(header_background.getSizeX(), header_background.getSizeY());
    header_container.setElement(header_text);
    header_container.setAlignment(CACenter, CACenter);
    header_text.setZOrdering(3);
    header_container.addFloatingElement(header_background, "background", vec2(0.0f, (0.0f)), 1);
    header_holder.append(header_container);
    buttons_holder.append(header_holder);
    buttons_holder.setBorderColor(vec4(0,0,1,1));

    buttons_holder.append(IMSpacer(DOVertical, 25.0f));

    buttons_holder.setAlignment(CACenter, CACenter);

    mainDiv.append(buttons_holder);
    AddCategoryButton("Graphics", buttons_holder);
    AddCategoryButton("Audio", buttons_holder);
    AddCategoryButton("Game", buttons_holder);
    AddCategoryButton("Controls", buttons_holder);
    controller_connected = IsControllerConnected();
    if(controller_connected) {
        AddCategoryButton("Gamepad", buttons_holder);
    }

    buttons_holder.append(IMSpacer(DOVertical, 200.0f));

    AddButton("Back", buttons_holder, arrow_icon, button_back, true, 350.0f, 10.0f);

    IMDivider temp_settings_content("settings_content", DOVertical);
    temp_settings_content.setAlignment(CALeft, CACenter);
    @settings_content = @temp_settings_content;

    mainDiv.append(settings_content);

    main_container.addFloatingElement(mainDiv, "menu_content", vec2(0.0f, middle_y - mainDiv.getSizeY() / 2.0f), 0);
    main_container.addFloatingElement(menu_background, "menu_background", vec2(buttons_holder.getSizeX() / 2.0f - menu_background.getSizeX() / 2.0f, middle_y - menu_background.getSizeY() / 2.0f), 0);
    main_container.addFloatingElement(settings_background, "settings_background", vec2(buttons_holder.getSizeX() / 2.0f, middle_y - settings_background.getSizeY() / 2.0f), 0);

    settings_background.addLeftMouseClickBehavior(IMFixedMessageOnClick("close_all_open_menus"), "");
    imGUI.getMain().setElement(@main_container);
    current_screen = "none";
    if(landing_screen_override.isEmpty()) {
        SwitchSettingsScreen(landing_screen);
    } else {
        SwitchSettingsScreen(landing_screen_override);
        landing_screen_override = "";
    }
    controller_wraparound = false;
}

void RestoreCategoryButtons(){
    for(uint i = 0; i < category_elements.size(); i++){
        if(category_elements[i].element_open){
            category_elements[i].DisableElement();
            category_elements[i].element_open = false;
        }
    }
}

void SetActiveCategory(string name){
    for(uint i = 0; i < category_elements.size(); i++){
        if(category_elements[i].name == name){
            category_elements[i].EnableElement();
        }
    }
}

void SwitchSettingsScreen(string screen_name){
    settings_content.clear();
    gui_elements.resize(0);

    if(current_screen != screen_name) {
        show_keybinds_offset = 0;
    }

    ClearControllerItems(category_elements.size() + 1);

    RestoreCategoryButtons();
    SetActiveCategory(screen_name);

    if(screen_name == "Graphics"){
        AddGraphicsScreen();
    }
    else if(screen_name == "Audio"){
        AddAudioScreen();
    }
    else if(screen_name == "Game"){
        AddGameScreen();
    }
    else if(screen_name == "Controls"){
        AddInputScreen();
    }
    else if(screen_name == "Gamepad"){
        AddControllerScreen();
    }
    list_created = false;
    current_screen = screen_name;
}

void checkCustomResolution(){
    vec2 currentResolution = vec2(GetConfigValueInt("screenwidth"), GetConfigValueInt("screenheight"));
    array<vec2> possibleResolutions = GetPossibleResolutions();

    bool found = false;
    for(uint i = 0; i < possibleResolutions.size(); ++i) {
        if(possibleResolutions[i] == currentResolution) {
            found = true;
            break;
        }
    }

    // Don't set custom_resolution to true automatically;
    // the user may have entered a possible resolution manually
    if(!found)
        SetConfigValueBool("custom_resolution", true);
}

void AddGraphicsScreen(){
    current_screen = "Graphics";

    array<string> overall_settings = GetConfigValueOptions("overall");
    AddDropDown("Overall", settings_content, DropdownConfiguration(overall_settings, overall_settings, "overall"));

    array<int> monitorIndices;
    array<string> monitorStrings;
    
    int monitorCount = GetMonitorCount();
    for(int i = 0; i < monitorCount; ++i) {
        monitorIndices.insertLast(i);
        monitorStrings.insertLast("" + (i + 1));
    }

    if(monitorCount > 1)
        AddDropDown("Target monitor", settings_content, DropdownConfiguration(monitorIndices, monitorStrings, "target_monitor"));
    
    array<vec2> possible_resolutions = GetPossibleResolutions();
    array<string> resolutions;
    for(uint i = 0; i < possible_resolutions.size(); i++){
        resolutions.insertLast(possible_resolutions[i].x + "x" + possible_resolutions[i].y);
    }
    resolutions.insertLast("Custom");
    
    array<string> config_names = {"screenwidth", "screenheight"};
    AddResolutionDropDown("Resolution", settings_content, DropdownConfiguration(possible_resolutions, resolutions, config_names ));
    
    if(GetConfigValueBool("custom_resolution"))
        AddCustomResolutionInput(settings_content);
    
    array<int> window_type_values = { 0, 1, 2, 3 };
    array<string> window_type_options = { "Windowed", "Fullscreen", "Windowed borderless", "Fullscreen borderless"};
    AddDropDown("Window mode", settings_content, DropdownConfiguration(window_type_values, window_type_options, "fullscreen"));

    array<int> aa_values = { 1, 2, 4, 8 };
    array<string> aa_options = {"none", "2X", "4X", "8X"};
    AddDropDown("Anti-aliasing", settings_content, DropdownConfiguration(aa_values, aa_options, "multisample"));
    
    array<int> anisotropy_values = { 1, 2, 4, 8 };
    array<string> anisotropy_options = {"none", "2X", "4X", "8X"};
    AddDropDown("Anisotropy", settings_content, DropdownConfiguration( anisotropy_values, anisotropy_options, "anisotropy" ));
    
    array<int> texture_detail_values = {0, 1, 2, 3};
    array<string> texture_detail_options = {"Full", "1/2", "1/4", "1/8"};
    AddDropDown("Texture Detail", settings_content, DropdownConfiguration( texture_detail_values, texture_detail_options, "texture_reduce"), "*");

    array<array<int>> detail_object_config_values = { {0, 1, 1, 0}, {1, 1, 1 ,0}, {1, 0, 0, 0}, {1, 0, 0, 1} };
    array<string> detail_object_config_variables = { "detail_objects", "detail_object_lowres", "detail_object_disable_shadows", "detail_object_decals" };
    array<string> detail_object_options = {"Off", "Low", "Medium", "High"};
    AddDropDown("Detail objects", settings_content, DropdownConfiguration( detail_object_config_values, detail_object_options, detail_object_config_variables ), "*");
    AddLabel("*Requires Restart", settings_content);
    
    DualColumns dual_columns("checkboxes_column", 800);
    AddCheckBox("VSync", dual_columns.left, "vsync", 0.0f);
    AddCheckBox("Simple Shadows", dual_columns.left, "simple_shadows", 0.0f);
    AddCheckBox("Simple Fog", dual_columns.left, "simple_fog", 0.0f);
    AddCheckBox("Simple Water", dual_columns.left, "simple_water", 0.0f);
    AddCheckBox("Fewer Detail Objects", dual_columns.left, "detail_objects_reduced", 0.0f);
    //AddCheckBox("Use tet mesh lighting", dual_columns, "tet_mesh_lighting");
    //AddCheckBox("Use ambient light volumes", dual_columns, "light_volume_lighting");
    AddCheckBox("GPU Particle Field", dual_columns.right, "particle_field", 0.0f);
    AddCheckBox("Simple GPU Particles", dual_columns.right, "particle_field_simple", 0.0f);
    AddCheckBox("Enable Custom Shaders", dual_columns.right, "custom_level_shaders", 0.0f);
    AddCheckBox("No reflection capture", dual_columns.right, "no_reflection_capture", 0.0f);
    AddCheckBox("Depth of field", dual_columns.right, "depth_of_field", 0.0f);
    AddCheckBox("Reduce depth of field", dual_columns.right, "depth_of_field_reduced", 0.0f);

    settings_content.append(dual_columns.main);
    
    AddSlider("Motion Blur", settings_content, "motion_blur_amount", 1.0f);
    AddSlider("Brightness", settings_content, "brightness", 2.0f, 200.0f);
}

void AddAudioScreen(){
    current_screen = "Audio";
    
    AddSlider("Music volume", settings_content, "music_volume", 1.0f, 100.0f, 0.0f, 0.0f, true);
    AddSlider("Master volume", settings_content, "master_volume", 1.0f, 100.0f, 0.0f, 0.0f, true);
}

void AddGameScreen(){
    current_screen = "Game";

    AddCheckBox("Show frame rate and frame time", settings_content, "fps_label");

    array<string> locale_values = GetLocaleShortcodes();
    array<string> locale_names = GetLocaleNames();
    AddDropDown("Language", settings_content, DropdownConfiguration( locale_values, locale_names, "language", " (English)" ));

    array<string> difficulty_options = GetConfigValueOptions("difficulty_preset");
    AddDropDown("Difficulty Preset", settings_content, DropdownConfiguration( difficulty_options, difficulty_options, "difficulty_preset" ));

    AddSlider("Game Speed", settings_content, "global_time_scale_mult", 1.0f, 100.0f, 0.5f, 50.0f);
    AddSlider("Game Difficulty", settings_content, "game_difficulty", 1.0f);

    AddCheckBox("Tutorials", settings_content, "tutorials");
    AddCheckBox("Automatic Ledge Grab", settings_content, "auto_ledge_grab"); 
    array<int> blood_amount_values = { 0, 1, 2};
    array<string> blood_amount_options = {"None", "No dripping", "Full"};
    AddDropDown("Blood Amount", settings_content, DropdownConfiguration( blood_amount_values, blood_amount_options, "blood" ));
    
    array<string> blood_color_values = {"0.4 0 0", "0 0.4 0", "0 0.4 0.4", "0.1 0.1 0.1"};
    array<string> blood_color_options = {"Red", "Green", "Cyan", "Black"};
    AddDropDown("Blood Color", settings_content, DropdownConfiguration( blood_color_values, blood_color_options, "blood_color" ));
    AddCheckBox("Splitscreen", settings_content, "split_screen");
}

void AddInputScreen(){
    current_screen = "Controls";

    AddSlider("Mouse sensitivity", settings_content, "mouse_sensitivity", 2.5f, 200.0f, 0.01f, 1.0f, true);

    DualColumns dual_columns("input_columns", 800);

    AddCheckBox("Invert mouse X", dual_columns.left, "invert_x_mouse_look", 0.0f);
    AddCheckBox("Invert mouse Y", dual_columns.left, "invert_y_mouse_look", 0.0f);

    AddCheckBox("Raw mouse input", dual_columns.right, "use_raw_input", 0.0f);
    AddCheckBox("Automatic camera", dual_columns.right, "auto_camera", 0.0f);

    settings_content.append(dual_columns.main);

    @key_rebind_container = @IMContainer(0, 0);
    AddKeyBinds(key_rebind_container);
    settings_content.append(key_rebind_container);

    AddBasicButton("Reset to defaults", "reset_bindings", 400, settings_content);
}

void AddControllerScreen(){
    current_screen = "Gamepad";

    array<int> player_values = { 0, 1, 2, 3 };
    array<string> player_options = { "1", "2", "3", "4" };
    AddDropDown("Current player", settings_content, DropdownConfiguration(player_values, player_options, "menu_player_config"));

    AddSlider("Sensitivity", settings_content, "gamepad_" + GetConfigValueInt("menu_player_config") + "_look_sensitivity", 5.0f, 200.0f, 0.01f, 1.0f, false);
    AddSlider("Deadzone", settings_content, "gamepad_" + GetConfigValueInt("menu_player_config") + "_deadzone", 0.5f);

    @key_rebind_container = @IMContainer(0, 0);
    AddControllerBinds(key_rebind_container);
    settings_content.append(key_rebind_container);

    AddBasicButton("Reset to defaults", "reset_bindings", 400, settings_content);
}

const array<string> keyboard_bind_names = {
    "Forward",
    "Backwards",
    "Left",
    "Right",
    "Jump",
    "Crouch",
    "Slow Motion",
    "Equip/sheathe item",
    "Throw/pick-up item",
    "Skip dialogue",
    "Attack",
    "Grab",
    "Walk"
};
const array<string> keyboard_keybinds = {
    "up",
    "down",
    "left",
    "right",
    "jump",
    "crouch",
    "slow",
    "item",
    "drop",
    "skip_dialogue",
    "attack",
    "grab",
    "walk"
};
void AddKeyBinds(IMContainer@ parent) {
    // Left is keybinds, right is scrollbar
    IMDivider key_rebind_horizontal("hori", DOHorizontal);

    IMDivider key_rebind_vertical("vert", DOVertical);
    for(int i = 0; i < kMaxShownKeybinds; ++i) {
        AddKeyRebind(keyboard_bind_names[i + show_keybinds_offset], key_rebind_vertical, "key", keyboard_keybinds[i + show_keybinds_offset]);
    }
    key_rebind_horizontal.append(key_rebind_vertical);

    AddScrollBar(key_rebind_horizontal, 50.0f, 700.0f, keyboard_keybinds.size(), kMaxShownKeybinds, show_keybinds_offset);

    parent.setElement(key_rebind_horizontal);
}

const array<string> controller_bind_names = {
    "Forward",
    "Backwards",
    "Left",
    "Right",
    "Look Up",
    "Look Down",
    "Look Left",
    "Look Right",
    "Jump",
    "Crouch",
    "Equip/sheathe item",
    "Throw/pick-up item",
    "Skip dialogue",
    "Attack",
    "Grab"
};
const array<string> controller_keybinds = {
    "up",
    "down",
    "left",
    "right",
    "look_up",
    "look_down",
    "look_left",
    "look_right",
    "jump",
    "crouch",
    "item",
    "drop",
    "skip_dialogue",
    "attack",
    "grab"
};
void AddControllerBinds(IMContainer@ parent) {
    // Left is keybinds, right is scrollbar
    IMDivider key_rebind_horizontal("hori", DOHorizontal);

    IMDivider key_rebind_vertical("vert", DOVertical);
    for(int i = 0; i < kMaxShownKeybinds; ++i) {
        AddKeyRebind(controller_bind_names[i + show_keybinds_offset], key_rebind_vertical, "gamepad_" + GetConfigValueInt("menu_player_config"), controller_keybinds[i + show_keybinds_offset]);
    }
    key_rebind_horizontal.append(key_rebind_vertical);

    AddScrollBar(key_rebind_horizontal, 50.0f, 700.0f, controller_keybinds.size(), kMaxShownKeybinds, show_keybinds_offset);

    parent.setElement(key_rebind_horizontal);

}

void SaveUnsavedChanges() {
    bool unsaved_values = false;
    for(uint i = 0; i < gui_elements.size(); i++){
        if(gui_elements[i].unsaved_values){
            unsaved_values = true;
            gui_elements[i].SaveValueToConfig();
        }
    }
    if(unsaved_values)
        ReloadStaticValues();
}

void DiscardUnsavedChanges() {
    for(uint i = 0; i < gui_elements.size(); i++){
        if(gui_elements[i].unsaved_values){
            gui_elements[i].PutCurrentValue();
        }
    }
}

void ProcessSettingsMessage(IMMessage@ message){
    if(active_rebind == -1){
        CloseAllOptionMenus();
        if( message.name == "ui_element_clicked" ){
            DiscardUnsavedChanges();
            ToggleUIElement(message.getString(0));
        }
        else if( message.name == "option_changed" ){
            DiscardUnsavedChanges();
            for(uint i = 0; i < gui_elements.size(); i++){
                if(gui_elements[i].name == message.getString(0)){
                    gui_elements[i].SwitchOption(message.getString(1));
                }
            }
            SwitchSettingsScreen(current_screen); // Possible resolutions may have changed
        }
        else if( message.name == "close_all_open_menus" ){
            DiscardUnsavedChanges();
            CloseAllOptionMenus();
        }
        else if( message.name == "save_unsaved_changes" ){
            SaveUnsavedChanges();
            SwitchSettingsScreen(current_screen);
        }
        else if( message.name == "discard_unsaved_changes" ){
            DiscardUnsavedChanges();
        }
        else if( message.name == "slider_deactivate" ){
            //Print("deactivate slider check\n");
            if(!checking_slider_movement){
                active_slider = -1;
            }
        }
        else if( message.name == "slider_activate" ){
            //Print("activate slider check\n");
            old_mouse_pos = imGUI.guistate.mousePosition;
            active_slider = message.getInt(0);
        }
        else if( message.name == "slider_move_check" ){
            /*continues_updating_element = message.getInt(0);*/
        }
        else if( message.name == "switch_category" ){
            SwitchSettingsScreen(message.getString(0));
        }
        else if( message.name == "rebind_activate" && allow_rebind ){
            if(controller_active) {
                if(current_screen == "Controls")
                    EnterRebindMode(message.getInt(0), true);
                else
                    EnterRebindMode(message.getInt(0), false);
            } else {
                // Need to check the mouse. If not, rebinding LMB might trigger another rebind_active
                array<MousePress> mouse_inputs = GetRawMouseInputs();
                if(mouse_inputs.size() > 0 && int(mouse_inputs[mouse_inputs.size() - 1].s_id) != mouse_sequence_id) {
                    if(current_screen == "Controls")
                        EnterRebindMode(message.getInt(0), true);
                    else
                        EnterRebindMode(message.getInt(0), false);
                }
            }
        }
        else if( message.name == "reset_bindings" ){
            if(current_screen == "Controls") {
                for(uint i = 0; i < keyboard_keybinds.size(); ++i) {
                    ResetBinding("key", keyboard_keybinds[i]);
                }
            } else if(current_screen == "Gamepad") {
                for(uint i = 0; i < controller_keybinds.size(); ++i) {
                    ResetBinding("gamepad_" + GetConfigValueInt("menu_player_config"), controller_keybinds[i]);
                }
            }
            RefreshAllOptions();
        }
        else if( message.name == "back" ){
            //Log(info,"Received back message");
            if(OptionMenuOpen()){
                CloseAllOptionMenus();
            }else{
                imGUI.receiveMessage(IMMessage("Back"));
            }
        }
        else if(message.name == "enable_element"){
            if(message.numStrings() > 0) {
                string target_name = message.getString(0);

                for(uint i = 0; i < gui_elements.size(); ++i) {
                    if(gui_elements[i].name == target_name) {
                        gui_elements[i].EnableElement();
                        break;
                    }
                }
            }
        }
        else if(message.name == "shift_menu"){
            if(current_screen == "Controls" || current_screen == "Gamepad") {
                int offset = message.getInt(0);
                show_keybinds_offset += offset;
                show_keybinds_offset = max(0, show_keybinds_offset);
                if(current_screen == "Controls") {
                    show_keybinds_offset = min(show_keybinds_offset, keyboard_keybinds.size() - kMaxShownKeybinds);
                } else {
                    show_keybinds_offset = min(show_keybinds_offset, controller_keybinds.size() - kMaxShownKeybinds);
                }
                SwitchSettingsScreen(current_screen);
            }
        }
        else if(message.name.findFirst("menu_player") == 0) {
            int player = parseInt(message.name.substr(11));
            SetControllerPlayer(player);
            SetConfigValueInt("menu_player_config", player);
        }
        else{
            Log( info, "Unknown processMessage " + message.getString(0) );
        }
    } else {
        if(message.name == "apply_bind") {
            ApplyBind();
        }
        else if(message.name == "cancel_bind") {
            ExitRebindMode(false);
        }
        else if(message.name == "open_menu") {
            ExitRebindMode(false);
        }
        else if(message.name == "close_all_open_menus" || message.name == "back"){
            ExitRebindMode(false);
            DiscardUnsavedChanges();
            CloseAllOptionMenus();
        }
        else{
            Log( info, "Unknown processMessage " + message.getString(0) );
        }
    }
}

void ApplyBind() {
    KeyRebind@ element = cast<KeyRebind>(gui_elements[active_rebind]);
    if(potential_rebind_controller_button != kInvalidControllerButton) {
        SetControllerBindingValue(element.binding_category, element.binding, potential_rebind_controller_button);
    } else if(potential_rebind_scancode != kInvalidKeyboardButton) {
        SetKeyboardBindingValue(element.binding_category, element.binding, potential_rebind_scancode);
    } else if(potential_rebind_mouse_button != kInvalidMouseButton) {
        SetMouseBindingValue(element.binding_category, element.binding, potential_rebind_mouse_button);
    } else if(potential_rebind_scroll_dir != SCROLL_NONE) {
        if(potential_rebind_scroll_dir == SCROLL_UP) {
            SetMouseBindingValue(element.binding_category, element.binding, "mousescrollup");
        } else if(potential_rebind_scroll_dir == SCROLL_DOWN) {
            SetMouseBindingValue(element.binding_category, element.binding, "mousescrolldown");
        } else if(potential_rebind_scroll_dir == SCROLL_LEFT) {
            SetMouseBindingValue(element.binding_category, element.binding, "mousescrollleft");
        } else if(potential_rebind_scroll_dir == SCROLL_RIGHT) {
            SetMouseBindingValue(element.binding_category, element.binding, "mousescrollright");
        } else {
            Log(error, "Unknown scroll dir");
        }
    }
    element.PutCurrentValue();
    ExitRebindMode(true);
}

void UpdateSettings(bool auto_exit_rebind = false){
    if(!allow_rebind) {
        allow_rebind = !GetInputDown(controller_player, "menu_return");
        if(controller_active) {
            controller_paused = !allow_rebind;
        } else {
            controller_paused = false;
        }
    }
    if(active_rebind != -1) {
        if(current_screen == "Controls") {
            UpdateKeyRebinding(auto_exit_rebind);
        } else {
            UpdateControllerRebinding(auto_exit_rebind);
        }
    } else {
        if(GetInputDown(0, "mousescrollup")){
            imGUI.receiveMessage(IMMessage( "shift_menu", -1));
        } else if(GetInputDown(0, "mousescrolldown")){
            imGUI.receiveMessage(IMMessage( "shift_menu", 1));
        }
        UpdateMovingSlider();
        for(uint i = 0; i < gui_elements.size(); i++) {
            gui_elements[i].Update();
        }
    }
    if(controller_connected != IsControllerConnected()) {
        if(current_screen == "Gamepad") {
            landing_screen_override = "Controls";
        } else {
            landing_screen_override = current_screen;
        }
        imGUI.receiveMessage(IMMessage("rebuild_settings"));
    }
}

void EnterRebindMode(int rebind_index, bool keyboard) {
    potential_rebind_scancode = kInvalidKeyboardButton;
    potential_rebind_mouse_button = kInvalidMouseButton;
    potential_rebind_controller_button = kInvalidControllerButton;
    potential_rebind_scroll_dir = SCROLL_NONE;

    controller_paused = true;

    active_rebind = rebind_index;
    rebind_key = gui_elements[active_rebind].name;
    array<KeyboardPress> keyboard_inputs = GetRawKeyboardInputs();
    if(keyboard_inputs.size() > 0) {
        keyboard_sequence_id = keyboard_inputs[keyboard_inputs.size()-1].s_id;
    } else {
        keyboard_sequence_id = -1;
    }
    array<MousePress> mouse_inputs = GetRawMouseInputs();
    if(mouse_inputs.size() > 0) {
        mouse_sequence_id = mouse_inputs[mouse_inputs.size()-1].s_id;
    } else {
        mouse_sequence_id = -1;
    }
    array<ControllerPress> controller_inputs = GetRawControllerInputs(controller_player);
    if(controller_inputs.size() > 0) {
        controller_sequence_id = controller_inputs[controller_inputs.size()-1].s_id;
    } else {
        controller_sequence_id = -1;
    }

    const float POPUP_WIDTH = 1000;
    const float POPUP_HEIGHT = 500;

    IMDivider@ popup = OpenPopup(POPUP_WIDTH, POPUP_HEIGHT);

    popup.append(IMText("Rebinding " + rebind_key, button_font));

    @rebind_text = @IMText("", button_font);
    popup.append(rebind_text);
    if(keyboard)
        @hint_text = @IMText("Press key to bind, or escape to cancel", button_font);
    else
        @hint_text = @IMText("Press key to bind, or back to cancel", button_font);
    popup.append(hint_text);

    if(keyboard) {
        @apply_label = @IMText("Apply", button_font);
        apply_label.setColor(vec4(1.0f, 1.0f, 1.0f, 0.5f));
        @cancel_label = @IMText("Cancel", button_font);
        cancel_label.addMouseOverBehavior(text_color_mouse_over, "");
        cancel_label.addLeftMouseClickBehavior(IMFixedMessageOnClick("cancel_bind"), "");

        IMDivider buttons("buttons", DOHorizontal);

        popup.appendSpacer(100.0f);
        popup.append(buttons);
        buttons.append(apply_label);
        buttons.appendSpacer(200.0f);
        buttons.append(cancel_label);
    }

    imGUI.receiveMessage(IMMessage("capture_input"));
}

void ExitRebindMode(bool key_rebound) {
    potential_rebind_scancode = kInvalidKeyboardButton;
    potential_rebind_mouse_button = kInvalidMouseButton;
    potential_rebind_controller_button = kInvalidControllerButton;
    potential_rebind_scroll_dir = SCROLL_NONE;
    active_rebind = -1;
    rebind_key = "";
    imGUI.receiveMessage(IMMessage("release_input"));
    ClosePopup();
    if(!key_rebound) {
        controller_paused = false;
    }
    allow_rebind = false;
}

void ChangeKeybindLabel() {
    if(potential_rebind_mouse_button == -1 && potential_rebind_scancode == -1 && potential_rebind_scroll_dir == SCROLL_NONE) {
        apply_label.addMouseOverBehavior(text_color_mouse_over, "");
        apply_label.addLeftMouseClickBehavior(IMFixedMessageOnClick(IMMessage("apply_bind")), "");
        apply_label.setColor(button_font.color);
        hint_text.setText("Press key again or apply");
    }
}

void UpdateKeyRebinding(bool auto_exit_rebind){
    array<KeyboardPress> keyboard_inputs = GetRawKeyboardInputs();
    if(keyboard_inputs.size() > 0){
        KeyboardPress last_pressed = keyboard_inputs[keyboard_inputs.size()-1];
        if( last_pressed.s_id != uint16(keyboard_sequence_id) ) {
            keyboard_sequence_id = last_pressed.s_id;
            if(!apply_label.isMouseOver() && !cancel_label.isMouseOver()) {
                // 27 == escape
                if(last_pressed.keycode == 27 && auto_exit_rebind){
                    ExitRebindMode(false);
                } else if(last_pressed.scancode == uint(potential_rebind_scancode)) {
                    ApplyBind();
                } else {
                    ChangeKeybindLabel();
                    rebind_text.setText("to " + GetLocaleStringForScancode(last_pressed.scancode));
                    potential_rebind_scancode = last_pressed.scancode;
                    potential_rebind_mouse_button = -1;
                    potential_rebind_scroll_dir = SCROLL_NONE;
                }
            }
        }
    }
    array<MousePress> mouse_inputs = GetRawMouseInputs();
    if(mouse_inputs.size() > 0){
        MousePress last_pressed = mouse_inputs[mouse_inputs.size()-1];
        if( last_pressed.s_id != uint16(mouse_sequence_id)) {
            mouse_sequence_id = uint16(last_pressed.s_id);
            if(!apply_label.isMouseOver() && !cancel_label.isMouseOver()) {
                if(last_pressed.button == potential_rebind_mouse_button) {
                    ApplyBind();
                } else {
                    ChangeKeybindLabel();
                    rebind_text.setText("to " + GetStringForMouseButton(last_pressed.button));
                    potential_rebind_scancode = -1;
                    potential_rebind_mouse_button = last_pressed.button;
                    potential_rebind_scroll_dir = SCROLL_NONE;
                }
            }
        }
    }
    if(GetInputDown(0, "mousescrollup")){
        if(potential_rebind_scroll_dir == SCROLL_UP) {
            ApplyBind();
        }
        ChangeKeybindLabel();
        rebind_text.setText("to scroll up");
        potential_rebind_scancode = -1;
        potential_rebind_mouse_button = -1;
        potential_rebind_scroll_dir = SCROLL_UP;
    } else if(GetInputDown(0, "mousescrolldown")){
        if(potential_rebind_scroll_dir == SCROLL_DOWN) {
            ApplyBind();
        }
        ChangeKeybindLabel();
        rebind_text.setText("to scroll down");
        potential_rebind_scancode = -1;
        potential_rebind_mouse_button = -1;
        potential_rebind_scroll_dir = SCROLL_DOWN;
    } else if(GetInputDown(0, "mousescrollleft")){
        if(potential_rebind_scroll_dir == SCROLL_LEFT) {
            ApplyBind();
        }
        ChangeKeybindLabel();
        rebind_text.setText("to scroll left");
        potential_rebind_scancode = -1;
        potential_rebind_mouse_button = -1;
        potential_rebind_scroll_dir = SCROLL_LEFT;
    } else if(GetInputDown(0, "mousescrollright")){
        if(potential_rebind_scroll_dir == SCROLL_RIGHT) {
            ApplyBind();
        }
        ChangeKeybindLabel();
        rebind_text.setText("to scroll right");
        potential_rebind_scancode = -1;
        potential_rebind_mouse_button = -1;
        potential_rebind_scroll_dir = SCROLL_RIGHT;
    }
}

// Used to make sure the user releases the thumbstick, and then moves it back
// again to confirm the bind
bool confirmed = false;

void UpdateControllerRebinding(bool auto_exit_rebind){
    array<ControllerPress> controller_inputs = GetRawControllerInputs(controller_player);
    if(controller_inputs.size() > 0) {
        uint start_index = 0;
        for(start_index = controller_inputs.size() - 1; start_index >= 0; start_index--) {
            if(controller_inputs[start_index].s_id <= uint32(controller_sequence_id)) {
                break;
            }
        }
        for(; start_index < controller_inputs.size(); start_index++) {
            ControllerPress last_pressed = controller_inputs[start_index];
            if( last_pressed.s_id != uint32(controller_sequence_id)) {
                controller_sequence_id = last_pressed.s_id;
                if(last_pressed.depth > 0.5f) {
                    if(last_pressed.input == BACK && auto_exit_rebind) {
                        ExitRebindMode(false);
                        break;
                    } else if(last_pressed.input == potential_rebind_controller_button && confirmed) {
                        ApplyBind();
                        break;
                    } else {
                        confirmed = false;
                    }

                    if(potential_rebind_controller_button == kInvalidControllerButton) {
                        hint_text.setText("Repeat input to apply");
                    }
                    rebind_text.setText("to " + GetStringForControllerInput(last_pressed.input));

                    potential_rebind_controller_button = last_pressed.input;

                    // Buttons are automatically confirmed, since they are eiter on
                    // or off. The next time an input event is received from a
                    // button it has been released and then pressed again.
                    switch(last_pressed.input) {
                        case ControllerInput::L_STICK_XN:
                        case ControllerInput::L_STICK_XP:
                        case ControllerInput::L_STICK_YN:
                        case ControllerInput::L_STICK_YP:
                        case ControllerInput::R_STICK_XN:
                        case ControllerInput::R_STICK_XP:
                        case ControllerInput::R_STICK_YN:
                        case ControllerInput::R_STICK_YP:
                        case ControllerInput::L_TRIGGER:
                        case ControllerInput::R_TRIGGER:
                            break;
                        default:
                            confirmed = true;
                            break;
                    }
                } else if(last_pressed.input == potential_rebind_controller_button) {
                    confirmed = true;
                }
            }
        }
    }
}
