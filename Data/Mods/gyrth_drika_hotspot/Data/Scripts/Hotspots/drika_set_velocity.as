enum set_velocity_modes {
							_set_velocity_with_placeholder = 0,
							_set_velocity_towards_target = 1,
							_set_velocity_multiplier = 2
						};

class DrikaSetVelocity : DrikaElement{
	float velocity_magnitude;
	bool add_velocity;
	set_velocity_modes set_velocity_mode;
	int current_set_velocity_mode;
	DrikaTargetSelect@ towards_target;
	float height_offset;
	float placeholder_min_scale = 0.25;
	float placeholder_scale_multiplier = 7.0f;
	float placeholder_velocity_arrow_multiplier = 0.25f;
	float velocity_multiplier = 1.0;
	bool apply_multiplier_x;
	bool apply_multiplier_y;
	bool apply_multiplier_z;

	array<string> set_velocity_mode_names = {	"Velocity With Placeholder",
												"Velocity Towards Target",
												"Velocity Multiplier"
											};

	DrikaSetVelocity(JSONValue params = JSONValue()){
		placeholder.Load(params);
		placeholder.name = "Set Velocity Helper";

		velocity_magnitude = GetJSONFloat(params, "velocity_magnitude", 5);
		height_offset = GetJSONFloat(params, "height_offset", 0.0);

		set_velocity_mode = set_velocity_modes(GetJSONInt(params, "set_velocity_mode", _set_velocity_with_placeholder));
		current_set_velocity_mode = set_velocity_mode;

		@towards_target = DrikaTargetSelect(this, params, "towards_target");
		towards_target.target_option = id_option | name_option | character_option | reference_option | team_option | item_option;

		@target_select = DrikaTargetSelect(this, params);
		target_select.target_option = id_option | name_option | character_option | reference_option | team_option;

		add_velocity = GetJSONBool(params, "add_velocity", false);
		
		apply_multiplier_x = GetJSONBool(params, "apply_multiplier_x", true);
		apply_multiplier_y = GetJSONBool(params, "apply_multiplier_y", true);
		apply_multiplier_z = GetJSONBool(params, "apply_multiplier_z", true);

		velocity_multiplier = GetJSONFloat(params, "velocity_multiplier", 1.0);

		drika_element_type = drika_set_velocity;
		connection_types = {_movement_object, _item_object};
		has_settings = true;
	}

	void PostInit(){
		placeholder.Retrieve();
		target_select.PostInit();
		towards_target.PostInit();
	}

	bool UsesPlaceholderObject(){
		return (set_velocity_mode == _set_velocity_with_placeholder);
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["velocity_magnitude"] = JSONValue(velocity_magnitude);
		data["add_velocity"] = JSONValue(add_velocity);
		target_select.SaveIdentifier(data);
		data["set_velocity_mode"] = JSONValue(set_velocity_mode);
		data["apply_multiplier_x"] = JSONValue(apply_multiplier_x);
		data["apply_multiplier_y"] = JSONValue(apply_multiplier_y);
		data["apply_multiplier_z"] = JSONValue(apply_multiplier_z);

		if(set_velocity_mode == _set_velocity_with_placeholder){
			placeholder.Save(data);
		}else if(set_velocity_mode == _set_velocity_towards_target){
			towards_target.SaveIdentifier(data);
			data["height_offset"] = JSONValue(height_offset);
		}else if(set_velocity_mode == _set_velocity_multiplier){
			data["velocity_multiplier"] = JSONValue(velocity_multiplier);
		}

		return data;
	}

	void Delete(){
		placeholder.Remove();
		target_select.Delete();
		towards_target.Delete();
	}

