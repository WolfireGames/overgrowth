//-----------------------------------------------------------------------------
//           Name: play_menu.as
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

bool HasFocus() {
    return false;
}

void Initialize() {
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

array<string> hard_order = {"com-wolfire-overgrowth-campaign","com-wolfire-lugaru-campaign","com-wolfire-timbles-therium","com-wolfire-drika"};

void SetPlayMenuList() {
    for( uint i = 0; i < hard_order.length(); i++ ) {
        Campaign camp = GetCampaign(hard_order[i]);
        array<ModLevel>@ campaign_levels = camp.GetLevels();

        int completed_levels = 0;

        for(uint j = 0; j < campaign_levels.size(); ++j) {
            if(!campaign_levels[j].CompletionOptional() && GetHighestDifficultyFinished(camp.GetID(), campaign_levels[j].GetID()) != 0) {
                completed_levels++;
            }
        }

        int total_levels = 0;

        for(uint j = 0; j < campaign_levels.size(); ++j) {
            if(!campaign_levels[j].CompletionOptional()) {
                total_levels++;
            }
        }

        string camp_thumbnail_path = camp.GetThumbnail();
 
        play_menu.insertLast(LevelInfo(camp.GetMenuScript(), camp.GetTitle(), camp_thumbnail_path, camp.GetID(), GetHighestDifficultyFinishedCampaign(camp.GetID()), GetLastLevelPlayed(camp.GetID()).length() > 0, true, IsLastCampaignPlayed(camp), completed_levels, total_levels));
        
    }

    array<Campaign>@ campaigns = GetCampaigns();
    for( uint i = 0; i < campaigns.length(); i++ ) {
        Campaign camp = campaigns[i];
        bool skip = false;
        for( uint k = 0; k < hard_order.length(); k++ ) {
            if( hard_order[k] == camp.GetID() ) {
                skip = true;
            }
        }

        if( skip == false ) {
            string camp_thumbnail_path = camp.GetThumbnail();
            if (!camp.GetRequiresOnline()) {
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

    AddCustomLevelsMenuItem();

    array<ModID>@ active_sids = GetActiveModSids();
    for( uint i = 0; i < active_sids.length(); i++ ) {
        array<MenuItem>@ menu_items = ModGetMenuItems(active_sids[i]);
        for( uint k = 0; k < menu_items.length(); k++ ) {
            if( menu_items[k].GetCategory() == "play" ) {
                string thumbnail_path = menu_items[k].GetThumbnail();
                if( thumbnail_path == "" ) {
                    thumbnail_path = "../" + ModGetThumbnail(active_sids[i]);
                }
                LevelInfo li(menu_items[k].GetPath(), menu_items[k].GetTitle(), thumbnail_path, true, false);
                li.hide_stars = true;
				play_menu.insertLast(li);
            }
        }
    }
}

void BuildUI(){
    IMDivider mainDiv( "mainDiv", DOHorizontal );
	IMDivider header_divider( "header_div", DOHorizontal );
	header_divider.setAlignment(CACenter, CACenter);
	AddTitleHeader("Play", header_divider);
	imGUI.getHeader().setElement(header_divider);

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
	UpdateKeyboardMouse();
    // process any messages produced from the update
    while( imGUI.getMessageQueueSize() > 0 ) {
        IMMessage@ message = imGUI.getNextMessage();

        if( message.name == "run_file" )
        {
			string campaign_id = play_menu[message.getInt(0)].campaign_id;
			if(campaign_id != "") {
                SetCampaignID(campaign_id);
			}
            this_ui.SendCallback(message.getString(0));
        }
        else if( message.name == "Tutorial" )
        {
            this_ui.SendCallback("tutorial.xml");
        }
        else if( message.name == "Main Campaign" )
        {
            this_ui.SendCallback("campaign_menu.as");
        }
        else if( message.name == "Lugaru" )
        {
            this_ui.SendCallback("lugaru_menu.as");
        }
        else if( message.name == "Arena" )
        {
            this_ui.SendCallback( "arena_menu.as" );
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
        else if( message.name == "change_difficulty" )
        {
            this_ui.SendCallback( "difficulty_menu.as" );
        }
		else if( message.name == "shift_menu" ){
            StorageSetInt32("play_menu-shift_offset", ShiftMenu(message.getInt(0)));
            SetControllerItemBeforeShift();
            BuildUI();
            SetControllerItemAfterShift(message.getInt(0));
		}
        else if( message.name == "refresh_menu_by_name" ){
			string current_controller_item_name = GetCurrentControllerItemName();
			BuildUI();
			SetCurrentControllerItem(current_controller_item_name);
		}
		else if( message.name == "refresh_menu_by_id" ){
			int index = GetCurrentControllerItemIndex();
			BuildUI();
			SetCurrentControllerItem(index);
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
