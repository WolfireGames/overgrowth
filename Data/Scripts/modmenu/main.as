#include "menu_common.as"
#include "music_load.as"
#include "mod_common.as"

MusicLoad ml("Data/Music/menu.xml");

IMGUI@ imGUI;
int inspected = 0;
int num_mods = 0;
int show_nr_mods = 8;
int shift_amount = 1;
bool inspect_fadein = true;
array<WaitingAnimation@> waiting_anims;
IMPulseAlpha waiting_pulse(1.0f, 0.0f, 1.0f);
ModSearch search;
array<ModID> current_mods;
int last_shift_direction = 0;
WorkshopWatcher workshopwatcher;

FontSetup description_font("Cella", 27 , HexColor("#CCCCCC"), true);
FontSetup description_error_font("Cella", 27 , HexColor("#FF9999"), true);

class WaitingAnimation{
	IMContainer@ parent;
	IMContainer@ holder;
	IMImage@ image;
	float timer = 0.0f;
	bool added = false;
	bool delete_this = false;
	ModID mod_id;
	bool subscribed;
    UserVote vote;
    bool favorite;
    string variable_type;
	void Activate(){}
	void Update(){}
	void CheckVariableState(){
		if(added){
            if(variable_type == "subscribed"){
                if(subscribed != ModCanActivate(mod_id)){
                    imGUI.receiveMessage( IMMessage("refresh_menu_by_id") );
                    subscribed = ModCanActivate(mod_id);
                    delete_this = true;
                }
            } else if(variable_type == "vote"){
                if(vote != ModGetUserVote(mod_id)){
                    imGUI.receiveMessage( IMMessage("refresh_menu_by_id") );
                    vote = ModGetUserVote(mod_id);
                    delete_this = true;
                }
            } else if(variable_type == "favorite"){
                if(favorite != ModIsFavorite(mod_id)){
                    imGUI.receiveMessage( IMMessage("refresh_menu_by_id") );
                    favorite = ModIsFavorite(mod_id);
                    delete_this = true;
                }
            }
		}
	}
}

class Spinner : WaitingAnimation{
	Spinner(IMContainer@ _holder, IMContainer@ _parent, ModID _mod_id){
		@parent = @_parent;
		@holder = @_holder;
		mod_id = _mod_id;
		//Current state of subscribed.
		subscribed = ModCanActivate(mod_id);
        variable_type = "subscribed";
	}
    Spinner(IMContainer@ _parent, UserVote _vote, ModID _mod_id){
        vote = _vote;
        @parent = @_parent;
        @holder = @_parent;
        mod_id = _mod_id;
        variable_type = "vote";
    }
	void Activate(){
		if(image is null){
			holder.clear();
			parent.clearMouseOverBehaviors();
			parent.clearLeftMouseClickBehaviors();
			IMImage spinner_image(spinner_icon);
			spinner_image.scaleToSizeX(50.0f);
			spinner_image.setZOrdering(6);
			@image = @spinner_image;
			holder.setElement(spinner_image);
			added = true;
		}
	}
	void Update(){
		CheckVariableState();
		if(added){
			float speed = 120.0f;
			float new_rotation = image.getRotation() + time_step * speed;
			if(new_rotation > 360){
				image.setRotation(0.0f);
			}else{
				image.setRotation(new_rotation);
			}
		}
	}
}

class Pulse : WaitingAnimation {
	Pulse(IMContainer@ _holder, IMContainer@ _parent, ModID _mod_id){
		@parent = @_parent;
		@holder = @_holder;
		mod_id = _mod_id;
		//Current state of subscribed.
		subscribed = IsWorkshopSubscribed(mod_id);
	}
	void Activate(){
		if(image is null){
			holder.clear();
			parent.clearLeftMouseClickBehaviors();
			IMImage spinner_image(spinner_icon);
			spinner_image.scaleToSizeX(50.0f);
			spinner_image.setZOrdering(6);
			@image = @spinner_image;
			holder.setElement(spinner_image);
			spinner_image.addUpdateBehavior(waiting_pulse, "pulse");
			added = true;
		}
	}
	void Update(){
		CheckVariableState();
	}
}

Pulse@ ConstructUserVotePulse(IMContainer@ _parent, UserVote _vote, ModID _mod_id) {
    Pulse pulse(_parent,_parent,_mod_id);
    pulse.vote = _vote;
    pulse.variable_type = "vote";
    return pulse; 
}

Pulse@ ConstructFavoritePulse(IMContainer@ _parent, bool _fav, ModID _mod_id) {
    Pulse pulse(_parent,_parent,_mod_id);
    pulse.favorite = _fav;
    pulse.variable_type = "favorite";
    return pulse;
}

void ResetModsList(){
	current_mods = SortAlphabetically(FilterCore(GetModSids()));
}

bool HasFocus() {
    return false;
}

void Initialize() {
    @imGUI = CreateIMGUI();
    // Start playing some music
	PlaySong("overgrowth_main");

    imGUI.setHeaderHeight(225);
    imGUI.setFooterHeight(225);
	ReloadMods();
	imGUI.setFooterPanels(500.0f, 500.0f);
    // Actually setup the GUI -- must do this before we do anything
    imGUI.setup();
	current_mods = SortAlphabetically(FilterCore(GetModSids()));
	search.SetCollection(current_mods);
	BuildUI();
	SetDarkenedBackground();
}

array<ModID> SortAlphabetically(array<ModID> input_mods){
	array<ModID> return_mods;
	for(uint i = 32; i <= 126; i++){
		for(uint p = 0; p < input_mods.size(); p++){
            string modname = ModGetName(input_mods[p]);
            if( modname.length() > 0 ) {
                uint first_letter = modname[0];
                if(first_letter >= 65 && first_letter <= 90){
                    first_letter += 32;
                }
                if(i == first_letter){
                    return_mods.insertLast(input_mods[p]);
                }
            }
		}
	}
	return return_mods;
}

array<ModID> FilterCore(array<ModID> input_mods){
	array<ModID> return_mods;
    for(uint p = 0; p < input_mods.size(); p++){
        if(ModIsCore(input_mods[p]) == false) {
            return_mods.insertLast(input_mods[p]);
        }
    }
	return return_mods;
}

void BuildUI(){
    gui_elements.resize(0);
	waiting_anims.resize(0);
	ClearControllerItems();
	IMDivider mainDiv( "mainDiv", DOHorizontal );
	imGUI.getMain().setAlignment(CACenter, CATop);
	imGUI.getMain().setElement(mainDiv);
	CreateModMenu(mainDiv, search.current_index);
	AddModsHeader();
	//The diver where the downloads are shown.
	IMDivider downloader_divider("downloader_divider", DOVertical);;
	imGUI.getFooter(1).setElement(downloader_divider);
	workshopwatcher.SetParent(downloader_divider);
	AddBackButton();
	search.ShowSearchResults();
}

