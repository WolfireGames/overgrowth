class DrikaUIButton : DrikaUIImage{

	string button_image_path;
	string button_text;
	DrikaUIFont@ font_element = null;
	IMContainer@ text_holder;
	vec4 mouseover_color;
	float mouseover_effect;

	DrikaUIButton(JSONValue params = JSONValue()){
		button_image_path = GetJSONString(params, "button_image_path", "");
		button_text = GetJSONString(params, "button_text", "");
		@font_element = cast<DrikaUIFont@>(GetUIElement(GetJSONString(params, "font_id", "")));
		mouseover_effect = 0.35;

		super(params);

		drika_ui_element_type = drika_ui_button;
	}

	void PostInit(){
		mouseover_color = vec4(color.x + mouseover_effect, color.y + mouseover_effect, color.z + mouseover_effect, color.a + mouseover_effect);

		IMImage new_image(button_image_path);
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

		SetButtonText();
		holder.addFloatingElement(text_holder, "text_holder", vec2(0.0, 0.0), index + 1);

		@outline_container = IMContainer("outline_container");
		outline_container.setElement(holder);
		outline_container.setBorderColor(edit_outline_color);
		outline_container.setClip(false);
		outline_container.setZOrdering(index);

		holder.sendMouseOverToChildren(true);

		IMMessage on_click("drika_button_go_to_line");
		IMMessage on_hover_enter("drika_button_hover_enter");
	    IMMessage nil_message("");
		on_click.addString(ui_element_identifier);
		on_hover_enter.addString(ui_element_identifier);
		outline_container.addLeftMouseClickBehavior(IMFixedMessageOnClick(on_click), "");
		outline_container.addMouseOverBehavior(IMFixedMessageOnMouseOver(on_hover_enter, nil_message, nil_message), "");

		image_container.addFloatingElement(outline_container, image_name, vec2(position.x, position.y), 0);
		new_image.setRotation(rotation);
		new_image.setColor(color);
		SetSize();
		imGUI.update();
		UpdateContent();
		AddMouseOverBehavior();
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

		if(text_holder !is null){
			text_holder.setSize(new_size);
		}
	}

	void SetButtonText(){
		IMText@ text;
		vec4 text_mouseover_color;

		if(font_element is null){
			@text = IMText(ReplaceVariablesFromText(button_text), default_font);
			text_mouseover_color = default_font.color;
		}else{
			@text = IMText(ReplaceVariablesFromText(button_text), font_element.font);
			text_mouseover_color = font_element.font.color;
		}

		text_mouseover_color.x += mouseover_effect;
		text_mouseover_color.y += mouseover_effect;
		text_mouseover_color.z += mouseover_effect;
		text_mouseover_color.a += mouseover_effect;

		IMMouseOverPulseColor mouseover(text_mouseover_color, text_mouseover_color, inQuartTween);
		text.addMouseOverBehavior(mouseover, "");
		text.setZOrdering(index + 2);
		text.setClip(false);
		text.setRotation(rotation);
		text_holder.setElement(text);
	}

	void AddMouseOverBehavior(){
		image.removeMouseOverBehavior("mouseover");
		IMMouseOverPulseColor mouseover(mouseover_color, mouseover_color, inQuartTween);
		image.addMouseOverBehavior(mouseover, "mouseover");
	}

	void ReadUIInstruction(array<string> instruction){
		DrikaUIImage::ReadUIInstruction(instruction);

		// Log(warning, "Got instruction " + instruction[0]);

		if(instruction[0] == "set_button_image_path"){
			image_path = instruction[1];
			SetNewImage();
		}else if(instruction[0] == "font_changed"){
			@font_element = cast<DrikaUIFont@>(GetUIElement(instruction[1]));
			SetButtonText();
			SetSize();
		}else if(instruction[0] == "variable_changed"){
			SetButtonText();
			SetSize();
		}else if(instruction[0] == "set_content"){
			button_text = instruction[1].substr(0, instruction[1].length() - 1);
			SetButtonText();
			SetSize();
		}else if(instruction[0] == "button_clicked"){
			SendUIFunctionEvent({"button_clicked"});
		}else if(instruction[0] == "button_hovered"){
			SendUIFunctionEvent({"button_hovered"});
		}else if(instruction[0] == "set_color"){
			mouseover_color = vec4(color.x + mouseover_effect, color.y + mouseover_effect, color.z + mouseover_effect, color.a + mouseover_effect);
			AddMouseOverBehavior();
		}else if(instruction[0] == "set_rotation"){
			SetButtonText();
			SetSize();
		}
	}
}
