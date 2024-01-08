enum drika_input_box_modes	{
								everything,
								onlynumbers,
								onlyletters
							};

array<string> possible_inputs =				{
												"1","2","3","4","5","6","7","8","9","0","-","+",
												"q","w","e","r","t","y","u","i","o","p","\\",
												"a","s","d","f","g","h","j","k","l",";","'",
												"z","x","c","v","b","n","m",",",".","/"
											};
										
array<string> possible_inputs_caps =		{
												"!","@","#","$","%","^","&","*","(",")","_","+",
												"Q","W","E","R","T","Y","U","I","O","P","|",
												"A","S","D","F","G","H","J","K","L",":","\"",
												"Z","X","C","V","B","N","M","<",">","?"
											};
										
array<string> possible_letter_inputs =		{
												"q","w","e","r","t","y","u","i","o","p",
												"a","s","d","f","g","h","j","k","l",
												"z","x","c","v","b","n","m"
											};
										
array<string> possible_letter_inputs_caps =	{
												"Q","W","E","R","T","Y","U","I","O","P",
												"A","S","D","F","G","H","J","K","L",
												"Z","X","C","V","B","N","M"
											};
											
array<string> possible_number_inputs =		{
												"1","2","3","4","5","6","7","8","9","0"
											};


class DrikaUIInput : DrikaUIImage{
	drika_input_box_modes drika_input_box_mode;
	int current_input_box_mode;
	string 	input_image_path;
	string 	input_variable;
	string	input_cursor;
	int		input_max_length;
	bool	input_active;
	int		input_timer;
	int		input_timer_max = 75;
	bool	checked_for_mouse_down = false;
	DrikaUIFont@ font_element = null;
	IMContainer@ text_holder;

	DrikaUIInput(JSONValue params = JSONValue()){
		input_image_path = GetJSONString(params, "input_image_path", "");
		input_variable = GetJSONString(params, "input_variable", "");
		input_max_length = GetJSONInt(params, "input_max_length", 0);
		drika_input_box_mode = drika_input_box_modes(GetJSONInt(params, "drika_input_box_mode", everything));
		current_input_box_mode = drika_input_box_mode;
		@font_element = cast<DrikaUIFont@>(GetUIElement(GetJSONString(params, "font_id", "")));

		super(params);

		drika_ui_element_type = drika_ui_input;
	}
	
	void Update(){
		if(input_active == true){
			input_timer += -1;
			if (input_timer < 1) {
				input_timer = input_timer_max; input_cursor == ""? input_cursor = "|":input_cursor = ""; SetInputText();
			}
			switch(current_input_box_mode){
				case everything:
					for(uint i = 0; i < possible_inputs.size(); i++){
						if(GetInputPressed(0, possible_inputs[i])){
							GetInputDown(0,"lshift") ? AddToParamValue(possible_inputs_caps[i]):AddToParamValue(possible_inputs[i]);
						}
					}
					break;
					
				case onlynumbers:
					for(uint i = 0; i < possible_number_inputs.size(); i++){
						if(GetInputPressed(0, possible_number_inputs[i])){
							AddToParamValue(possible_number_inputs[i]);
						}
					}
					break;
					
				case onlyletters:
					for(uint i = 0; i < possible_letter_inputs.size(); i++){
						if(GetInputPressed(0, possible_letter_inputs[i])){
							GetInputDown(0,"lshift") ? AddToParamValue(possible_letter_inputs_caps[i]):AddToParamValue(possible_letter_inputs[i]);
						}
					}
					break;
					
				default:
					break;
			}
			
			if(GetInputPressed(0, "return")){
				input_active = false;
				SetInputText();
			}
			
			if(GetInputPressed(0, "backspace")){
				SubtractFromParamValue();
			}
		}
		//Log(fatal, "input_variable content is " + ReadParamValue(input_variable));
		/*
		array<KeyboardPress> inputs = GetRawKeyboardInputs();
			if(inputs.size() > 0){
				uint16 possible_new_input = inputs[inputs.size()-1].s_id;
				uint32 keycode = inputs[inputs.size()-1].keycode;
				if(keycode == 13){
                        input_active = false;
                    }
			}
		*/
		if(ImGui_IsMouseDown(0) == true && checked_for_mouse_down == false){
			checked_for_mouse_down = true;
			input_active = outline_container.isMouseOver();
			SetInputText();
		}
		if(ImGui_IsMouseDown(0) == false && checked_for_mouse_down == true){
			checked_for_mouse_down = false;
		}
	}
	