bool CanShift(int direction){
	int new_start_item = search.current_index + (shift_amount * direction);
	if(new_start_item <= num_mods - show_nr_mods && new_start_item > -1){
		return true;
	} else{
		return false;
	}
	/*int new_start_item = search.current_index + (max_rows * max_items * direction);
	if(uint(new_start_item) < custom_levels.size() && new_start_item > -1){*/
}

void CreateModMenu(IMDivider@ parent, int start_at){
	int mods_added = 0;
	float holder_width = 2000;
	float holder_height = 900;
	float top_bar_height = 100;
	float bottom_bar_height = 100;
	
	IMDivider vert_holder("vert_holder", DOVertical);
	
	IMDivider top_bar("top_bar", DOHorizontal);
	IMDivider search_bar_holder("search_bar_holder", DOHorizontal);
	top_bar.setAlignment(CALeft, CABottom);
	
	AddSearchbar(search_bar_holder, @search);
	search_bar_holder.setSizeX(holder_width / 2.0f);
	top_bar.append(search_bar_holder);
	if(IsWorkshopAvailable()) {
		IMContainer get_more_container(holder_width / 2.0f, top_bar_height);
		IMDivider get_more_holder("get_more_holder", DOVertical);
		get_more_container.setElement(get_more_holder);
		get_more_holder.setAlignment(CARight, CACenter);
		get_more_container.setAlignment(CARight, CACenter);
	    AddGetMoreMods(get_more_holder);
		get_more_holder.appendSpacer(50.0f);
		get_more_holder.setSizeX(holder_width / 2.0f);
		top_bar.append(get_more_container);
    }else{
		top_bar.appendSpacer(holder_width / 2.0f);
	}
	vert_holder.append(top_bar);
	
	IMDivider horiz_holder("horiz_holder", DOHorizontal);
	IMDivider mods_holder("mods_holder", DOVertical);
	vert_holder.append(horiz_holder);
	horiz_holder.append(mods_holder);

	IMContainer mods_container(10, 10);
	mods_container.setElement(vert_holder);
	
	num_mods = current_mods.size();
	for(uint i = start_at; mods_added < show_nr_mods && i < current_mods.size(); i++, mods_added++){
		bool fadein = false;
		if(last_shift_direction == -1){
			if(i == uint(start_at)){
				fadein = true;
			}
		}else if(last_shift_direction == 1){
			if(mods_added == (show_nr_mods - 1)){
				fadein = true;
			}
		}
		IMContainer@ new_mod_item = AddModItem(mods_holder, current_mods[i], i, fadein);
	}
	last_shift_direction = 0;
	parent.append(mods_container);
	parent.appendSpacer(15.0f);
	if(inspected > int(current_mods.size())){
		inspected = 0;
	}
	top_bar.setSizeX(holder_width);
	if(current_mods.size() < 1){
		horiz_holder.appendSpacer(715);
		AddNoResults(horiz_holder, inspect_fadein);
		return;
	}
	AddScrollBar(horiz_holder, 50.0f, 700.0f, num_mods, show_nr_mods, search.current_index);
	horiz_holder.appendSpacer(15.0f);


    {
	    IMDivider bottom_bar("bottom_bar", DOHorizontal);
	    bottom_bar.setAlignment(CALeft, CABottom);

		IMContainer deactivate_mods_container(holder_width + 130, bottom_bar_height);
		IMDivider deactivate_mods_holder("deactivate_mods_holder", DOVertical);
		deactivate_mods_container.setElement(deactivate_mods_holder);
		deactivate_mods_holder.setAlignment(CALeft, CACenter);
		deactivate_mods_container.setAlignment(CALeft, CACenter);
		deactivate_mods_holder.appendSpacer(50.0f);
	    AddDeactivateAllMods(deactivate_mods_holder);
		deactivate_mods_holder.setSizeX(holder_width + 130);
        bottom_bar.append(deactivate_mods_container);
		vert_holder.append(bottom_bar);

	    bottom_bar.setSizeX(holder_width);
    }

	AddModInspector(horiz_holder, current_mods[inspected], inspect_fadein, inspected);
	inspect_fadein = false;
}