	string GetDisplayString(){
		string display_text = (add_velocity ? "Add Velocity ":"Set Velocity ") + target_select.GetTargetDisplayText();
		vec3 velocity_axis = vec3(apply_multiplier_x ? 1:0,apply_multiplier_y ? 1:0,apply_multiplier_z ? 1:0);
		if(set_velocity_mode == _set_velocity_towards_target){
			display_text +=  " Vel:" + velocity_magnitude + " Target:" + towards_target.GetTargetDisplayText();
		}else if(set_velocity_mode == _set_velocity_with_placeholder){
			display_text +=  " Vel:" + velocity_magnitude + " Placeholder";
		}else if(set_velocity_mode == _set_velocity_multiplier){
			display_text +=  " Mul:" + velocity_multiplier;
		}
		
		if (velocity_axis.x + velocity_axis.y + velocity_axis.z < 3){
			display_text += ", applied in ";
			if (velocity_axis.x + velocity_axis.y + velocity_axis.z < 1){
				display_text += "no axis";
			}else{
				display_text += apply_multiplier_x ? "X ": "";
				display_text += apply_multiplier_y ? "Y ": "";
				display_text += apply_multiplier_z ? "Z": "";
			}
		}
		return display_text;
	}

	void StartSettings(){
		target_select.CheckAvailableTargets();
		towards_target.CheckAvailableTargets();
	}

	void DrawSettings(){

		float option_name_width = 140.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Set Velocity Mode");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_Combo("##Set Velocity Mode", current_set_velocity_mode, set_velocity_mode_names, set_velocity_mode_names.size())){
			set_velocity_mode = set_velocity_modes(current_set_velocity_mode);
			StartSettings();
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Target");
		ImGui_NextColumn();
		ImGui_NextColumn();
		target_select.DrawSelectTargetUI();

		if(set_velocity_mode == _set_velocity_towards_target){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Towards Target");
			ImGui_NextColumn();
			ImGui_NextColumn();
			towards_target.DrawSelectTargetUI();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Height Offset");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_DragFloat("###Height Offset", height_offset, 0.0f, 0.0f, 5.0f, "%.1f");
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}

		if(set_velocity_mode == _set_velocity_towards_target || set_velocity_mode == _set_velocity_with_placeholder){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Velocity");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_DragFloat("###Velocity", velocity_magnitude, 0.01f, 0.0f, 1000.0f, "%.2f")){
				placeholder.SetScale(placeholder_min_scale + (velocity_magnitude / placeholder_scale_multiplier));
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
			
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Applied in axis:");
			ImGui_NextColumn();
			ImGui_Checkbox("X", apply_multiplier_x);
			ImGui_SameLine();
			ImGui_Checkbox("Y", apply_multiplier_y);
			ImGui_SameLine();
			ImGui_Checkbox("Z", apply_multiplier_z);
			ImGui_NextColumn();
			ImGui_NextColumn();
			
			ImGui_PopItemWidth();
			ImGui_NextColumn();
			
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Add Velocity");
			ImGui_NextColumn();
			ImGui_Checkbox("###Add Velocity", add_velocity);
			ImGui_NextColumn();
		}else if(set_velocity_mode == _set_velocity_multiplier){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Multiplier");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_DragFloat("###Multiplier", velocity_multiplier, 0.01f, -2.0f, 2.0f, "%.2f")){
				
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
			
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Applied in axis:");
			ImGui_NextColumn();
			ImGui_Checkbox("X", apply_multiplier_x);
			ImGui_SameLine();
			ImGui_Checkbox("Y", apply_multiplier_y);
			ImGui_SameLine();
			ImGui_Checkbox("Z", apply_multiplier_z);
			ImGui_NextColumn();
			ImGui_NextColumn();
			
			ImGui_PopItemWidth();
			ImGui_NextColumn();
			
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Add Velocity");
			ImGui_NextColumn();
			ImGui_Checkbox("###Add Velocity", add_velocity);
			ImGui_NextColumn();
		}
	}

