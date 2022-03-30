//-----------------------------------------------------------------------------
//           Name: difficulty_menu.as
//      Developer: Wolfire Games LLC
//    Script Type: Menu
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

#include "menu_common.as"
#include "music_load.as"

MusicLoad ml("Data/Music/menu.xml");

IMGUI@ imGUI;
array<LevelInfo@> play_menu = {};

array<array<string> > difficulty_description = {
{"Experience the story of Overgrowth", " ", "Reduced enemy difficulty, 80% speed, countering attacks is easier"},
{"Fight like a ninja rabbit", " ", "Normal enemy difficulty, 100% speed"},
{"Prove your mastery of combat", " ", "Maximum enemy difficulty, 100% speed, countering attacks is harder"}
};

array<string> difficulty_logos = {
    "Textures/Thumbnails/difficulty/casual.png",
    "Textures/Thumbnails/difficulty/hardcore.png",
    "Textures/Thumbnails/difficulty/expert.png"
};

array<vec4> difficulty_colors = {
    HexColor("#49EA86"),
    HexColor("#FFD53D"),
    HexColor("#C00000")
};

array<bool> tutorials_preset_values = {
    true,
    true,
    false
};

array<bool> ledge_grab_preset_values = {
    true,
    true,
    false
};

FontSetup description_font("Cella", 36 , HexColor("#CCCCCC"), true);
FontSetup subtitle_font("Cella", 36 , HexColor("#CCCCCC"), true);

float button_width = 400.0f;

IMDivider@ description_divider;
IMDivider@ right_panel;
int current_difficulty_description = -1;
int current_difficulty = -1;
bool ledge_grab = false;
bool tutorials = false;

bool settings_changed = false;

bool HasFocus() {
    return false;
}

void Initialize() {
    @imGUI = CreateIMGUI();
    // Start playing some music
    PlaySong("overgrowth_main");
    int current_difficulty_value = -1;
    if(!settings_changed) {
        // First-time setup
        ledge_grab = GetConfigValueBool("auto_ledge_grab");
        tutorials = GetConfigValueBool("tutorials");
        current_difficulty_value = GetConfigValueInt("difficulty_preset_value");
        if(current_difficulty_value != 0)
            current_difficulty = current_difficulty_value - 1;
    }

    // We're going to want a 200 'gui space' pixel header/footer
	imGUI.setHeaderHeight(200);
    imGUI.setFooterHeight(200);

	imGUI.setFooterPanels(200.0f, 1400.0f);
    // Actually setup the GUI -- must do this before we do anything

    imGUI.setup();
    SetList();
    BuildHeader();
    BuildMain();
    BuildFooter();
	SetDarkenedBackground();
	AddVerticalBar();
}

void SetList() {
    array<string> diff = GetConfigValueOptions("difficulty_preset");
    play_menu = array<LevelInfo@>();
    for( uint i = 0; i < diff.size(); i++ ) {
        LevelInfo li("", diff[i], difficulty_logos[i],i+1, true, false);
        play_menu.insertLast(li);
    }
}

void BuildHeader() {
	IMDivider header_divider( "header_div", DOHorizontal );
	header_divider.setAlignment(CACenter, CACenter);
	AddTitleHeader("Choose Difficulty", header_divider);
	imGUI.getHeader().setElement(header_divider);
}

void BuildMain() {
    IMDivider upperMainDiv("upperMainDiv", DOVertical);
    IMDivider mainDiv( "mainDiv", DOHorizontal );

	float subtitle_width = 200;
	float subtitle_height = 100;

	IMContainer subtitle_container(subtitle_width, subtitle_height);
	subtitle_container.setAlignment(CACenter, CACenter);
	IMDivider subtitle_divider("subtitle_divider", DOVertical);
	subtitle_container.setElement(subtitle_divider);

    subtitle_divider.append(IMText("Note: You can change the difficulty at any point." , subtitle_font));

    upperMainDiv.append(subtitle_container);

	CreateDifficultyButtons(mainDiv, play_menu, "play_menu", 2000.0f, 300.0f);

	float description_width = 200;
	float description_height = 150;

	IMContainer description_container(description_width, description_height);
	description_container.setAlignment(CACenter, CACenter);
	@description_divider = @IMDivider("description_divider", DOVertical);
	description_container.setElement(description_divider);

    IMDivider checkbox_container("checkbox_container", DOHorizontal);
    if(current_difficulty != -1) {
        AddCheckBox("Automatic Ledge Grab", checkbox_container, "");
        if(!ledge_grab)
            gui_elements[gui_elements.length() - 1].SwitchOption("");
        AddCheckBox("Tutorials", checkbox_container, "");
        if(!tutorials)
            gui_elements[gui_elements.length() - 1].SwitchOption("");
    } else {
        checkbox_container.setSizeY(65.0f);
    }

    upperMainDiv.append(mainDiv);
    upperMainDiv.append(description_container);
    upperMainDiv.append(checkbox_container);

    imGUI.getMain().setElement( @upperMainDiv );

    if(current_difficulty != -1)
        SetDescription(current_difficulty);
}

