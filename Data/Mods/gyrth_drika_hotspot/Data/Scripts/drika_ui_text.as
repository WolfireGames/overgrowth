class DrikaUIText : DrikaUIElement{
	IMDivider@ holder;
	IMContainer@ outline_container;
	array<IMText@> text_elements;
	array<string> split_content;
	string text_content;
	ivec2 position;
	float rotation;
	DrikaUIGrabber@ grabber_center;
	string holder_name;
	DrikaUIFont@ font_element = null;
	array<FadeOut@> fade_out_animations;

	DrikaUIText(JSONValue params = JSONValue()){
		drika_ui_element_type = drika_ui_text;

		text_content = GetJSONString(params, "text_content", "");
		split_content = text_content.split("\n");
		rotation = GetJSONFloat(params, "rotation", 0.0);
		position = GetJSONIVec2(params, "position", ivec2());
		index = GetJSONInt(params, "index", 0);
		hotspot_id = GetJSONInt(params, "hotspot_id", -1);

		@font_element = cast<DrikaUIFont@>(GetUIElement(GetJSONString(params, "font_id", "")));

		ui_element_identifier = GetJSONString(params, "ui_element_identifier", "");
		PostInit();
	}

	void PostInit(){
		IMDivider text_holder(ui_element_identifier + "textholder", DOVertical);
		@holder = text_holder;
		text_holder.setAlignment(CALeft, CATop);
		text_holder.setClip(false);
		text_holder.setZOrdering(index);

		@grabber_center = DrikaUIGrabber("center", 1, 1, mover);
		grabber_center.margin = 0.0;
		holder_name = imGUI.getUniqueName("text");

		@outline_container = IMContainer("outline_container");
		outline_container.setElement(text_holder);
		outline_container.setBorderColor(edit_outline_color);

		text_container.addFloatingElement(outline_container, holder_name, vec2(position.x, position.y), 0);
		CreateText();
	}

	void Update(){
		for(uint i = 0; i < fade_out_animations.size(); i++){
			if(fade_out_animations[i].Update()){
				fade_out_animations.removeAt(i);
			}
		}
	}

	void ReadUIInstruction(array<string> instruction){
		/* Log(warning, "Got instruction " + instruction[0]); */
		if(instruction[0] == "set_position"){
			position.x = atoi(instruction[1]);
			position.y = atoi(instruction[2]);
			SetPosition();
		}else if(instruction[0] == "set_rotation"){
			rotation = atof(instruction[1]);
			CreateText();
		}else if(instruction[0] == "set_content"){
			text_content = instruction[1].substr(0, instruction[1].length() - 1);
			split_content = text_content.split("\n");
			CreateText();
		}else if(instruction[0] == "font_changed"){
			@font_element = cast<DrikaUIFont@>(GetUIElement(instruction[1]));
			CreateText();
		}else if(instruction[0] == "variable_changed"){
			SetNewText();
		}else if(instruction[0] == "set_z_order"){
			index = atoi(instruction[1]);
			SetZOrder();
		}else if(instruction[0] == "add_update_behaviour"){
			string update_behaviour = instruction[1];
			if(update_behaviour == "fade_in" || update_behaviour == "fade_out"){

				int duration = atoi(instruction[2]);
				int tween_type = atoi(instruction[3]);
				string identifier = instruction[4];
				bool preview = (instruction[7] == "true");

				if(update_behaviour == "fade_out"){
					float starting_alpha = (font_element is null)?default_font.color.a:font_element.font_color.a;
					for(uint i = 0; i < text_elements.size(); i++){
						fade_out_animations.insertLast(FadeOut(update_behaviour, identifier, duration, tween_type, text_elements[i], preview, starting_alpha));
					}
				}else{
					for(uint i = 0; i < text_elements.size(); i++){
						IMFadeIn new_fade(duration, IMTweenType(tween_type));
						text_elements[i].addUpdateBehavior(new_fade, update_behaviour + "2");
					}
				}
			}else if(update_behaviour == "move_in" || update_behaviour == "move_out"){
				int duration = atoi(instruction[2]);
				int tween_type = atoi(instruction[3]);
				string identifier = instruction[4];
				vec2 offset(atoi(instruction[5]), atoi(instruction[6]));

				if(update_behaviour == "move_out"){
					IMMoveIn new_move(duration, offset * -1.0f, IMTweenType(tween_type));
					holder.addUpdateBehavior(new_move, identifier);

					imGUI.update();

					for(uint i = 0; i < text_elements.size(); i++){
						text_elements[i].setDisplacement(offset);
					}
				}else{
					IMMoveIn new_move(duration, offset, IMTweenType(tween_type));
					holder.addUpdateBehavior(new_move, identifier);
					imGUI.update();
				}
			}
		}else if(instruction[0] == "remove_update_behaviour"){
			string identifier = instruction[1];

			if(holder.hasUpdateBehavior(identifier)){
				holder.removeUpdateBehavior(identifier);
			}

			for(uint i = 0; i < text_elements.size(); i++){
				text_elements[i].setDisplacement(vec2());
				if(text_elements[i].hasUpdateBehavior(identifier)){
					text_elements[i].removeUpdateBehavior(identifier);
				}
			}

			bool remove_element = false;
			bool skip = false;

			for(uint i = 0; i < fade_out_animations.size(); i++){

				if(identifier == fade_out_animations[i].name + fade_out_animations[i].identifier){
					if(fade_out_animations[i].preview){
						//Check if a fadeout made it completely transparent. Then just set to alpha to the color alpha.
						if(font_element !is null){
							fade_out_animations[i].target.setAlpha(font_element.font_color.a);
						}else{
							fade_out_animations[i].target.setAlpha(default_font.color.a);
						}
					}else{
						remove_element = true;
					}
					fade_out_animations.removeAt(i);
					i--;
				}
				skip = true;
			}

			if(remove_element){
				level.SendMessage("drika_ui_remove_element " + ui_element_identifier);
			}

			if(skip){
				return;
			}
		}
		UpdateContent();
	}

	void Delete(){
		text_container.removeElement(holder_name);
		grabber_center.Delete();
	}

	void SetZOrder(){
		holder.setZOrdering(index);
		outline_container.setZOrdering(index);
		for(uint i = 0; i < text_elements.size(); i++){
			text_elements[i].setZOrdering(index);
		}
		grabber_center.SetZOrder(index);
	}

	void CreateText(){
		text_elements.resize(0);
		holder.clear();
		holder.setSize(vec2(-1,-1));

		split_content = text_content.split("\n");

		for(uint i = 0; i < split_content.size(); i++){
			split_content[i] = ReplaceVariablesFromText(split_content[i]);

			IMText@ new_text;
			DisposeTextAtlases();
			if(font_element is null){
				@new_text = IMText(split_content[i], default_font);
			}else{
				@new_text = IMText(split_content[i], font_element.font);
			}
			text_elements.insertLast(@new_text);
			holder.append(new_text);
			new_text.setRotation(rotation);
		}

		// imgui needs to update once or else the position of the grabber isn't calculated correctly.
		imGUI.update();
		UpdateContent();
	}

	void SetNewText(){
		split_content = text_content.split("\n");

		for(uint i = 0; i < split_content.size(); i++){
			split_content[i] = ReplaceVariablesFromText(split_content[i]);

			if(i < text_elements.size()){
				text_elements[i].setText(split_content[i]);
			}
		}
	}

	string ReadParamValue(string key){
		SavedLevel@ data = save_file.GetSavedLevel("drika_data");

		return (data.GetValue("[" + key + "]") == "true")? data.GetValue(key) : "--ERROR - " + key + " does not exist--";
	}

	void UpdateContent(){
		outline_container.showBorder(editing);
		grabber_center.SetVisible(editing);

		vec2 position = text_container.getElementPosition(holder_name);
		grabber_center.SetPosition(position);
		vec2 size = holder.getSize();
		if(size.x + size.y > 0.0){
			grabber_center.SetSize(vec2(size.x, size.y));
		}

		if(outline_container !is null){
			outline_container.setSize(vec2(size.x, size.y));
		}

		SetZOrder();
	}

	void SetEditing(bool _editing){
		editing = _editing;
		UpdateContent();
	}

	DrikaUIGrabber@ GetGrabber(string grabber_name){
		if(grabber_name == "center"){
			return grabber_center;
		}else{
			return null;
		}
	}

	void AddPosition(ivec2 added_positon){
		text_container.moveElementRelative(holder_name, vec2(added_positon.x, added_positon.y));
		position += added_positon;
		UpdateContent();
		SendUIInstruction("set_position", {position.x, position.y});
	}

	void AddUpdateBehavior(IMUpdateBehavior@ behavior, string name){
		for(uint i = 0; i < text_elements.size(); i++){
			text_elements[i].addUpdateBehavior(behavior, name);
		}
	}

	void RemoveUpdateBehavior(string name){
		for(uint i = 0; i < text_elements.size(); i++){
			text_elements[i].removeUpdateBehavior(name);
		}
	}

	void SetPosition(){
		text_container.moveElement(holder_name, vec2(position.x, position.y));
	}
}
