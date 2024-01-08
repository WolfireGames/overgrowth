class DrikaUIFullscreenImage : DrikaUIElement{
	string fullscreen_image_path;
	string image_name;
	vec4 color;
	float fade_value = 1.0f;
	bool fading_out = false;
	bool fading_in = false;
	float fade_in_duration = 1.0f;
	float fade_out_duration = 1.0f;
	float timer = 0.0;
	int fade_in_tween_type;
	int fade_out_tween_type;
	string fade_in_identifier;
	string fade_out_identifier;
	bool fade_in_preview = false;
	bool fade_out_preview = false;
	float previous_ui_time;
	bool ready = false;

	DrikaUIFullscreenImage(JSONValue params = JSONValue()){
		drika_ui_element_type = drika_ui_fullscreen_image;

		fullscreen_image_path = GetJSONString(params, "fullscreen_image_path", "");
		color = GetJSONVec4(params, "color", vec4());
		hotspot_id = GetJSONInt(params, "hotspot_id", -1);

		ui_element_identifier = GetJSONString(params, "ui_element_identifier", "");
		previous_ui_time = ui_time;
	}

	void Update(){
		ready = true;
	}

	void Draw(){
		if(!ready){return;}

		if(fading_out){
			float step = (ui_time - previous_ui_time);
			timer += step;

			if(timer >= fade_out_duration){
				fading_out = false;
				//Don't remove the UIELement when DHS is editing and previewing the transitions.
				if(fade_out_preview){
					timer = fade_out_duration;
				}else{
					level.SendMessage("drika_ui_remove_element " + ui_element_identifier);
				}
			}

			fade_value = mix(color.a, 0.0f, ApplyTween((timer / fade_out_duration), IMTweenType(fade_out_tween_type)));
		}else if(fading_in){
			float step = (ui_time - previous_ui_time);
			timer += step;

			if(timer >= fade_in_duration){
				fading_in = false;
				if(fade_in_preview){
					timer = fade_in_duration;
				}
			}

			fade_value = mix(0.0f, color.a, ApplyTween((timer / fade_in_duration), IMTweenType(fade_in_tween_type)));
		}

		previous_ui_time = ui_time;

		float width = GetScreenWidth();
		float height = GetScreenHeight();

		HUDImage @fullscreen_image = hud.AddImage();
		fullscreen_image.SetImageFromPath(fullscreen_image_path);
		fullscreen_image.position.y = 0.0f;
		fullscreen_image.position.x = 0.0f;
		fullscreen_image.position.z = -5.0f;
		fullscreen_image.scale = vec3(width / fullscreen_image.GetWidth(), height / fullscreen_image.GetHeight(), 1.0);
		fullscreen_image.color = vec4(color.x, color.y, color.z, color.a * fade_value);
	}

	void ReadUIInstruction(array<string> instruction){
		// Log(warning, "Got instruction " + instruction[0]);
		if(instruction[0] == "set_color"){
			color.x = atof(instruction[1]);
			color.y = atof(instruction[2]);
			color.z = atof(instruction[3]);
			color.a = atof(instruction[4]);
		}else if(instruction[0] == "set_image_path"){
			fullscreen_image_path = instruction[1];
		}else if(instruction[0] == "set_z_order"){
			
		}else if(instruction[0] == "add_update_behaviour"){
			string update_behaviour = instruction[1];
			if(update_behaviour == "fade_in"){
				timer = 0.0f;
				fading_in = true;
				fading_out = false;
				fade_in_duration = atoi(instruction[2]) / 1000.0f;
				fade_in_tween_type = atoi(instruction[3]);
				fade_in_identifier = instruction[4];
				fade_in_preview = (instruction[7] == "true");
			}else if(update_behaviour == "fade_out"){
				fading_out = true;
				fading_in = false;
				fade_out_duration = atoi(instruction[2]) / 1000.0f;
				fade_out_tween_type = atoi(instruction[3]);
				fade_out_identifier = instruction[4];
				fade_out_preview = (instruction[7] == "true");
				timer = 0.0f;
			}
		}else if(instruction[0] == "remove_update_behaviour"){
			string update_behaviour = instruction[1];

			if(update_behaviour == "fade_in"){
				fading_in = false;
			}else if(update_behaviour == "fade_out"){
				fading_out = false;
			}
		}
	}

	void Delete(){
		
	}

	void AddUpdateBehavior(IMUpdateBehavior@ behavior, string name){
		
	}

	void RemoveUpdateBehavior(string name){
		
	}

	void SetEditing(bool _editing){
		editing = _editing;
	}
}
