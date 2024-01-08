
array<array<IMText@>> dialogue_cache;
IMContainer@ dialogue_ui_container;
IMContainer@ dialogue_holder_container;
IMContainer@ fixed_size_container;
IMDivider @dialogue_holder;
IMDivider @dialogue_line;
int line_counter = 0;
vec2 dialogue_holder_size = vec2(1700, 300);
bool dialogue_move_in = false;
float dialogue_move_in_timer = 0.0;
float dialogue_move_in_duration = 0.15;
int dialogue_location;
int dialogue_progress = 0;
array<DialogueScriptEntry@> dialogue_script;
special_fonts special_font = none;
IMText @dialogue_text;
int counter = 0;
float last_continue_sound_time;
int maximum_amount_of_lines;
int line_number = -1;
ContainerAlignment dialogue_x_alignment;
ContainerAlignment dialogue_y_alignment;
array<DialogueScriptEntry@> last_word;
bool allow_dialogue_move_in = true;
array<IMContainer@> choice_ui_elements;
array<string> choices;
int selected_choice = 0;
bool showing_choice = false;

void DialogueAddSay(string actor_name, string text){

	dialogue_cache = {array<IMText@>()};
	counter = 0;
	dialogue_progress = 0;
	special_font = none;
	line_number = -1;
	set_dialogue_displacement = false;
	dialogue_displacement_target = 0.0f;
	smooth_dialogue_displacement = true;

	//Find the actor settings for so that the UI can be build.
	if(current_actor_settings.name != actor_name){
		bool settings_found = false;

		for(uint i = 0; i < actor_settings.size(); i++){
			if(actor_settings[i].name == actor_name){
				settings_found = true;
				@current_actor_settings = actor_settings[i];
				break;
			}
		}

		if(!settings_found){
			ActorSettings new_settings();
			new_settings.name = actor_name;
			actor_settings.insertLast(@new_settings);
			@current_actor_settings = new_settings;
		}
	}

	dialogue_script = InterpDialogueScript(text);

	if(!show_dialogue){
		show_dialogue = true;
		BuildDialogueUI();
	}

	AddNewText("");
	maximum_amount_of_lines = -1;
	last_word.resize(0);

	for(uint i = 0; i < dialogue_script.size(); i++){
		DialogueScriptEntry@ entry = dialogue_script[i];
		/* Log(warning, entry.character + " " + entry.script_entry_type); */

		entry.line = counter;
		switch(entry.script_entry_type){
			case character_entry:
				last_word.insertLast(entry);
				if(entry.character == " "){
					AddNewText(" ");
					@entry.text = dialogue_text;
					last_word.resize(0);
					CheckDialogueWidth();
					AddNewText("");
				}else{
					dialogue_text.setText(dialogue_text.getText() + entry.character);
					@entry.text = dialogue_text;
					CheckDialogueWidth();
				}
				break;
			case new_line_entry:
				//If a new line is found then add a new divider.
				line_counter += 1;
				counter += 1;
				last_word.resize(0);
				@dialogue_line = IMDivider("dialogue_line" + counter, DOHorizontal);
				//Set the y size of the divider so that an empty line still takes up the same amount of height.
				dialogue_line.setSizeY(dialogue_font.size);
				dialogue_line.setAlignment(CALeft, CATop);
				dialogue_holder.append(dialogue_line);
				dialogue_line.setZOrdering(2);

				dialogue_cache.insertLast(array<IMText@>());
				AddNewText("");
				break;
			case start_red_text_entry:
				special_font = red;
				AddNewText("");
				continue;
			case start_green_text_entry:
				special_font = green;
				AddNewText("");
				continue;
			case start_blue_text_entry:
				special_font = blue;
				AddNewText("");
				continue;
			case end_coloured_text_entry:
				special_font = none;
				AddNewText("");
				continue;
			default :
				continue;
		}
	}

	//Hide the whole dialogue to start with by setting all the texts to nothing.
	for(uint i = 0; i < dialogue_cache.size(); i++){
		for(uint j = 0; j < dialogue_cache[i].size(); j++){
			dialogue_cache[i][j].setText("");
		}
	}
}

void AddNewText(string text){
	switch(special_font){
		case red:
			@dialogue_text = IMText(text, red_dialogue_font);
			break;
		case green:
			@dialogue_text = IMText(text, green_dialogue_font);
			break;
		case blue:
			@dialogue_text = IMText(text, blue_dialogue_font);
			break;
		default:
			@dialogue_text = IMText(text, dialogue_font);
			break;
	}

	dialogue_cache[counter].insertLast(dialogue_text);
	dialogue_line.append(dialogue_text);
}

void CheckDialogueWidth(){
	imGUI.update();

	bool add_previous_text_to_new_line = dialogue_holder.getSizeX() > dialogue_holder_size.x;
	if(add_previous_text_to_new_line){
		dialogue_cache[counter].removeLast();
		dialogue_cache.insertLast(array<IMText@>());
		counter += 1;
		dialogue_cache[counter].insertLast(dialogue_text);

		//Set the line to the next one.
		for(uint j = 0; j < last_word.size(); j++){
			last_word[j].line = counter;
		}
		last_word.resize(0);

		dialogue_holder.clear();
		dialogue_holder.setSize(dialogue_holder_size);
		/* dialogue_holder.showBorder(); */

		//Remake the dialogue using the cache.
		for(uint j = 0; j < dialogue_cache.size(); j++){
			@dialogue_line = IMDivider("dialogue_line" + counter, DOHorizontal);
			//Set the y size of the divider so that an empty line still takes up the same amount of height.
			dialogue_line.setSizeY(dialogue_font.size);
			dialogue_line.setAlignment(CALeft, CATop);
			dialogue_holder.append(dialogue_line);
			dialogue_line.setZOrdering(2);
			for(uint k = 0; k < dialogue_cache[j].size(); k++){
				dialogue_line.append(dialogue_cache[j][k]);
			}
		}
	}

	imGUI.update();

	if(maximum_amount_of_lines == -1 && floor(dialogue_holder.getSizeY()) > dialogue_holder_size.y){
		Log(warning, "maximum_amount_of_lines " + counter);
		dialogue_y_alignment = CATop;
		dialogue_holder_container.setAlignment(dialogue_x_alignment, dialogue_y_alignment);
		maximum_amount_of_lines = counter;

		Log(warning, "Override y size");
		//Set the y size so that the text is correctly centered.
		fixed_size_container.setSizeY(dialogue_font.size * maximum_amount_of_lines);
	}
}

