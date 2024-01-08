class DrikaUIFont : DrikaUIElement{
	string font_name;
	int font_size;
	vec4 font_color;
	bool shadowed;
	array<DrikaUIText@> text_elements;
	FontSetup font("edosz", 75, HexColor("#CCCCCC"), true);

	DrikaUIFont(JSONValue params = JSONValue()){
		drika_ui_element_types drika_ui_element_type = drika_ui_font;

		font_name = GetJSONString(params, "font_name", "");
		font_size = GetJSONInt(params, "font_size", 0);
		font_color = GetJSONVec4(params, "font_color", vec4());
		shadowed = GetJSONBool(params, "shadowed", false);
		hotspot_id = GetJSONInt(params, "hotspot_id", -1);

		font.fontName = font_name;
		font.size = font_size;
		font.color = font_color;
		font.shadowed = shadowed;
		ui_element_identifier = GetJSONString(params, "ui_element_identifier", "");
	}

	void ReadUIInstruction(array<string> instruction){
		/* Log(warning, "UIFont got instruction " + instruction[0]); */
		if(instruction[0] == "set_font"){
			font_name = instruction[1];
			font.fontName = font_name;
		}else if(instruction[0] == "set_font_color"){
			font_color.x = atof(instruction[1]);
			font_color.y = atof(instruction[2]);
			font_color.z = atof(instruction[3]);
			font_color.a = atof(instruction[4]);
			font.color = font_color;
		}else if(instruction[0] == "set_shadowed"){
			shadowed = instruction[1] == "true";
			font.shadowed = shadowed;
		}else if(instruction[0] == "set_font_size"){
			font_size = atoi(instruction[1]);
			font.size = font_size;
		}
	}
}
