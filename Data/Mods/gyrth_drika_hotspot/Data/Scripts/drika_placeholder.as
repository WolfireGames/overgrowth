enum placeholder_modes{
							_at_cube,
							_at_hotspot
						};

class DrikaPlaceholder{
	int cube_id = -1;
	int placeholder_id = -1;
	//The object and placeholder_object are two different models.
	//object is a simple cube that can be translated, rotated and scaled by the user.
	//placeholder_object is a representation of the model with texture using a stipple effect shader.
	Object@ cube_object;
	Object@ placeholder_object;
	string name;
	vec3 default_scale = vec3(0.25);
	string path = "Data/Objects/drika_hotspot_cube.xml";
	string object_path;
	DrikaElement@ parent;
	string placeholder_path;
	vec3 bounding_box;
	placeholder_modes placeholder_mode;
	bool updating_placeholder_preview = false;

	DrikaPlaceholder(){

	}

	void Save(JSONValue &inout data){
		if(Exists()){
			data["placeholder_id"] = JSONValue(cube_id);
			// The bounding box data needs to be saved as well or else the scaling isn't going to be correct.
			// This bounding box is calculated by using the placeholder scale, which isn't available outside of Editor Mode.
			data["bounding_box"] = JSONValue(JSONarrayValue);
			data["bounding_box"].append(bounding_box.x);
			data["bounding_box"].append(bounding_box.y);
			data["bounding_box"].append(bounding_box.z);
		}
	}

	void Load(JSONValue params){
		if(params.isMember("placeholder_id")){
			cube_id = params["placeholder_id"].asInt();
		}
		bounding_box = GetJSONVec3(params, "bounding_box", vec3(1.0));
	}

	void SetMode(placeholder_modes mode){
		placeholder_mode = mode;
	}

	void Create(){
		if(placeholder_mode == _at_cube){
			cube_id = CreateObject(path, false);
			@cube_object = ReadObjectFromID(cube_id);
			cube_object.SetName(name);
			cube_object.SetSelectable(editing);
			cube_object.SetTranslatable(true);
			cube_object.SetScalable(true);
			cube_object.SetRotatable(true);

			cube_object.SetDeletable(false);
			cube_object.SetCopyable(false);

			cube_object.SetScale(default_scale);
			cube_object.SetTranslation(this_hotspot.GetTranslation() + vec3(0.0, 2.0, 0.0));
		}
	}

	void AddPlaceholderObject(){
		if(@placeholder_object !is null){
			placeholder_object.SetEnabled(true);
			return;
		}else if(object_path == ""){
			return;
		}
		UpdatePlaceholderPreview();
	}

	void HidePlaceholderObject(){
		if(@placeholder_object !is null){
			placeholder_object.SetEnabled(false);
		}
	}

	void UpdatePlaceholderPreview(){
		if(updating_placeholder_preview){return;}
		updating_placeholder_preview = true;
		RemovePlaceholderObject();
		level.SendMessage("drika_read_file " + hotspot.GetID() + " " + parent.index + " " + object_path + " " + "xml_content");
	}

	void RemovePlaceholderObject(){
		if(@placeholder_object !is null){
			QueueDeleteObjectID(placeholder_object.GetID());
		}
		@placeholder_object = null;
		placeholder_id = -1;
		bounding_box = vec3(1.0);
	}

	void RemoveCubeObject(){
		if(Exists()){
			QueueDeleteObjectID(cube_id);
		}
		cube_id = -1;
		@cube_object = null;
	}

	void ReceiveMessage(string message, string identifier){
		if(identifier == "xml_content"){
			//Remove all spaces to eliminate style differences.
			string xml_content = join(message.split(" "), "");
			string model = GetStringBetween(xml_content, "<Model>", "</Model>");
			string colormap = GetStringBetween(xml_content, "<ColorMap>", "</ColorMap>");
			if(model != ""){
				string data = GetPlaceholderXMLData(model, colormap);

				int unique_index = StorageGetInt32("unique_index");
				placeholder_path = "Data/Objects/placeholder/drika_placeholder_" + unique_index + ".xml";
				StorageSetInt32("unique_index", unique_index + 1);

				level.SendMessage("drika_write_file " + hotspot.GetID() + " " + parent.index + " " + placeholder_path + " " + data);
			}else{
				//Check if the target xml is an ItemObject or a Character.
				string obj_path = GetStringBetween(xml_content, "obj_path=\"", "\"");
				if(obj_path != ""){
					object_path = obj_path;
					level.SendMessage("drika_read_file " + hotspot.GetID() + " " + parent.index + " " + obj_path + " " + "xml_content");
				}else{
					//Check if the target xml is an Actor.
					string actor_model = GetStringBetween(xml_content, "<Character>", "</Character>");
					if(actor_model != ""){
						object_path = actor_model;
						level.SendMessage("drika_read_file " + hotspot.GetID() + " " + parent.index + " " + actor_model + " " + "xml_content");
					}else{
						Log(warning, "Could not find model in " + object_path);
					}
				}
			}
		}
	}