void BuildDialogueUI(){
	ui_created = false;
	line_counter = 0;
	DisposeTextAtlases();
	dialogue_container.clear();
	@dialogue_ui_container = IMContainer(2560, 500);
	if(dialogue_location == dialogue_bottom){
		dialogue_container.setAlignment(CACenter, CABottom);
	}else if(dialogue_location == dialogue_center){
		dialogue_container.setAlignment(CACenter, CACenter);
	}else{
		dialogue_container.setAlignment(CACenter, CATop);
	}
	dialogue_container.setElement(dialogue_ui_container);
	dialogue_container.setSize(vec2(2560, 1440));

	switch(dialogue_layout){
		case default_layout:
			DefaultUI(dialogue_ui_container);
			break;
		case simple_layout:
			SimpleUI(dialogue_ui_container);
			break;
		case breath_of_the_wild_layout:
			BreathOfTheWildUI(dialogue_ui_container);
			break;
		case chrono_trigger_layout:
			ChronoTriggerUI(dialogue_ui_container);
			break;
		case fallout_3_layout:
			Fallout3UI(dialogue_ui_container);
			break;
		case luigis_mansion_layout:
			LuigisMansionUI(dialogue_ui_container);
			break;
		case mafia_layout:
			MafiaUI(dialogue_ui_container);
			break;
		default :
			break;
	}

	CreateNameTag(dialogue_ui_container);
	CreateBackground(dialogue_ui_container);
	CreateChoiceUI();

	ui_created = true;
}

void CreateChoiceUI(){
	if(!showing_choice){
		return;
	}

	choice_ui_elements.resize(0);

	for(uint i = 0; i < choices.size(); i++){
		line_counter += 1;
		IMDivider line_divider("dialogue_line_holder" + line_counter, DOHorizontal);
		line_divider.setZOrdering(2);

		IMContainer choice_container(dialogue_holder_size.x, dialogue_font.size * 1.3f);
		IMMessage on_click("drika_dialogue_choice_pick", i);
		IMMessage on_hover_enter("drika_dialogue_choice_select", i);
	    IMMessage nil_message("");
		choice_container.addLeftMouseClickBehavior(IMFixedMessageOnClick(on_click), "");
		choice_container.addMouseOverBehavior(IMFixedMessageOnMouseOver(on_hover_enter, nil_message, nil_message), "");
		dialogue_holder.append(choice_container);
		choice_container.setZOrdering(2);
		choice_container.setBorderColor(dialogue_font.color);
		choice_container.setBorderSize(5.0f);

		IMText choice_text(choices[i], dialogue_font);
		choice_container.setElement(line_divider);
		choice_container.setAlignment(dialogue_x_alignment, CACenter);

		line_divider.appendSpacer(20.0f);
		line_divider.append(choice_text);
		line_divider.appendSpacer(20.0f);
		choice_ui_elements.insertLast(@choice_container);
	}

	imGUI.update();
	SelectChoice(selected_choice);
}

void DefaultUI(IMContainer@ parent){
	parent.setSizeY(500.0);
	dialogue_holder_size = vec2(1740, 300);
	vec2 dialogue_holder_offset = vec2(100.0, 130.0);
	dialogue_x_alignment = CALeft;
	dialogue_y_alignment = CATop;

	/* IMContainer guide(dialogue_holder_size.x, dialogue_holder_size.y);
	guide.showBorder();
	parent.addFloatingElement(guide, "guide", dialogue_holder_offset, -1); */

	@fixed_size_container = IMContainer(dialogue_holder_size.x, dialogue_holder_size.y);

	SizePolicy x_policy(dialogue_holder_size.x);
	SizePolicy y_policy(dialogue_holder_size.y);
	x_policy.inheritMax();
	y_policy.inheritMax();
	@dialogue_holder_container = IMContainer(x_policy, y_policy);
	dialogue_holder_container.setAlignment(dialogue_x_alignment, dialogue_y_alignment);

	//By default the center will vertically center, but set to top when text scrolling is happening.
	/* dialogue_holder_container.setAlignment(dialogue_x_alignment, dialogue_y_alignment); */

	fixed_size_container.setElement(dialogue_holder_container);
	parent.addFloatingElement(fixed_size_container, "fixed_size_container", dialogue_holder_offset, -1);

	@dialogue_holder = IMDivider("dialogue_holder", DOVertical);
	dialogue_holder.setSize(dialogue_holder_size);
	dialogue_holder_container.setElement(dialogue_holder);
	dialogue_holder.setAlignment(dialogue_x_alignment, dialogue_y_alignment);

	@dialogue_line = IMDivider("dialogue_line" + line_counter, DOHorizontal);
	dialogue_line.setAlignment(CALeft, CATop);
	dialogue_holder.append(dialogue_line);
	dialogue_line.setZOrdering(2);

	bool use_keyboard = (max(last_mouse_event_time, last_keyboard_event_time) > last_controller_event_time);

	IMContainer controls_container(2425.0, 420.0);
	controls_container.setAlignment(CABottom, CARight);
	IMDivider controls_divider("controls_divider", DOVertical);
	controls_divider.setAlignment(CATop, CALeft);
	controls_divider.setZOrdering(1);
	controls_container.setElement(controls_divider);
	@lmb_continue = IMText(GetStringDescriptionForBinding(use_keyboard?"key":"gamepad_0", "attack") + " to continue", controls_font);
	@rtn_skip = IMText(GetStringDescriptionForBinding(use_keyboard?"key":"gamepad_0", "skip_dialogue")+" to skip", controls_font);
	controls_divider.append(lmb_continue);
	controls_divider.append(rtn_skip);

	lmb_continue.setVisible(false);
	rtn_skip.setVisible(false);
	parent.addFloatingElement(controls_container, "controls_container", vec2(0.0, 0.0), -1);
}