	void DrawEditing(){
		if(set_velocity_mode == _set_velocity_towards_target){
			if(placeholder.Exists()){
				placeholder.Remove();
			}

			array<Object@> towards_targets = towards_target.GetTargetObjects();
			array<Object@> targets = target_select.GetTargetObjects();

			for(uint i = 0; i < targets.size(); i++){
				vec3 target_location = GetTargetTranslation(targets[i]);
				DebugDrawLine(target_location, this_hotspot.GetTranslation(), vec3(0.0, 0.0, 1.0), _delete_on_draw);
				for(uint j = 0; j < towards_targets.size(); j++){
					vec3 towards_target_location = GetTargetTranslation(towards_targets[j]);
					DebugDrawLine(target_location, towards_target_location, vec3(0.0, 1.0, 0.0), _delete_on_draw);
				}
			}
		}else if(set_velocity_mode == _set_velocity_with_placeholder){
			if(placeholder.Exists()){
				//Force the placeholder to be a minimum size.
				vec3 placeholder_scale = placeholder.GetScale();
				if(placeholder_scale.x < placeholder_min_scale || placeholder_scale.y < placeholder_min_scale || placeholder_scale.z < placeholder_min_scale){
					placeholder_scale.x = placeholder_min_scale;
					placeholder.SetScale(placeholder_min_scale);
				}

				array<Object@> targets = target_select.GetTargetObjects();
				for(uint i = 0; i < targets.size(); i++){
					DebugDrawLine(targets[i].GetTranslation(), placeholder.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
				}
				DebugDrawLine(placeholder.GetTranslation(), this_hotspot.GetTranslation(), vec3(0.0, 0.0, 1.0), _delete_on_draw);
				mat4 gizmo_transform_y;
				gizmo_transform_y.SetTranslationPart(placeholder.GetTranslation());
				gizmo_transform_y.SetRotationPart(Mat4FromQuaternion(placeholder.GetRotation()));

				velocity_magnitude = (((placeholder.GetScale().x + placeholder.GetScale().y + placeholder.GetScale().z) / 3.0f) - placeholder_min_scale) * placeholder_scale_multiplier;

				mat4 scale_mat_y;
				scale_mat_y[0] = 1.0;
				scale_mat_y[5] = max(placeholder_min_scale, velocity_magnitude * placeholder_velocity_arrow_multiplier);
				scale_mat_y[10] = 1.0;
				scale_mat_y[15] = 1.0f;
				gizmo_transform_y = gizmo_transform_y * scale_mat_y;

				DebugDrawWireMesh("Data/Models/drika_gizmo_y.obj", gizmo_transform_y, vec4(1.0f, 0.0f, 0.0f, 1.0f), _delete_on_draw);

				mat4 mesh_transform;
				mesh_transform.SetTranslationPart(placeholder.GetTranslation());
				mat4 rotation = Mat4FromQuaternion(placeholder.GetRotation());
				mesh_transform.SetRotationPart(rotation);

				mat4 scale_mat;
				scale_mat[0] = placeholder.GetScale().x;
				scale_mat[5] = placeholder.GetScale().y;
				scale_mat[10] = placeholder.GetScale().z;
				scale_mat[15] = 1.0f;
				mesh_transform = mesh_transform * scale_mat;

				vec4 color = placeholder.IsSelected()?vec4(0.0f, 0.85f, 0.0f, 0.75f):vec4(0.0f, 0.35f, 0.0f, 0.75f);
				DebugDrawWireMesh("Data/Models/drika_hotspot_cube.obj", mesh_transform, color, _delete_on_draw);
			}else{
				placeholder.Create();
				placeholder.SetScale(placeholder_min_scale + (velocity_magnitude / placeholder_scale_multiplier));
			}
		}
	}

	void StartEdit(){
		placeholder.SetScale(placeholder_min_scale + (velocity_magnitude / placeholder_scale_multiplier));
		DrikaElement::StartEdit();
	}

	bool Trigger(){
		if(set_velocity_mode == _set_velocity_multiplier){
			return MultiplyVelocity();
		}else{
			return ApplyVelocity();
		}
	}

	void TargetChanged(){
		array<Object@> targets = target_select.GetTargetObjects();
		for(uint i = 0; i < targets.size(); i++){
			placeholder.SetTranslation(targets[i].GetTranslation());
			placeholder.SetRotation(targets[i].GetRotation());
		}
	}

	bool ApplyVelocity(){
		vec3 velocity_axis = vec3(apply_multiplier_x ? 1:0,apply_multiplier_y ? 1:0,apply_multiplier_z ? 1:0);
		array<Object@> targets = target_select.GetTargetObjects();
		for(uint i = 0; i < targets.size(); i++){


			vec3 up_direction;

			if(set_velocity_mode == _set_velocity_with_placeholder){
				up_direction = placeholder.GetRotation() * vec3(0, 1, 0);
			}else if(set_velocity_mode == _set_velocity_towards_target){
				array<Object@> towards_targets = towards_target.GetTargetObjects();
				if(towards_targets.size() == 0){
					return false;
				}

				up_direction = normalize((GetTargetTranslation(towards_targets[0]) + vec3(0.0, height_offset, 0.0)) - GetTargetTranslation(targets[i]));
			}


			if(targets[i].GetType() == _movement_object){
				MovementObject@ char = ReadCharacterID(targets[i].GetID());
				if(char.GetIntVar("state") == _ragdoll_state){
					if(add_velocity){
						char.rigged_object().ApplyForceToRagdoll(char.rigged_object().GetAvgVelocity() + up_direction * velocity_magnitude * 1000.0 * velocity_axis, char.rigged_object().skeleton().GetCenterOfMass());
					}else{
						char.rigged_object().ApplyForceToRagdoll(up_direction * velocity_magnitude * 1000.0 * velocity_axis, char.rigged_object().skeleton().GetCenterOfMass());
					}
		        }else{
					if(add_velocity){
						char.velocity = char.velocity + up_direction * velocity_magnitude * velocity_axis;
					}else{
						char.velocity = up_direction * velocity_magnitude * velocity_axis;
					}
					char.Execute("SetOnGround(false);");
					char.Execute("pre_jump = false;");
				}
			}else if(targets[i].GetType() == _item_object){
				ItemObject@ io = ReadItemID(targets[i].GetID());
				io.ActivatePhysics();
				// For some reason the transform needs to be set or else the io doesn't wake up.
				mat4 transform = io.GetPhysicsTransform();
				io.SetPhysicsTransform(transform);
				if(add_velocity){
					io.SetLinearVelocity(io.GetLinearVelocity() + up_direction * velocity_magnitude * velocity_axis);
				}else{
					io.SetLinearVelocity(up_direction * velocity_magnitude * velocity_axis);
				}
				io.ActivatePhysics();
				io.SetThrown();
			}
		}
		return true;
	}

	bool MultiplyVelocity(){
		vec3 velocity_axis = vec3(apply_multiplier_x ? 1:0,apply_multiplier_y ? 1:0,apply_multiplier_z ? 1:0);
		array<Object@> targets = target_select.GetTargetObjects();
		for(uint i = 0; i < targets.size(); i++){
			vec3 old_velocity = vec3(0);

			if(targets[i].GetType() == _movement_object){
				MovementObject@ char = ReadCharacterID(targets[i].GetID());
				if (add_velocity){
					old_velocity = char.velocity;
				}
				if(char.GetIntVar("state") == _ragdoll_state){
					char.rigged_object().ApplyForceToRagdoll(old_velocity + (char.velocity * velocity_multiplier * 1000.0) * velocity_axis, char.rigged_object().skeleton().GetCenterOfMass());
		        }else{
					char.velocity = old_velocity + (char.velocity * velocity_multiplier) * velocity_axis;
				}
			}else if(targets[i].GetType() == _item_object){
				ItemObject@ io = ReadItemID(targets[i].GetID());
				if (add_velocity){
					old_velocity = io.GetLinearVelocity();
				}
				io.ActivatePhysics();
				// For some reason the transform needs to be set or else the io doesn't wake up.
				mat4 transform = io.GetPhysicsTransform();
				io.SetPhysicsTransform(transform);
				io.SetLinearVelocity(old_velocity + (io.GetLinearVelocity() * velocity_multiplier) * velocity_axis);
				io.ActivatePhysics();
				io.SetThrown();
			}
		}
		return true;
	}
}
