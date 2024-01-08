class DrikaCreateParticle : DrikaElement{
	string particle_path;
	int amount;
	vec3 tint;
	float velocity;
	float spread = 0.0f;
	bool use_blood_tint;
	bool connect_particles = false;
	int previous_particle_id = -1;
	float placeholder_min_scale = 0.25;
	float placeholder_scale_multiplier = 7.0f;
	float placeholder_velocity_arrow_multiplier = 0.25f;

	DrikaCreateParticle(JSONValue params = JSONValue()){
		placeholder.Load(params);
		placeholder.name = "Create Particle Helper";

		particle_path = GetJSONString(params, "particle_path", "Data/Particles/blooddrop.xml");
		drika_element_type = drika_create_particle;

		amount = GetJSONInt(params, "amount", 5);
		tint = GetJSONVec3(params, "tint", vec3(1.0f));
		velocity = GetJSONFloat(params, "velocity", 5.0f);
		spread = GetJSONFloat(params, "spread", 0.0f);
		use_blood_tint = GetJSONBool(params, "use_blood_tint", true);
		connect_particles = GetJSONBool(params, "connect_particles", false);
		has_settings = true;
	}

	JSONValue GetSaveData(){
		JSONValue data;
		placeholder.Save(data);
		data["particle_path"] = JSONValue(particle_path);
		data["amount"] = JSONValue(amount);
		data["velocity"] = JSONValue(velocity);
		data["spread"] = JSONValue(spread);
		data["use_blood_tint"] = JSONValue(use_blood_tint);
		data["connect_particles"] = JSONValue(connect_particles);
		data["tint"] = JSONValue(JSONarrayValue);
		data["tint"].append(tint.x);
		data["tint"].append(tint.y);
		data["tint"].append(tint.z);
		return data;
	}

	void PostInit(){
		placeholder.Retrieve();
	}

	void Delete(){
		placeholder.Remove();
	}

	string GetDisplayString(){
		return "CreateParticle " + particle_path;
	}

	void DrawSettings(){

		float option_name_width = 120.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Particle Path");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);

		if(ImGui_Button("Set Particle Path")){
			string new_path = GetUserPickedReadPath("xml", "Data/Particles");
			if(new_path != ""){
				particle_path = ShortenPath(new_path);
			}
		}
		ImGui_SameLine();
		ImGui_Text(particle_path);
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Amount");
		ImGui_NextColumn();
		ImGui_PushItemWidth(second_column_width);
		ImGui_InputInt("##Amount", amount);
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Velocity");
		ImGui_NextColumn();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_DragFloat("##Velocity", velocity, 0.01f, 0.0f, 1000.0f)){
			placeholder.SetScale(placeholder_min_scale + (velocity / placeholder_scale_multiplier));
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Spread");
		ImGui_NextColumn();
		ImGui_PushItemWidth(second_column_width);
		ImGui_DragFloat("##Spread", spread, 0.001f, 0.0f, 1.0f, "%.3f");
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Connect Particles");
		ImGui_NextColumn();
		ImGui_Checkbox("###Connect Particles", connect_particles);
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Use Blood Tint");
		ImGui_NextColumn();
		ImGui_Checkbox("###Use Blood Tint", use_blood_tint);
		ImGui_NextColumn();

		if(!use_blood_tint){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Particle Tint");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_ColorEdit3("##Particle Tint", tint);
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}
	}

	void DrawEditing(){
		if(placeholder.Exists()){
			//Force the placeholder to be a minimum size.
			vec3 placeholder_scale = placeholder.GetScale();
			if(placeholder_scale.x < placeholder_min_scale || placeholder_scale.y < placeholder_min_scale || placeholder_scale.z < placeholder_min_scale){
				placeholder_scale.x = placeholder_min_scale;
				placeholder.SetScale(placeholder_min_scale);
			}

			vec3 forward_direction = placeholder.GetRotation() * vec3(1, 0, 0);
			mat4 gizmo_transform_y;
			gizmo_transform_y.SetTranslationPart(placeholder.GetTranslation());
			gizmo_transform_y.SetRotationPart(Mat4FromQuaternion(placeholder.GetRotation()));

			velocity = (((placeholder.GetScale().x + placeholder.GetScale().y + placeholder.GetScale().z) / 3.0f) - placeholder_min_scale) * placeholder_scale_multiplier;

			mat4 scale_mat_y;
			float spread_width = 1.0f + (spread * 2.0f);
			scale_mat_y[0] = max(placeholder_min_scale, velocity * placeholder_velocity_arrow_multiplier);
			scale_mat_y[5] = spread_width;
			scale_mat_y[10] = spread_width;
			scale_mat_y[15] = 1.0f;
			gizmo_transform_y = gizmo_transform_y * scale_mat_y;

			DebugDrawWireMesh("Data/Models/drika_gizmo_x.obj", gizmo_transform_y, vec4(1.0f, 0.0f, 0.0f, 1.0f), _delete_on_draw);

			/* DebugDrawLine(placeholder.GetTranslation(), placeholder.GetTranslation() + (forward_direction * (velocity / 10.0)), vec3(1, 0, 0), _delete_on_draw); */
			DebugDrawLine(placeholder.GetTranslation(), this_hotspot.GetTranslation(), vec3(0.0, 0.0, 1.0), _delete_on_draw);
			DebugDrawBillboard("Data/Textures/ui/stealth_debug/zzzz.tga", placeholder.GetTranslation(), 0.25, vec4(1.0), _delete_on_draw);

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
			StartEdit();
		}
	}

	void Reset(){
		previous_particle_id = -1;
	}

	bool Trigger(){
		if(placeholder.Exists()){
			vec3 particle_tint = GetBloodTint();
			if(!use_blood_tint){
				particle_tint = tint;
			}
			vec3 forward_direction = placeholder.GetRotation() * vec3(1, 0, 0);
			for(int i = 0; i < amount; i++){
				vec3 added_spread = vec3(RangedRandomFloat(-spread,spread), RangedRandomFloat(-spread,spread), RangedRandomFloat(-spread,spread));
				vec3 particle_velocity = normalize(forward_direction + added_spread) * velocity;
				int new_particle_id = MakeParticle(particle_path, placeholder.GetTranslation(), particle_velocity + added_spread, particle_tint);

				//Connecting particles are used to create bloodspurts.
				if(connect_particles){
					if(previous_particle_id != -1){
						ConnectParticles(previous_particle_id, new_particle_id);
					}
					previous_particle_id = new_particle_id;
				}
			}
			return true;
		}else{
			placeholder.Create();
			return false;
		}
	}
}