void SimpleUI(IMContainer@ parent){
	parent.setSizeY(500.0);
	dialogue_holder_size = vec2(1400, 350);
	dialogue_x_alignment = CACenter;
	dialogue_y_alignment = CACenter;

	/* vec2 dialogue_holder_offset = vec2((2560 - dialogue_holder_size.x) / 2.0f, 100.0);
	IMContainer guide(dialogue_holder_size.x, dialogue_holder_size.y);
	guide.showBorder();
	parent.addFloatingElement(guide, "guide", dialogue_holder_offset, -1); */

	@fixed_size_container = IMContainer(dialogue_holder_size.x, dialogue_holder_size.y);

	SizePolicy x_policy(dialogue_holder_size.x);
	SizePolicy y_policy(dialogue_holder_size.y);
	x_policy.inheritMax();
	y_policy.inheritMax();
	@dialogue_holder_container = IMContainer(x_policy, y_policy);
	//By default the center will vertically center, but set to top when text scrolling is happening.
	dialogue_holder_container.setAlignment(dialogue_x_alignment, dialogue_y_alignment);

	fixed_size_container.setElement(dialogue_holder_container);
	parent.setElement(fixed_size_container);

	@dialogue_holder = IMDivider("dialogue_holder", DOVertical);
	dialogue_holder.setSize(dialogue_holder_size);
	dialogue_holder_container.setElement(dialogue_holder);

	@dialogue_line = IMDivider("dialogue_line" + line_counter, DOHorizontal);
	dialogue_line.setAlignment(CALeft, CATop);
	dialogue_holder.append(dialogue_line);
	dialogue_line.setZOrdering(2);
}

void BreathOfTheWildUI(IMContainer@ parent){
	parent.setSizeY(500.0);
	dialogue_holder_size = vec2(1500, 275);
	dialogue_x_alignment = CACenter;
	dialogue_y_alignment = CACenter;

	@fixed_size_container = IMContainer(dialogue_holder_size.x, dialogue_holder_size.y);

	SizePolicy x_policy(dialogue_holder_size.x);
	SizePolicy y_policy(dialogue_holder_size.y);
	x_policy.inheritMax();
	y_policy.inheritMax();
	@dialogue_holder_container = IMContainer(x_policy, y_policy);
	dialogue_holder_container.setAlignment(dialogue_x_alignment, dialogue_y_alignment);

	@dialogue_holder = IMDivider("dialogue_holder", DOVertical);
	dialogue_holder.setSize(dialogue_holder_size);
	dialogue_holder_container.setElement(dialogue_holder);

	fixed_size_container.setElement(dialogue_holder_container);
	parent.setElement(fixed_size_container);

	@dialogue_line = IMDivider("dialogue_line" + line_counter, DOHorizontal);
	dialogue_line.setAlignment(CALeft, CATop);
	dialogue_holder.append(dialogue_line);
	dialogue_line.setZOrdering(2);
}

void ChronoTriggerUI(IMContainer@ parent){
	parent.setSizeY(500.0);
	dialogue_holder_size = vec2(1400, 300);
	dialogue_x_alignment = CALeft;
	dialogue_y_alignment = CATop;

	@fixed_size_container = IMContainer(dialogue_holder_size.x, dialogue_holder_size.y);

	SizePolicy x_policy(dialogue_holder_size.x);
	SizePolicy y_policy(dialogue_holder_size.y);
	x_policy.inheritMax();
	y_policy.inheritMax();
	@dialogue_holder_container = IMContainer(x_policy, y_policy);
	dialogue_holder_container.setAlignment(dialogue_x_alignment, dialogue_y_alignment);

	@dialogue_holder = IMDivider("dialogue_holder", DOVertical);
	dialogue_holder.setSize(dialogue_holder_size);
	dialogue_holder_container.setElement(dialogue_holder);
	dialogue_holder.setAlignment(dialogue_x_alignment, dialogue_y_alignment);

	fixed_size_container.setElement(dialogue_holder_container);

	vec2 fixed_size_container_offset = vec2(575.0, 75.0f + dialogue_font.size);
	parent.addFloatingElement(fixed_size_container, "fixed_size_container", fixed_size_container_offset, -1);

	@dialogue_line = IMDivider("dialogue_line" + line_counter, DOHorizontal);
	dialogue_line.setAlignment(CALeft, CATop);
	dialogue_holder.append(dialogue_line);
	dialogue_line.setZOrdering(2);
}

void Fallout3UI(IMContainer@ parent){
	parent.setSizeY(600.0);
	dialogue_holder_size = vec2(1400, 350);
	dialogue_x_alignment = CALeft;
	dialogue_y_alignment = CACenter;

	@fixed_size_container = IMContainer(dialogue_holder_size.x, dialogue_holder_size.y);

	SizePolicy x_policy(dialogue_holder_size.x);
	SizePolicy y_policy(dialogue_holder_size.y);
	x_policy.inheritMax();
	y_policy.inheritMax();
	@dialogue_holder_container = IMContainer(x_policy, y_policy);
	dialogue_holder_container.setAlignment(dialogue_x_alignment, dialogue_y_alignment);

	@dialogue_holder = IMDivider("dialogue_holder", DOVertical);
	dialogue_holder.setSize(dialogue_holder_size);
	dialogue_holder_container.setElement(dialogue_holder);
	dialogue_holder.setAlignment(dialogue_x_alignment, dialogue_y_alignment);

	fixed_size_container.setElement(dialogue_holder_container);
	parent.setElement(fixed_size_container);

	@dialogue_line = IMDivider("dialogue_line" + line_counter, DOHorizontal);
	dialogue_line.setAlignment(CALeft, CATop);
	dialogue_holder.append(dialogue_line);
	dialogue_line.setZOrdering(2);
}

