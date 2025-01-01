enum target_options {	id_option = (1<<0),
						reference_option = (1<<1),
						team_option = (1<<2),
						name_option = (1<<3),
						character_option = (1<<4),
						item_option = (1<<5),
						batch_option = (1<<6),
						camera_option = (1<<7),
						box_select_option = (1<<8),
						any_character_option = (1<<9),
						any_player_option = (1<<10),
						any_npc_option = (1<<11),
						any_item_option = (1<<12)
					};

class BatchObject{
	string text;
	vec4 color;
	int id;

	BatchObject(int _id, string _text, vec4 _color){
		text = _text;
		color = _color;
		id = _id;
	}
}

/* vec4(156, 255, 159, 255),
vec4(153, 255, 193, 255),
vec4(255, 0, 255, 255),
vec4(0, 149, 255, 255), */

vec4 GetBatchObjectColor(int object_type){
	switch(object_type){
		case _env_object:
			return vec4(230, 184, 175, 255);
		case _movement_object:
			return vec4(255, 153, 0, 255);
		case _spawn_point:
			return vec4(0, 243, 255, 255);
		case _decal_object:
			return vec4(0, 173, 182, 255);
		case _hotspot_object:
			return vec4(0, 255, 0, 255);
		case _group:
			return vec4(0, 255, 149, 255);
		case _item_object:
			return vec4(255, 255, 0, 255);
		case _path_point_object:
			return vec4(255, 0, 0, 255);
		case _ambient_sound_object:
			return vec4(150, 0, 0, 255);
		case _placeholder_object:
			return vec4(221, 126, 107, 255);
		case _light_probe_object:
			return vec4(150, 0, 150, 255);
		case _dynamic_light_object:
			return vec4(0, 113, 194, 255);
		case _navmesh_hint_object:
			return vec4(0, 43, 255, 255);
		case _navmesh_region_object:
			return vec4(126, 139, 226, 255);
		case _navmesh_connection_object:
			return vec4(0, 182, 0, 255);
		case _reflection_capture_object:
			return vec4(162, 250, 255, 255);
		case _light_volume_object:
			return vec4(203, 200, 255, 255);
		case _prefab:
			return vec4(0, 173, 101, 255);
		default :
			break;
	}
	return vec4(1.0);
}

string GetObjectTypeString(int object_type){
	switch(object_type){
		case _env_object:
			return "EnvObject";
		case _movement_object:
			return "MovementObject";
		case _spawn_point:
			return "SpawnPoint";
		case _decal_object:
			return "DecalObject";
		case _hotspot_object:
			return "Hotspot";
		case _group:
			return "Group";
		case _item_object:
			return "ItemObject";
		case _path_point_object:
			return "PathPointObject";
		case _ambient_sound_object:
			return "AmbientSound";
		case _placeholder_object:
			return "Placeholder";
		case _light_probe_object:
			return "LightProbe";
		case _dynamic_light_object:
			return "DynamicLight";
		case _navmesh_hint_object:
			return "NavMeshHint";
		case _navmesh_region_object:
			return "NavMeshRegion";
		case _navmesh_connection_object:
			return "NavMeshConnection";
		case _reflection_capture_object:
			return "ReflectionCapture";
		case _light_volume_object:
			return "LightVolume";
		case _prefab:
			return "Prefab";
		default :
			break;
	}
	return "NA";
}

class DrikaTargetSelect{
	int object_id = -1;
	string reference_string = "drika_reference";
	string character_team = "team_drika";
	string object_name = "drika_object";

	string identifier_type_tag = "identifier_type";
	string identifier_tag = "identifier";
	string tag = "";

	identifier_types identifier_type;
	array<string> available_references;
	array<string> available_character_names;
	array<int> available_character_ids;
	array<string> available_item_names;
	array<int> available_item_ids;
	array<int> batch_ids;
	int target_option;
	DrikaElement@ parent;
	array<BatchObject@> batch_objects;
	DrikaPlaceholder box_select_placeholder();
	bool include_envobject;
	bool include_group;
	bool include_movement_object;
	bool include_item_object;
	bool include_hotspot;