void AddModInspector(IMDivider@ parent, ModID mod_id, bool fadein, int index){
	vec2 preview_image_size(700, 400);
	int fadein_time = 250;
	vec4 title_background_color(0.2f,0.2f,0.2f,1.0f);
	vec4 background_color = vec4(0.10f,0.10f,0.10f,0.90f);
	vec2 inspector_size = vec2(1400, 700);
	vec2 buttons_size(500, 90);
	float button_vert_offset = 10.0f;
	float background_size_offset = 250.0f;
	bool steamworks_mod = IsWorkshopMod(mod_id);
	
	IMContainer inspector_container(inspector_size.x, inspector_size.y);
	IMDivider inspector_divider("inspector_divider", DOVertical);
	inspector_divider.appendSpacer(25.0f);
	inspector_container.setElement(inspector_divider);
	parent.append(inspector_container);
	
	//The main background
	IMImage main_background( white_background );
	if(fadein){
		main_background.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
	}
	main_background.setZOrdering(0);
	main_background.setSize(vec2(inspector_size.x - background_size_offset, inspector_size.y));
	inspector_container.addFloatingElement(main_background, "main_background", vec2(background_size_offset / 2.0f, 0.0f));
	main_background.setColor(background_color);
	
	IMDivider horiz_holder("horiz_holder", DOHorizontal);
	inspector_divider.append(horiz_holder);
	
	//The preview image
	IMContainer preview_image_container(preview_image_size.x, preview_image_size.y);
	IMImage background_preview_image(white_background);
	if(fadein){
		background_preview_image.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
	}
	
	background_preview_image.setColor(title_background_color);
	background_preview_image.setSize(preview_image_size);
	background_preview_image.setZOrdering(3);
	background_preview_image.setClip(false);
	preview_image_container.addFloatingElement(background_preview_image, "background_preview_image", vec2(0));
	horiz_holder.append(preview_image_container);

	IMImage@ preview_image = ModGetThumbnailImage(mod_id);
	if(fadein){
		preview_image.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
	}
	preview_image.setSize(preview_image_size - 25.0f);
	preview_image.setZOrdering(5);
	preview_image_container.setElement(preview_image);
	
	horiz_holder.appendSpacer(50);
	
	//The details, configure and details buttons at the end.
	IMContainer buttons_container(buttons_size.x, buttons_size.y);
	IMDivider buttons_divider("buttons_divider", DOVertical);
	buttons_container.setElement(buttons_divider);
	buttons_divider.setZOrdering(4);

	bool can_be_configured = false;
	if(can_be_configured){
		IMContainer configure_button_container(buttons_size.x, buttons_size.y + button_vert_offset);
		configure_button_container.sendMouseOverToChildren(true);
		configure_button_container.addLeftMouseClickBehavior(IMFixedMessageOnClick("configure", ModGetID(mod_id)), "");
		AddControllerItem(configure_button_container, IMMessage("configure", ModGetID(mod_id)));
		IMText configure_button_text("Configure", button_font_large);
		configure_button_text.addMouseOverBehavior( text_color_mouse_over, "" );
		if(fadein){
			configure_button_text.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
		}
		configure_button_container.setZOrdering(4);
		configure_button_text.setZOrdering(6);
		configure_button_container.setElement(configure_button_text);
		IMImage configure_button_background( button_background_diamond_thin );
		configure_button_background.setSize(buttons_size);
		if(fadein){
			configure_button_background.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
		}
		configure_button_container.addFloatingElement(configure_button_background, "configure_button_background", vec2(0, button_vert_offset / 2.0f));
		buttons_divider.append(configure_button_container);
	}

	if(steamworks_mod){
		//Detail button
		IMContainer details_button_container(buttons_size.x, buttons_size.y + button_vert_offset);
		IMMessage details_message("details", index);
		details_button_container.addLeftMouseClickBehavior(IMFixedMessageOnClick(details_message), "");
		AddControllerItem(details_button_container, details_message);
		details_button_container.sendMouseOverToChildren(true);
		IMDivider details_button_divider("details_button_divider", DOHorizontal);
		details_button_container.setElement(details_button_divider);
		details_button_container.setZOrdering(4);

        details_button_container.addLeftMouseClickBehavior( button_press_sound, "" );
        details_button_container.addMouseOverBehavior(button_hover_sound, "");
		
		IMImage small_steam_image( steam_icon );
		if(fadein){
			small_steam_image.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
		}
		small_steam_image.setColor(small_font.color);
		small_steam_image.addMouseOverBehavior( text_color_mouse_over, "" );
		small_steam_image.setSize(vec2(75.0f));
		details_button_divider.append(small_steam_image);
		small_steam_image.setZOrdering(6);
		
		details_button_divider.appendSpacer(25.0f);
		
		IMText details_button_text("Details", button_font_large);
		if(fadein){
			details_button_text.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
		}
		details_button_text.addMouseOverBehavior( text_color_mouse_over, "" );

		details_button_divider.append(details_button_text);
		details_button_text.setZOrdering(6);
		IMImage details_button_background( button_background_diamond_thin );
		if(fadein){
			details_button_background.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
		}
		details_button_background.setSize(buttons_size);
		
		details_button_container.addFloatingElement(details_button_background, "details_button_background", vec2(0, button_vert_offset / 2.0f));
		buttons_divider.append(details_button_container);
		
		//Unsubscribe button
		IMContainer subscribe_button_container(buttons_size.x, buttons_size.y + button_vert_offset);
		subscribe_button_container.sendMouseOverToChildren(true);
        subscribe_button_container.addLeftMouseClickBehavior( button_press_sound, "" );
        subscribe_button_container.addMouseOverBehavior(button_hover_sound, "");

		IMText subscribe_button_text("", button_font_large);
		if(fadein){
			subscribe_button_text.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
		}
		if(IsWorkshopSubscribed(mod_id)){
			subscribe_button_text.setText("Unsubscribe");
		}else{
			subscribe_button_text.setText("Subscribe");
		}
		subscribe_button_text.addMouseOverBehavior( text_color_mouse_over, "" );
		subscribe_button_container.setZOrdering(4);
		subscribe_button_text.setZOrdering(6);
		IMContainer text_holder(buttons_size.x, buttons_size.y + button_vert_offset);
		text_holder.setElement(subscribe_button_text);
		subscribe_button_container.setElement(text_holder);

		IMMessage subscribe_message("un/subscribe", ModGetID(mod_id));
		subscribe_message.addInt(waiting_anims.size());
		subscribe_message.addInt(index);

		subscribe_button_container.addLeftMouseClickBehavior(IMFixedMessageOnClick(subscribe_message), "");
		AddControllerItem(subscribe_button_container, subscribe_message);
		//Add a spinner for waiting to un/subscribe
		waiting_anims.insertLast(Pulse(text_holder, subscribe_button_container, mod_id));
		//waiting_anims.insertLast(Spinner(text_holder, subscribe_button_container, mod_id));

		IMImage subscribe_button_background( button_background_diamond_thin );
		if(fadein){
			subscribe_button_background.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
		}
		subscribe_button_background.setSize(buttons_size);
		subscribe_button_container.addFloatingElement(subscribe_button_background, "subscribe_button_background", vec2(0, button_vert_offset / 2.0f));
		buttons_divider.append(subscribe_button_container);
        
        {
            //Upvote and Downvote buttons
    		IMContainer container(buttons_size.x, buttons_size.y + button_vert_offset);
    		IMDivider divider("divider", DOHorizontal);
    		container.setElement(divider);
            vec4 active = vec4(1.0f,1.0f,1.0f,1.0f);
            vec4 not_active = vec4(0.3f,0.3f,0.3f,1.0f);
            UserVote vote = ModGetUserVote(mod_id);
            {
                //Upvote
                IMContainer vote_container(buttons_size.y + button_vert_offset, buttons_size.y + button_vert_offset);
                IMMessage message("upvote", index);
                message.addInt(waiting_anims.size());
                vote_container.sendMouseOverToChildren(true);
        		divider.append(vote_container);

                vote_container.addLeftMouseClickBehavior( button_press_sound, "" );
                vote_container.addMouseOverBehavior(button_hover_sound, "");
                
        		IMImage image( arrow_icon );
                AddControllerItem(image, message);
        		if(fadein){
        			image.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
        		}
                if(vote == k_VoteUp){
                    image.setColor(active);
                }else{
                    vote_container.addLeftMouseClickBehavior(IMFixedMessageOnClick(message), "");
                    waiting_anims.insertLast(ConstructUserVotePulse(vote_container, vote, mod_id));
                    image.setColor(not_active);
                }
        		image.addMouseOverBehavior( text_color_mouse_over, "" );
        		image.setSize(vec2(buttons_size.y));
                image.setRotation(-90);
                vote_container.setElement(image);
        		image.setZOrdering(6);
        		
        		IMImage background( button_back_square );
                vote_container.setZOrdering(3);
        		if(fadein){
        			background.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
        		}
        		background.setSize(vec2(buttons_size.y, buttons_size.y));
        		vote_container.addFloatingElement(background, "background", vec2(button_vert_offset / 2.0f));
            }
            
            {
                //Downvote
                IMContainer vote_container(buttons_size.y + button_vert_offset, buttons_size.y + button_vert_offset);
                IMMessage message("downvote", index);
                message.addInt(waiting_anims.size());
                vote_container.sendMouseOverToChildren(true);
        		divider.append(vote_container);

                vote_container.addLeftMouseClickBehavior( button_press_sound, "" );
                vote_container.addMouseOverBehavior(button_hover_sound, "");
                
        		IMImage image( arrow_icon );
                AddControllerItem(image, message);
        		if(fadein){
        			image.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
        		}
                
                if(vote == k_VoteDown){
                    image.setColor(active);
                }else{
                    vote_container.addLeftMouseClickBehavior(IMFixedMessageOnClick(message), "");
                    waiting_anims.insertLast(ConstructUserVotePulse(vote_container, vote, mod_id));
                    image.setColor(not_active);
                }
        		image.addMouseOverBehavior( text_color_mouse_over, "" );
                image.setSize(vec2(buttons_size.y));
                image.setRotation(90);
                vote_container.setElement(image);
        		image.setZOrdering(6);
        		
        		IMImage background( button_back_square );
                vote_container.setZOrdering(3);
        		if(fadein){
        			background.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
        		}
        		background.setSize(vec2(buttons_size.y, buttons_size.y));
        		vote_container.addFloatingElement(background, "background", vec2(button_vert_offset / 2.0f));
            }
            
    		buttons_divider.append(container);
        }
        
        {
            //Favourite button
            
            bool already_favorited = ModIsFavorite(mod_id);

            string message_str;
            if( already_favorited ) {
                message_str = "unfavorite";
            } else {
                message_str = "favorite";
            }
            
            IMContainer container(buttons_size.x, buttons_size.y + button_vert_offset);
    		IMMessage message(message_str, index);
            message.addInt(waiting_anims.size());
    		AddControllerItem(container, message);
    		container.sendMouseOverToChildren(true);

    		container.addLeftMouseClickBehavior(IMFixedMessageOnClick(message), "");
            waiting_anims.insertLast(ConstructFavoritePulse(container,already_favorited,mod_id));
    		IMDivider divider("divider", DOHorizontal);
    		container.setElement(divider);
    		container.setZOrdering(4);
            container.addLeftMouseClickBehavior( button_press_sound, "" );
            container.addMouseOverBehavior(button_hover_sound, "");
    		
    		IMImage image( settings_icon );
    		if(fadein){
    			image.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
    		}
    		image.setColor(small_font.color);
    		image.addMouseOverBehavior( text_color_mouse_over, "" );
    		image.setSize(vec2(75.0f));
    		divider.append(image);
    		image.setZOrdering(6);
    		
    		divider.appendSpacer(25.0f);
    		
    		IMText text("Favorite", button_font_large);
            
            if(already_favorited){
                text.setText("Favorited");
            }
            
    		if(fadein){
    			text.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
    		}
    		text.addMouseOverBehavior( text_color_mouse_over, "" );
    		divider.append(text);
    		text.setZOrdering(6);
    		IMImage background( button_background_diamond_thin );
    		if(fadein){
    			background.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
    		}
    		background.setSize(buttons_size);
    		
    		container.addFloatingElement(background, "background", vec2(0, button_vert_offset / 2.0f));
    		buttons_divider.append(container);
        }
	}	
	horiz_holder.append(buttons_container);
	
	inspector_divider.appendSpacer(50.0f);
	
	/*//Version number
	IMDivider version_divider("version_divider", DOVertical);
	inspector_divider.append(version_divider);
	version_divider.setZOrdering(4);
	IMText version_text("Version: " + ModGetVersion(mod_id), button_font_extra_small);
	if(fadein){
		version_text.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
	}
	version_divider.append(version_text);
	
	//Tags
	IMDivider tags_divider("tags_divider", DOVertical);
	inspector_divider.append(tags_divider);
	tags_divider.setZOrdering(4);
	IMText tags_text("Tags: " + ModGetTags(mod_id), button_font_extra_small);
	tags_text.setZOrdering(4);
	if(fadein){
		tags_text.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
	}
	tags_divider.append(tags_text);*/
	
	//Description
	float description_width = inspector_size.x;
	float description_height = 250;
	
	IMContainer description_container(description_width, description_height);
	description_container.setAlignment(CACenter, CACenter);
	IMDivider description_divider("description_divider", DOVertical);
	description_container.setElement(description_divider);
	
	//The description background
	float border_size = 10.0f;
	IMImage description_background1( white_background );
	description_background1.setColor(vec4(0.3,0.3,0.3,1.0f));
	if(fadein){
		description_background1.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
	}
	description_background1.setZOrdering(3);
	description_background1.setSize(description_container.getSize());
	description_container.addFloatingElement(description_background1, "description_background1", vec2(0.0f));
	
	IMImage description_background2( white_background );
	description_background2.setColor(vec4(0.0,0.0,0.0,1.0f));
	if(fadein){
		description_background2.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
	}
	description_background2.setZOrdering(4);
	description_background2.setSize(description_container.getSize() - border_size);
	description_container.addFloatingElement(description_background2, "description_background2", vec2(border_size / 2.0f));

	description_divider.setAlignment(CALeft, CACenter);
	description_divider.setZOrdering(5);
	description_divider.appendSpacer(15.0f);

    string description;
    FontSetup font;

    if(ModCanActivate(mod_id)) {
        description = ModGetDescription(mod_id);
        font = description_font;
    }else{
        string collision_mod_name = GetModNameWithID(ModGetID(mod_id));
        description = "Errors: " + ModGetValidityString(mod_id);
        if( collision_mod_name.length() > 0 ) {
            description = description + ". The active mod \"" + collision_mod_name + "\" has the same ID.";
        }
        font = description_error_font;
    }

    IMText@ current_line = @IMText("", font);

	if(fadein){
		current_line.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
	}
	description_divider.append(current_line);
	
	//Remove new line_chars
	description = join(description.split("\n"), "");

	array<string> description_divided = description.split(" ");
	int line_chars = 0;
	int max_char_per_line = 45;
	int num_lines = 1;
	int max_lines = 6;
	
	for(uint i = 0; i < description_divided.size(); i++){
		if(line_chars + description_divided[i].length() > max_char_per_line ){
			num_lines++;
			if(num_lines > max_lines){
				break;
			}
			line_chars = 0;
			IMText new_line("", font);
			if(fadein){
				new_line.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
			}
			@current_line = @new_line;
			description_divider.append(@current_line);
		}
		current_line.setText( current_line.getText() + description_divided[i] + " ");
		line_chars += description_divided[i].length();
	}
	inspector_divider.append(description_container);
	
	uint max_title_length = 35;
	//Title
	string mod_title = ModGetName(mod_id);
	if(mod_title.length() > max_title_length){
		mod_title.resize(max_title_length);
		mod_title += "...";
	}
	vec2 title_size(max(300, mod_title.length() * 24), 75);
	IMDivider title_divider("title_divider", DOVertical);
	IMContainer title_container(title_size.x, title_size.y);
	description_container.addFloatingElement(title_container, "title_container", vec2(0, -40.0f));
	title_container.setElement(title_divider);
	title_divider.setSizeX(850);
	title_divider.setZOrdering(6);
	
	//Title background
	IMImage title_background(brushstroke_background);
	title_background.setZOrdering(3);
	title_background.setClip(false);
	title_background.setSize(vec2(title_size.x, title_size.y));
	title_background.setAlpha(0.85f);
	title_container.addFloatingElement(title_background, "title_background", vec2(title_divider.getSizeX() / 2.0f - title_background.getSizeX() / 2.0f,0));
	
	IMText title_text(mod_title, button_font_small);
	if(fadein){
		title_background.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
		title_text.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
	}
	title_divider.append(title_text);
	
	uint max_author_length = 20;
	//Author
	string author_title = ModGetAuthor(mod_id);
	if(author_title.length() > max_author_length){
		author_title.resize(max_author_length);
		author_title += "...";
	}
	vec2 author_size(max(300, author_title.length() * 25), 75);
	IMDivider author_divider("author_divider", DOVertical);
	IMContainer author_container(author_size.x, author_size.y);
	description_container.addFloatingElement(author_container, "author_container", vec2(description_container.getSizeX() - 850, -40.0f));
	author_container.setElement(author_divider);
	author_divider.setSizeX(850);
	author_divider.setZOrdering(6);
	
	//Author background
	IMImage author_background(brushstroke_background);
	author_background.setZOrdering(3);
	author_background.setClip(false);
	author_background.setSize(vec2(author_size.x, author_size.y));
	author_background.setAlpha(0.85f);
	author_container.addFloatingElement(author_background, "author_background", vec2(author_divider.getSizeX() / 2.0f - author_background.getSizeX() / 2.0f,0));
	
	IMText author_text(author_title, button_font_small);
	if(fadein){
		author_background.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
		author_text.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
	}
	author_divider.append(author_text);
}

