//-----------------------------------------------------------------------------
//           Name: theriumplay_menu.as
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

#include "menu_common.as"
#include "music_load.as"

MusicLoad ml("Data/Music/menu.xml");

const int item_per_screen = 3;

IMGUI imGUI;
array<LevelInfo@> play_menu = {	LevelInfo("t2/prologue.xml",		"Prologue",			"Textures/ui/menus/main/tutorial.jpg"),
								LevelInfo("t2/begin.xml",	"Begin",	"Textures/ui/menus/main/main_campaign.jpg", true),
								LevelInfo("t2/hub.xml",		"Hub",	"Textures/lugarumenu/Village_2.jpg")};

bool HasFocus() {
    return false;
}

void Initialize() {

    // Start playing some music
    PlaySong("overgrowth_main");

    // We're going to want a 100 'gui space' pixel header/footer
	imGUI.setHeaderHeight(200);
    imGUI.setFooterHeight(200);

	imGUI.setFooterPanels(200.0f, 1400.0f);
    // Actually setup the GUI -- must do this before we do anything
    imGUI.setup();

    IMDivider mainDiv( "mainDiv", DOHorizontal );
	AddTitleHeader("Select Level");
	
    array<ModID>@ active_sids = GetActiveModSids();
    for( uint i = 0; i < active_sids.length(); i++ ) {
        array<MenuItem>@ menu_items = ModGetMenuItems(active_sids[i]); 
        for( uint k = 0; k < menu_items.length(); k++ ) {
            if( menu_items[k].GetCategory() == "play" ) {
                string thumbnail_path = menu_items[k].GetThumbnail();
                if( thumbnail_path == "" ) {
                    thumbnail_path = "../" + ModGetThumbnail(active_sids[i]);
                }
                Log(info, thumbnail_path + "\n");
				play_menu.insertLast(LevelInfo(menu_items[k].GetPath(), menu_items[k].GetTitle(), thumbnail_path));
            }
        }
        Campaign camp = ModGetCampaign(active_sids[i]);
        if( camp.GetType() == "general" ) {
            string camp_thumbnail_path = camp.GetThumbnail();
            if( camp_thumbnail_path == "" ) {
                camp_thumbnail_path = "../" + ModGetThumbnail(active_sids[i]);
            }
            Log(info, camp_thumbnail_path + "\n");
			play_menu.insertLast(LevelInfo("general_campaign_menu.as", camp.GetTitle(), camp_thumbnail_path, ModGetID(active_sids[i])));
        }
    }

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

	CreateMenu(mainDiv, play_menu, "play_menu", initial_offset, item_per_screen, 1, false, false, 1800, 350);
	
    // Add it to the main panel of the GUI
    imGUI.getMain().setElement( @mainDiv );
	
	IMDivider back_button_divider("back_button_divider", DOHorizontal);
    back_button_divider.setAlignment(CALeft, CABottom);
    back_button_divider.append(IMSpacer(DOHorizontal, 100));
    AddButton("Back", back_button_divider, arrow_icon, button_back, true, 400.0f);
	imGUI.getFooter(0).setElement(back_button_divider);

	if(NrCustomLevels() != 0){
		IMDivider custom_levels_divider("custom_levels_divider", DOHorizontal);
		float text_trailing_space = 75.0f;
		float button_width = 500.0f;
		AddButton("Custom Levels", custom_levels_divider, "null", button_background_diamond, true, button_width, text_trailing_space, mouseover_scale_button);
		imGUI.getFooter(1).setElement(custom_levels_divider);
	}
	
	setBackGround();
	AddVerticalBar();
}

void Dispose() {
    imGUI.clear();
}

bool CanGoBack() {
    return true;
}

void Update() {
    // Do the general GUI updating
    imGUI.update();
	UpdateController();
    // process any messages produced from the update
    while( imGUI.getMessageQueueSize() > 0 ) {
        IMMessage@ message = imGUI.getNextMessage();

        if( message.name == "run_file" ) 
        {
			string inter_level_data = play_menu[message.getInt(0)].inter_level_data;
			if(inter_level_data != ""){
				SetInterlevelData("current_mod_campaign", inter_level_data);
			}
            this_ui.SendCallback(message.getString(0));
        }
        else if( message.name == "Prologue" ) 
        { 
            this_ui.SendCallback("t2/prologue.xml");
        } 
        else if( message.name == "Begin" ) 
        {
            this_ui.SendCallback("t2/begin.xml");
        } 
        else if( message.name == "Hub" ) 
        {
            this_ui.SendCallback("t2/hub.xml");
        }
        else if( message.name == "Arena" )
        {
            this_ui.SendCallback( "arena_menu.as" );
        }
        else if( message.name == "mod_campaign" ) 
        {
            SetInterlevelData("current_mod_campaign",message.getString(0));
            this_ui.SendCallback("general_campaign_menu.as");
        }
        else if( message.name == "Play" )
        {
            this_ui.SendCallback(message.getString(0));
        }
        else if( message.name == "Versus" )
        {
            this_ui.SendCallback("Project60/22_grass_beach.xml");
        }
		else if( message.name == "Custom Levels" )
        {
			this_ui.SendCallback( "custom_levels.as" );
        }
        else if( message.name == "Back" )
        {
            this_ui.SendCallback( "back" );
        }
		else if( message.name == "shift_menu" ){
			StorageSetInt32("play_menu-shift_offset",ShiftMenu(message.getInt(0)));
		}
    }
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