void LuigisMansionUI(IMContainer@ parent){
	parent.setSizeY(500.0);
	dialogue_holder_size = vec2(1500, 300);
	dialogue_x_alignment = CALeft;
	dialogue_y_alignment = CACenter;

	@fixed_size_container = IMContainer(dialogue_holder_size.x, dialogue_holder_size.y);
	/* fixed_size_container.showBorder(); */

	SizePolicy x_policy(dialogue_holder_size.x);
	SizePolicy y_policy(dialogue_holder_size.y);
	x_policy.inheritMax();
	y_policy.inheritMax();
	@dialogue_holder_container = IMContainer(x_policy, y_policy);
	dialogue_holder_container.setAlignment(dialogue_x_alignment, dialogue_y_alignment);

	@dialogue_holder = IMDivider("dialogue_holder", DOVertical);
	dialogue_holder.setSize(dialogue_holder_size);
	dialogue_holder_container.setElement(dialogue_holder);
	dialogue_holder.setAlignment(dialogue_x_alignment, dialogue_y_alignment);

	fixed_size_container.setElement(dialogue_holder_container);
	parent.setElement(fixed_size_container);

	@dialogue_line = IMDivider("dialogue_line" + line_counter, DOHorizontal);
	dialogue_line.setAlignment(CALeft, CATop);
	dialogue_holder.append(dialogue_line);
	dialogue_line.setZOrdering(2);
}

void MafiaUI(IMContainer@ parent){
	parent.setSizeY(500.0);
	dialogue_holder_size = vec2(1500, 400);
	dialogue_x_alignment = CACenter;
	dialogue_y_alignment = CACenter;

	@fixed_size_container = IMContainer(dialogue_holder_size.x, dialogue_holder_size.y);
	/* fixed_size_container.showBorder(); */

	SizePolicy x_policy(dialogue_holder_size.x);
	SizePolicy y_policy(dialogue_holder_size.y);
	x_policy.inheritMax();
	y_policy.inheritMax();
	@dialogue_holder_container = IMContainer(x_policy, y_policy);
	dialogue_holder_container.setAlignment(dialogue_x_alignment, dialogue_y_alignment);

	@dialogue_holder = IMDivider("dialogue_holder", DOVertical);
	dialogue_holder.setSize(dialogue_holder_size);
	dialogue_holder_container.setElement(dialogue_holder);
	dialogue_holder.setAlignment(dialogue_x_alignment, dialogue_y_alignment);

	fixed_size_container.setElement(dialogue_holder_container);
	parent.setElement(fixed_size_container);

	@dialogue_line = IMDivider("dialogue_line" + line_counter, DOHorizontal);
	dialogue_line.setAlignment(CALeft, CATop);
	dialogue_holder.append(dialogue_line);
	dialogue_line.setZOrdering(2);
}

void CreateBackground(IMContainer@ parent){
	if(!show_dialogue){
		return;
	}

	switch(dialogue_layout){
		case default_layout:
			DefaultBackground(parent);
			break;
		case simple_layout:
			SimpleBackground(parent);
			break;
		case breath_of_the_wild_layout:
			BreathOfTheWildBackground(parent);
			break;
		case chrono_trigger_layout:
			ChronoTriggerBackground(parent);
			break;
		case fallout_3_layout:
			Fallout3Background(parent);
			break;
		case luigis_mansion_layout:
			LuigisMansionBackground(parent);
			break;
		default :
			break;
	}
}

void DefaultBackground(IMContainer@ parent){
	//Remove any background that's already there.
	parent.removeElement("bg_container");

	IMContainer bg_container(0.0, 0.0);
	IMDivider bg_divider("bg_divider", DOHorizontal);
	bg_divider.setZOrdering(-1);
	bg_container.setElement(bg_divider);
	/* bg_container.showBorder(); */

	float bg_alpha = 0.5;
	float bg_height = 350.0;
	vec2 background_offset = vec2(0.0, 75.0);

	vec4 color = showing_choice?dialogue_font.color:current_actor_settings.color;

	IMImage left_fade("Textures/ui/dialogue/dialogue_bg-fade.png");
	left_fade.setColor(color);
	left_fade.setSizeX(500.0);
	left_fade.setSizeY(bg_height);
	left_fade.setAlpha(bg_alpha);
	left_fade.setClip(false);
	bg_divider.append(left_fade);

	IMImage middle_fade("Textures/ui/dialogue/dialogue_bg.png");
	middle_fade.setColor(color);
	middle_fade.setSizeX(1560.0);
	middle_fade.setSizeY(bg_height);
	middle_fade.setAlpha(bg_alpha);
	middle_fade.setClip(false);
	bg_divider.append(middle_fade);

	IMImage right_fade("Textures/ui/dialogue/dialogue_bg-fade_reverse.png");
	right_fade.setColor(color);
	right_fade.setSizeX(500.0);
	right_fade.setSizeY(bg_height);
	right_fade.setAlpha(bg_alpha);
	right_fade.setClip(false);
	bg_divider.append(right_fade);

	parent.addFloatingElement(bg_container, "bg_container", background_offset, -1);
	/* parent.showBorder(); */
}

void SimpleBackground(IMContainer@ parent){
	//Remove any background that's already there.
	parent.removeElement("bg_container");

	IMContainer bg_container(0.0, 0.0);
	IMDivider bg_divider("bg_divider", DOHorizontal);
	bg_divider.setZOrdering(-1);
	bg_container.setElement(bg_divider);

	float bg_alpha = 1.0;
	float bg_height = 450.0;
	vec2 background_offset = vec2(0.0, 25.0);

	IMImage middle_fade("Textures/dialogue_bg_nametag_faded.png");
	middle_fade.setSizeX(2560);
	middle_fade.setSizeY(bg_height);
	middle_fade.setAlpha(bg_alpha);
	middle_fade.setClip(false);
	bg_divider.append(middle_fade);

	parent.addFloatingElement(bg_container, "bg_container", background_offset, -1);
}

