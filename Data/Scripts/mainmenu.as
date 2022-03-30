//-----------------------------------------------------------------------------
//           Name: mainmenu.as
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

#include "music_load.as"
#include "menu_common.as"
#include "campaign_common.as"

MusicLoad ml("Data/Music/menu.xml");

IMGUI@ imGUI;

bool HasFocus() {
    return false;
}

void Initialize() {
    @imGUI = CreateIMGUI();
    // Start playing some music
    PlaySong("overgrowth_main");

    // We're going to want a 100 'gui space' pixel header/footer
    imGUI.setFooterHeight(200);

    // Actually setup the GUI -- must do this before we do anything
    imGUI.setup();
    BuildUI();
	if(GetInterlevelData("background") == ""){
		SetInterlevelData("background", GetRandomBackground());
	}
	setBackGround();
	AddVerticalBar();
	controller_wraparound = true;
}

void BuildUI(){
    IMDivider mainDiv( "mainDiv", DOHorizontal ); 

    IMDivider left_panel("left_panel", DOVertical);
    left_panel.setAlignment(CALeft, CACenter);
    mainDiv.append(left_panel);

    IMImage logo("Textures/ui/menus/main/overgrowth.png");
    IMDivider logo_holder(DOVertical);
    IMDivider logo_holder_holder(DOHorizontal);
    logo_holder_holder.setSize(vec2(UNDEFINEDSIZE,UNDEFINEDSIZE));
    logo_holder.append(IMSpacer(DOVertical, 50));
    logo_holder.append(logo);
    logo_holder_holder.append(logo_holder);
    left_panel.append(logo_holder);

    left_panel.append(IMSpacer(DOVertical, 100));
    IMDivider horizontal_buttons_holder(DOHorizontal);
    horizontal_buttons_holder.append(IMSpacer(DOHorizontal, 75));
    IMDivider buttons_holder("buttons_holder", DOVertical);
    buttons_holder.append(IMSpacer(DOHorizontal, 200));
    buttons_holder.setAlignment(CACenter, CACenter);
    horizontal_buttons_holder.append(buttons_holder);
    left_panel.append(horizontal_buttons_holder);

    string last_campaign_id = GetGlobalSave().GetValue("last_campaign_played");
    Campaign camp = GetCampaign(last_campaign_id);  
    if(camp.GetRequiresOnline() == false && camp.GetLevels().size() > 0 && GetConfigValueInt("difficulty_preset_value") != 0) {
        AddButton("Continue", buttons_holder, forward_chevron);
    }

    AddButton("Play", buttons_holder, play_icon);
    AddButton("Multiplayer", buttons_holder, online_icon);
    AddButton("Settings", buttons_holder, settings_icon);
    AddButton("Mods",     buttons_holder, mods_icon);
    AddButton("Exit",     buttons_holder, exit_icon);

    // Align the contained element to the left
    imGUI.getMain().setAlignment( CALeft, CATop );

    // Add it to the main panel of the GUI
    imGUI.getMain().setElement( @mainDiv );
}

void Dispose() {
    imGUI.clear();
}

bool CanGoBack() {
    return true;
}

void Update() {
    // process any messages produced from the update
    while( imGUI.getMessageQueueSize() > 0 ) {
        IMMessage@ message = imGUI.getNextMessage();

		if( message.name == "run_file" )
        {
            this_ui.SendCallback(message.getString(0));
        }
        else if( message.name == "Editor" )
        {
            LoadEditorLevel();
        }
        else if( message.name == "Credits" )
        {
            this_ui.SendCallback( "credits.as" );
        }
        else if( message.name == "Mods" )
        {
            this_ui.SendCallback( "mods" );
        }
        else if ( message.name == "Multiplayer")
        {
            if(Online_HasActiveIncompatibleMods()) {
                QueueBasicPopup("Error", "Unsupported mods detected!\nPlease disable them before trying to play online!\n\n" + Online_GetActiveIncompatibleModsString());
            } else {
                if(GetConfigValueInt("difficulty_preset_value") == 0){
                    this_ui.SendCallback( "multiplayer_menu.as" );
                    this_ui.SendCallback( "difficulty_menu.as" );
                } else {
                    this_ui.SendCallback( "multiplayer_menu.as" );
                }
            }
        }
        else if( message.name == "Play" ) 
        {
            // Ugly way to make "back" work
            // When the continue button is pressed in the difficulty menu, "back" is called
            if(GetConfigValueInt("difficulty_preset_value") == 0){
                this_ui.SendCallback( "play_menu.as" );
                this_ui.SendCallback( "difficulty_menu.as" );
            } else
                this_ui.SendCallback( "play_menu.as" );
        }
        else if( message.name == "Continue" )
        {
            string campaign_id = GetGlobalSave().GetValue("last_campaign_played");
            string level_id = GetGlobalSave().GetValue("last_level_played");

            SetCampaignID(campaign_id);
            LoadLevelID(level_id);
        }
        else if( message.name == "Exit" )
        {
            this_ui.SendCallback( "exit" );
        }
        else if( message.name == "Settings" )
        {
			this_ui.SendCallback( "main_menu_settings.as" );
        }
        else if( message.name == "Credits" ) 
        {
            this_ui.SendCallback( "credits.as" );
        }
        else if( message.name == "News" ) 
        {
            Log( info, "Placeholder for news button" );
        }
    }
	// Do the general GUI updating
	imGUI.update();
	UpdateController();
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
