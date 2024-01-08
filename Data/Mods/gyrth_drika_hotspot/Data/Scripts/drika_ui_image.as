class DrikaUIImage : DrikaUIElement{
	IMImage@ image;
	IMContainer@ outline_container;
	IMContainer@ holder;
	DrikaUIGrabber@ grabber_top_left;
	DrikaUIGrabber@ grabber_top_right;
	DrikaUIGrabber@ grabber_bottom_left;
	DrikaUIGrabber@ grabber_bottom_right;
	DrikaUIGrabber@ grabber_center;
	string image_path;
	ivec2 position;
	ivec2 size;
	float rotation;
	string image_name;
	vec4 color;
	bool keep_aspect;
	ivec2 position_offset;
	ivec2 size_offset;
	bool animated;
	FadeOut@ fade_out;
	AnimatedImage@ animated_image;
	float animation_speed;

	ivec2 max_offset(0, 0);

	DrikaUIImage(JSONValue params = JSONValue()){
		drika_ui_element_type = drika_ui_image;

		image_path = GetJSONString(params, "image_path", "");
		rotation = GetJSONFloat(params, "rotation", 0.0);
		position = GetJSONIVec2(params, "position", ivec2());
		color = GetJSONVec4(params, "color", vec4());
		keep_aspect = GetJSONBool(params, "keep_aspect", false);
		size = GetJSONIVec2(params, "size", ivec2());
		index = GetJSONInt(params, "index", 0);
		hotspot_id = GetJSONInt(params, "hotspot_id", -1);
		animated = GetJSONBool(params, "animated", false);
		animation_speed = GetJSONFloat(params, "animation_speed", 60.0);

		position_offset = GetJSONIVec2(params, "position_offset", ivec2());
		size_offset = GetJSONIVec2(params, "size_offset", ivec2());

		ui_element_identifier = GetJSONString(params, "ui_element_identifier", "");
		PostInit();
	}

	void PostInit(){
		IMImage new_image(image_path);
		@image = new_image;
		ReadMaxOffsets();
		if(size_offset.x == 0.0 || size_offset.y == 0.0){
			size_offset = max_offset;
		}
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
		holder.setElement(image);

		@outline_container = IMContainer("outline_container");
		outline_container.setElement(holder);
		outline_container.setBorderColor(edit_outline_color);
		outline_container.setClip(false);
		outline_container.setZOrdering(index);

		image_container.addFloatingElement(outline_container, image_name, vec2(position.x, position.y), 0);
		new_image.setRotation(rotation);
		new_image.setColor(color);
		SetSize();
		SetOffset();
		imGUI.update();
		UpdateContent();

		if(animated){
			@animated_image = AnimatedImage(image_path, image, animation_speed);
		}
	}

	void Update(){
		if(fade_out !is null){
			if(fade_out.Update()){
				@fade_out = null;
			}
		}

		if(animated_image !is null){
			animated_image.Update();
		}
	}

	void ReadUIInstruction(array<string> instruction){
		/* Log(warning, "Got instruction " + instruction[0]); */
		if(instruction[0] == "set_position"){
			position.x = atoi(instruction[1]);
			position.y = atoi(instruction[2]);
			SetPosition();
		}else if(instruction[0] == "set_size"){
			size.x = atoi(instruction[1]);
			size.y = atoi(instruction[2]);
			SetSize();
		}else if(instruction[0] == "set_position_offset"){
			position_offset.x = atoi(instruction[1]);
			position_offset.y = atoi(instruction[2]);
			SetOffset();
		}else if(instruction[0] == "set_size_offset"){
			size_offset.x = atoi(instruction[1]);
			size_offset.y = atoi(instruction[2]);
			SetOffset();
		}else if(instruction[0] == "set_rotation"){
			rotation = atof(instruction[1]);
			SetRotation();
		}else if(instruction[0] == "set_color"){
			color.x = atof(instruction[1]);
			color.y = atof(instruction[2]);
			color.z = atof(instruction[3]);
			color.a = atof(instruction[4]);
			SetColor();
		}else if(instruction[0] == "set_aspect_ratio"){
			keep_aspect = instruction[1] == "true";
			SetSize();
		}else if(instruction[0] == "set_image_path"){
			image_path = instruction[1];
			SetNewImage();
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
					@fade_out = FadeOut(update_behaviour, identifier, duration, tween_type, @image, preview, color.a);
				}else{
					IMFadeIn new_fade(duration, IMTweenType(tween_type));
					image.addUpdateBehavior(new_fade, update_behaviour + identifier);
				}
			}else if(update_behaviour == "move_in" || update_behaviour == "move_out"){
				int duration = atoi(instruction[2]);
				int tween_type = atoi(instruction[3]);
				string identifier = instruction[4];
				vec2 offset(atoi(instruction[5]), atoi(instruction[6]));

				if(update_behaviour == "move_out"){
					IMMoveIn new_move(duration, offset * -1.0f, IMTweenType(tween_type));
					image.addUpdateBehavior(new_move, update_behaviour + identifier);

					imGUI.update();

					holder.setDisplacement(offset);
				}else{
					IMMoveIn new_move(duration, offset, IMTweenType(tween_type));
					image.addUpdateBehavior(new_move, update_behaviour + identifier);
					imGUI.update();
				}
			}
		}else if(instruction[0] == "remove_update_behaviour"){
			string identifier = instruction[1];

			holder.setDisplacement(vec2());
			if(image.hasUpdateBehavior(identifier)){
				image.removeUpdateBehavior(identifier);
				image.setDisplacement(vec2());
			}

			if(fade_out !is null && identifier == fade_out.name + fade_out.identifier){
				if(fade_out.preview){
					//Check if a fadeout made it completely transparent. Then just set to alpha to the color alpha.
					image.setAlpha(color.a);
				}else{
					level.SendMessage("drika_ui_remove_element " + ui_element_identifier);
				}
				@fade_out = null;
				return;
			}
		}else if(instruction[0] == "set_animated"){
			animated = instruction[1] == "true";
			if(animated){
				@animated_image = AnimatedImage(image_path, image, animation_speed);
			}else{
				animated_image.Delete();
				@animated_image = null;
			}
		}else if(instruction[0] == "set_animation_speed"){
			if(animated_image !is null){
				animation_speed = atof(instruction[1]);
				animated_image.speed = animation_speed;
			}
		}
		UpdateContent();
	}

	void Delete(){
		image_container.removeElement(image_name);
		grabber_top_left.Delete();
		grabber_top_right.Delete();
		grabber_bottom_left.Delete();
		grabber_bottom_right.Delete();
		grabber_center.Delete();
	}

	void SetZOrder(){
		image.setZOrdering(index);
		outline_container.setZOrdering(index);
		grabber_top_left.SetZOrder(index);
		grabber_top_right.SetZOrder(index);
		grabber_bottom_left.SetZOrder(index);
		grabber_bottom_right.SetZOrder(index);
		grabber_center.SetZOrder(index);
	}

	void SetNewImage(){
		vec2 old_size = image.getSize();
		if(animated_image !is null){
			animated_image.Delete();
			@animated_image = null;
		}

		size = ivec2(int(old_size.x), int(old_size.y));
		image.setImageFile(image_path);
		ReadMaxOffsets();
		size_offset = max_offset;
		SetSize();
		SetOffset();

		if(animated){
			@animated_image = AnimatedImage(image_path, image, animation_speed);
		}
	}

	void UpdateContent(){
		vec2 position = image_container.getElementPosition(image_name);
		vec2 size = image.getSize();

		grabber_top_left.SetPosition(position);
		grabber_top_right.SetPosition(position + vec2(size.x, 0));
		grabber_bottom_left.SetPosition(position + vec2(0, size.y));
		grabber_bottom_right.SetPosition(position + vec2(size.x, size.y));
		grabber_center.SetPosition(position);
		grabber_center.SetSize(size);

		grabber_top_left.SetVisible(editing);
		grabber_top_right.SetVisible(editing);
		grabber_bottom_left.SetVisible(editing);
		grabber_bottom_right.SetVisible(editing);
		grabber_center.SetVisible(editing);

		outline_container.showBorder(editing);
		SetZOrder();
	}

	void AddSize(ivec2 added_size, int direction_x, int direction_y){
		if(direction_x == 1){
			if(keep_aspect){
				if(direction_y != -1){
					image.scaleToSizeX(image.getSizeX() + added_size.x);
					size.x += added_size.x;
					size.y = int(image.getSizeY());
				}
			}else{
				size.x += added_size.x;
			}
		}else{
			if(keep_aspect){
				if(direction_y != -1){
					image.scaleToSizeX(image.getSizeX() - added_size.x);
					size.x -= added_size.x;
					image_container.moveElementRelative(image_name, vec2(added_size.x, 0.0));
					position.x += added_size.x;
					size.y = int(image.getSizeY());
				}
			}else{
				size.x -= added_size.x;
				image_container.moveElementRelative(image_name, vec2(added_size.x, 0.0));
				position.x += added_size.x;
			}
		}
		if(direction_y == 1){
			if(!keep_aspect){
				size.y += added_size.y;
			}
		}else{
			if(keep_aspect){
				if(direction_x == 1){
					image.scaleToSizeY(image.getSizeY() - added_size.y);
					size.y -= added_size.y;
					image_container.moveElementRelative(image_name, vec2(0.0, added_size.y));
					position.y += added_size.y;
					size.x = int(image.getSizeX());
				}else{
					float size_x_before = image.getSizeX();
					image.scaleToSizeY(image.getSizeY() - added_size.y);
					size.y -= added_size.y;
					float moved_x;
					//The image will start drifting if the float value isn't rounded.
					if(size_x_before > 0.0f){
						moved_x = ceil(size_x_before - image.getSizeX());
					}else{
						moved_x = floor(size_x_before - image.getSizeX());
					}
					image_container.moveElementRelative(image_name, vec2(moved_x, added_size.y));
					position.y += added_size.y;
					size.x = int(image.getSizeX());
				}
			}else{
				size.y -= added_size.y;
				image_container.moveElementRelative(image_name, vec2(0.0, added_size.y));
				position.y += added_size.y;
			}
		}
		SetSize();
		UpdateContent();
		SendUIInstruction("set_size", {size.x, size.y});
	}

	void SetPosition(){
		image_container.moveElement(image_name, vec2(position.x, position.y));
	}

	void SetSize(){
		vec2 new_size = vec2(size.x, size.y);
		image.setSize(new_size);
		if(outline_container !is null){
			outline_container.setSize(new_size);
		}
		if(holder !is null){
			holder.setSize(new_size);
		}
	}

	void AddPosition(ivec2 added_positon){
		image_container.moveElementRelative(image_name, vec2(added_positon.x, added_positon.y));
		position += added_positon;
		UpdateContent();
		SendUIInstruction("set_position", {position.x, position.y});
	}

	DrikaUIGrabber@ GetGrabber(string grabber_name){
		if(grabber_name == "top_left"){
			return grabber_top_left;
		}else if(grabber_name == "top_right"){
			return grabber_top_right;
		}else if(grabber_name == "bottom_left"){
			return grabber_bottom_left;
		}else if(grabber_name == "bottom_right"){
			return grabber_bottom_right;
		}else if(grabber_name == "center"){
			return grabber_center;
		}else{
			return null;
		}
	}

	void AddUpdateBehavior(IMUpdateBehavior@ behavior, string name){
		image.addUpdateBehavior(behavior, name);
	}

	void RemoveUpdateBehavior(string name){
		image.removeUpdateBehavior(name);
	}

	void ReadMaxOffsets(){
		max_offset = ivec2(int(image.getSizeX()),int(image.getSizeY()));
	}

	void SetOffset(){
		image.setImageOffset(vec2(position_offset.x, position_offset.y), vec2(size_offset.x, size_offset.y));
	}

	void SetRotation(){
		image.setRotation(rotation);
	}

	void SetColor(){
		image.setColor(color);
	}

	void SetEditing(bool _editing){
		editing = _editing;
		UpdateContent();
	}
}
