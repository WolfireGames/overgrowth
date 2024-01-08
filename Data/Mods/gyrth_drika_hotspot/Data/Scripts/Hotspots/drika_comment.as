class DrikaComment : DrikaElement{
	string display_comment;

	DrikaComment(JSONValue params = JSONValue()){
		display_comment = GetJSONString(params, "display_comment", "");
		drika_element_type = drika_comment;
		has_settings = true;
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["display_comment"] = JSONValue(display_comment);
		return data;
	}

	string GetDisplayString(){
		array<string> split_comment = display_comment.split("\n");
		return "Comment: " + split_comment[0];
	}

	void StartSettings(){
		ImGui_SetTextBuf(display_comment);
	}

	void DrawSettings(){
		if(ImGui_InputTextMultiline("##TEXT", vec2(-1.0, -1.0))){
			display_comment = ImGui_GetTextBuf();
		}
	}
	bool Trigger(){
		return true;
	}
}
