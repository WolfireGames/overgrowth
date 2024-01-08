
class DocData{
	string data;
	string file_path;
	string hierarchy_path;
	array<string> split_path;
	int index;
	string name;
	int function_color;

	DocData(string _file_path, string _hierarchy_path, int _function_color = drika_comment){
		file_path = _file_path;
		hierarchy_path = _hierarchy_path;
		index = doc_index;
		doc_index += 1;
		function_color = _function_color;

		split_path = hierarchy_path.split("+");
		name = split_path[split_path.size() - 1];
	}

	void LoadDoc(){
		if(FileExists(file_path)){
			level.SendMessage("drika_read_file " + hotspot.GetID() + " " + index + " " + file_path + " " + "doc");
		}
	}
}

class DocImage{
	string path;
	string name;
	string image_title;
	TextureAssetRef texture;

	DocImage(string _path, string _name, string _image_title, TextureAssetRef _texture){
		path = _path;
		name = _name;
		image_title = _image_title;
		texture = _texture;
	}
}

int doc_index = 0;
DocData @current_page = null;
float sub_page_indent = 20.0f;
array<DocImage@> doc_images = {};
array<DocData@> doc_data = {	DocData("Data/Docs/about.md", "About"),
								DocData("Data/Docs/functions.md", "Functions"),
								DocData("Data/Docs/animation.md", "Functions+Animation", drika_animation),
								DocData("Data/Docs/on_enter_exit.md", "Functions+On Enter/Exit", drika_on_enter_exit),
								DocData("Data/Docs/billboard.md", "Functions+Billboard", drika_billboard),
								DocData("Data/Docs/character_control.md", "Functions+Character Control", drika_character_control),
								DocData("Data/Docs/user_interface.md", "Functions+User Interface", drika_user_interface)
							};

void OpenDocPage(string page_name){
	for(uint i = 0; i < doc_data.size(); i++){
		if(doc_data[i].name == page_name){
			@current_page = doc_data[i];
			break;
		}
	}
}

void LoadDocs(){
	for(uint i = 0; i < doc_data.size(); i++){
		doc_data[i].LoadDoc();
	}
}

void SetDocData(string data, int index){
	doc_data[index].data = data;
}

void DrawDocsModal(){
	if(@current_page == null){
		LoadDocs();
		OpenDocPage("About");
	}

	ImGui_SetNextWindowSize(vec2(1200.0f, 900.0f), ImGuiSetCond_Once);
	ImGui_PushStyleVar(ImGuiStyleVar_WindowMinSize, vec2(350, 350));

	if(ImGui_BeginPopupModal("Docs", 0)){
		ImGui_BeginChild("Docsss", vec2(-1.0, -1.0), false);

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, 180.0);
		/* ImGui_NextColumn(); */

		ImGui_BeginChild("Index", vec2(-1.0, -1.0), true);

		for(uint i = 0; i < doc_data.size(); i++){
			int indent_amount = doc_data[i].split_path.size() - 1;

			for(int j = 0; j < indent_amount; j++){
				ImGui_Indent(sub_page_indent);
			}

			ImGui_PushStyleColor(ImGuiCol_Text, display_colors[doc_data[i].function_color]);
			if(ImGui_Selectable(doc_data[i].name, current_page.name == doc_data[i].name, ImGuiSelectableFlags_DontClosePopups)){
				OpenDocPage(doc_data[i].name);
			}
			ImGui_PopStyleColor();

			for(int j = 0; j < indent_amount; j++){
				ImGui_Unindent(sub_page_indent);
			}
		}

		ImGui_EndChild();

		ImGui_NextColumn();

		ImGui_BeginChild("Content", vec2(-1.0, -1.0), true);

		array<string> data_lines = current_page.data.split("\n");
		for(uint i = 0; i < data_lines.size(); i++){
			if(CheckForHeaders(data_lines[i])){

			}else if(CheckForImage(data_lines[i])){

			}else{
				ImGui_TextWrapped(data_lines[i]);
			}
		}

		ImGui_EndChild();

		ImGui_EndChild();

		vec4 window_info = GetWindowInfo();

		ImGui_EndPopup();

		CheckClosePopup(window_info);
	}
	ImGui_PopStyleVar();
}

bool CheckForHeaders(string text){
	if(text.substr(0, 4) == "### "){
		text.erase(0, 4);
		ImGui_SetWindowFontScale(1.75f);
		ImGui_TextWrapped(text);
		ImGui_SetWindowFontScale(1.0f);
		return true;
	}else if(text.substr(0, 3) == "## "){
		text.erase(0, 3);
		ImGui_SetWindowFontScale(1.5f);
		ImGui_TextWrapped(text);
		ImGui_SetWindowFontScale(1.0f);
		return true;
	}else if(text.substr(0, 2) == "# "){
		text.erase(0, 2);
		ImGui_SetWindowFontScale(1.25f);
		ImGui_TextWrapped(text);
		ImGui_SetWindowFontScale(1.0f);
		return true;
	}

	return false;
}

bool CheckForImage(string text){
	if(text.substr(0, 2) == "!["){
		text.erase(0, 2);

		string alt_text;
		int close_bracket_pos = text.findFirst("]");
		if(close_bracket_pos != -1){
			alt_text = text.substr(0, close_bracket_pos - 1);

			for(uint i = 0; i < doc_images.size(); i++){
				if(doc_images[i].name == alt_text){
					ImGui_Image(doc_images[i].texture, vec2(622.0, 350.0));
					if(ImGui_IsItemHovered()){
						ImGui_PushStyleColor(ImGuiCol_PopupBg, titlebar_color);
						ImGui_SetTooltip(doc_images[i].image_title);
						ImGui_PopStyleColor();
					}
					return true;
				}
			}

			text.erase(0, close_bracket_pos);

			int open_parentaces_pos = text.findFirst("(");
			int close_parentaces_pos = text.findFirst(")");
			if(open_parentaces_pos != -1 && close_parentaces_pos != -1){
				text.erase(0, open_parentaces_pos + 1);

				string image_path;
				for(uint i = 0; i < text.length(); i++){
					string letter = text.substr(i, 1);
					if(letter == " " || letter == ")"){
						ImGui_TextWrapped(image_path);
						break;
					}
					image_path += text.substr(i, 1);
				}

				string image_title = "";
				int first_quote_pos = text.findFirst("\"");
				int second_quote_pos;

				if(first_quote_pos != -1){
					second_quote_pos = text.findFirst("\"", first_quote_pos + 1);
					if(second_quote_pos != -1){
						image_title = text.substr(first_quote_pos + 1, second_quote_pos - first_quote_pos - 1);
					}
				}

				if(FileExists(image_path)){
					TextureAssetRef texture = LoadTexture(image_path, TextureLoadFlags_NoMipmap | TextureLoadFlags_NoConvert |TextureLoadFlags_NoReduce);
					doc_images.insertLast(DocImage(image_path, alt_text, image_title, texture));
				}
			}

			ImGui_TextWrapped(alt_text);
			return true;
		}
	}

	return false;
}