void BreathOfTheWildBackground(IMContainer@ parent){
	//Remove any background that's already there.
	parent.removeElement("bg_container");

	IMContainer bg_container(0.0, 0.0);
	IMDivider bg_divider("bg_divider", DOHorizontal);
	bg_divider.setZOrdering(-1);
	bg_container.setElement(bg_divider);

	float bg_alpha = 0.5;
	float bg_height = 400.0;
	vec2 background_offset = vec2(0.0, 25.0);

	IMImage left_fade("Textures/dialogue_bg_botw_left.png");
	left_fade.setSizeX(bg_height);
	left_fade.setSizeY(bg_height);
	left_fade.setAlpha(bg_alpha);
	left_fade.setClip(false);
	bg_divider.append(left_fade);

	IMImage middle_fade("Textures/dialogue_bg_botw_middle.png");
	middle_fade.setSizeX(1000.0);
	middle_fade.setSizeY(bg_height);
	middle_fade.setAlpha(bg_alpha);
	middle_fade.setClip(false);
	bg_divider.append(middle_fade);

	IMImage right_fade("Textures/dialogue_bg_botw_right.png");
	right_fade.setSizeX(bg_height);
	right_fade.setSizeY(bg_height);
	right_fade.setAlpha(bg_alpha);
	right_fade.setClip(false);
	bg_divider.append(right_fade);

	float whole_width = (bg_height * 2.0 + 1000.0);
	parent.addFloatingElement(bg_container, "bg_container", vec2((2560 / 2.0) - (whole_width / 2.0), 50.0), -1);
}

void ChronoTriggerBackground(IMContainer@ parent){
	//Remove any background that's already there.
	parent.removeElement("bg_container");

	IMContainer bg_container(0.0, 0.0);
	IMDivider bg_divider("bg_divider", DOHorizontal);
	bg_divider.setZOrdering(-1);
	bg_container.setElement(bg_divider);

	float bg_alpha = 1.0;
	float bg_height = 450.0;
	float side_width = 50.0;

	IMImage left_fade("Textures/dialogue_bg_ct_left.png");
	left_fade.setSizeX(side_width);
	left_fade.setSizeY(bg_height);
	left_fade.setAlpha(bg_alpha);
	left_fade.setClip(false);
	bg_divider.append(left_fade);

	IMImage middle_fade("Textures/dialogue_bg_ct_middle.png");
	middle_fade.setSizeX(1500.0);
	middle_fade.setSizeY(bg_height);
	middle_fade.setAlpha(bg_alpha);
	middle_fade.setClip(false);
	bg_divider.append(middle_fade);

	IMImage right_fade("Textures/dialogue_bg_ct_right.png");
	right_fade.setSizeX(side_width);
	right_fade.setSizeY(bg_height);
	right_fade.setAlpha(bg_alpha);
	right_fade.setClip(false);
	bg_divider.append(right_fade);

	float whole_width = (side_width * 2.0 + 1500.0);
	parent.addFloatingElement(bg_container, "bg_container", vec2((2560 / 2.0) - (whole_width / 2.0), 25.0), -1);
}

void Fallout3Background(IMContainer@ parent){
	//Remove any background that's already there.
	parent.removeElement("bg_container");

	IMContainer bg_container(0.0, 0.0);
	IMDivider bg_divider("bg_divider", DOHorizontal);
	bg_divider.setZOrdering(-1);
	bg_container.setElement(bg_divider);

	float bg_alpha = 0.75;
	float bg_height = 400.0;
	float side_width = 5.0;
	vec4 color = showing_choice?dialogue_font.color:current_actor_settings.color;

	IMImage left_fade("Textures/dialogue_bg_fo3_end.png");
	left_fade.setColor(color);
	left_fade.setSizeX(side_width);
	left_fade.setSizeY(bg_height);
	left_fade.setAlpha(bg_alpha);
	left_fade.setClip(false);
	bg_divider.append(left_fade);

	IMImage middle_fade("Textures/ui/dialogue/dialogue_bg.png");
	middle_fade.setColor(color);
	middle_fade.setSizeX(1500.0);
	middle_fade.setSizeY(bg_height);
	middle_fade.setAlpha(bg_alpha);
	middle_fade.setClip(false);
	bg_divider.append(middle_fade);

	IMImage right_fade("Textures/dialogue_bg_fo3_end.png");
	right_fade.setColor(color);
	right_fade.setSizeX(side_width);
	right_fade.setSizeY(bg_height);
	right_fade.setAlpha(bg_alpha);
	right_fade.setClip(false);
	bg_divider.append(right_fade);

	float whole_width = (side_width * 2.0 + 1500.0);
	parent.addFloatingElement(bg_container, "bg_container", vec2((2560 / 2.0) - (whole_width / 2.0), 100.0), -1);
}

