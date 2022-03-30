//-----------------------------------------------------------------------------
//           Name: multiplayer_menu.as
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

#include "save/general.as"
#include "menu_common.as"
#include "music_load.as"

MusicLoad ml("Data/Music/menu.xml");

const int item_per_screen = 4;
const int rows_per_screen = 3;

IMGUI@ imGUI;
array<LevelInfo@> play_menu = {};

FontSetup invalid_name_font("edosz", 50 , HexColor("#ff5555"), true);

bool HasFocus() {
    return false;
}

void Initialize() {
    if (Online_IsHosting()) {
        Online_Close();
    }

    @imGUI = CreateIMGUI();
    // Start playing some music
    PlaySong("overgrowth_main");
    play_menu.resize(0);

    // We're going to want a 100 'gui space' pixel header/footer
	imGUI.setHeaderHeight(200);
    imGUI.setFooterHeight(200);

	imGUI.setFooterPanels(200.0f, 1400.0f);
    // Actually setup the GUI -- must do this before we do anything
    imGUI.setup();
    SetPlayMenuList();
    BuildUI();
	setBackGround();
	AddVerticalBar();
}

void SetPlayMenuList() {
    array<Campaign>@ campaigns = GetCampaigns();
    for( uint i = 0; i < campaigns.length(); i++ ) {
        Campaign camp = campaigns[i];
        if (camp.GetSupportsOnline()) {
            string camp_thumbnail_path = camp.GetThumbnail();
            play_menu.insertLast(
                LevelInfo(  camp.GetMenuScript(),
                    camp.GetTitle(),
                    camp_thumbnail_path,
                    camp.GetID(),
                    0,
                    GetLastLevelPlayed(camp.GetID()).length() > 0,
                    true,
                    IsLastCampaignPlayed(camp),
                    -1,
                    -1
                )
            );
        }
    }
}

TextBox text_box_ip;
TextBox text_box_playername;

void BuildUI(){
    IMDivider mainDiv( "mainDiv", DOHorizontal ); 

    IMDivider left_panel("left_panel", DOVertical);
    left_panel.setAlignment(CALeft, CACenter);
    mainDiv.append(left_panel);


    left_panel.append(IMSpacer(DOVertical, 100));
    IMDivider horizontal_buttons_holder(DOHorizontal);
    horizontal_buttons_holder.append(IMSpacer(DOHorizontal, 75));
    IMDivider buttons_holder("buttons_holder", DOVertical);
    buttons_holder.append(IMSpacer(DOHorizontal, 200));
    buttons_holder.setAlignment(CACenter, CACenter);
    horizontal_buttons_holder.append(buttons_holder);
    left_panel.append(horizontal_buttons_holder);

    int initial_offset = 0;
    if( StorageHasInt32("play_menu-shift_offset") ) {
        initial_offset = StorageGetInt32("play_menu-shift_offset");
    }
    while( initial_offset >= int(play_menu.length()) ) {
        initial_offset -= item_per_screen;
        if( initial_offset < 0 ) {
            initial_offset = 0;
            break;
        }
    }
	CreateMenu(mainDiv, play_menu, "play_menu", initial_offset, item_per_screen, rows_per_screen, false, false, menu_width, menu_height, false,false,false,true);
    // Add it to the main panel of the GUI
    imGUI.getMain().setElement( @mainDiv );
    float button_trailing_space = 100.0f;
    float button_width = 550.0f;
    bool animated = false;

    // Setup name Textbox
    text_box_playername.on_value_changed_message = "update_playername";
    text_box_playername.input_text = GetConfigValueString("playername");
    text_box_playername.default_text = "Enter Player Name...";
    @text_box_playername.validator = AlphaNumericalTextValidator();
    AddTextInput(buttons_holder, text_box_playername, "activate_playername_text_input");

    // Setup ip Textbox
    text_box_ip.on_enter_message = "Connect by IP";
    text_box_ip.on_value_changed_message = "update_last_ip";
    text_box_ip.input_text = GetConfigValueString("last_ip");
    text_box_ip.default_text = "Enter IP...";
    AddTextInput(buttons_holder, text_box_ip, "activate_ip_text_input");
    AddButton("Connect by IP", buttons_holder, play_icon);

    AddBackButton(true, true);
}

void AddCustomLevelsMenuItem() {
	if(NrCustomLevels() != 0) {
        LevelInfo li("custom_levels.as", "Custom Levels", "Textures/ui/menus/main/custom_level_thumbnail.jpg", true, false);
        li.hide_stars = true;
		play_menu.insertLast(li);
	}
}

void Dispose() {
    imGUI.clear();
}

bool CanGoBack() {
    return true;
}

void Update() {
    if(!text_box_ip.active && !text_box_playername.active) {
        UpdateController();
    }
    text_box_ip.Update();
    text_box_playername.Update();

	UpdateKeyboardMouse();
    // process any messages produced from the update
    while( imGUI.getMessageQueueSize() > 0 ) {
        IMMessage@ message = imGUI.getNextMessage();

        if( message.name == "run_file" )
        {
			string campaign_id = play_menu[message.getInt(0)].campaign_id;
			if(campaign_id != "")  {
                SetCampaignID(campaign_id);
            }

            // We assume that running a file in this menu means we start hosting
            // In reality, mods can and will place their own menu before a map loads.
            Online_Host();

            this_ui.SendCallback(message.getString(0));
        }
        else if (message.name == "Connect by IP")
        {
            this_ui.SendCallback("multiplayer_menu_connecting.as");
            Online_Connect(text_box_ip.GetText());
        }
		else if( message.name == "Custom Levels" )
        {
			this_ui.SendCallback( "custom_levels.as" );
        }
        else if( message.name == "Back" )
        {
            this_ui.SendCallback( "back" );
        }
        else if( message.name == "change_difficulty" )
        {
            this_ui.SendCallback( "difficulty_menu.as" );
        }
        else if (message.name == "activate_ip_text_input")
        {
            text_box_playername.Deactivate();
            text_box_ip.Activate();
        }
        else if (message.name == "activate_playername_text_input")
        {
            text_box_ip.Deactivate();
            text_box_playername.Activate();
        }
        else if (message.name == "update_playername")
        {
            if(Online_IsValidPlayerName(text_box_playername.input_text)) {
                text_box_playername.text_font = button_font_small;
                SetConfigValueString("playername", text_box_playername.input_text);
            } else {
                text_box_playername.text_font = invalid_name_font;
            }
            text_box_playername.UpdateText();
        }
        else if (message.name == "update_last_ip")
        {
            SetConfigValueString("last_ip", text_box_ip.input_text);
        }
    }

	// Do the general GUI updating
    imGUI.update();
}

void Resize() {
    imGUI.doScreenResize(); // This must be called first
	setBackGround();
	AddVerticalBar();
}

void ScriptReloaded() {
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