void BuildFooter() {
    float button_trailing_space = 100.0f;
    
    IMDivider holder("bottom_holder", DOHorizontal);

    IMDivider left_panel("left_panel", DOHorizontal);
    left_panel.setBorderColor(vec4(0,1,0,1));
    left_panel.setAlignment(CALeft, CABottom);
    left_panel.append(IMSpacer(DOHorizontal, button_trailing_space));

    AddButton("Back", left_panel, arrow_icon, button_back, false, button_width);

    @right_panel = @IMDivider("right_panel", DOHorizontal);
    right_panel.setBorderColor(vec4(0,1,0,1));
    right_panel.setAlignment(CARight, CABottom);
    if(settings_changed && current_difficulty != -1)
        AddButtonReversed("Continue", right_panel, arrow_icon, button_back, false, button_width);

    holder.append(left_panel);
    holder.append(IMSpacer(DOHorizontal, 1500));
    holder.append(right_panel);

    imGUI.getFooter().setAlignment(CALeft, CACenter);
    imGUI.getFooter().setElement(holder);
}

IMDivider@ button_row;

void CreateDifficultyButtons(IMDivider@ menu_holder, array<LevelInfo@>@ _levels, string _campaign_name, float _menu_width, float _menu_height){

    levels = _levels;
    campaign_name = _campaign_name;
    
    //Create the actual level select menu between the two arrows.
    IMContainer menu_container(_menu_width, _menu_height);
    IMDivider level_holder("level_holder", DOVertical);
    menu_container.setElement(level_holder);
    level_holder.setAlignment(CALeft, CACenter);
    
    IMDivider row_container("row_container", DOHorizontal);
    level_holder.append(row_container);

    @button_row = @row_container;

    for(uint i = 0; i < levels.size(); i++){
        AddButton(row_container, levels[i], i, _menu_width / 4, _menu_height);
    }

    menu_holder.append(menu_container);
}

void AddButton(IMDivider@ row, LevelInfo@ level, int number, float level_item_width, float level_item_height){
    float background_size_offset = 50.0f;
    float background_icon_size = int((level_item_width + level_item_height) / 4.0f);

    FontSetup menu_item_font(button_font_small.fontName, int((level_item_width + level_item_height) / max(13.0f, level.name.length() * 0.65f)) , button_font_small.color, button_font_small.shadowed);

    //This divider has all the elements of a level.
    IMDivider level_divider("button_divider" + number, DOHorizontal);

    level_divider.addLeftMouseClickBehavior( button_press_sound, "" );
    level_divider.addMouseOverBehavior(button_hover_sound, "");

    //The container is used to add floating elements.
    IMContainer level_container(level_item_width, level_item_height);
    level_divider.sendMouseOverToChildren(true);
    level_divider.append(level_container);
    IMImage background( white_background );
    if(kAnimateMenu){
        background.addUpdateBehavior(IMFadeIn( fade_in_time, inSineTween ), "");
    }
    background.setSize(vec2(level_item_width - background_size_offset, level_item_height - background_size_offset));
    if(number == current_difficulty)
        background.setColor(difficulty_colors[number]);
    else
        background.setColor(button_background_color);
    //Calculate the middle of the container so that elements added to it can calculate when to be as floating elements.
    float middle_y = level_container.getSizeY() / 2.0f;
    float middle_x = level_container.getSizeX() / 2.0f;

    level_container.addFloatingElement(background, "background", vec2(middle_x - (background.getSizeX() / 2.0f), middle_y - (background.getSizeY() / 2.0f)), 1);

    IMMessage on_click("run_file");
    IMMessage on_hover_enter("hover_enter_file");
    IMMessage on_hover_leave("hover_leave_file");
    IMMessage nil_message("");
    float number_background_width = level_item_width / 7.0f;
    float star_width = level_item_width / 10.0f;
    float preview_size_offset = 75.0f;
    float text_holder_height = level_item_width / 8.0f;

    IMImage level_preview( level.image );
    level_preview.setSize(vec2(level_item_width - preview_size_offset, level_item_height - preview_size_offset));
    level_container.addFloatingElement(level_preview, "preview", vec2(middle_x - (level_preview.getSizeX() / 2.0f), middle_y - (level_preview.getSizeY() / 2.0f)), 3);
    
    on_click.addString(level.file);
    on_click.addInt(number);
    on_hover_enter.addString(level.file);
    on_hover_enter.addInt(number);
    on_hover_leave.addString(level.file);
    on_hover_leave.addInt(number);
    level_divider.addLeftMouseClickBehavior( IMFixedMessageOnClick(on_click), "" );
    level_divider.addMouseOverBehavior(IMFixedMessageOnMouseOver(on_hover_enter, nil_message, on_hover_leave), "");
    background.addMouseOverBehavior(mouseover_scale_background, "");
    if(kAnimateMenu){
        level_preview.addUpdateBehavior(IMFadeIn( fade_in_time, inSineTween ), "");
    }
    level_preview.addMouseOverBehavior(mouseover_scale, "");

    //This is the background for the level name.
    IMImage title_background( button_background_diamond );        
    if(kAnimateMenu){
        title_background.addUpdateBehavior(IMFadeIn( fade_in_time, inSineTween ), "");
    }
    title_background.setSize(vec2(level_item_width, text_holder_height));
    level_container.addFloatingElement(title_background, "title_background", vec2(middle_x - (title_background.getSizeX() / 2.0f), level_container.getSizeY() - title_background.getSizeY()), 4);

    //This is the text used to show the level name.
    IMText level_text(level.name, menu_item_font);
    if(kAnimateMenu){
        level_text.addUpdateBehavior(IMFadeIn( fade_in_time, inSineTween ), "");
    }
    IMDivider text_holder_holder("text_holder_holder", DOVertical);
    IMDivider text_holder("text_holder", DOHorizontal);
    text_holder.setSize(vec2(level_item_width, text_holder_height));
    text_holder_holder.setSize(vec2(level_item_width, text_holder_height));
    text_holder_holder.append(text_holder);
    text_holder.append(level_text);
    level_text.setZOrdering(7);
    level_container.addFloatingElement(text_holder_holder, "title_text", vec2(middle_x - (text_holder.getSizeX() / 2.0f), level_container.getSizeY() - text_holder.getSizeY()), 4);

    ControllerItem new_controller_item();
    @new_controller_item.message = on_click;
    level_container.setName("menu_item" + (number));
    @new_controller_item.element = level_container;
    level_container.sendMouseOverToChildren(true);
    AddControllerItem(@new_controller_item);
    row.append(level_divider);
}