void LuigisMansionBackground(IMContainer@ parent){
	//Remove any background that's already there.
	parent.removeElement("bg_container");

	IMContainer bg_container(0.0, 0.0);
	IMDivider bg_divider("bg_divider", DOHorizontal);
	bg_divider.setZOrdering(-1);
	bg_container.setElement(bg_divider);

	float bg_alpha = 0.75;
	float bg_height = 350.0;
	float side_width = bg_height / 2.0;
	float middle_width = 1800.0;
	vec4 color = showing_choice?dialogue_font.color:current_actor_settings.color;

	IMImage left_fade("Textures/dialogue_bg_lm_left.png");
	left_fade.setColor(color);
	left_fade.setAlpha(bg_alpha);
	left_fade.scaleToSizeY(bg_height);
	left_fade.setClip(false);
	bg_divider.append(left_fade);

	IMImage middle_fade("Textures/dialogue_bg_lm_middle.png");
	middle_fade.setColor(color);
	middle_fade.setAlpha(bg_alpha);
	middle_fade.setSizeX(middle_width);
	middle_fade.setSizeY(bg_height);
	middle_fade.setClip(false);
	bg_divider.append(middle_fade);

	IMImage right_fade("Textures/dialogue_bg_lm_right.png");
	right_fade.setColor(color);
	right_fade.setAlpha(bg_alpha);
	right_fade.scaleToSizeY(bg_height);
	right_fade.setClip(false);
	bg_divider.append(right_fade);

	float whole_width = (side_width * 2.0 + middle_width);
	parent.addFloatingElement(bg_container, "bg_container", vec2((2560 / 2.0) - (whole_width / 2.0), 75.0), -1);
	dialogue_move_in_timer = dialogue_move_in_duration;
	if(allow_dialogue_move_in){
		dialogue_move_in = true;
	}
}

void CreateNameTag(IMContainer@ parent){
	if(!show_dialogue || showing_choice){
		return;
	}

	switch(dialogue_layout){
		case default_layout:
			DefaultNameTag(parent);
			break;
		case simple_layout:
			SimpleNameTag(parent);
			break;
		case breath_of_the_wild_layout:
			BreathOfTheWildNameTag(parent);
			break;
		case chrono_trigger_layout:
			ChronoTriggerNameTag(parent);
			break;
		case fallout_3_layout:
			Fallout3NameTag(parent);
			break;
		case luigis_mansion_layout:
			LuigisMansionNameTag(parent);
			break;
		case mafia_layout:
			MafiaNameTag(parent);
			break;
		default :
			break;
	}
}

void DefaultNameTag(IMContainer@ parent){
	//Remove any nametag that's already there.
	parent.removeElement("name_container");

	IMContainer name_container(0.0, 125.0);

	if(show_names){
		IMDivider name_divider("name_divider", DOHorizontal);
		name_divider.setZOrdering(2);
		name_divider.setAlignment(CACenter, CACenter);
		name_container.setElement(name_divider);

		vec2 offset(0.0, 15.0);
		IMText name(current_actor_settings.name, name_font);
		name_divider.appendSpacer(50.0);
		name_divider.append(name);
		name_divider.appendSpacer(50.0);
		name.setColor(current_actor_settings.color);

		IMImage name_background("Textures/ui/menus/main/brushStroke.png");
		name_background.setClip(false);
		parent.addFloatingElement(name_container, "name_container", offset, 3);

		imGUI.update();
		name_background.setSize(name_container.getSize());
		name_container.addFloatingElement(name_background, "name_background", vec2(0, 0), 1);
		name_background.setZOrdering(1);
	}
}

void SimpleNameTag(IMContainer@ parent){
	//Remove any nametag that's already there.
	parent.removeElement("nametag_container");

	IMContainer nametag_container(0.0, 0.0);
	IMDivider nametag_divider("nametag_divider", DOVertical);
	nametag_container.setElement(nametag_divider);
	nametag_divider.setAlignment(CACenter, CATop);

	if(show_names){
		IMContainer name_container(-1.0, dialogue_font.size);
		name_container.setAlignment(CACenter, CACenter);
		IMDivider name_divider("name_divider", DOHorizontal);
		name_divider.setZOrdering(3);
		name_divider.setAlignment(CACenter, CACenter);
		name_container.setElement(name_divider);
		nametag_divider.append(name_container);

		IMText name(current_actor_settings.name, name_font_arial);
		name_divider.appendSpacer(60.0);
		name_divider.append(name);
		name_divider.appendSpacer(60.0);
		name.setColor(current_actor_settings.color);

		IMImage name_background("Textures/dialogue_bg_nametag_faded.png");
		name_background.setClip(false);
		name_background.setAlpha(0.75);

		imGUI.update();
		name_background.setSize(name_container.getSize());
		name_container.addFloatingElement(name_background, "name_background", vec2(0, 0), 1);
		name_background.setZOrdering(1);
	}

	if(current_actor_settings.avatar_path != "None" && show_avatar){
		IMImage avatar_image(current_actor_settings.avatar_path);
		avatar_image.setSize(vec2(400, 400));
		nametag_divider.append(avatar_image);
	}

	parent.addFloatingElement(nametag_container, "nametag_container", vec2(200, 0), 3);
}

void BreathOfTheWildNameTag(IMContainer@ parent){
	//Remove any nametag that's already there.
	parent.removeElement("name_container");

	IMContainer name_container(0.0, 100.0);

	if(current_actor_settings.avatar_path != "None" && show_avatar){
		IMImage avatar_image(current_actor_settings.avatar_path);
		avatar_image.setSize(vec2(350, 350));
		avatar_image.setClip(false);
		name_container.addFloatingElement(avatar_image, "avatar", vec2(-500, 50), 3);
	}

	if(show_names){
		IMDivider name_divider("name_divider", DOHorizontal);
		name_divider.setZOrdering(3);
		name_divider.setAlignment(CACenter, CACenter);
		name_container.setElement(name_divider);
		IMText name(current_actor_settings.name, dialogue_font);
		name_divider.appendSpacer(30.0);
		name_divider.append(name);
		name_divider.appendSpacer(30.0);
		name.setColor(current_actor_settings.color);
	}

	parent.addFloatingElement(name_container, "name_container", vec2(550, 50.0 -(dialogue_font.size / 4.0)), 3);
}