IMContainer@ AddModItem(IMDivider@ parent, ModID mod_id, uint index, bool fadein){
	float mod_item_width = 650;
	float mod_item_height = 75;
	float main_background_offset = 10.0f;
	float vertical_offset = 10.0f;
	int fadein_time = 250;
	float left_trailing_space = 20.0f;
	vec4 background_color = vec4(0.15f,0.15f,0.15f,1.0f);
	vec4 background_color_inspected = vec4(5.0,5.0,5.0,1.0);
	string validity = ModGetValidityString(mod_id);
	string mod_path = ModGetPath(mod_id);
	uint max_title_length = 27;
	
	IMContainer mod_item_container("mod_item_container" + ModGetID(mod_id), mod_item_width, mod_item_height + vertical_offset);
	mod_item_container.setAlignment(CALeft, CACenter);
	IMDivider mod_item_divider("mod_item_divider", DOHorizontal);
	mod_item_divider.appendSpacer(left_trailing_space);
	mod_item_container.setElement(mod_item_divider);
	
	//The main background
	IMImage main_background( button_background_flag_extended );
	if(fadein){
		main_background.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
	}
	if(int(index) == inspected){
		main_background.setColor(background_color_inspected);
	}
	main_background.setZOrdering(0);
	main_background.setClip(false);
	main_background.addMouseOverBehavior(IMMouseOverScale( move_time, 12.0f, inQuartTween ), "");
	mod_item_container.addFloatingElement(main_background, "main_background", vec2(0, vertical_offset / 2.0f));
	main_background.setSize(vec2(mod_item_width, mod_item_height));
	
	//The activate button
	float diamond_size = 200.0f;
	vec4 diamond_color(0.1f,0.1f,0.1f,1.0f);
	vec4 title_background_color(0.2f,0.2f,0.2f,1.0f);
	float checkbox_size = 50.0f;
	float extra = 25.0f;
	vec2 description_size(950, 250);
	float checkmark_size = 50.0f;
	vec4 error_color(1.0f,0.0f,0.0f,1.0f);
	vec4 ok_color(0.0f,1.0f,0.0f,1.0f);
	vec4 warn_color(1.0f,1.0f,0.0f,1.0f);

	IMContainer checkbox_container(checkbox_size, checkbox_size);
	checkbox_container.setAlignment(CACenter, CACenter);
	mod_item_divider.append(checkbox_container);

	IMImage checkbox_image(checkbox);
	if(fadein){
		checkbox_image.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
	}
	checkbox_image.addMouseOverBehavior(mouseover_scale_button, "");
	checkbox_image.setZOrdering(2);
	checkbox_image.setClip(false);
	checkbox_image.setSize(vec2(checkbox_size));
	checkbox_container.addFloatingElement(checkbox_image, "checkbox_image", vec2(0));
	IMMessage on_click("", ModGetID(mod_id));
	//Mod has some kind of error.
	if(!ModCanActivate(mod_id)){
		checkbox_image.setColor(error_color);
		IMContainer exclamation_mark_container(checkmark_size, checkmark_size);
		IMText exclamation_mark("!", button_font_small);
		if(fadein){
			exclamation_mark.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
		}
		exclamation_mark.setZOrdering(6);
		exclamation_mark_container.setElement(exclamation_mark);
		exclamation_mark.setColor(error_color);
		checkbox_container.addFloatingElement(exclamation_mark_container, "exclamation_mark", vec2(0));
		
		IMMessage add_tooltip("add_tooltip", validity);
		add_tooltip.addInt(gui_elements.size());
		
		IMMessage remove_tooltip("remove_tooltip");
		remove_tooltip.addInt(gui_elements.size());
		
		IMMessage do_nothing("");
		checkbox_container.addMouseOverBehavior(IMFixedMessageOnMouseOver(add_tooltip, do_nothing, remove_tooltip), "");
	}
	//Mod is valid
	else{
        bool version_supported = ModGetSupportsCurrentVersion(mod_id);
		if(ModIsActive(mod_id)){
			IMImage checkmark_image(checkmark);
			if(fadein){
				checkmark_image.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
			}
            if (version_supported) {
			    checkmark_image.setColor(ok_color);
            } else {
			    checkmark_image.setColor(warn_color);
            }
			checkmark_image.scaleToSizeX(checkmark_size);
			checkmark_image.setClip(false);
			checkmark_image.setZOrdering(6);
			checkbox_container.addFloatingElement(checkmark_image, "checkmark", vec2(0));
		}
		on_click.name = "toggle_mod";
		on_click.addInt(index);
		checkbox_container.addLeftMouseClickBehavior(IMFixedMessageOnClick(on_click), "");

        if (!version_supported) {
            IMMessage add_tooltip("add_tooltip", validity);
            add_tooltip.addInt(gui_elements.size());

            IMMessage remove_tooltip("remove_tooltip");
            remove_tooltip.addInt(gui_elements.size());

            IMMessage do_nothing("");
            checkbox_container.addMouseOverBehavior(IMFixedMessageOnMouseOver(add_tooltip, do_nothing, remove_tooltip), "");
        }
	}
	//Title
	IMContainer title_container(mod_item_width - (checkmark_size + left_trailing_space * 2.0f), mod_item_height);
	//Add controller items
	IMMessage inspect("inspect", index);
	ControllerItem new_controller_item();
	if(int(index) == search.current_index && index != 0){
		IMMessage press_up("shift_menu", -1);
		@new_controller_item.message_up = press_up;
		@new_controller_item.element = mod_item_container;
	}else if(int(index) == (search.current_index + (show_nr_mods - 1)) && index != search.collection.size() - 1){
		IMMessage press_down("shift_menu", 1);
		@new_controller_item.message_down = press_down;
		@new_controller_item.element = mod_item_container;
	}else{
		@new_controller_item.element = mod_item_container;
	}
	@new_controller_item.message = on_click;
	@new_controller_item.message_on_select = inspect;
	new_controller_item.execute_on_select = true;
	//new_controller_item.skip_show_border = true;
	AddControllerItem(@new_controller_item);
	mod_item_divider.appendSpacer(left_trailing_space);
	
	/*title_container.showBorder();*/
	title_container.sendMouseOverToChildren(true);
	title_container.setAlignment(CALeft, CACenter);
	string title = ModGetName(mod_id);
	if(title.length() > max_title_length){
		title = title.substr(0, max_title_length - 3) + "...";
	}
	IMText title_text(title, button_font_small);
	title_text.addMouseOverBehavior( text_color_mouse_over, "" );
	if(fadein){
		title_text.addUpdateBehavior(IMFadeIn( fadein_time, inSineTween ), "");
	}
	title_text.setZOrdering(4);
	title_container.setElement(title_text);
    
    if (ModGetSupportsOnline(mod_id)) {
        // Add floating background
        IMImage online_marker("Textures/ui/menus/main/icon-online.png");
        float image_size = title_container.getSizeY() * 0.8f;

        online_marker.setSize(vec2(image_size, image_size));
        online_marker.setZOrdering(2);
        online_marker.setColor(vec4(1,1,1,0.45f));
        title_container.addFloatingElement(online_marker, "online_marker", vec2(title_container.getSizeX() - image_size * 2, title_container.getSizeY() * 0.2f / 2.0f), 0);
    }

	mod_item_divider.append(title_container);
	title_container.addLeftMouseClickBehavior(IMFixedMessageOnClick(inspect), "");
	parent.append(mod_item_container);
	if(last_shift_direction != 0){
		mod_item_container.addUpdateBehavior(IMMoveIn ( 250.0f, vec2(0, mod_item_height * last_shift_direction), outExpoTween ), "");
	}
	gui_elements.insertLast(ModItem(checkbox_container, validity));
	return @mod_item_container;
}

