enum drika_ui_element_types	{
								none,
								drika_ui_image,
								drika_ui_text,
								drika_ui_font,
								drika_ui_button,
								drika_ui_input,
								drika_ui_fullscreen_image
							};

vec4 edit_outline_color = vec4(0.5, 0.5, 0.5, 1.0);

class DrikaUIElement{
	drika_ui_element_types drika_ui_element_type = none;
	bool visible;
	string ui_element_identifier;
	bool editing;
	int index = 0;
	string json_string;
	int hotspot_id;

	void Update(){}
	void Draw(){}
	void AddPosition(ivec2 added_positon){}
	void AddSize(ivec2 added_size, int direction_x, int direction_y){}
	DrikaUIGrabber@ GetGrabber(string grabber_name){return null;}
	void AddUpdateBehavior(IMUpdateBehavior@ behavior, string name){};
	void RemoveUpdateBehavior(string behavior_name){};
	void SetEditing(bool editing){}
	void Delete(){}
	void SetIndex(int _index){}
	bool SetVisible(bool _visible){
		visible = _visible;
		return visible;
	}
	void SelectAgain(){}
	void RefreshTarget(){}
	void ParseInput(bool left_mouse, bool right_mouse){}
	void ReadUIInstruction(array<string> instruction){}

	void SendUIInstruction(string param_1, array<string> params){
		if(hotspot_id != -1 && ObjectExists(hotspot_id)){
			Object@ hotspot_obj = ReadObjectFromID(hotspot_id);

			string msg = "drika_ui_instruction ";
			msg += param_1 + " ";
			for(uint i = 0; i < params.size(); i++){
				msg += params[i] + " ";
			}

			hotspot_obj.ReceiveScriptMessage(msg);
		}
	}

	void SendUIFunctionEvent(array<string> params){
		if(hotspot_id != -1 && ObjectExists(hotspot_id)){
			Object@ hotspot_obj = ReadObjectFromID(hotspot_id);

			string msg = "drika_ui_function_event ";
			msg += ui_element_identifier + " ";
			for(uint i = 0; i < params.size(); i++){
				msg += params[i] + " ";
			}

			hotspot_obj.ReceiveScriptMessage(msg);
		}
	}
}

class FadeOut{
	string name;
	string identifier;
	float timer;
	float duration;
	IMElement @target;
	IMTweenType tween_type;
	bool preview;
	float starting_alpha;
	float previous_ui_time;

	FadeOut(string _name, string _identifier, float _duration, int _tween_type, IMElement@ _target, bool _preview, float _starting_alpha){
		name = _name;
		identifier = _identifier;
		duration = _duration / 1000.0f;
		tween_type = IMTweenType(_tween_type);
		preview = _preview;
		@target = @_target;
		starting_alpha = _starting_alpha;
		previous_ui_time = ui_time;
	}

	bool Update(){
		float step = (ui_time - previous_ui_time);
		timer += step;
		previous_ui_time = ui_time;

		if(timer >= duration){
			//Don't remove the UIELement when DHS is editing and previewing the transitions.
			if(preview){
				timer = duration;
				target.setAlpha(mix(starting_alpha, 0.0f, ApplyTween((timer / duration), tween_type)));
				return false;
			}else{
				level.SendMessage("drika_ui_remove_element " + identifier);
				return true;
			}
		}

		target.setAlpha(mix(starting_alpha, 0.0f, ApplyTween((timer / duration), tween_type)));
		return false;
	}

	bool Remove(){
		if(preview){
			//Check if a fadeout made it completely transparent. Then just set to alpha to the color alpha.
			target.setAlpha(starting_alpha);
			return false;
		}else{
			return true;
		}
	}
}

class AnimatedImage{
	string image_path;
	int range_min;
	int range_max;
	IMImage@ image;
	bool valid;
	string base_path;
	string extention;
	int integer_padding;
	float timer;
	int image_index;
	float speed;
	float previous_ui_time;

	AnimatedImage(string _image_path, IMImage@ _image, float _speed){
		image_path = _image_path;
		@image = @_image;
		speed = _speed;
		previous_ui_time = ui_time;

		array<string> split_path = image_path.split("_");
		if(split_path.size() >= 2){
			for(int i = 0; i < int(split_path.size()) - 1; i++){
				base_path += split_path[i];
			}
			base_path += "_";

			array<string> extention_split = split_path[split_path.size() - 1].split(".");
			extention = "." + extention_split[extention_split.size() - 1];

			//Animated files can either start with 0 or 1, check for both.
			for(int i = 0; i < 2; i++){
				for(int j = 1; j <= 5; j++){

					integer_padding = j;
					string check_path = "Data/" + base_path + ApplyPadding(i) + extention;
					if(FileExists(check_path)){
						// Log(warning, "Valid " + check_path);
						range_min = i;
						//Now find the highest number of frame.
						for(int k = range_min; k <= 500; k++){
							check_path = "Data/" + base_path + ApplyPadding(k) + extention;
							if(!FileExists(check_path)){
								range_max = k - 1;
								valid = true;
								image_index = range_min;
								return;
							}
						}
						return;
					}
				}
			}
		}
		Log(error, "Invalid image for animation : " + image_path);
	}

	string ApplyPadding(int value){
		string out_value;
		out_value = formatInt(value, '0', integer_padding);
		return out_value;
	}

	void Delete(){
		SetNewImage(image_path);
	}

	void Update(){
		if(valid){
			float step = (ui_time - previous_ui_time);
			timer += step;
			previous_ui_time = ui_time;

			if(timer > (1.0f / max(0.001, speed))){
				timer = 0.0f;
				image_index += 1;
				if(image_index > range_max){
					image_index = range_min;
				}
				SetNewImage(base_path + ApplyPadding(image_index) + extention);
			}
		}
	}

	void SetNewImage(string new_path){
		vec2 old_size = image.getSize();
		image.setImageFile(new_path);
		image.setSize(old_size);
	}
}