void ChronoTriggerNameTag(IMContainer@ parent){
	//Remove any nametag that's already there.
	parent.removeElement("name_container");

	IMContainer name_container(0.0, 100.0);

	if(current_actor_settings.avatar_path != "None" && show_avatar){
		IMImage avatar_image(current_actor_settings.avatar_path);
		avatar_image.setSize(vec2(350, 350));
		avatar_image.setClip(false);
		name_container.addFloatingElement(avatar_image, "avatar", vec2(-450, 25), 3);
	}

	if(show_names){
		IMDivider name_divider("name_divider", DOHorizontal);
		name_divider.setZOrdering(3);
		name_divider.setAlignment(CACenter, CACenter);
		name_container.setElement(name_divider);
		IMText name(current_actor_settings.name + " : ", dialogue_font);
		name_divider.appendSpacer(30.0);
		name_divider.append(name);
		name_divider.appendSpacer(30.0);
		name.setColor(current_actor_settings.color);
	}

	parent.addFloatingElement(name_container, "name_container", vec2(500.0, dialogue_font.size), 3);
}

void Fallout3NameTag(IMContainer@ parent){
	//Remove any nametag that's already there.
	parent.removeElement("name_container");

	IMContainer name_container(parent.getSizeX(), dialogue_font.size);
	name_container.setAlignment(CACenter, CACenter);

	if(current_actor_settings.avatar_path != "None" && show_avatar){
		IMImage avatar_image(current_actor_settings.avatar_path);
		avatar_image.setSize(vec2(350, 350));
		avatar_image.setClip(false);
		name_container.addFloatingElement(avatar_image, "avatar", vec2(-1050, 100), 3);
	}

	if(show_names){
		IMDivider name_divider("name_divider", DOHorizontal);
		name_divider.setZOrdering(3);
		name_divider.setAlignment(CACenter, CACenter);
		name_container.setElement(name_divider);
		IMText name(current_actor_settings.name, dialogue_font);
		name_divider.append(name);
		name.setColor(current_actor_settings.color);
	}

	parent.addFloatingElement(name_container, "name_container", vec2(0.0, 100.0 -dialogue_font.size), 3);
}

void LuigisMansionNameTag(IMContainer@ parent){
	//Remove any nametag that's already there.
	parent.removeElement("name_container");

	IMContainer name_container(parent.getSizeX(), dialogue_font.size);
	name_container.setAlignment(CACenter, CACenter);

	if(current_actor_settings.avatar_path != "None" && show_avatar){
		IMImage avatar_image(current_actor_settings.avatar_path);
		avatar_image.setSize(vec2(250, 250));
		avatar_image.setClip(false);
		name_container.addFloatingElement(avatar_image, "avatar", vec2(250.0, 125.0), 3);
	}

	if(show_names){
		IMDivider name_divider("name_divider", DOVertical);
		name_divider.setSizeX(parent.getSizeX());
		name_divider.setZOrdering(3);
		name_divider.setAlignment(CACenter, CATop);
		name_container.addFloatingElement(name_divider, "name_divider", vec2(0.0, 75.0 -dialogue_font.size), 3);
		IMText name(current_actor_settings.name, dialogue_font);
		name_divider.append(name);
		name.setColor(current_actor_settings.color);
	}

	parent.addFloatingElement(name_container, "name_container", vec2(0.0), 3);
}

void MafiaNameTag(IMContainer@ parent){
	//Remove any nametag that's already there.
	parent.removeElement("name_container");

	if(show_names){
		IMDivider name_divider("name_divider", DOVertical);
		name_divider.setZOrdering(3);
		name_divider.setAlignment(CACenter, CATop);
		dialogue_line.append(name_divider);
		IMText name(current_actor_settings.name + " : ", dialogue_font);
		name_divider.append(name);
		name.setColor(current_actor_settings.color);
	}
}

void UpdateDialogueMoveIn(){
	int direction = (dialogue_location == dialogue_top)?-1:1;
	dialogue_container.setDisplacementY(InQuad(dialogue_move_in_timer / dialogue_move_in_duration) * dialogue_holder_size.y * direction);

	if(dialogue_move_in_timer <= 0.0){
		dialogue_move_in = false;
		allow_dialogue_move_in = false;
		return;
	}

	dialogue_move_in_timer -= time_step;
}

float dialogue_displacement_target;
bool set_dialogue_displacement = false;
bool smooth_dialogue_displacement = false;

void UpdateDialogueDisplacement(){
	if(set_dialogue_displacement){
		if(smooth_dialogue_displacement){
			float current_displacement = dialogue_holder.getDisplacementY();
			float new_displacement = current_displacement - (time_step * 2000.0);
			if(new_displacement <= dialogue_displacement_target){
				set_dialogue_displacement = false;
				dialogue_holder.setDisplacementY(dialogue_displacement_target);
			}else{
				dialogue_holder.setDisplacementY(new_displacement);
			}
		}else{
			set_dialogue_displacement = false;
			dialogue_holder.setDisplacementY(dialogue_displacement_target);
		}
	}
}

void DialogueNext(){
	DialogueScriptEntry@ entry = dialogue_script[dialogue_progress];
	dialogue_progress += 1;

	if(line_number != entry.line){
		line_number = entry.line;
		//If the max is -1 then there is enough room for all the lines.
		if(maximum_amount_of_lines != -1 && line_number > maximum_amount_of_lines - 1){
			//Hide the top most line first.
			int top_line_index = line_number - maximum_amount_of_lines;
			for(uint i = 0; i < dialogue_cache[top_line_index].size(); i++){
				dialogue_cache[top_line_index][i].setText("");
			}
			dialogue_displacement_target = dialogue_displacement_target - dialogue_font.size;
			set_dialogue_displacement = true;
		}
	}

	switch(entry.script_entry_type){
		case character_entry:
			entry.text.setText(entry.text.getText() + entry.character);
			if(the_time - last_continue_sound_time > 0.1){
				last_continue_sound_time = the_time;
				PlayLineContinueSound();
			}
			break;
		case new_line_entry:
			break;
		case wait_entry:
			break;
		default:
			break;
	}
}