void AddModsHeader(){
	IMContainer header_container(2560, 200);
	header_container.setAlignment(CALeft, CACenter);
	IMDivider header_divider( "header_div", DOHorizontal );
	header_container.setElement(header_divider);
	AddTitleHeader("Mods", header_divider);
	AddAdvanced(header_divider);
	imGUI.getHeader().setElement(header_container);
}

void AddGetMoreMods(IMDivider@ parent){
	//Get More Mods
	float get_more_width = 550;
	float get_more_height = 100;
	IMContainer get_more_container(get_more_width, get_more_height);
	get_more_container.sendMouseOverToChildren(true);
	get_more_container.addLeftMouseClickBehavior( IMFixedMessageOnClick("open_workshop"), "");
    get_more_container.addLeftMouseClickBehavior( button_press_sound, "" );
    get_more_container.addMouseOverBehavior(button_hover_sound, "");

	AddControllerItem(get_more_container, IMMessage("open_workshop"));
	get_more_container.setZOrdering(0);
	IMDivider get_more_divider("get_more_divider", DOHorizontal);
	get_more_container.setElement(get_more_divider);
	IMImage steam_icon_image( steam_icon );
	steam_icon_image.setColor( button_font_small.color );
	steam_icon_image.addMouseOverBehavior( text_color_mouse_over, "" );
	steam_icon_image.scaleToSizeX(get_more_height);
	get_more_divider.append(steam_icon_image);
	get_more_divider.appendSpacer(25);
	IMText get_more_text( "Get More Mods", button_font_small );
	get_more_text.addMouseOverBehavior( text_color_mouse_over, "" );
	IMImage get_more_background( button_background_diamond_thin );
	get_more_background.setSize(vec2(get_more_width, get_more_height));
	get_more_divider.append(get_more_text);
	get_more_container.addFloatingElement(get_more_background, "background", vec2(0,0));
	parent.append(get_more_container);
}