	DrikaTargetSelect(DrikaElement@ _parent, JSONValue params, string tag = ""){
		@parent = _parent;

		if(tag != ""){
			this.tag = tag;
			identifier_type_tag = "identifier_type_" + tag;
			identifier_tag = "identifier_" + tag;
		}

		box_select_placeholder.path = og_stable?"Data/Objects/drika_box_select_placeholder_stable.xml":"Data/Objects/drika_box_select_placeholder_it.xml";
		box_select_placeholder.name = "Box Select Helper";
		box_select_placeholder.default_scale = vec3(0.5);
		include_envobject = GetJSONBool(params, "include_envobject", true);
		include_group = GetJSONBool(params, "include_group", true);
		include_movement_object = GetJSONBool(params, "include_movement_object", true);
		include_item_object = GetJSONBool(params, "include_item_object", true);
		include_hotspot = GetJSONBool(params, "include_hotspot", true);

		LoadIdentifier(params);
	}

	void PostInit(){
		if(identifier_type == box_select){
			box_select_placeholder.Retrieve();
		}

		if(adding_function){
			AttemptSelectedAsTarget();
		}
	}

	bool UsesPlaceholderObject(){
		return (identifier_type == box_select);
	}

	void AttemptSelectedAsTarget(){
		array<int> selected = GetSelected();

		for(uint i = 0; i < selected.size(); i++){
			if(selected[i] == -1 || !ObjectExists(selected[i])){continue;}
			Object@ target_obj = ReadObjectFromID(selected[i]);

			if((target_option & character_option) != 0 && target_obj.GetType() == _movement_object){
				MovementObject@ char = ReadCharacterID(selected[i]);
				ConnectTo(target_obj);
				return;
			}

			if((target_option & item_option) != 0 && target_obj.GetType() == _item_object){
				ConnectTo(target_obj);
				return;
			}
		}
	}

	bool ConnectTo(Object @other){
		if(other.GetID() == object_id){
			return false;
		}
		if(object_id != -1 && ObjectExists(object_id)){
			PreTargetChanged();
			Disconnect(ReadObjectFromID(object_id));
		}
		object_id = other.GetID();
		if(other.GetType() == _movement_object){
			identifier_type = character;
		}else if(other.GetType() == _item_object){
			identifier_type = item;
		}else{
			identifier_type = id;
		}
		TargetChanged();
		return false;
	}

	bool Disconnect(Object @other){
		if(other.GetID() == object_id){
			object_id = -1;
			return false;
		}
		return false;
	}

	void CheckCharactersAvailable(){
		available_character_ids.resize(0);
		available_character_names.resize(0);

		for(int i = 0; i < GetNumCharacters(); i++){
			MovementObject@ char = ReadCharacter(i);
			Object@ char_obj = ReadObjectFromID(char.GetID());

			if(char_obj.GetName() == ""){
				available_character_names.insertLast("Character id " + char.GetID());
			}else{
				available_character_names.insertLast(char_obj.GetName());
			}

			available_character_ids.insertLast(char.GetID());
		}
	}

	void CheckItemsAvailable(){
		available_item_ids.resize(0);
		available_item_names.resize(0);

		for(int i = 0; i < GetNumItems(); i++){
			ItemObject@ item = ReadItem(i);
			Object@ obj = ReadObjectFromID(item.GetID());

			string description;
			description += (item.GetLabel() == "")?"Item ":item.GetLabel() + " ";
			description += item.GetID() + " ";
			description += obj.GetName();

			available_item_names.insertLast(description);

			available_item_ids.insertLast(item.GetID());
		}
	}

	void CheckAvailableTargets(){
		if((target_option & character_option) != 0){
			CheckCharactersAvailable();
		}
		if((target_option & reference_option) != 0){
			CheckReferenceAvailable();
		}
		if((target_option & item_option) != 0){
			CheckItemsAvailable();
		}
		if((target_option & batch_option) != 0){
			CheckBatchObjects();
		}
	}

	void CheckBatchObjects(){
		if(batch_objects.size() == 0){
			for(uint i = 0; i < batch_ids.size(); i++){
				AddBatchObject(batch_ids[i]);
			}
		}
	}

	void CheckReferenceAvailable(){
		available_references = GetReferences();
	}