	string GetPlaceholderXMLData(string model, string colormap){
		string data = "";

		data += "<?xml\" version=\\\"1.0\\\" ?>\n";
		data += "<Object>\n";
		data += "\t<Model>" + model + "</Model>\n";
		data += "\t<ColorMap>" + colormap + "</ColorMap>\n";
		data += "\t<NormalMap>Data/Textures/chest_n.png</NormalMap>\n";
		data += "\t<ShaderName>drika_placeholder</ShaderName>\n";
		data += "\t<flags no_collision=true/>\n";
		data += "</Object>\n";

		return data;
	}

	void ReceiveMessage(string message){
		if(message == "drika_write_placeholder_done"){
			if(!FileExists(placeholder_path)){
				Log(warning, "Path does not exist " + placeholder_path);
				return;
			}else{
				Log(warning, "Path does exist! " + placeholder_path);
			}

			CreatePlaceholderMesh();
			UpdatePlaceholderTransform();
		}
	}

	void CreatePlaceholderMesh(){
		placeholder_id = CreateObject(placeholder_path, true);
		//Failing to load the path results in a -1 being given back.
		if(placeholder_id != -1){
			@placeholder_object = ReadObjectFromID(placeholder_id);
			SetBoundingBox(placeholder_object);
			if(placeholder_mode == _at_cube){
				cube_object.SetScale(bounding_box);
			}
			placeholder_object.SetName(name + " Mesh");
		}else{
			Log(error, "Failed to load placeholder path : " + placeholder_path);
		}

		updating_placeholder_preview = false;
	}

	void SetBoundingBox(Object@ obj){
		bounding_box = obj.GetBoundingBox();
		bounding_box.x = (bounding_box.x == 0.0)?1.0:bounding_box.x;
		bounding_box.y = (bounding_box.y == 0.0)?1.0:bounding_box.y;
		bounding_box.z = (bounding_box.z == 0.0)?1.0:bounding_box.z;
	}

	void DrawEditing(){
		if(placeholder_mode == _at_hotspot){
			if(@placeholder_object is null){
				return;
			}
		}else if(placeholder_mode == _at_cube){
			if(@cube_object is null or @placeholder_object is null){
				return;
			}
		}
		UpdatePlaceholderTransform();
	}

	void UpdatePlaceholderTransform(){
		if(@placeholder_object !is null){
			if(placeholder_mode == _at_hotspot){
				placeholder_object.SetTranslation(this_hotspot.GetTranslation());
				placeholder_object.SetRotation(this_hotspot.GetRotation());
				placeholder_object.SetScale(vec3(1.0));
			}else if(placeholder_mode == _at_cube){
				placeholder_object.SetTranslation(cube_object.GetTranslation());
				placeholder_object.SetRotation(cube_object.GetRotation());
				placeholder_object.SetScale(vec3(cube_object.GetScale().x / bounding_box.x, cube_object.GetScale().y / bounding_box.y, cube_object.GetScale().z / bounding_box.z));
			}
		}
	}

	void Remove(){
		RemoveCubeObject();
		RemovePlaceholderObject();
	}

	void Retrieve(){
		if(placeholder_mode == _at_cube){
			if(duplicating_hotspot || duplicating_function){
				if(ObjectExists(cube_id)){
					//Use the same transform as the original placeholder.
					Object@ old_placeholder = ReadObjectFromID(cube_id);
					Create();
					cube_object.SetScale(old_placeholder.GetScale());
					cube_object.SetTranslation(old_placeholder.GetTranslation());
					cube_object.SetRotation(old_placeholder.GetRotation());
				}else{
					cube_id = -1;
				}
			}else{
				if(ObjectExists(cube_id)){
					@cube_object = ReadObjectFromID(cube_id);
					cube_object.SetName(name);
					SetSelectable(false);
				}else{
					Create();
				}
			}
		}
	}