void SetDarkenedBackground() {
    setBackGround(0.5f);
}

void Dispose() {
    imGUI.clear();
}

bool CanGoBack() {
    return true;
}

void Update() {
	UpdateKeyboardMouse();
    // process any messages produced from the update
    while( imGUI.getMessageQueueSize() > 0 ) {
        IMMessage@ message = imGUI.getNextMessage();
        if( message.name == "run_file" ) {
            settings_changed = true;
            current_difficulty = int(message.getInt(0));

            ledge_grab = ledge_grab_preset_values[current_difficulty];
            tutorials = tutorials_preset_values[current_difficulty];

            ClearControllerItems();
            BuildMain();
            BuildFooter();
        } else if( message.name == "hover_leave_file" ) {
            if( current_difficulty != -1 ) {
                current_difficulty_description = current_difficulty;
            } else if( current_difficulty_description == message.getInt(0) ) {
                ClearDescription();
            }
        } else if( message.name == "hover_enter_file" ) {
            SetDescription(uint(message.getInt(0)));
        } else if( message.name == "Back"){
            if(GetConfigValueInt("difficulty_preset_value") == 0)
                this_ui.SendCallback( "back_to_main_menu" );
            else
                this_ui.SendCallback( "back" );
        } else if( message.name == "Continue") {
            SetConfigValueString("difficulty_preset", play_menu[current_difficulty].name);
            SetConfigValueInt("difficulty_preset_value", current_difficulty + 1);
            SetConfigValueBool("auto_ledge_grab", ledge_grab);
            SetConfigValueBool("tutorials", tutorials);
            this_ui.SendCallback( "back" );
        } else if( message.name == "option_changed" ) {
            if(!settings_changed)
                AddButtonReversed("Continue", right_panel, arrow_icon, button_back, false, button_width);
            settings_changed = true;
            if(message.getString(0) == "Automatic Ledge Grab") {
                ledge_grab = !ledge_grab;
            } else if(message.getString(0) == "Tutorials") {
                tutorials = !tutorials;
            }
            for(uint i = 0; i < gui_elements.size(); i++){
                if(gui_elements[i].name == message.getString(0)){
                    gui_elements[i].SwitchOption(message.getString(1));
                }
            }
            //ScriptReloaded();
        }
    }
	// Do the general GUI updating
    imGUI.update();
	UpdateController();
}

void ClearDescription() {
    current_difficulty_description = -1;
    description_divider.clear();
}

void SetDescription(uint description_index) {
    description_divider.clear();
    current_difficulty_description = description_index;
    if( description_index < difficulty_description.size() ) {
        for( uint i = 0; i < difficulty_description[description_index].size(); i++ ) {
            IMText@ description_t = @IMText(difficulty_description[description_index][i], description_font);
            description_divider.append(description_t);
        }
    }
}

void Resize() {
    imGUI.doScreenResize(); // This must be called first
	SetDarkenedBackground();
	AddVerticalBar();
}

void ScriptReloaded() {
    gui_elements.resize(0);
    // Clear the old GUI
    imGUI.clear();
    // Rebuild it
    Initialize();
}

void DrawGUI() {
    imGUI.render();
}

void Draw() {

}

void Init(string str) {

}
