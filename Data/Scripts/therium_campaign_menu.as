//-----------------------------------------------------------------------------
//           Name: therium_campaign_menu.as
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
#include "save/therium.as"
#include "levelinfo.as"
#include "music_load.as"
#include "menu_common.as"
#include "utility/set.as"

MusicLoad ml("Data/Music/menu.xml");

IMGUI@ imGUI;

bool HasFocus() {
    return false;
}

const int item_per_screen = 4;
const int rows_per_screen = 3;

bool is_last = true;

string this_campaign_name = "custom_campaign";

array<LevelInfo@> level_list;

void LoadModCampaign() {
    string campaign_id = GetCurrCampaignID();
    this_campaign_name = campaign_id;
    level_list.removeRange(0, level_list.length());
    //Log(info, campaign_id);
    Campaign c = GetCampaign(campaign_id);

    array<ModLevel>@ campaign_levels = c.GetLevels();

    Log( info, "size: " + campaign_levels.length());

    SavedLevel@ campaign_save = CampaignSave();

    for( uint k = 0; k < campaign_levels.length(); k++ ) {
        SavedLevel@ level_save = LevelSave(campaign_levels[k]);
        level_list.insertLast(LevelInfo(
            campaign_levels[k],
            GetHighestDifficultyIndex(level_save.GetArray("finished_difficulties")),
            level_save.GetValue("level_played")=="true",
            IsLevelUnlocked(campaign_levels[k]),
            IsLastPlayedLevel(campaign_levels[k])
        ));
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
    Campaign c = GetCampaign(this_campaign_name);
    Parameter campaign_param = c.GetParameter();

    string current_sub = campaign_param["menu_start"].asString();
    if(StorageHasString(this_campaign_name + "-sub_menu")) {
        current_sub = StorageGetString( this_campaign_name + "-sub_menu" );
    }

    uint current_page = 0;
    if(StorageHasInt32(this_campaign_name + "-" + current_sub + "-menu_page")){
        current_page = StorageGetInt32(this_campaign_name + "-" + current_sub + "-menu_page");
    }
    //Log(info, "current_page "+current_page);
    IMDivider mainDiv( "mainDiv", DOHorizontal );
    mainDiv.setAlignment(CACenter, CACenter);


    Parameter menu_structure = campaign_param["menu_structures"];
    Parameter current_page_data = menu_structure[current_sub];
    Parameter levels = current_page_data["levels"];
    Parameter level_paths = current_page_data["paths"];
    Parameter level_links = current_page_data["links"];

    float arrow_width = 200.0f;
    float arrow_height = 400.0f;
    //The extra space is needed for the arrow scaleonmouseover animation, or else they push the other elements.
    float extra = 50.0f;

    float vertical_arrow_width = 100.0f;
    float vertical_arrow_height = 200.0f;

    //Create the actual level select menu between the two arrows.
    IMContainer menu_container("menu_container", menu_width, menu_height);

    float item_width = 500; 
    float item_height = 300;
    float preview_size_offset = 75.0f / 2.0f;

    // offset is calculated before rotation, and rotation is done around origin, so don't need to account for non-square dimensions
    vec2 vertical_arrow_middle_offset((item_width - vertical_arrow_width) / 2.0f, 0.0f);
    vec2 vertical_arrow_margin(0.0f, 10.0f);

    //Log(info, "current_sub: " + current_sub);
    //Log(info, "current_page_data.size: " + current_page_data.size());
    //Log(info, "levels.size: " + levels.size());
    //Log(info, "level_paths.size: " + level_paths.size());
    //Log(info, "level_links.size: " + level_links.size());
    //Log(info, "menu_structure.size: " + menu_structure.size());

    array<LevelInfo@> current_screen_levels;
    array<int> current_screen_level_indices;
    array<vec2> positions;
    array<int> columns;
    array<bool> visible;

    uint current_column_a = 0;
    uint current_column_b = 0;
    uint visible_columns = 0;

    for(uint i = 0; i < level_list.size(); i++) {
        LevelInfo@ li = level_list[i];
        if(levels.contains(li.id)) {
            string menu_page = level_paths[li.id].asString();

            int menu_column = -1;
            int menu_row = -1;

            if( menu_page == "c" ) {
                menu_row = 3; 
                menu_column = max(current_column_a,current_column_b);
                current_column_a = menu_column+1;
                current_column_b = menu_column+1;
            } else if( menu_page == "a" ) {
                menu_row = 1; 
                menu_column = current_column_a;
                current_column_a++;
            } else if( menu_page == "b" ) {
                menu_row = 5; 
                menu_column = current_column_b;
                current_column_b++;
            }

            if( 1+menu_column > int(visible_columns) ) {
                visible_columns = menu_column+1;
            }

            if( menu_column >= int(current_page) && menu_column < int(current_page) + 4  ) {
                positions.push_back(vec2((menu_column-current_page) * item_width, menu_row * ((item_height+20.f)/4.0f) + 50.0f));
                columns.push_back(menu_column);
                current_screen_levels.push_back(li);
                current_screen_level_indices.push_back(i);
                visible.push_back(li.unlocked);
                //Log(info, "showing: " + li.name);
            }
        }
    }

    for(uint i = 0; i < current_screen_levels.size(); i++) {
        IMDivider@ level_item_container = @IMDivider("level_item_container" + i);
        LevelInfo@ li = current_screen_levels[i];
        ModLevel ml = li.GetModLevel();
        Parameter connections = ml.GetParameter()["levelnext"];

        string menu_page = level_paths[li.id].asString();

        if( visible[i] ) {
            //Log(info,"connections: "+connections.size());
            //Log(info,"current_screen_levels "+current_screen_levels.size());
            set existing_connections;
            for(uint k = 0; k < connections.size(); k++) {
                for(uint j = 0; j < current_screen_levels.size(); j++) {
                    if(connections[k].asString() == current_screen_levels[j].id && visible[j]) {
                        if(existing_connections.insert(li.id + "_to_" + current_screen_levels[j].id)){
                            //Log(info, "Connection from " + li.id + " to " + current_screen_levels[j].id);
                            if( columns[i]+1 == columns[j] ) {
                                LevelInfo@ to = current_screen_levels[j];
                                string to_menu_page = level_paths[to.id].asString();
                                //Log(info, to_menu_page);
                                IMContainer level_link_container("menulink_level_link" + li.id + "_to_" + to.id, DOHorizontal);

                                IMImage up_level_link(line_connector);

                                up_level_link.setZOrdering(-10);
                                vec2 level_link_top_offset(400.0f,140.0f);
                                vec2 rotation_offset;
                                level_link_container.addFloatingElement(up_level_link, "up_level_link"+li.id + "_to_" + to.id, 0, 1);
                                level_link_container.setZOrdering(-10);
                                //up_level_link.scaleToSizeX(vertical_level_link_width);
                                //up_level_link.setColor(button_font.color);
                                if( menu_page == "c" ) {
                                    if( to_menu_page == "a" ) {
                                        up_level_link.setRotation(40);
                                        rotation_offset = vec2(0.0f,-80.0f);
                                    } else if( to_menu_page == "b" ) {
                                        up_level_link.setRotation(-40);
                                        rotation_offset = vec2(0.0f,80.0f);
                                    }
                                } else if( menu_page == "a" ) {
                                    if( to_menu_page == "b" ) {
                                        up_level_link.setRotation(-40);
                                        rotation_offset = vec2(0.0f,180.0f);
                                    } else if( to_menu_page == "c" ) {
                                        up_level_link.setRotation(-40);
                                        rotation_offset = vec2(0.0f,80.0f);
                                    }
                                } else if( menu_page == "b" ) {
                                    if( to_menu_page == "a" ) {
                                        up_level_link.setRotation(40);
                                        rotation_offset = vec2(0.0f,-145.0f);
                                    } else if( to_menu_page == "c" ) {
                                        up_level_link.setRotation(40);
                                        rotation_offset = vec2(0.0f,-80.0f);
                                    }

                                }
                                menu_container.addFloatingElement(level_link_container, "level_link_container_"+li.id + "_to_" + to.id, positions[i] + level_link_top_offset + rotation_offset, 1);
                            }
                        }
                    }
                }
            }

            if( columns[i] == 0 && current_page_data["back"].isEmpty() == false) {
                li.disabled = true;
            } else {
                li.disabled = false;
            }

            CreateMenuItem(level_item_container, li, true, li.last_played, current_screen_level_indices[i]+1, item_width, item_height, false, false, false, true);

            menu_container.addFloatingElement(level_item_container, "level_" + li.id + "_container", positions[i], 1);

            if( level_links[ml.GetID()].isEmpty() == false ) {
                if( IsSubMenuUnlocked( level_links[ml.GetID()].asString() ) ) {
                    string menulink = level_links[ml.GetID()].asString();
                    //IMContainer arrow_container("menulink_arrow"+menulink);
                    IMImage arrow(navigation_arrow);
                    if(kAnimateMenu){
                        arrow.addUpdateBehavior(IMMoveIn ( move_in_time, vec2(move_in_distance * -1, 0), inQuartTween ), "");
                    }
                    IMMessage message("set_sub_menu", menulink);
                    arrow.setColor(button_font.color);
                    arrow.addMouseOverBehavior(mouseover_scale_arrow, "");
                    arrow.addLeftMouseClickBehavior( IMFixedMessageOnClick(message), "");
                    vec2 arrow_position;
                    if( menu_page == "a" ) {
                        vec2 offset(0.0f, -(vertical_arrow_height - preview_size_offset));
                        arrow_position = offset + vertical_arrow_middle_offset - vertical_arrow_margin;
                        //arrow_container.addFloatingElement(up_arrow, "up_arrow_"+menulink, arrow_top_offset + vertical_arrow_middle_offset - vertical_arrow_margin, 12);
                        arrow.scaleToSizeX(vertical_arrow_width);
                        arrow.setRotation(270);
                    } else { 
                        vec2 offset(0.0f, item_height);
                        arrow_position = offset + vertical_arrow_middle_offset + vertical_arrow_margin;
                        //arrow_container.addFloatingElement(down_arrow, "down_arrow_"+menulink, arrow_bottom_offset + vertical_arrow_middle_offset + vertical_arrow_margin, 12);
                        arrow.setClip(false);
                        arrow.scaleToSizeX(100.0f);
                        arrow.setRotation(90);
                    }
                    menu_container.addFloatingElement(arrow, "arrow"+menulink, positions[i] + arrow_position, 12);
                    AddControllerItem(arrow, message);
                    //menu_container.addFloatingElement(arrow_container, "arrow_container_"+menulink, positions[i], 1);
                }
            }

            if( columns[i] == 0 && current_page_data["back"].isEmpty() == false) {
                string menulink = level_links[ml.GetID()].asString();
                //IMContainer arrow_container("menulink_arrow");
                IMImage arrow(navigation_arrow);
                if(kAnimateMenu){
                    arrow.addUpdateBehavior(IMMoveIn ( move_in_time, vec2(move_in_distance * -1, 0), inQuartTween ), "");
                }
                arrow.addMouseOverBehavior(mouseover_scale_arrow, "");
                IMMessage message("set_sub_menu", current_page_data["back"].asString());
                arrow.addLeftMouseClickBehavior( IMFixedMessageOnClick(message), "");
                vec2 arrow_position;
                if( current_page_data["back_direction"].asString() == "up" ) {
                    arrow_position = vec2(0.0f, 50.0f) + vertical_arrow_middle_offset - vertical_arrow_margin;
                    //arrow_container.addFloatingElement(up_arrow, "up_arrow_"+menulink, arrow_top_offset + vertical_arrow_middle_offset - vertical_arrow_margin, 12);
                    arrow.scaleToSizeX(vertical_arrow_width);
                    //up_arrow.setColor(vec4(0.0f,0.0f,0.0f,1.0f));
                    arrow.setRotation(270);
                } else { 
                    arrow_position = vec2(0.0f, 50.0f) + vertical_arrow_middle_offset + vertical_arrow_margin;
                    //arrow_container.addFloatingElement(down_arrow, "down_arrow_"+menulink, arrow_bottom_offset + vertical_arrow_middle_offset + vertical_arrow_margin, 12);
                    arrow.setClip(false);
                    arrow.scaleToSizeX(100.0f);
                    //down_arrow.setColor(vec4(0.0f,0.0f,0.0f,1.0f));
                    arrow.setRotation(90);
                }
                menu_container.addFloatingElement(arrow, "arrow"+menulink, positions[i] + arrow_position, 12);
                AddControllerItem(arrow, message);
                //menu_container.addFloatingElement(arrow_container, "arrow_container_"+menulink, positions[i], 1);
            }
        } else {
            li.disabled = true;
            CreateMenuItem(level_item_container, li, false, false, current_screen_level_indices[i]+1, item_width, item_height, false, false, false, true);
            menu_container.addFloatingElement(level_item_container, "level_" + li.id + "_container", positions[i], 1);
        }
    }

    uint page_count = visible_columns;
    //Log(info, "col_a " + current_column_a);
    //Log(info, "col_b " + current_column_b);
    //Log(info, "pages " + page_count);

    //The left arrow in the level select menu.
    IMContainer left_arrow_container("left_arrow_container", DOHorizontal);
    left_arrow_container.setSize(vec2(arrow_width + extra, arrow_height + extra));
    if(current_page > 0){
        IMImage left_arrow( navigation_arrow );
        if(kAnimateMenu){
            left_arrow.addUpdateBehavior(IMMoveIn ( move_in_time, vec2(move_in_distance * -1, 0), inQuartTween ), "");
        }
        IMMessage message("shift_menu", -1);
        left_arrow_container.addFloatingElement(left_arrow, "left_arrow", vec2((extra / 2.0f), (extra / 2.0f)), 1);
        left_arrow.scaleToSizeX(arrow_width);
        left_arrow.setColor(button_font.color);
        left_arrow.addMouseOverBehavior(mouseover_scale_arrow, "");
        left_arrow.addLeftMouseClickBehavior( IMFixedMessageOnClick(message), "");
        AddControllerItem(left_arrow_container, message);
    } 
    mainDiv.append(left_arrow_container);

    mainDiv.append(menu_container);

    //The right arrow in the level select menu.
    IMContainer right_arrow_container("right_arrow_container", DOHorizontal);
    right_arrow_container.setAlignment(CACenter, CACenter);
    right_arrow_container.setSize(vec2(arrow_width + extra, arrow_height + extra));
    if(page_count != 0 && current_page+4 < page_count){
        IMImage right_arrow( navigation_arrow );
        if(kAnimateMenu){
            right_arrow.addUpdateBehavior(IMMoveIn ( move_in_time, vec2(move_in_distance * 1, 0), inQuartTween ), "");
        }
        IMMessage message("shift_menu", 1);
        right_arrow_container.addFloatingElement(right_arrow, "right_arrow", vec2((extra / 2.0f), (extra / 2.0f)), 1);
        right_arrow.scaleToSizeX(arrow_width);
        right_arrow.setColor(button_font.color);
        right_arrow.addMouseOverBehavior(mouseover_scale_arrow, "");
        right_arrow.addLeftMouseClickBehavior( IMFixedMessageOnClick(message), "");
        //To make the right arrow point in the opposite direction, just rotate it 180 degrees.
        right_arrow.setRotation(180);
        AddControllerItem(right_arrow_container, message);
        is_last = false;
    } else {
        is_last = true;
    }
    mainDiv.append(right_arrow_container);
    // Add it to the main panel of the GUI
    imGUI.getMain().setElement( @mainDiv );
	IMDivider header_divider( "header_div", DOHorizontal );
	AddTitleHeader(GetModTitle(), header_divider);
	imGUI.getHeader().setElement(header_divider);
    AddBackButton(true, true,campaign_param["credits"].asString());
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

        if( message.name == "Back" ) {
            this_ui.SendCallback( "back" );
        } else if( message.name == "run_file" ) {
            //this_ui.SendCallback(message.getString(0));
            //Log(info, "Running " + message.getInt(0));
            LoadLevelID(level_list[message.getInt(0)].id);
        } else if( message.name == "set_sub_menu" ) {
            //Log(info, "setting submenu to " + message.getString(0) );
            StorageSetString(this_campaign_name + "-sub_menu", message.getString(0));
            SetControllerItemBeforeShift();
            BuildUI();
            SetControllerItemAfterShift(message.getInt(0));
        } else if( message.name == "shift_menu" ) {
            string current_sub = "main";
            int current_page = 0;
            if(StorageHasString(this_campaign_name + "-sub_menu")) {
                current_sub = StorageGetString( this_campaign_name + "-sub_menu" );
            }
            if( StorageHasInt32( this_campaign_name + "-" + current_sub + "-menu_page" )){
                current_page = StorageGetInt32( this_campaign_name + "-" + current_sub + "-menu_page" );
            }
            if(!is_last || message.getInt(0) < 0) {
                current_page += message.getInt(0);
                if(current_page < 0)
                    current_page = 0;
            }
            StorageSetInt32( this_campaign_name + "-" + current_sub + "-menu_page", current_page);

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
        else if( message.name == "credits") { 
            this_ui.SendCallback(message.getString(0));
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
    Log(info,"Reloading");
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
