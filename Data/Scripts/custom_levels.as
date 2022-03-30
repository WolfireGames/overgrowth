//-----------------------------------------------------------------------------
//           Name: custom_levels.as
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

MusicLoad ml("Data/Music/lugaru_new.xml");

IMGUI@ imGUI;
array<LevelInfo@> custom_levels;
LevelSearch search;

const int item_per_screen = 4;
const int rows_per_screen = 3;

bool HasFocus() {
    return false;
}

void Initialize() {
    @imGUI = CreateIMGUI();
    // Start playing some music
    PlaySong("lugaru_menu");
    // We're going to want a 100 'gui space' pixel header/footer
    imGUI.setHeaderHeight(200);
    imGUI.setFooterHeight(200);
    // Actually setup the GUI -- must do this before we do anything
    imGUI.setup();
	ResetLevelList();
	BuildUI();
	search.SetCollection(custom_levels);
	setBackGround();
}

void BuildUI(){
    ClearControllerItems();
	IMDivider mainDiv( "mainDiv", DOHorizontal );
    int initial_offset = 0;
    if( StorageHasInt32("custom_levels-shift_offset") ) {
        initial_offset = StorageGetInt32("custom_levels-shift_offset");
    }
    while( initial_offset >= int(custom_levels.length()) ) {
        initial_offset -= item_per_screen;
        if( initial_offset < 0 ) {
            initial_offset = 0;
            break;
        }
    }
	if(custom_levels.size() == 0){
		AddNoResults(mainDiv, true);
	}else{
		CreateMenu(mainDiv, custom_levels, "custom_levels", initial_offset, item_per_screen, rows_per_screen, false, false, menu_width, menu_height, false, false, false, true);
	}
	imGUI.getMain().setElement(mainDiv);
    AddCustomLevelsHeader();
	AddBackButton(true,true);
	search.ShowSearchResults();
}

bool CanShift(int direction){
	int new_start_item = search.current_index + (max_rows * max_items * direction);
	if(uint(new_start_item) < custom_levels.size() && new_start_item > -1){
		return true;
	} else{
		return false;
	}
}

void ResetLevelList(){
	custom_levels.resize(0);
	array<ModID>@ active_sids = GetActiveModSids();
    for( uint i = 0; i < active_sids.length(); i++ ) {
        array<ModLevel>@ menu_items = ModGetSingleLevels(active_sids[i]); 
        for( uint k = 0; k < menu_items.length(); k++ ) {
			custom_levels.insertLast(LevelInfo(menu_items[k],0,false,true,false));
        }
    }
}

void AddCustomLevelsHeader(){
	IMContainer header_container(2560, 200);
	IMDivider header_divider( "header_div", DOHorizontal );
	header_container.setElement(header_divider);
	
	AddTitleHeader("Custom Levels", header_divider);
	AddSearchbar(header_divider, @search);
	imGUI.getHeader().setElement(header_divider);
}

void Dispose() {
	imGUI.clear();
}

bool CanGoBack() {
    return true;
}

void Update() {
	if(!search.active){
		UpdateController();
	}
	search.Update();
	UpdateKeyboardMouse();
    // process any messages produced from the update
    while( imGUI.getMessageQueueSize() > 0 ) {
        IMMessage@ message = imGUI.getNextMessage();

        //Log( info, "Got processMessage " + message.name );

        if( message.name == "Back" )
        {
            this_ui.SendCallback( "back" );
        }
        else if( message.name == "load_level" )
        { 
        	
            this_ui.SendCallback( "Data/Levels/" + message.getString(0) );
        }
		else if( message.name == "shift_menu" ){
            StorageSetInt32("custom_levels-shift_offset", ShiftMenu(message.getInt(0)));
            SetControllerItemBeforeShift();
            BuildUI();
            SetControllerItemAfterShift(message.getInt(0));
		}
		else if( message.name == "run_file" ) 
        {
            this_ui.SendCallback(message.getString(0));
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
		else if( message.name == "activate_search" ){
			search.Activate();
		}
		else if( message.name == "clear_search_results" ){
			ResetLevelList();
			search.ResetSearch();
            SetCurrentControllerItem(0);
			imGUI.receiveMessage( IMMessage("refresh_menu_by_id") );
		}
    }
	// Do the general GUI updating
	imGUI.update();
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

class LevelSearch : Search{
	array<LevelInfo@>@ collection;
	LevelSearch(){
		
	}
	void SetCollection(array<LevelInfo@>@ _collection){
		@collection = @_collection;
	}
	void GetSearchResults(string query){
		array<LevelInfo@> results;
		array<ModID>@ active_sids = GetActiveModSids();
	    for( uint i = 0; i < active_sids.length(); i++ ) {
	        array<ModLevel>@ menu_items = ModGetSingleLevels(active_sids[i]); 
	        for( uint k = 0; k < menu_items.length(); k++ ) {
				if(ToLowerCase(menu_items[k].GetTitle()).findFirst(query) != -1){
					results.insertLast(LevelInfo(menu_items[k].GetPath(), menu_items[k].GetTitle(), menu_items[k].GetThumbnail(),false,true,false));
					continue;
				}
	        }
	    }
		collection = results;
	}
}