void AddAdvanced(IMDivider@ parent){
	bool show_advanced = false;
	if(show_advanced){
		//Advanced
		float advanced_width = 300;
		float advanced_height = 100;
		IMContainer advanced_container(advanced_width, advanced_height);
		IMText advanced_text( "Advanced", button_font_small );
		advanced_text.setZOrdering(3);
		IMImage advanced_background( white_background );
		advanced_background.setSize(vec2(advanced_width, advanced_height));
		advanced_background.setColor(button_background_color);
		advanced_container.setElement(advanced_text);
		advanced_container.addFloatingElement(advanced_background, "background", vec2(0,0));
		parent.append(advanced_container);
		parent.appendSpacer(50);
	}
}

void AddDeactivateAllMods(IMDivider@ parent) {
	float deactivate_mods_width = 500;
	float deactivate_mods_height = 75;
	float vertical_offset = 10.0f;
	IMContainer deactivate_mods_container(deactivate_mods_width, deactivate_mods_height + vertical_offset);
	deactivate_mods_container.sendMouseOverToChildren(true);
	deactivate_mods_container.addLeftMouseClickBehavior( IMFixedMessageOnClick("deactivate_all_mods"), "");
    deactivate_mods_container.addLeftMouseClickBehavior( button_press_sound, "" );
    deactivate_mods_container.addMouseOverBehavior(button_hover_sound, "");
	AddControllerItem(deactivate_mods_container, IMMessage("deactivate_all_mods"));
	deactivate_mods_container.setZOrdering(0);
	IMDivider deactivate_mods_divider("deactivate_mods_divider", DOHorizontal);
	deactivate_mods_container.setElement(deactivate_mods_divider);
	deactivate_mods_divider.appendSpacer(25);
	IMText deactivate_mods_text( "Deactivate All Mods", button_font_small );
	deactivate_mods_text.addMouseOverBehavior( text_color_mouse_over, "" );
	IMImage deactivate_mods_background( button_background_diamond_thin );
	deactivate_mods_background.setSize(vec2(deactivate_mods_width, deactivate_mods_height));
	deactivate_mods_background.setClip(false);
	deactivate_mods_background.addMouseOverBehavior(IMMouseOverScale( move_time, 12.0f, inQuartTween ), "");
	deactivate_mods_divider.append(deactivate_mods_text);
	deactivate_mods_container.addFloatingElement(deactivate_mods_background, "background", vec2(0, vertical_offset / 2.0));
	parent.append(deactivate_mods_container);
}

