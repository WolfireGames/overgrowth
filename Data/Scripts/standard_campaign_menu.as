//-----------------------------------------------------------------------------
//           Name: standard_campaign_menu.as
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
#include "save/linear.as"

MusicLoad ml("Data/Music/menu.xml");

IMGUI@ imGUI;

bool HasFocus() {
    return false;
}

const int item_per_screen = 4;
const int rows_per_screen = 3;

string this_campaign_name = "custom_campaign";

array<LevelInfo@> level_list;

void LoadModCampaign() {
    string campaign_id = GetCurrCampaignID();
    this_campaign_name = campaign_id;
    level_list.removeRange(0, level_list.length());
    Log(info, campaign_id);
    Campaign c = GetCampaign(campaign_id);

    array<ModLevel>@ campaign_levels = c.GetLevels();

    Log( info, "size: " + campaign_levels.length());
    for( uint k = 0; k < campaign_levels.length(); k++ ) {
        level_list.insertLast(LevelInfo(campaign_levels[k],GetHighestDifficultyFinished(campaign_id,campaign_levels[k].GetID()),GetLevelPlayed(campaign_levels[k].GetID()),IsLevelUnlocked(campaign_levels[k]),IsLastPlayedLevel(campaign_levels[k])));
    }
}

string GetModTitle() {
    string campaign_id = GetCurrCampaignID();
    this_campaign_name = campaign_id;
    Campaign c = GetCampaign(campaign_id);
    return c.GetTitle();
}

void Initialize() {
    @imGUI = CreateIMGUI();
    LoadModCampaign();

    // Start playing some music
    PlaySong("overgrowth_main");

    // We're going to want a 100 'gui space' pixel header/footer
    imGUI.setHeaderHeight(200);
    imGUI.setFooterHeight(200);

    // Actually setup the GUI -- must do this before we do anything
    imGUI.setup();
    BuildUI();
	// setup our background
	setBackGround();
}

void BuildUI(){
    int initial_offset = 0;
    if( StorageHasInt32( this_campaign_name + "-shift_offset" )){
        initial_offset = StorageGetInt32( this_campaign_name + "-shift_offset" );
    }
    while( initial_offset >= int(level_list.length()) ) {
        initial_offset -= item_per_screen;
        if( initial_offset < 0 ) {
            initial_offset = 0;
            break;
        }
    }
    IMDivider mainDiv( "mainDiv", DOHorizontal );
    mainDiv.setAlignment(CACenter, CACenter);
    CreateMenu(mainDiv, level_list, this_campaign_name, initial_offset, item_per_screen, rows_per_screen, true, false, menu_width, menu_height, false, false, false, true);
    // Add it to the main panel of the GUI
    imGUI.getMain().setElement( @mainDiv );
	IMDivider header_divider( "header_div", DOHorizontal );
	AddTitleHeader(GetModTitle(), header_divider);
	imGUI.getHeader().setElement(header_divider);
    AddBackButton(true, true);
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

        //Log( info, "Got processMessage " + message.name );

        if( message.name == "Back" )
        {
            this_ui.SendCallback( "back" );
        }
        else if( message.name == "run_file" ) 
        {
            //this_ui.SendCallback(message.getString(0));
            LoadLevelID(level_list[message.getInt(0)].id);
        }
        else if( message.name == "shift_menu" ){
            StorageSetInt32( this_campaign_name + "-shift_offset", ShiftMenu(message.getInt(0)));
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
        else if( message.name == "change_difficulty" ){
            this_ui.SendCallback( "difficulty_menu.as" );
        }
    }
	// Do the general GUI updating
	imGUI.update();
	UpdateController();
}

void Resize() {
    imGUI.doScreenResize(); // This must be called first
    setBackGround();
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