	bool Exists(){
		return (cube_id != -1 && ObjectExists(cube_id));
	}

	bool PlaceholderExists(){
		return (placeholder_id != -1 && ObjectExists(placeholder_id));
	}

	void SetSelected(bool selected){
		if(placeholder_mode == _at_cube){
			if(Exists()){
				cube_object.SetSelected(selected);
			}
		}
	}

	void SetSelectable(bool selectable){
		if(placeholder_mode == _at_cube){
			if(Exists()){
				if(cube_object.IsSelected() && !selectable){
					cube_object.SetSelected(false);
				}
				cube_object.SetSelectable(selectable);
				cube_object.SetEnabled(selectable);
			}
		}
	}

	vec3 GetTranslation(){
		if(placeholder_mode == _at_cube){
			if(Exists()){
				return cube_object.GetTranslation();
			}else{
				return vec3(1.0);
			}
		}else{
			return this_hotspot.GetTranslation();
		}
	}

	quaternion GetRotation(){
		if(placeholder_mode == _at_cube){
			if(Exists()){
				return cube_object.GetRotation();
			}else{
				return quaternion();
			}
		}else{
			return this_hotspot.GetRotation();
		}
	}

	vec4 GetRotationVec4(){
		if(placeholder_mode == _at_cube){
			if(Exists()){
				return cube_object.GetRotationVec4();
			}else{
				return vec4(0.0);
			}
		}else{
			return this_hotspot.GetRotationVec4();
		}
	}

	vec3 GetScale(){
		if(placeholder_mode == _at_cube){
			if(Exists()){
				return vec3(cube_object.GetScale().x / bounding_box.x, cube_object.GetScale().y / bounding_box.y, cube_object.GetScale().z / bounding_box.z);
			}else{
				return vec3(1.0);
			}
		}else{
			return this_hotspot.GetScale();
		}
	}

	bool IsSelected(){
		if(placeholder_mode == _at_cube){
			if(Exists()){
				return cube_object.IsSelected();
			}else{
				return false;
			}
		}else{
			return false;
		}
	}

	void SetTranslation(vec3 translation){
		if(placeholder_mode == _at_cube){
			if(Exists()){
				cube_object.SetTranslation(translation);
			}
		}
	}

	void RelativeTranslate(vec3 offset){
		if(placeholder_mode == _at_cube){
			if(Exists()){
				cube_object.SetTranslation(cube_object.GetTranslation() + offset);
				UpdatePlaceholderTransform();
			}
		}
	}

	void SetRotation(quaternion rotation){
		if(placeholder_mode == _at_cube){
			if(Exists()){
				cube_object.SetRotation(rotation);
			}
		}
	}

	void RelativeRotate(vec3 origin, mat4 before_mat, mat4 after_mat){
		if(placeholder_mode == _at_cube){
			if(Exists()){
				vec3 current_translation = cube_object.GetTranslation();

				mat4 inverse_mat = after_mat * invert(before_mat);
				vec3 rotated_point = origin + (inverse_mat * (current_translation - origin));

				mat4 object_mat = cube_object.GetTransform();
				mat4 object_inverse_mat = object_mat * invert(before_mat);
				mat4 rotation_mat = object_inverse_mat * after_mat;
				cube_object.SetRotation(QuaternionFromMat4(rotation_mat));

				cube_object.SetTranslation(rotated_point);
				UpdatePlaceholderTransform();
			}
		}
	}

	void SetScale(vec3 scale){
		if(placeholder_mode == _at_cube){
			if(Exists()){
				cube_object.SetScale(scale);
			}
		}
	}

	void SetSpecialType(int special_type){
		if(placeholder_mode == _at_cube){
			if(Exists()){
				PlaceholderObject@ cast_object = cast<PlaceholderObject@>(cube_object);
				cast_object.SetSpecialType(special_type);
			}
		}
	}

	void SetPreview(string preview_path){
		if(placeholder_mode == _at_cube){
			if(Exists()){
				PlaceholderObject@ cast_object = cast<PlaceholderObject@>(cube_object);
				cast_object.SetPreview(preview_path);
			}
		}
	}

	void SetEditorDisplayName(string editor_display_name){
		if(placeholder_mode == _at_cube){
			if(Exists()){
				PlaceholderObject@ cast_object = cast<PlaceholderObject@>(cube_object);
				cast_object.SetEditorDisplayName(editor_display_name);
			}
		}
	}
}