void SetDarkenedBackground() {
    setBackGround(0.5f);
}

void Dispose() {
	imGUI.clear();
    SaveModConfig();
}

bool CanGoBack() {
    return true;
}

void Update() {
	workshopwatcher.Update();
	UpdateWaitingAnims();
	UpdateKeyboardMouse();
    // process any messages produced from the update
    while( imGUI.getMessageQueueSize() > 0 ) {
        IMMessage@ message = imGUI.getNextMessage();
		/*Log(info, "message " + message.name);*/
        if( message.name == "Back" )
        {
            this_ui.SendCallback( "back" );
        }
        else if( message.name == "load_level" )
        {
            this_ui.SendCallback( "Data/Levels/" + message.getString(0) );
        }
		else if( message.name == "shift_menu" ){
			if(!CanShift(message.getInt(0)))return;
			search.current_index = search.current_index + (shift_amount * message.getInt(0));
			imGUI.receiveMessage( IMMessage("refresh_menu_by_id", "also_execute_on_select") );
		}
		else if( message.name == "toggle_mod" ){
			int index = message.getInt(0);

            Log(info, "I" + index );
            if(index < int(current_mods.size())) {
                ModID current_mod = current_mods[index];
                if(ModCanActivate(current_mod)){
                    if(ModIsActive(current_mod)){
                        ModActivation(current_mod, false);
                    }else{
                        ModActivation(current_mod, true);
                    }
                }else{
                    imGUI.reportError("Could not activate mod " + message.getString(0));
                }
                SaveConfig();
                imGUI.receiveMessage( IMMessage("refresh_menu_by_id") );
            } else {
                Log(error, "Invalid index" + index );
            }
		}
		else if( message.name == "refresh_menu_by_name" ){
			string current_controller_item_name = GetCurrentControllerItemName();
			BuildUI();
			SetCurrentControllerItem(current_controller_item_name);
		}
		else if( message.name == "refresh_menu_by_id" ){
			//int index = GetCurrentControllerItemIndex();
			BuildUI();
			//SetCurrentControllerItem(index);
			//if(message.getString(0) == "also_execute_on_select"){
			//	ExecuteOnSelect();
			//}
		}
		else if( message.name == "inspect" ){
			/*if(inspected == message.getInt(0)){
				SetItemActive(current_item);
				return;
			}*/
			inspected = message.getInt(0);
			inspect_fadein = true;
			imGUI.receiveMessage( IMMessage("refresh_menu_by_id") );
		}
		else if( message.name == "un/subscribe" ){
			int index = message.getInt(1);
			ModID current_mod = current_mods[index];
			string validity = ModGetValidityString(current_mod);
			if(IsWorkshopSubscribed(current_mod)){
				RequestWorkshopUnSubscribe(current_mod);
			}else{
				RequestWorkshopSubscribe(current_mod);
			}
			waiting_anims[message.getInt(0)].Activate();
		}
		else if( message.name == "details" ){
			//Print("Open steam workshop page for " + message.getString(0) + "\n");
			int index = message.getInt(0);
			ModID current_mod = current_mods[index];
			OpenModWorkshopPage(current_mod);
		}
		else if( message.name == "configure" ){
			//Print("Open configure page for " + message.getString(0) + "\n");
			this_ui.SendCallback("configure.as");
		}
		else if( message.name == "open_workshop" ){
			//Print("Open steam workshop for Overgrowth\n");
			OpenWorkshop();
		} 
        else if( message.name == "deactivate_all_mods" ) {
            DeactivateAllMods(); 
        }
		else if( message.name == "activate_search" ){
			if(!search.active) {
				search.Activate();
			} else {
				//ResetModsList();
				//SetCurrentControllerItem(0);
				search.active = false;
				search.pressed_return = true;
				imGUI.receiveMessage( IMMessage("refresh_menu_by_name") );
			}
		}
		else if( message.name == "clear_search_results" ){
			ResetModsList();
			search.ResetSearch();
			//SetCurrentControllerItem(0);
			imGUI.receiveMessage( IMMessage("refresh_menu_by_id") );
		}
		else if( message.name == "add_tooltip" ){
			gui_elements[message.getInt(0)].AddTooltip();
		}
		else if( message.name == "remove_tooltip" ){
			gui_elements[message.getInt(0)].RemoveTooltip();
		}
		else if( message.name == "slider_deactivate" ){
			/*Print("deactivate slider check\n");*/
			if(!checking_slider_movement){
				active_slider = -1;
			}
		}
		else if( message.name == "slider_activate" ){
			/*Print("activate slider check\n");*/
			old_mouse_pos = imGUI.guistate.mousePosition;
			active_slider = message.getInt(0);
		}
        else if( message.name == "upvote" ){
            int index = message.getInt(0);
			ModID current_mod = current_mods[index];
            waiting_anims[message.getInt(1)].Activate();
            RequestModSetUserVote(current_mod, true);
        }
        else if( message.name == "downvote" ){
            int index = message.getInt(0);
			ModID current_mod = current_mods[index];
            waiting_anims[message.getInt(1)].Activate();
            RequestModSetUserVote(current_mod, false);
        }
        else if( message.name == "favorite" ) {
            int index = message.getInt(0);
			ModID current_mod = current_mods[index];
            waiting_anims[message.getInt(1)].Activate();
            RequestModSetFavorite(current_mod, true);
        }
        else if( message.name == "unfavorite" ) { 
            int index = message.getInt(0);
			ModID current_mod = current_mods[index];
            waiting_anims[message.getInt(1)].Activate();
            RequestModSetFavorite(current_mod, false);
        }
    }
	// Do the general GUI updating
	search.Update();
	imGUI.update();
	UpdateController();
	UpdateMovingSlider();
}