	void DrawSelectTargetUI(){
		array<string> identifier_choices = {};

		if((target_option & id_option) != 0){
			identifier_choices.insertLast("ID");
		}

		if((target_option & character_option) != 0){
			identifier_choices.insertLast("Character");
		}

		if((target_option & reference_option) != 0 && (available_references.size() > 0 || target_option == reference_option)){
			identifier_choices.insertLast("Reference");
		}

		if((target_option & team_option) != 0){
			identifier_choices.insertLast("Team");
		}

		if((target_option & name_option) != 0){
			identifier_choices.insertLast("Name");
		}

		if((target_option & item_option) != 0 && available_item_ids.size() > 0){
			identifier_choices.insertLast("Item");
		}

		if((target_option & batch_option) != 0){
			identifier_choices.insertLast("Batch");
		}

		if((target_option & camera_option) != 0){
			identifier_choices.insertLast("Camera");
		}

		if((target_option & box_select_option) != 0){
			identifier_choices.insertLast("Box Select");
		}

		if((target_option & any_character_option) != 0){
			identifier_choices.insertLast("Any Character");
		}

		if((target_option & any_player_option) != 0){
			identifier_choices.insertLast("Any Player");
		}

		if((target_option & any_npc_option) != 0){
			identifier_choices.insertLast("Any NPC");
		}

		if((target_option & any_item_option) != 0 && available_item_ids.size() > 0){
			identifier_choices.insertLast("Any Item");
		}

		int current_identifier_type = -1;

		for(uint i = 0; i < identifier_choices.size(); i++){
			if(	identifier_type == id && identifier_choices[i] == "ID"||
			 	identifier_type == team && identifier_choices[i] == "Team"||
				identifier_type == reference && identifier_choices[i] == "Reference"||
				identifier_type == character && identifier_choices[i] == "Character"||
				identifier_type == item && identifier_choices[i] == "Item"||
				identifier_type == batch && identifier_choices[i] == "Batch"||
				identifier_type == name && identifier_choices[i] == "Name"||
				identifier_type == cam && identifier_choices[i] == "Camera" ||
				identifier_type == box_select && identifier_choices[i] == "Box Select" ||
				identifier_type == any_character && identifier_choices[i] == "Any Character" ||
				identifier_type == any_player && identifier_choices[i] == "Any Player" ||
				identifier_type == any_npc && identifier_choices[i] == "Any NPC" ||
				identifier_type == any_item && identifier_choices[i] == "Any Item" ){
				current_identifier_type = i;
				break;
			}
		}

		bool refresh_target = false;
		bool target_changed = false;
		if(current_identifier_type == -1){
			current_identifier_type = 0;
			refresh_target = true;
		}

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Identifier Type");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);

		if(ImGui_Combo("##Identifier Type" + tag, current_identifier_type, identifier_choices, identifier_choices.size()) || refresh_target){
			PreTargetChanged();
			if(identifier_choices[current_identifier_type] == "ID"){
				identifier_type = id;
			}else if(identifier_choices[current_identifier_type] == "Team"){
				identifier_type = team;
			}else if(identifier_choices[current_identifier_type] == "Reference"){
				identifier_type = reference;
			}else if(identifier_choices[current_identifier_type] == "Name"){
				identifier_type = name;
			}else if(identifier_choices[current_identifier_type] == "Character"){
				identifier_type = character;
			}else if(identifier_choices[current_identifier_type] == "Item"){
				identifier_type = item;
			}else if(identifier_choices[current_identifier_type] == "Batch"){
				identifier_type = batch;
			}else if(identifier_choices[current_identifier_type] == "Camera"){
				identifier_type = cam;
			}else if(identifier_choices[current_identifier_type] == "Box Select"){
				identifier_type = box_select;
			}else if(identifier_choices[current_identifier_type] == "Any Character"){
				identifier_type = any_character;
			}else if(identifier_choices[current_identifier_type] == "Any Player"){
				identifier_type = any_player;
			}else if(identifier_choices[current_identifier_type] == "Any NPC"){
				identifier_type = any_npc;
			}else if(identifier_choices[current_identifier_type] == "Any Item"){
				identifier_type = any_item;
			}
			target_changed = true;
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		if(identifier_type == id){
			int new_object_id = object_id;
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Object ID");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_InputInt("##Object ID" + tag, new_object_id)){
				PreTargetChanged();
				object_id = new_object_id;
				TargetChanged();
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(identifier_type == reference){
			int current_reference = -1;

			if(available_references.size() > 0){
				//Find the index of the chosen reference or set the default to the first one.
				current_reference = 0;
				for(uint i = 0; i < available_references.size(); i++){
					if(available_references[i] == reference_string){
						current_reference = i;
						break;
					}
				}
				reference_string = available_references[current_reference];
			}else if((target_option & id_option) != 0){
				//Force the identifier type to id when no references are available and id target option is availble.
				identifier_type = id;
				return;
			}

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Reference");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_Combo("##Pick Reference" + tag, current_reference, available_references, available_references.size())){
				PreTargetChanged();
				reference_string = available_references[current_reference];
				TargetChanged();
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(identifier_type == team){
			string new_character_team = character_team;

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Team");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_InputText("##Team" + tag, new_character_team, 64)){
				PreTargetChanged();
				character_team = new_character_team;
				TargetChanged();
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(identifier_type == name){
			string new_object_name = object_name;

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Name");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_InputText("##Name" + tag, new_object_name, 64)){
				PreTargetChanged();
				object_name = new_object_name;
				TargetChanged();
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(identifier_type == character){
			int current_character = -1;
			for(uint i = 0; i < available_character_ids.size(); i++){
				if(object_id == available_character_ids[i]){
					current_character = i;
					break;
				}
			}

			//Pick the first character if the object_id can't be found.
			if(current_character == -1 && available_character_ids.size() > 0){
				current_character = 0;
				object_id = available_character_ids[0];
			}

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Character");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_Combo("##Character" + tag, current_character, available_character_names, available_character_names.size())){
				PreTargetChanged();
				object_id = available_character_ids[current_character];
				TargetChanged();
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(identifier_type == item){
			int current_item = -1;
			for(uint i = 0; i < available_item_ids.size(); i++){
				if(object_id == available_item_ids[i]){
					current_item = i;
					break;
				}
			}

			//Pick the first item if the object_id can't be found.
			if(current_item == -1 && available_item_ids.size() > 0){
				Log(warning, "Object item does not exist " + object_id);
				current_item = 0;
				object_id = available_item_ids[0];
				Log(warning, "Setting to " + object_id);
			}

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Item");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_Combo("##Item" + tag, current_item, available_item_names, available_item_names.size())){
				PreTargetChanged();
				object_id = available_item_ids[current_item];
				TargetChanged();
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(identifier_type == batch){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Batch");
			ImGui_NextColumn();

			ImGui_BeginChild("batch_select_ui" + tag, vec2(0, 200), false, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);

			if(ImGui_Button("Add Selected")){
				array<int> object_ids = GetSelected();
				for(uint i = 0; i < object_ids.size(); i++){
					Object@ obj = ReadObjectFromID(object_ids[i]);
					if(batch_ids.find(object_ids[i]) == -1){
						batch_ids.insertLast(object_ids[i]);
						AddBatchObject(obj.GetID());
					}
				}
			}
			ImGui_SameLine();
			if(ImGui_Button("Remove Selected")){
				array<int> object_ids = GetSelected();
				for(uint i = 0; i < object_ids.size(); i++){
					Object@ obj = ReadObjectFromID(object_ids[i]);
					int batch_id_index = batch_ids.find(object_ids[i]);
					if(batch_id_index != -1){
						batch_ids.removeAt(batch_id_index);
					}

					for(uint j = 0; j < batch_objects.size(); j++){
						if(batch_objects[j].id == object_ids[i]){
							batch_objects.removeAt(j);
						}
					}
				}
			}
			ImGui_SameLine();
			if(ImGui_Button("Clear")){
				batch_ids.resize(0);
				batch_objects.resize(0);
			}

			if(ImGui_BeginChildFrame(123, vec2(-1, 200), ImGuiWindowFlags_AlwaysAutoResize)){

				for(uint i = 0; i < batch_objects.size(); i++){
					/* ImGui_PushID("delete" + i); */
					if(ImGui_Button("Delete " + batch_objects[i].id)){
					/* if(ImGui_ImageButton(delete_icon, vec2(10), vec2(0), vec2(1), 2, vec4(0))){ */
						Object@ obj = ReadObjectFromID(batch_objects[i].id);
						int batch_id_index = batch_ids.find(batch_objects[i].id);
						if(batch_id_index != -1){
							batch_ids.removeAt(batch_id_index);
						}
						batch_objects.removeAt(i);
						continue;
					}
					/* ImGui_PopID(); */
					ImGui_SameLine();

					vec4 text_color = batch_objects[i].color;
					ImGui_PushStyleColor(ImGuiCol_Text, text_color);
					ImGui_PushItemWidth(150.0);
					if(ImGui_Selectable(batch_objects[i].text, false)){

					}
					ImGui_PopItemWidth();
					ImGui_PopStyleColor();
				}

				ImGui_EndChildFrame();
			}
			ImGui_EndChild();
			ImGui_NextColumn();
		}else if(identifier_type == box_select){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("EnvObject");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_Checkbox("##EnvObject", include_envobject);
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Group");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_Checkbox("##Group", include_group);
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("MovementObject");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_Checkbox("##MovementObject", include_movement_object);
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("ItemObject");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_Checkbox("##ItemObject", include_item_object);
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Hotspot");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_Checkbox("##Hotspot", include_hotspot);
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}

		if(target_changed){
			TargetChanged();
		}
	}

	void AddBatchObject(int batch_object_id){
		Object@ obj = ReadObjectFromID(batch_object_id);
		string label = obj.GetLabel();
		string editor_label = obj.GetEditorLabel();
		string name = obj.GetName();
		string type_string = GetObjectTypeString(obj.GetType());

		string text = obj.GetID() + " ";
		text += (label != "")?label + " ":"";
		text += (editor_label != "")?editor_label + " ":"";
		text += (name != "")?name + " ":"";
		text += type_string;
		batch_objects.insertLast(BatchObject(obj.GetID(), text, GetBatchObjectColor(obj.GetType())));
	}

	void SaveIdentifier(JSONValue &inout data){
		data[identifier_type_tag] = JSONValue(identifier_type);
		if(identifier_type == id){
			data[identifier_tag] = JSONValue(object_id);
		}else if(identifier_type == reference){
			data[identifier_tag] = JSONValue(reference_string);
		}else if(identifier_type == team){
			data[identifier_tag] = JSONValue(character_team);
		}else if(identifier_type == name){
			data[identifier_tag] = JSONValue(object_name);
		}else if(identifier_type == character){
			data[identifier_tag] = JSONValue(object_id);
		}else if(identifier_type == item){
			data[identifier_tag] = JSONValue(object_id);
		}else if(identifier_type == batch){
			data[identifier_tag] = JSONValue(JSONarrayValue);
			for(uint i = 0; i < batch_ids.size(); i++){
				data[identifier_tag].append(batch_ids[i]);
			}
		}else if(identifier_type == box_select){
			box_select_placeholder.Save(data);
			data["include_envobject"] = JSONValue(include_envobject);
			data["include_group"] = JSONValue(include_group);
			data["include_movement_object"] = JSONValue(include_movement_object);
			data["include_item_object"] = JSONValue(include_item_object);
			data["include_hotspot"] = JSONValue(include_hotspot);
		}
	}

	void LoadIdentifier(JSONValue params){
		if(params.isMember(identifier_type_tag)){
			identifier_type = identifier_types(params[identifier_type_tag].asInt());

			if(identifier_type == id){
				object_id = params[identifier_tag].asInt();
			}else if(identifier_type == reference){
				reference_string = params[identifier_tag].asString();
			}else if(identifier_type == team){
				character_team = params[identifier_tag].asString();
			}else if(identifier_type == name){
				object_name = params[identifier_tag].asString();
			}else if(identifier_type == character){
				object_id = params[identifier_tag].asInt();
			}else if(identifier_type == item){
				object_id = params[identifier_tag].asInt();
			}else if(identifier_type == batch){
				batch_ids = GetJSONIntArray(params, identifier_tag, {});
			}else if(identifier_type == box_select){
				box_select_placeholder.Load(params);
			}
		}else{
			//By default the id is used as identifier with -1 as the target id.
			identifier_type = identifier_types(id);
		}
	}

	void CheckTargetAvailable(){
		if(identifier_type == id || identifier_type == character || identifier_type == item){
			if(object_id != -1 && !ObjectExists(object_id)){
				Log(warning, "The object with id " + object_id + " doesn't exist anymore, so resetting to -1.");
				object_id = -1;
			}
		}else if(identifier_type == reference){
			DrikaElement@ reference_element = GetReferenceElement(reference_string);
			if(reference_element !is null){
				if(reference_element.deleted){
					reference_string = "";
				}
			}
		}else if(identifier_type == box_select){
			if(!box_select_placeholder.Exists()){
				box_select_placeholder.Create();
				/* box_select_placeholder.CreatePlaceholderMesh(); */
			}
			box_select_placeholder.DrawEditing();
		}
	}

	array<Object@> GetTargetObjects(){
		CheckTargetAvailable();

		array<Object@> target_objects;
		if(identifier_type == id){
			if(object_id != -1 && ObjectExists(object_id)){
				target_objects.insertLast(ReadObjectFromID(object_id));
			}
		}else if(identifier_type == reference){
			DrikaElement@ reference_element = GetReferenceElement(reference_string);
			if(reference_element !is null){
				array<int> registered_object_ids = reference_element.GetReferenceObjectIDs();
				for(uint i = 0; i < registered_object_ids.size(); i++){
					if(ObjectExists(registered_object_ids[i])){
						target_objects.insertLast(ReadObjectFromID(registered_object_ids[i]));
					}
				}
			}
		}else if(identifier_type == team){
			array<int> object_ids = GetObjectIDsType(_movement_object);
			for(uint i = 0; i < object_ids.size(); i++){
				if(ObjectExists(object_ids[i])){
					Object@ obj = ReadObjectFromID(object_ids[i]);
					ScriptParams@ obj_params = obj.GetScriptParams();

					if(obj_params.HasParam("Teams")){
						array<string> teams;
						//Teams are , seperated or space comma.
						array<string> space_comma_separated = obj_params.GetString("Teams").split(", ");
						for(uint j = 0; j < space_comma_separated.size(); j++){
							array<string> comma_separated = space_comma_separated[j].split(",");
							teams.insertAt(0, comma_separated);
						}

						if(teams.find(character_team) != -1){
							target_objects.insertLast(obj);
						}
					}
				}
			}
		}else if(identifier_type == name){
			int cache_index = object_cache_names.find(object_name);

			// Use the cached object ids if this name has already been searched.
			if(cache_index != -1){
				for(uint i = 0; i < object_cache[cache_index].size(); i++){
					if(ObjectExists(object_cache[cache_index][i])){
						Object@ obj = ReadObjectFromID(object_cache[cache_index][i]);
						target_objects.insertLast(obj);
					}
				}
			}else{
				array<int> object_ids = GetObjectIDs();
				array<int> object_ids_with_name = {};

				for(uint i = 0; i < object_ids.size(); i++){
					if(ObjectExists(object_ids[i])){
						Object@ obj = ReadObjectFromID(object_ids[i]);
						if(obj.GetName() == object_name){
							target_objects.insertLast(obj);
							object_ids_with_name.insertLast(object_ids[i]);
						}
					}
				}

				// Add the target objects to the cache so it can be used in the next update.
				object_cache_names.insertLast(object_name);
				object_cache.insertLast(object_ids_with_name);
			}
		}else if(identifier_type == character){
			if(object_id != -1 && ObjectExists(object_id)){
				target_objects.insertLast(ReadObjectFromID(object_id));
			}
		}else if(identifier_type == item){
			if(object_id != -1 && ObjectExists(object_id)){
				target_objects.insertLast(ReadObjectFromID(object_id));
			}
		}else if(identifier_type == batch){
			for(uint i = 0; i < batch_ids.size(); i++){
				if(ObjectExists(batch_ids[i])){
					target_objects.insertLast(ReadObjectFromID(batch_ids[i]));
				}
			}
		}else if(identifier_type == box_select){
			mat4 placeholder_transform = box_select_placeholder.cube_object.GetTransform();

			/* mat4 scale_mat;
			scale_mat[0] = placeholder_transform[0] * 0.25;
			scale_mat[5] = placeholder_transform[5] * 0.25;
			scale_mat[10] = placeholder_transform[10] * 0.25;;
			scale_mat[15] = placeholder_transform[15] * 0.25;
			placeholder_transform = placeholder_transform * scale_mat; */

			if(include_envobject){
				BoxSelectCheck(target_objects, placeholder_transform, _env_object);
			}
			if(include_group){
				BoxSelectCheck(target_objects, placeholder_transform, _group);
			}
			if(include_item_object){
				BoxSelectCheck(target_objects, placeholder_transform, _item_object);
			}
			if(include_movement_object){
				BoxSelectCheck(target_objects, placeholder_transform, _movement_object);
			}
			if(include_hotspot){
				BoxSelectCheck(target_objects, placeholder_transform, _hotspot_object);
			}
		}else if(identifier_type == any_character){
			for(int i = 0; i < GetNumCharacters(); i++){
				MovementObject@ char = ReadCharacter(i);
				if(ObjectExists(char.GetID())){
					target_objects.insertLast(ReadObjectFromID(char.GetID()));
				}
			}
		}else if(identifier_type == any_player){
			for(int i = 0; i < GetNumCharacters(); i++){
				MovementObject@ char = ReadCharacter(i);
				if(ObjectExists(char.GetID())){
					if(char.is_player){
						target_objects.insertLast(ReadObjectFromID(char.GetID()));
					}
				}
			}
		}else if(identifier_type == any_npc){
			for(int i = 0; i < GetNumCharacters(); i++){
				MovementObject@ char = ReadCharacter(i);
				if(ObjectExists(char.GetID())){
					if(!char.is_player){
						target_objects.insertLast(ReadObjectFromID(char.GetID()));
					}
				}
			}
		}else if(identifier_type == any_item){
			for(int i = 0; i < GetNumItems(); i++){
				ItemObject@ item = ReadItem(i);
				if(ObjectExists(item.GetID())){
					target_objects.insertLast(ReadObjectFromID(item.GetID()));
				}
			}
		}
		return target_objects;
	}

	void BoxSelectCheck(array<Object@> &inout target_objects, mat4 box_transform, EntityType type){
		array<int> object_ids = GetObjectIDsType(type);

		for(uint i = 0; i < object_ids.size(); i++){
			if(ObjectExists(object_ids[i])){
				Object@ obj = ReadObjectFromID(object_ids[i]);
				//Exclude all the helper objects.
				if(obj.GetName().findFirst("Helper") != -1){
					continue;
				}
				vec3 obj_translation = obj.GetTranslation();
				vec3 local_space_translation = invert(box_transform) * obj_translation;

				if(local_space_translation.x >= -2 && local_space_translation.x <= 2 &&
					local_space_translation.y >= -2 && local_space_translation.y <= 2 &&
					local_space_translation.z >= -2 && local_space_translation.z <= 2){
					target_objects.insertLast(obj);
				}
			}
		}
	}

	array<MovementObject@> GetTargetMovementObjects(){
		CheckTargetAvailable();

		array<MovementObject@> target_movement_objects;
		if(identifier_type == id){
			if(object_id != -1 && ObjectExists(object_id)){
				Object@ char_obj = ReadObjectFromID(object_id);
				if(char_obj.GetType() == _movement_object){
					target_movement_objects.insertLast(ReadCharacterID(object_id));
				}
			}
		}else if (identifier_type == reference){
			DrikaElement@ reference_element = GetReferenceElement(reference_string);
			if(reference_element !is null){
				array<int> registered_object_ids = reference_element.GetReferenceObjectIDs();
				for(uint i = 0; i < registered_object_ids.size(); i++){
					if(ObjectExists(registered_object_ids[i])){
						Object@ char_obj = ReadObjectFromID(registered_object_ids[i]);
						if(char_obj.GetType() == _movement_object){
							target_movement_objects.insertLast(ReadCharacterID(registered_object_ids[i]));
						}
					}
				}
			}
		}else if (identifier_type == team){
			array<int> mo_ids = GetObjectIDsType(_movement_object);
			for(uint i = 0; i < mo_ids.size(); i++){
				if(mo_ids[i] != -1 && ObjectExists(mo_ids[i])){
					Object@ char_obj = ReadObjectFromID(mo_ids[i]);
					if(char_obj.GetType() == _movement_object){
						MovementObject@ mo = ReadCharacterID(mo_ids[i]);
						ScriptParams@ obj_params = char_obj.GetScriptParams();
						if(obj_params.HasParam("Teams")){
							//Removed all the spaces.
							string no_spaces_param = join(obj_params.GetString("Teams").split(" "), "");
							//Teams are , seperated.
							array<string> teams = no_spaces_param.split(",");
							if(teams.find(character_team) != -1){
								target_movement_objects.insertLast(mo);
							}
						}
					}
				}
			}
		}else if (identifier_type == name){
			array<int> mo_ids = GetObjectIDsType(_movement_object);
			for(uint i = 0; i < mo_ids.size(); i++){
				if(mo_ids[i] != -1 && ObjectExists(mo_ids[i])){
					Object@ char_obj = ReadObjectFromID(mo_ids[i]);
					if(char_obj.GetType() == _movement_object){
						MovementObject@ mo = ReadCharacterID(mo_ids[i]);

						if(char_obj.GetName() == object_name){
							target_movement_objects.insertLast(mo);
						}
					}
				}
			}
		}else if(identifier_type == character){
			if(object_id != -1 && ObjectExists(object_id)){
				Object@ char_obj = ReadObjectFromID(object_id);
				if(char_obj.GetType() == _movement_object){
					target_movement_objects.insertLast(ReadCharacterID(object_id));
				}
			}
		}else if(identifier_type == batch){
			for(uint i = 0; i < batch_ids.size(); i++){
				if(batch_ids[i] != -1 && ObjectExists(batch_ids[i])){
					Object@ char_obj = ReadObjectFromID(batch_ids[i]);
					if(char_obj.GetType() == _movement_object){
						target_movement_objects.insertLast(ReadCharacterID(batch_ids[i]));
					}
				}
			}
		}else if(identifier_type == any_character){
			for(int i = 0; i < GetNumCharacters(); i++){
				MovementObject@ char = ReadCharacter(i);
				if(ObjectExists(char.GetID())){
					target_movement_objects.insertLast(ReadCharacter(i));
				}
			}
		}else if(identifier_type == any_player){
			for(int i = 0; i < GetNumCharacters(); i++){
				MovementObject@ char = ReadCharacter(i);
				if(ObjectExists(char.GetID())){
					if(char.is_player){
						target_movement_objects.insertLast(char);
					}
				}
			}
		}else if(identifier_type == any_npc){
			for(int i = 0; i < GetNumCharacters(); i++){
				MovementObject@ char = ReadCharacter(i);
				if(ObjectExists(char.GetID())){
					if(!char.is_player){
						target_movement_objects.insertLast(char);
					}
				}
			}
		}
		return target_movement_objects;
	}

	string GetTargetDisplayText(){
		CheckTargetAvailable();
		if(identifier_type == id){
			return "" + object_id;
		}else if (identifier_type == reference){
			DrikaElement@ reference_element = GetReferenceElement(reference_string);
			return (reference_element !is null)?reference_element.reference_string:"NA";
		}else if (identifier_type == team){
			return character_team;
		}else if (identifier_type == name){
			return object_name;
		}else if(identifier_type == character){
			if(object_id != -1){
				if(ObjectExists(object_id)){
					Object@ char_obj = ReadObjectFromID(object_id);

					if(char_obj.GetName() != ""){
						return char_obj.GetName();
					}else{
						return char_obj.GetID() + "";
					}
				}
			}
		}else if(identifier_type == item){
			if(object_id != -1){
				if(ObjectExists(object_id)){
					ItemObject@ item_obj = ReadItemID(object_id);
					Object@ obj = ReadObjectFromID(object_id);

					string description;
					description += (item_obj.GetLabel() == "")?"Item ":item_obj.GetLabel() + " ";
					description += item_obj.GetID() + " ";
					description += obj.GetName();

					return description;
				}
			}
		}else if (identifier_type == batch){
			return "batch";
		}else if (identifier_type == cam){
			return "Camera";
		}else if (identifier_type == box_select){
			return "Box Select";
		}else if (identifier_type == any_character){
			return "Any Character";
		}else if (identifier_type == any_player){
			return "Any Player";
		}else if (identifier_type == any_npc){
			return "Any NPC";
		}else if (identifier_type == any_item){
			return "Any Item";
		}
		return "NA";
	}

	void ClearTarget(){
		object_id = -1;
		reference_string = "";
		character_team = "team_drika";
		object_name = "drika_object";
	}

	void SetSelected(bool selected){
		array<Object@> target_objects = GetTargetObjects();
		for(uint i = 0 ; i < target_objects.size(); i++){
			target_objects[i].SetSelected(selected);
		}
	}

	void StartEdit(){
		SetReferencePlaceholderSelectable(true);
		box_select_placeholder.SetSelectable(true);
	}

	void EditDone(){
		SetReferencePlaceholderSelectable(false);
		box_select_placeholder.SetSelectable(false);
	}

	void PreTargetChanged(){
		box_select_placeholder.Remove();
		SetReferencePlaceholderSelectable(false);
		parent.PreTargetChanged();
	}

	void TargetChanged(){
		parent.TargetChanged();
		SetReferencePlaceholderSelectable(true);
	}

	void Delete(){
		SetReferencePlaceholderSelectable(false);
		box_select_placeholder.Remove();
	}

	// When the reference has a placeholder then it is unselectable by default.
	// So when this function is using that placeholder make it selectable or unselectable when it's no longer used.
	void SetReferencePlaceholderSelectable(bool selectable){
		if(identifier_type == reference){
			DrikaElement@ reference_element = GetReferenceElement(reference_string);
			if(reference_element !is null){
				reference_element.placeholder.SetSelectable(selectable);
			}
		}
	}

	array<vec3> GetTargetPositions(){
		array<vec3> target_positions;
		array<Object@> target_objects = GetTargetObjects();

		for(uint i = 0; i < target_objects.size(); i++){
			vec3 target_location = target_objects[i].GetTranslation();

			if(target_objects[i].GetType() == _item_object){
				ItemObject@ item_obj = ReadItemID(target_objects[i].GetID());
				target_location = item_obj.GetPhysicsPosition();
			}else if(target_objects[i].GetType() == _movement_object){
				MovementObject@ char = ReadCharacterID(target_objects[i].GetID());
				target_location = char.position;
			}

			target_positions.insertLast(target_location);
		}

		return target_positions;
	}
}