	void SubtractFromParamValue(){
		SavedLevel@ data = save_file.GetSavedLevel("drika_data");
		string original_string = data.GetValue(input_variable);
		uint cutoff_pos = original_string.length()-1;
		string subtracted_string = original_string.substr(0,cutoff_pos);
		
		data.SetValue(input_variable, subtracted_string);
	}
	
	void AddToParamValue(string input){
		SavedLevel@ data = save_file.GetSavedLevel("drika_data");
		string original_value = data.GetValue(input_variable);
		if (uint(input_max_length) > original_value.length()){
			data.SetValue(input_variable, original_value + input);
		}
	}
	
	void PostInit(){
		IMImage new_image(input_image_path);
		@image = new_image;
		new_image.setZOrdering(index);
		new_image.setClip(false);
		image_name = imGUI.getUniqueName("image");

		@grabber_top_left = DrikaUIGrabber("top_left", -1, -1, scaler);
		@grabber_top_right = DrikaUIGrabber("top_right", 1, -1, scaler);
		@grabber_bottom_left = DrikaUIGrabber("bottom_left", -1, 1, scaler);
		@grabber_bottom_right = DrikaUIGrabber("bottom_right", 1, 1, scaler);
		@grabber_center = DrikaUIGrabber("center", 1, 1, mover);

		@holder = IMContainer("holder");
		holder.setZOrdering(index);
		holder.setClip(false);
		holder.setSize(vec2(size.x, size.y));
		holder.setElement(image);

		@text_holder = IMContainer("text_holder");
		text_holder.setZOrdering(index + 1);
		text_holder.setClip(false);
		text_holder.setSize(vec2(size.x, size.y));
		text_holder.setAlignment(CALeft,CACenter);
		
		SetInputText();
		holder.addFloatingElement(text_holder, "text_holder", vec2(0.0, 0.0), index + 1);

		@outline_container = IMContainer("outline_container");
		outline_container.setElement(holder);
		outline_container.setBorderColor(edit_outline_color);
		outline_container.setClip(false);
		outline_container.setZOrdering(index);
		
		holder.sendMouseOverToChildren(true);

		IMMessage on_click("drika_input_clicked");
		on_click.addString(ui_element_identifier);
		outline_container.addLeftMouseClickBehavior(IMFixedMessageOnClick(on_click), "");

		image_container.addFloatingElement(outline_container, image_name, vec2(position.x, position.y), 0);
		new_image.setRotation(rotation);
		new_image.setColor(color);
		SetSize();
		imGUI.update();
		UpdateContent();
	}

	
	void SetSize(){
		vec2 new_size = vec2(size.x, size.y);
		image.setSize(new_size);
		text_holder.setDisplacementX(holder.getSizeX()* 0.1);

		if(outline_container !is null){
			outline_container.setSize(new_size);
		}

		if(holder !is null){
			holder.setSize(new_size);
		}

		if(text_holder !is null){
			text_holder.setSize(new_size);
		}
	}

	void SetInputText(){
		IMText@ text;

		if(font_element is null){
			@text = IMText(ReadParamValue(input_variable), default_font);
		}else{
			@text = IMText(ReadParamValue(input_variable), font_element.font);
		}

		text.setZOrdering(index + 2);
		text.setClip(false);
		text.setRotation(rotation);
		text_holder.setElement(text);
	}

	string ReadParamValue(string key){
		SavedLevel@ data = save_file.GetSavedLevel("drika_data");
		if(input_active == true){return data.GetValue(key) + input_cursor;}
		else {return data.GetValue(key);}
	}
	
	void ReadUIInstruction(array<string> instruction){
		DrikaUIImage::ReadUIInstruction(instruction);

		/* Log(warning, "Got instruction " + instruction[0]); */
		if(instruction[0] == "set_input_image_path"){
			image_path = instruction[1];
			SetNewImage();
		}else if(instruction[0] == "font_changed"){
			@font_element = cast<DrikaUIFont@>(GetUIElement(instruction[1]));
			SetInputText();
			SetSize();
		}else if(instruction[0] == "variable_changed"){
			SetInputText();
			SetSize();
		}else if(instruction[0] == "set_content"){
			input_variable = instruction[1].substr(0, instruction[1].length() - 1);
			SetInputText();
			SetSize();
		}else if(instruction[0] == "input_clicked"){
			SendUIFunctionEvent({"input_clicked"});
		}else if(instruction[0] == "set_color"){
		}else if(instruction[0] == "set_rotation"){
			SetInputText();
			SetSize();
		}else if(instruction[0] == "set_input_max_length"){
			input_max_length = atoi(instruction[1]);
			//Log(fatal, "input_max_length = " + input_max_length + ", value given was " + instruction[1]);
		}else if(instruction[0] == "set_input_box_mode"){
			current_input_box_mode = atoi(instruction[1]);
		}
	}
}