ModID getModID(string id){
	array<ModID> all_mods = GetModSids();
	ModID current_mod;
	for(uint i = 0; i < all_mods.size(); i++){
		if(ModGetID(all_mods[i]) == id){
			current_mod = all_mods[i];
			return current_mod;
		}
	}
	return current_mod;
}

void UpdateWaitingAnims(){
	for(uint i = 0; i < waiting_anims.size(); i++){
		waiting_anims[i].Update();
		if(waiting_anims[i].delete_this){
			waiting_anims.removeAt(i);
		}
	}
}

class WorkshopWatcher{
    IMDivider@ parent;
    IMDivider@ download_count_divider;
    IMDivider@ download_pending_divider;
    IMDivider@ not_installed_divider;
    IMDivider@ needs_update_divider;
    IMDivider@ info1_div;
    IMDivider@ info2_div;

    uint count_downloads = 0;
    uint pending_downloads = 0;
    uint not_installed_downloads = 0;
    uint needs_update_downloads = 0;

    uint update_reset_timer = 0;

    float workshop_total_download_progress = 0.0f;
    float workshop_prev_progress = 0.0f;
    uint download_counter = 0;

    void SetParent(IMDivider@ parent_){
        @parent = @parent_;

        @download_count_divider = IMDivider("download_count");
        @download_pending_divider = IMDivider("download_pending");
        @not_installed_divider = IMDivider("download_installing");
        @needs_update_divider = IMDivider("needs_update");
        @info1_div = IMDivider("info1");
        @info2_div = IMDivider("info2");
        
        //parent.append(download_pending_divider);
        //parent.append(not_installed_divider);
        parent.append(needs_update_divider);
        //parent.append(download_count_divider);
        parent.append(info1_div);
        parent.append(info2_div);
    }

    void Update() {
        if( update_reset_timer == 0 ) {
            count_downloads = WorkshopSubscribedNotInstalledCount();
            pending_downloads = WorkshopDownloadingCount();
            not_installed_downloads = WorkshopDownloadPendingCount();
            needs_update_downloads = WorkshopNeedsUpdateCount();

            if( count_downloads + pending_downloads + not_installed_downloads + needs_update_downloads > 0 ) {
                update_reset_timer = 1000;
            } else {
                update_reset_timer = 10;
                download_counter = 0;
            }
        } else {
            update_reset_timer--;
        }

        /*
        //Show nr of current mods that are pending
        download_pending_divider.clear();
        if(pending_downloads > 0){
            IMText label("Pending download: " + pending_downloads, button_font_small);
            download_pending_divider.append(label);
        }

        //Show nr of current mods that are not yet installed
        not_installed_divider.clear();
        if(not_installed_downloads > 0){
            IMText label("Not yet installed: " + not_installed_downloads , button_font_small);
            not_installed_divider.append(label);
        }
        */

        needs_update_divider.clear();
        if(needs_update_downloads > 0){
            IMText label("Mods needing update: " + needs_update_downloads , button_font_small);
            needs_update_divider.append(label);
        }

        /*
        download_count_divider.clear();
        if(count_downloads > 0){
            IMText label("Downloading: " + count_downloads + "...", button_font_small);
            download_count_divider.append(label);
        }
        */

        info1_div.clear();
        info2_div.clear();
        if( count_downloads + pending_downloads + not_installed_downloads + needs_update_downloads > 0 ) {
            workshop_total_download_progress = WorkshopTotalDownloadProgress();

            IMText label("Progress: " + int(100 * workshop_total_download_progress) + "%", button_font_small);
            info1_div.append(label);

            if(workshop_prev_progress == workshop_total_download_progress) {
                download_counter++;
            } else {
                download_counter = 0;
            }

            workshop_prev_progress = workshop_total_download_progress;

            if( download_counter > 3000 ) {
                IMText label2("Slow, Downloads Paused?", button_font_small);
                info2_div.append(label2);
            }
        } else {
            IMText label2("Up To Date", button_font_small);
            info2_div.append(label2);
        }
    }
}


void Resize() {
    imGUI.doScreenResize(); // This must be called first
	SetDarkenedBackground();
}

void ScriptReloaded() {
	//Print("Script reloaded!\n");
    // Clear the old GUI
    /*imGUI.clear();*/
    // Rebuild it
    /*Initialize();*/
}

void ModActivationReload(){
	Log(info, "ModActivationReload!");

    imGUI.receiveMessage( IMMessage("refresh_menu_by_id") );
}

void DrawGUI() {
    imGUI.render();
}

void Draw() {
}

void Init(string str) {
}

class ModSearch : Search{
	array<ModID>@ collection;
	ModSearch(){
		
	}
	void SetCollection(array<ModID>@ _collection){
		@collection = @_collection;
	}
	void GetSearchResults(string query){
		inspected = 0;
		array<ModID> results;
		array<ModID>@ all_mods = GetModSids();
		//If the searchfield is empty just return all mods.
        if(query == ""){
        	results = all_mods;
        }else{
        	for(uint i = 0; i < all_mods.size(); i++){
        		if(ToLowerCase(ModGetName(all_mods[i])).findFirst(query) != -1){
        			results.insertLast(all_mods[i]);
        			continue;
        		}else if(ToLowerCase(ModGetDescription(all_mods[i])).findFirst(query) != -1){
        			results.insertLast(all_mods[i]);
        			continue;
        		}
        	}
        }
		collection = SortAlphabetically(FilterCore(results));
	}
}
