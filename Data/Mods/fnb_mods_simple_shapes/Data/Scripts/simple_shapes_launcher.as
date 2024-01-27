#include "ss_materials.as"
#include "ss_shapes.as"
											
string path_to_icons = "Data/Custom/FnB/Textures/menu_icons/";

string current_object_path;

int current_material_choice;
int icon_size = 48;
int padding = 10;

bool draw_simple_shapes_window = false;
bool close_on_create = false;

IMGUI@ imGUI;

class MaterialEntry{
	string title_name;
	string folder_name;
	
	MaterialEntry(string titlename, string foldername){
		title_name = titlename;
		folder_name = foldername;
	}
}

class ShapeEntry{
	string file_name;
	string icon_file_name;
	string folder_location;
	
	ShapeEntry(string filename, string iconfilename, string folderlocation){
		file_name = filename;
		icon_file_name = iconfilename;
		folder_location = folderlocation;
	}
}

void Init(string str){
	icon_size = GetConfigValueInt("ss_icon_size");
	if(icon_size < 16){icon_size = 48;}
	close_on_create = GetConfigValueBool("ss_close_on_create");
	@imGUI = CreateIMGUI();
	imGUI.setup();
	imGUI.setBackgroundLayers(1);
	imGUI.getMain().setZOrdering(-1);
}

void Update(int paused){
	if(EditorModeActive()){
		if(GetInputDown(0, "lctrl") && GetInputPressed(0, "b") && !GetInputDown(0, "attack")){
			draw_simple_shapes_window == false ? draw_simple_shapes_window = true : draw_simple_shapes_window = false;
		};
		
		if(current_object_path != "" && GetInputPressed(0, "attack")){
			SpawnObject(current_object_path);
			current_object_path = "";
		}else if(GetInputPressed(0, "grab") || GetInputPressed(0, "attack")){
			current_object_path = "";
		}
	}
}

void Menu(){
	if(ImGui_BeginMenu("Simple Shapes")){
		
		ImGui_AlignTextToFramePadding();
		
		if(ImGui_Button("Launch Simple Shapes Menu",vec2(200,16))){
			draw_simple_shapes_window == false ? draw_simple_shapes_window = true : draw_simple_shapes_window = false;
		}
		
		ImGui_Separator();
		
		ImGui_Text("Press CTRL + B in the editor to quickly open the menu.");
		
		ImGui_EndMenu();
	}
}

void DrawGUI(){
		imGUI.render();
		array<string> material_names;
		
		vec2 draw_posx = vec2(ImGui_GetMousePos().x + 20, ImGui_GetMousePos().y + 20);
		vec2 draw_posy = vec2(ImGui_GetMousePos().x + 17, ImGui_GetMousePos().y + 23);
		
		for(uint i = 0; i < material_entries.size(); ++i){
		material_names.insertLast(material_entries[i].title_name);
		}
		if(current_object_path != ""){
			imGUI.drawBox(draw_posx,vec2(2,8),vec4(1.0, 1.0, 1.0, 1.0),0,false);
			imGUI.drawBox(draw_posy,vec2(8,2),vec4(1.0, 1.0, 1.0, 1.0),0,false);
		}else{
			imGUI.clear();
		}
		if(draw_simple_shapes_window == true){
			ImGui_Begin("Simple Shapes Menu (press B to open/close)", draw_simple_shapes_window);
			ImGui_Columns(3,false);
			ImGui_AlignTextToFramePadding();
			
			if(ImGui_Combo("Materials", current_material_choice, material_names, material_names.size())){
			}
			ImGui_NextColumn();
			if(ImGui_SliderInt("Icon Size",icon_size,16,128,"%.0f")){
				SetConfigValueInt("ss_icon_size", icon_size);
			}
			ImGui_NextColumn();
			if(ImGui_Checkbox("Close After Creating Object",close_on_create)){
				SetConfigValueBool("ss_close_on_create", close_on_create);
			}
			ImGui_NextColumn();
			ImGui_Separator();
			
			ImGui_Columns(1,false);
			if(!ImGui_IsWindowCollapsed()){
				if(ImGui_BeginChildFrame(55, vec2(-1, -1), ImGuiWindowFlags_AlwaysAutoResize)){
			
					ImGui_BeginChild("start", vec2(ImGui_GetWindowWidth(), icon_size + padding), false, ImGuiWindowFlags_NoScrollWithMouse);
					float row_size = 0.0f;
					
					for(uint i = 0; i < shape_entries.size(); ++i){
						row_size += icon_size + padding;
						string displayed_icon_path = path_to_icons + shape_entries[i].icon_file_name;
						TextureAssetRef displayed_icon = LoadTexture(displayed_icon_path, TextureLoadFlags_NoMipmap | TextureLoadFlags_NoConvert |TextureLoadFlags_NoReduce);
						
						if(row_size > ImGui_GetWindowWidth()){
							row_size = icon_size + padding;
							ImGui_EndChild();
							ImGui_BeginChild("child " + i, vec2(ImGui_GetWindowWidth(), icon_size + padding), false, ImGuiWindowFlags_NoScrollWithMouse);
						}
						ImGui_SameLine();
						
						//ImGui_PushStyleColor(ImGuiCol_Button, vec4(1));
						ImGui_PushStyleColor(ImGuiCol_ButtonActive, vec4(0.2));
						ImGui_PushStyleColor(ImGuiCol_ButtonHovered, vec4(0.75));
						if(ImGui_ImageButton(displayed_icon, vec2(icon_size), vec2(0), vec2(1), 0, vec4(0), vec4(1))){
							current_object_path = shape_entries[i].folder_location + material_entries[current_material_choice].folder_name + "/" + shape_entries[i].file_name + ".xml";
						}
						
						ImGui_PopStyleColor(2);
					}
					
					ImGui_EndChild();
					ImGui_EndChildFrame();
				}
			}
			
			ImGui_End();
		}
}
//The following function is ripped straight from the spawner mod with only minor changes to make it behave more like vanilla spawning.
void SpawnObject(string load_item_path){
	if(FileExists(load_item_path)){
		Log(warning, "Creating simple shapes object " + load_item_path);
		int spawn_id = CreateObject(load_item_path, false);
		Object@ obj = ReadObjectFromID(spawn_id);
		vec3 spawn_position = col.GetRayCollision(camera.GetPos(), camera.GetPos() + (camera.GetMouseRay() * 500.0f));
		obj.SetCopyable(true);
		obj.SetSelectable(true);
		obj.SetTranslatable(true);
		obj.SetScalable(true);
		obj.SetRotatable(true);
		obj.SetDeletable(true);
		obj.SetTranslation(spawn_position);
		DeselectAll();
		current_object_path = "";
		//obj.SetSelected(true);
	}else{
		DisplayError("Error", "This xml file does not exist: " + load_item_path);
		current_object_path = "";
	}
	
	if(close_on_create == true){
		draw_simple_shapes_window = false;
	}
}