void PlayLineContinueSound(int test_voice = -1) {
	if(!use_voice_sounds){return;}
    switch(test_voice == -1?current_actor_settings.voice:test_voice){
        case 0: PlaySoundGroup("Data/Sounds/concrete_foley/fs_light_concrete_edgecrawl.xml"); break;
        case 1: PlaySoundGroup("Data/Sounds/drygrass_foley/fs_light_drygrass_crouchwalk.xml"); break;
        case 2: PlaySoundGroup("Data/Sounds/cloth_foley/cloth_fabric_crouchwalk.xml"); break;
        case 3: PlaySoundGroup("Data/Sounds/dirtyrock_foley/fs_light_dirtyrock_crouchwalk.xml"); break;
        case 4: PlaySoundGroup("Data/Sounds/cloth_foley/cloth_leather_crouchwalk.xml"); break;
        case 5: PlaySoundGroup("Data/Sounds/grass_foley/fs_light_grass_run.xml", 0.5); break;
        case 6: PlaySoundGroup("Data/Sounds/gravel_foley/fs_light_gravel_crouchwalk.xml"); break;
        case 7: PlaySoundGroup("Data/Sounds/sand_foley/fs_light_sand_crouchwalk.xml", 0.7); break;
        case 8: PlaySoundGroup("Data/Sounds/snow_foley/fs_light_snow_run.xml", 0.5); break;
        case 9: PlaySoundGroup("Data/Sounds/wood_foley/fs_light_wood_crouchwalk.xml", 0.4); break;
        case 10: PlaySoundGroup("Data/Sounds/water_foley/mud_fs_walk.xml", 0.4); break;
        case 11: PlaySoundGroup("Data/Sounds/concrete_foley/fs_heavy_concrete_walk.xml", 0.5); break;
        case 12: PlaySoundGroup("Data/Sounds/drygrass_foley/fs_heavy_drygrass_walk.xml", 0.4); break;
        case 13: PlaySoundGroup("Data/Sounds/dirtyrock_foley/fs_heavy_dirtyrock_walk.xml", 0.5); break;
        case 14: PlaySoundGroup("Data/Sounds/grass_foley/fs_heavy_grass_walk.xml", 0.3); break;
        case 15: PlaySoundGroup("Data/Sounds/gravel_foley/fs_heavy_gravel_walk.xml", 0.3); break;
        case 16: PlaySoundGroup("Data/Sounds/sand_foley/fs_heavy_sand_run.xml", 0.3); break;
        case 17: PlaySoundGroup("Data/Sounds/snow_foley/fs_heavy_snow_crouchwalk.xml", 0.3); break;
        case 18: PlaySoundGroup("Data/Sounds/wood_foley/fs_heavy_wood_walk.xml", 0.3); break;
    }
}

void PlayLineStartSound(){
	if(!use_voice_sounds){return;}
	switch(current_actor_settings.voice){
		case 0: PlaySoundGroup("Data/Sounds/concrete_foley/fs_light_concrete_run.xml"); break;
		case 1: PlaySoundGroup("Data/Sounds/drygrass_foley/fs_light_drygrass_walk.xml"); break;
		case 2: PlaySoundGroup("Data/Sounds/cloth_foley/cloth_fabric_choke_move.xml"); break;
		case 3: PlaySoundGroup("Data/Sounds/dirtyrock_foley/fs_light_dirtyrock_run.xml"); break;
		case 4: PlaySoundGroup("Data/Sounds/cloth_foley/cloth_leather_choke_move.xml"); break;
		case 5: PlaySoundGroup("Data/Sounds/grass_foley/bf_grass_medium.xml", 0.5); break;
		case 6: PlaySoundGroup("Data/Sounds/gravel_foley/fs_light_gravel_run.xml"); break;
		case 7: PlaySoundGroup("Data/Sounds/sand_foley/fs_light_sand_run.xml", 0.7); break;
		case 8: PlaySoundGroup("Data/Sounds/snow_foley/bf_snow_light.xml", 0.5); break;
		case 9: PlaySoundGroup("Data/Sounds/wood_foley/fs_light_wood_run.xml", 0.4); break;
		case 10: PlaySoundGroup("Data/Sounds/water_foley/mud_fs_run.xml", 0.4); break;
		case 11: PlaySoundGroup("Data/Sounds/concrete_foley/fs_heavy_concrete_run.xml", 0.5); break;
		case 12: PlaySoundGroup("Data/Sounds/drygrass_foley/fs_heavy_drygrass_run.xml", 0.4); break;
		case 13: PlaySoundGroup("Data/Sounds/dirtyrock_foley/fs_heavy_dirtyrock_run.xml", 0.5); break;
		case 14: PlaySoundGroup("Data/Sounds/grass_foley/fs_heavy_grass_run.xml", 0.3); break;
		case 15: PlaySoundGroup("Data/Sounds/gravel_foley/fs_heavy_gravel_run.xml", 0.3); break;
		case 16: PlaySoundGroup("Data/Sounds/sand_foley/fs_heavy_sand_jump.xml", 0.3); break;
		case 17: PlaySoundGroup("Data/Sounds/snow_foley/fs_heavy_snow_jump.xml", 0.3); break;
		case 18: PlaySoundGroup("Data/Sounds/wood_foley/fs_heavy_wood_run.xml", 0.3); break;
	}
}

void SelectChoice(int new_selected_choice){
	if(new_selected_choice != selected_choice){
		choice_ui_elements[selected_choice].showBorder(false);
		choice_ui_elements[selected_choice].removeElement("bg");
	}
	selected_choice = new_selected_choice;
	choice_ui_elements[selected_choice].showBorder(true);

	vec4 background_color = dialogue_font.color;
	background_color.a = 0.15f;
	IMImage background_image("Textures/ui/whiteblock.tga");
	background_image.setClip(true);
	background_image.setSize(choice_ui_elements[selected_choice].getSize());
	background_image.setEffectColor(background_color);
	choice_ui_elements[selected_choice].addFloatingElement(background_image, "bg", vec2(0.0));
}
