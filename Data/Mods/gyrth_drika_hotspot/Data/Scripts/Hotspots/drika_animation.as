#include "drika_shared.as";

enum animation_types {
						looping_forwards = 0,
						looping_backwards = 1,
						looping_forwards_and_backwards = 2,
						forward = 3,
						backward = 4
					}

enum animation_methods 	{
							timeline_method = 0,
							placeholder_method = 1
						}

enum duration_methods 	{
							constant_speed = 0,
							divide_between_keys = 1
						}

class AnimationKey{
	vec3 translation;
	quaternion rotation;
	vec3 scale;
	float time;
	bool moving = false;
	float moving_time;
}

class DrikaAnimation : DrikaElement{
	array<AnimationKey@> key_data;
	array<int> key_ids;
	animation_types animation_type;
	int current_animation_type;
	int current_duration_method;
	int current_animation_method;
	duration_methods duration_method;
	animation_methods animation_method;
	bool animate_scale;
	float duration;
	float extra_yaw;
	bool moved_spawn_point = false;
	bool update_collision;

	int key_index = 0;
	Object@ current_key;
	Object@ next_key;
	float animation_timer = 0.0;
	int loop_direction = 1;
	bool animation_finished = false;
	float timeline_position = 1.0;
	float timeline_duration = 1.0;
	bool timeline_snap = true;
	float alpha = 0.0;
	bool moving_animation_key = false;
	AnimationKey@ target_key;
	float margin = 20.0;
	float timeline_width;
	float timeline_height;
	bool draw_debug_lines = false;
	vec3 previous_translation = vec3();
	bool animation_started = false;
	vec3 new_translation;
	quaternion new_rotation;
	vec3 new_scale;
	bool done = false;
	IMTweenType tween_type;
	int current_tween_type;
	bool preview_animation = false;
	int skipped_collision_counter = 0;
	string last_error = "";

	array<string> animation_type_names = 	{
												"Looping Forwards",
												"Looping Backwards",
												"Looping Forwards and Backwards",
												"Forward",
												"Backward"
											};

	array<string> duration_method_names = 	{
												"Constant Speed",
												"Divide Between Keys"
											};

	array<string> animation_method_names = 	{
												"Timeline",
												"Placeholder"
											};

	DrikaAnimation(JSONValue params = JSONValue()){
		placeholder.default_scale = vec3(1.0);

		drika_element_type = drika_animation;
		connection_types = {_movement_object, _env_object, _decal_object, _item_object, _hotspot_object, _group};
		key_ids = GetJSONIntArray(params, "key_ids", {});
		key_data = InterpAnimationData(GetJSONValueArray(params, "key_data", {}));
		animation_type = animation_types(GetJSONInt(params, "animation_type", 3));
		current_animation_type = animation_type;
		duration_method = duration_methods(GetJSONInt(params, "duration_method", 0));
		current_duration_method = duration_method;
		animation_method = animation_methods(GetJSONInt(params, "animation_method", 0));
		current_animation_method  = animation_method;
		animate_scale = GetJSONBool(params, "animate_scale", false);
		duration = GetJSONFloat(params, "duration", 5.0);
		extra_yaw = GetJSONFloat(params, "extra_yaw", 0.0);
		parallel_operation = GetJSONBool(params, "parallel_operation", false);
		tween_type = IMTweenType(GetJSONInt(params, "ease_function", linearTween));
		current_tween_type = tween_type;
		update_collision = GetJSONBool(params, "update_collision", true);

		@target_select = DrikaTargetSelect(this, params);
		target_select.target_option = id_option | name_option | character_option | reference_option | team_option | camera_option | item_option;

		// TODO Used for backwards compatibility with older saves. Remove in the future.
		bool animate_camera = GetJSONBool(params, "animate_camera", false);
		if(animate_camera){
			target_select.identifier_type = cam;
		}

		has_settings = true;
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["animation_type"] = JSONValue(animation_type);
		data["duration_method"] = JSONValue(duration_method);
		data["animation_method"] = JSONValue(animation_method);
		data["animate_scale"] = JSONValue(animate_scale);
		data["duration"] = JSONValue(duration);
		data["extra_yaw"] = JSONValue(extra_yaw);
		data["parallel_operation"] = JSONValue(parallel_operation);
		data["ease_function"] = JSONValue(tween_type);
		data["update_collision"] = JSONValue(update_collision);

		data["key_ids"] = JSONValue(JSONarrayValue);
		for(uint i = 0; i < key_ids.size(); i++){
			data["key_ids"].append(key_ids[i]);
		}

		data["key_data"] = JSONValue(JSONarrayValue);
		for(uint i = 0; i < key_data.size(); i++){
			JSONValue current_key_data;
			current_key_data["time"] = JSONValue(key_data[i].time);

			current_key_data["translation"] = JSONValue(JSONarrayValue);
			current_key_data["translation"].append(key_data[i].translation.x);
			current_key_data["translation"].append(key_data[i].translation.y);
			current_key_data["translation"].append(key_data[i].translation.z);

			current_key_data["rotation"] = JSONValue(JSONarrayValue);
			current_key_data["rotation"].append(key_data[i].rotation.x);
			current_key_data["rotation"].append(key_data[i].rotation.y);
			current_key_data["rotation"].append(key_data[i].rotation.z);
			current_key_data["rotation"].append(key_data[i].rotation.w);

			current_key_data["scale"] = JSONValue(JSONarrayValue);
			current_key_data["scale"].append(key_data[i].scale.x);
			current_key_data["scale"].append(key_data[i].scale.y);
			current_key_data["scale"].append(key_data[i].scale.z);

			data["key_data"].append(current_key_data);

		}
		target_select.SaveIdentifier(data);
		return data;
	}

	array<AnimationKey@> InterpAnimationData(array<JSONValue> data){
		array<AnimationKey@> new_data;
		for(uint i = 0; i < data.size(); i++){
			JSONValue current_key_data = data[i];
			AnimationKey new_key;
			new_key.translation = vec3(current_key_data["translation"][0].asFloat(), current_key_data["translation"][1].asFloat(), current_key_data["translation"][2].asFloat());
			new_key.rotation = quaternion(current_key_data["rotation"][0].asFloat(), current_key_data["rotation"][1].asFloat(), current_key_data["rotation"][2].asFloat(), current_key_data["rotation"][3].asFloat());
			new_key.scale = vec3(current_key_data["scale"][0].asFloat(), current_key_data["scale"][1].asFloat(), current_key_data["scale"][2].asFloat());
			new_key.time = current_key_data["time"].asFloat();
			new_data.insertLast(new_key);
		}
		return new_data;
	}

	void PostInit(){
		target_select.PostInit();
	}

	JSONValue GetCheckpointData(){
		JSONValue data;
		data["animation_timer"] = animation_timer;
		data["loop_direction"] = loop_direction;
		data["animation_finished"] = animation_finished;
		data["done"] = done;
		data["animation_started"] = animation_started;
		return data;
	}

	void SetCheckpointData(JSONValue data = JSONValue()){
		animation_timer = data["animation_timer"].asFloat();
		loop_direction = data["loop_direction"].asInt();
		animation_finished = data["animation_finished"].asBool();
		done = data["done"].asBool();
		animation_started = data["animation_started"].asBool();
		if(animation_started){
			SetCurrentTransform();
		}
	}

	void Delete(){
		for(uint i = 0; i < key_ids.size(); i++){
			DeleteObjectID(key_ids[i]);
		}
		placeholder.Remove();
		target_select.Delete();
	}

	string GetDisplayString(){
		return "Animation " + target_select.GetTargetDisplayText();
	}

	void StartSettings(){
		target_select.CheckAvailableTargets();
		WriteAnimationKeyParams();
	}

	void DrawSettings(){

		float option_name_width = 170.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Animation Target");
		ImGui_NextColumn();
		ImGui_NextColumn();

		target_select.DrawSelectTargetUI();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Animation Method");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_Combo("##Animation Method", current_animation_method, animation_method_names, animation_method_names.size())){
			animation_method = animation_methods(current_animation_method);
			//Remove all the old data when switching between methods.
			if(animation_method == placeholder_method){
				key_data.resize(0);
			}else{
				for(uint i = 0; i < key_ids.size(); i++){
					QueueDeleteObjectID(key_ids[i]);
				}
				key_ids.resize(0);
			}
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		if(animation_method == placeholder_method){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Duration Method");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_Combo("##Duration Method", current_duration_method, duration_method_names, duration_method_names.size())){
				duration_method = duration_methods(current_duration_method);
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Animation Type");
		ImGui_NextColumn();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_Combo("##Animation Type", current_animation_type, animation_type_names, animation_type_names.size())){
			animation_type = animation_types(current_animation_type);
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Duration");
		ImGui_NextColumn();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_SliderFloat("##Duration", duration, 0.1f, 10.0f, "%.1f")){
			SetCurrentTransform();
		}
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Extra Yaw");
		ImGui_NextColumn();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_SliderFloat("##Extra Yaw", extra_yaw, 0.0f, 360.0f, "%.1f")){
			SetCurrentTransform();
		}
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Easing Function");
		ImGui_NextColumn();
		ImGui_PushItemWidth(second_column_width);

		if(ImGui_BeginCombo("###Easing Function", tween_types[current_tween_type], ImGuiComboFlags_HeightLarge)){
			for(uint i = 0; i < tween_types.size(); i++){
				if(ImGui_Selectable(tween_types[i], current_tween_type == int(i))){
					current_tween_type = i;
					tween_type = IMTweenType(current_tween_type);
				}
				DrawTweenGraph(tween_type);
			}
			ImGui_EndCombo();
		}

		ImGui_PopItemWidth();
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Animate Scale");
		ImGui_NextColumn();
		if(ImGui_Checkbox("###Animate Scale", animate_scale)){
			SetCurrentTransform();
		}
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Play In Parallel");
		ImGui_NextColumn();
		ImGui_Checkbox("###Play In Parallel", parallel_operation);
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Update Collision");
		ImGui_NextColumn();
		ImGui_Checkbox("###Update Collision", update_collision);
		ImGui_NextColumn();
	}

	void SetCurrentTransform(){
		if(animation_method == timeline_method){
			TimelineSetTransform(animation_timer);
		}else if(animation_method == placeholder_method){
			PlaceholderSetTransform();
		}
	}

	void ReceiveEditorMessage(array<string> messages){
		if(animation_method == placeholder_method){
			if(messages[0] == "added_object"){
				int obj_id = atoi(messages[1]);
				if(!ObjectExists(obj_id)){
					return;
				}
				Object@ obj = ReadObjectFromID(obj_id);
				ScriptParams@ obj_params = obj.GetScriptParams();
				if(obj_params.HasParam("Owner")){
					if(obj_params.GetInt("Owner") == this_hotspot.GetID()){
						//The new object is a duplicated animation key of this animation.
						key_ids.insertAt(obj_params.GetInt("Index") + 1, obj_id);
						WriteAnimationKeyParams();
					}
				}
			}
		}
	}

	bool Trigger(){
		array<Object@> targets = target_select.GetTargetObjects();
		// Don't do anything if the target object does not exist.
		// But cintinue when animation target is the camera. Which does not return a target.
		if(targets.size() == 0 && target_select.identifier_type != cam){
			return false;
		}

		if(!animation_started){
			animation_started = true;
			if(target_select.identifier_type == cam){
				level.SendMessage("animating_camera true " + hotspot.GetID());
			}
		}

		//When the animation is done, and the trigger is called again, then start the animation over.
		if(done){
			Reset();
			return false;
		}

		UpdateAnimation();

		if(animation_finished){
			done = true;
			if(target_select.identifier_type == cam){
				level.SendMessage("animating_camera false " + hotspot.GetID());
			}
			return true;
		}else{
			return false;
		}
	}

	void SkipAnimation(){
		if(animation_type == forward){
			animation_timer = duration;
		}else if(animation_type == backward){
			animation_timer = 0.0;
		}else if(animation_type == looping_forwards){
			animation_timer = duration;
		}else if(animation_type == looping_backwards){
			animation_timer = 0.0;
		}else if(animation_type == looping_forwards_and_backwards){
			animation_timer = duration;
		}
		UpdateAnimation();
	}

	void UpdateAnimationKeys(){
		//Make sure there is always at least two animation key available.
		while(key_ids.size() < 2){
			CreateKey();
			WriteAnimationKeyParams();
		}
	}

	void WriteAnimationKeyParams(){
		for(uint i = 0; i < key_ids.size(); i++){
			if(!ObjectExists(key_ids[i])){
				return;
			}
			Object@ key = ReadObjectFromID(key_ids[i]);
			ScriptParams@ key_params = key.GetScriptParams();
			key_params.SetInt("Index", i);
			key_params.SetInt("Owner", this_hotspot.GetID());
		}
	}

	void UpdateAnimation(){
		if(animation_method == timeline_method){
			TimelineUpdateAnimation();
		}else if(animation_method == placeholder_method){
			UpdateAnimationKeys();
			PlaceholderUpdateAnimation();
		}
		SetCurrentTransform();
	}

	float CalculateWholeDistance(){
		float whole_distance = 0.0f;
		for(uint i = 1; i < key_ids.size(); i++){
			whole_distance += distance(ReadObjectFromID(key_ids[i - 1]).GetTranslation(), ReadObjectFromID(key_ids[i]).GetTranslation());
		}
		//Add the distance between the fist and last node as well.
		/* whole_distance += distance(ReadObjectFromID(key_ids[0]).GetTranslation(), ReadObjectFromID(key_ids[(key_ids.size() - 1)]).GetTranslation()); */
		return whole_distance;
	}

	void NextAnimationKey(){
		if(animation_type == looping_forwards){
			if(key_index + 2 == int(key_ids.size())){
				key_index = 0;
				animation_timer = 0.0;
			}else{
				key_index += 1;
			}
			alpha = 0.0;
		}else if(animation_type == looping_backwards){
			if(key_index - 1 == -1){
				key_index = key_ids.size() - 2;
				animation_timer = duration;
			}else{
				key_index -= 1;
			}
			alpha = 1.0;
		}else if(animation_type == looping_forwards_and_backwards){
			if(loop_direction == 1){
				if(key_index + 2 == int(key_ids.size())){
					loop_direction = -1;
					alpha = 1.0;
				}else{
					key_index += 1;
					alpha = 0.0;
				}
			}else{
				if(key_index - 1 == -1){
					loop_direction = 1;
					alpha = 0.0;
				}else{
					key_index -= 1;
					alpha = 1.0;
				}
			}
		}else if(animation_type == forward){
			if(key_index + 2 == int(key_ids.size())){
				animation_finished = true;
			}else{
				key_index += 1;
				alpha = 0.0;
			}
		}else if(animation_type == backward){
			if(key_index - 1 == -1){
				animation_finished = true;
			}else{
				key_index -= 1;
				alpha = 1.0;
			}
		}

		@current_key = ReadObjectFromID(key_ids[key_index]);
		@next_key = ReadObjectFromID(key_ids[key_index + 1]);
	}

	void CameraPlaceholderCheck(){
		if(target_select.identifier_type == cam){
			if(!placeholder.Exists()){
				placeholder.path = "Data/Objects/placeholder/camera_placeholder.xml";
				placeholder.Create();
				placeholder.SetTranslation(this_hotspot.GetTranslation() + vec3(0.0, 2.0, 0.0));
			}

			if(show_editor){
				placeholder.SetSpecialType(kCamPreview);
			}else{
				placeholder.SetSpecialType(kSpawn);
			}
		}else{
			placeholder.Remove();
		}
	}

	void ApplyTransform(vec3 translation, quaternion rotation, vec3 scale){
		if(target_select.identifier_type == cam){
			vec3 direction = SingularityFix(rotation);
			const float zoom_sensitivity = 3.5f;
			float zoom = min(150.0f, 90.0f / max(0.001f,(1.0f+(scale.x-1.0f) * zoom_sensitivity)));

			string msg = "drika_dialogue_set_camera_position ";
			msg += direction.x + " ";
			msg += direction.y + " ";
			msg += direction.z + " ";
			msg += translation.x + " ";
			msg += translation.y + " ";
			msg += translation.z + " ";
			msg += animate_scale?zoom:90.0;
			level.SendMessage(msg);

			if(placeholder.Exists()){
				placeholder.SetTranslation(translation);
				placeholder.SetRotation(rotation);
				if(animate_scale){
					placeholder.SetScale(scale);
				}
			}
		}else{
			array<Object@> targets = target_select.GetTargetObjects();

			if(targets.size() > 1 && last_error != target_select.GetTargetDisplayText()){
				last_error = target_select.GetTargetDisplayText();
				DisplayError("DHS Warning", "Trying to animate multiple objects with identifier : " + last_error);
			}

			bool skip_setting_collision = false;

			// When the animation isn't being edited allow the collision update to be skipped.
			if(!editing){
				int allowed_skipped = 0;

				// Allow more skipped set collisions if the framerate is lower.
				if(fps < 15){
					allowed_skipped = 20;
				}else if(fps < 30){
					allowed_skipped = 10;
				}else if(fps < 50){
					allowed_skipped = 5;
				}

				// Keep skipping untill the allowed skip amount is reached.
				if(allowed_skipped > skipped_collision_counter){
					skipped_collision_counter++;
					skip_setting_collision = true;
				}else{
					skipped_collision_counter = 0;
				}
			}

			for(uint i = 0; i < targets.size(); i++){
				if(targets[i].GetType() == _movement_object){
					if(resetting){continue;}
					MovementObject@ char = ReadCharacterID(targets[i].GetID());

					vec3 facing = Mult(rotation, vec3(0,0,1));
					float rot = atan2(facing.x, facing.z) * 180.0f / PI;
					float new_rotation = floor(rot + 0.5f);
					vec3 new_facing = Mult(quaternion(vec4(0, 1, 0, new_rotation * PI / 180.0f)), vec3(1, 0, 0));

					if(char.GetBoolVar("dialogue_control")){
						vec3 direction = SingularityFix(rotation);
						char.ReceiveScriptMessage("set_rotation " + direction.y);
						char.ReceiveScriptMessage("set_dialogue_position " + translation.x + " " + translation.y + " " + translation.z);
						char.Execute("this_mo.velocity = vec3(0.0, 0.0, 0.0);");
						char.Execute("FixDiscontinuity();");
					}else{
						char.SetRotationFromFacing(new_facing);
						char.position = translation;
						char.velocity = vec3(0.0, 0.0, 0.0);
					}

					if(editing){
						targets[i].SetTranslation(translation);
						targets[i].SetRotation(rotation);
					}
				}else{
					if(update_collision && !skip_setting_collision){
						targets[i].SetTranslation(translation);
						targets[i].SetRotation(rotation);
						RefreshChildren(targets[i]);
					}else{
						targets[i].SetTranslationRotationFast(translation, rotation);
					}

					RefreshActors(targets[i]);

					if(animate_scale && !IsGroupDerived(targets[i].GetID())){
						targets[i].SetScale(scale);
					}
				}
			}
		}
	}

	void TimelineUpdateAnimation(){
		if(animation_type == forward){
			animation_timer += time_step;
			if(animation_timer >= duration){
				animation_finished = true;
			}
		}else if(animation_type == backward){
			animation_timer -= time_step;
			if(animation_timer <= 0.0){
				animation_finished = true;
			}
		}else if(animation_type == looping_forwards){
			animation_timer += time_step;
			if(animation_timer >= duration){
				animation_timer = 0.0;
			}
		}else if(animation_type == looping_backwards){
			animation_timer -= time_step;
			if(animation_timer <= 0.0){
				animation_timer = duration;
			}
		}else if(animation_type == looping_forwards_and_backwards){
			if(loop_direction == 1){
				animation_timer += time_step;
				if(animation_timer >= duration){
					loop_direction = -1;
				}
			}else if(loop_direction == -1){
				animation_timer -= time_step;
				if(animation_timer <= 0.0){
					loop_direction = 1;
				}
			}
		}
	}

	void TimelineSetTransform(float current_time){
		bool on_keyframe = false;

		for(uint i = 0; i < key_data.size(); i++){
			if(key_data[i].time == current_time){
				//If the timeline position is exactly on a keyframe then just apply that transform.
				on_keyframe = true;
				new_translation = key_data[i].translation;
				new_rotation = key_data[i].rotation;
				new_scale = key_data[i].scale;
			}
		}

		if(!on_keyframe){
			AnimationKey@ right_key = GetClosestAnimationFrame(current_time, 1, {});
			AnimationKey@ left_key = GetClosestAnimationFrame(current_time, -1, {});

			if(@left_key != null && @right_key != null){
				float whole_length = right_key.time - left_key.time;
				float current_length = right_key.time - current_time;
				alpha = 1.0 - (current_length / whole_length);

				alpha = ApplyTween(alpha, tween_type);

				new_scale = mix(left_key.scale, right_key.scale, alpha);
				new_rotation = mix(left_key.rotation, right_key.rotation, alpha);
				new_translation = mix(left_key.translation, right_key.translation, alpha);
			}else if(@right_key != null){
				new_translation = right_key.translation;
				new_rotation = right_key.rotation;
				new_scale = right_key.scale;
			}else if(@left_key != null){
				new_translation = left_key.translation;
				new_rotation = left_key.rotation;
				new_scale = left_key.scale;
			}else{
				//No keys found.
				return;
			}
		}

		float extra_y_rot = (extra_yaw / 180.0f * PI);
		new_rotation = new_rotation.opMul(quaternion(vec4(0,1,0,extra_y_rot)));

		ApplyTransform(new_translation, new_rotation, new_scale);
	}

	AnimationKey@ GetClosestAnimationFrame(float current_time, int direction, array<AnimationKey@> exceptions){
		AnimationKey@ key = null;
		for(uint i = 0; i < key_data.size(); i++){
			bool hit_exception = false;
			for(uint j = 0; j < exceptions.size(); j++){
				if(exceptions[j] is key_data[i]){
					hit_exception = true;
					break;
				}
			}
			if(hit_exception){
				continue;
			}
			if(	(key_data[i].time > current_time && direction == 1 && (@key == null || key_data[i].time - current_time < key.time - current_time)) ||
				(key_data[i].time < current_time && direction == -1 && (@key == null || current_time - key_data[i].time < current_time - key.time))){
				@key = key_data[i];
			}
		}
		return key;
	}

	void PlaceholderUpdateAnimation(){

		bool going_forward = false;
		if(animation_type == forward || animation_type == looping_forwards || (animation_type == looping_forwards_and_backwards && loop_direction == 1)){
			going_forward = true;
		}

		animation_timer += going_forward?time_step:time_step*-1.0;

		if(duration_method == constant_speed){
			//The animation will have a constant speed.
			float whole_distance = CalculateWholeDistance();
			//When the keys are all at the same location no animation can be performed.
			if(whole_distance == 0.0f){
				return;
			}else{
				float key_distance = distance(next_key.GetTranslation(), current_key.GetTranslation());
				//If the current and next keys are at the same location then just go to the next key.
				if(key_distance == 0.0f){
					NextAnimationKey();
				}else{
					float duration_already_done = 0.0;

					for(int i = 0; i < key_index; i++){
						Object@ key_a = ReadObjectFromID(key_ids[i]);
						Object@ key_b = ReadObjectFromID(key_ids[i + 1]);
						float dist = distance(key_a.GetTranslation(), key_b.GetTranslation());
						duration_already_done += duration * (dist / whole_distance);
					}

					//To make sure the time isn't 0, or else it will devide by zero.
					float duration_between_keys = max(0.0001f, duration * (key_distance / whole_distance));

					float current_length = animation_timer - duration_already_done;
					alpha = current_length / duration_between_keys;
				}
			}
		}else if(duration_method == divide_between_keys){
			//The animation will devide the time between the animation keys.
			float duration_between_keys = max(0.0001, duration / (key_ids.size() - 1));

			float current_length = (key_index * duration_between_keys) - animation_timer;
			alpha = -1.0 * (current_length / duration_between_keys);
		}

		// Clamp the alpha between 0 - 1.
		alpha = max(0.0, min(1.0, alpha));

		// When the timer is going forward.
		if(going_forward){
			// When the timer is over the threshold go to the next keypoint.
			if(alpha >= 1.0){
				NextAnimationKey();
			}
		}else{
			if(alpha <= 0.0){
				NextAnimationKey();
			}
		}
	}

	void PlaceholderSetTransform(){
		alpha = ApplyTween(alpha, tween_type);

		if(alpha == 0.0){
			new_translation = current_key.GetTranslation();
			new_rotation = current_key.GetRotation();
			new_scale = current_key.GetScale();
		}else if(alpha == 1.0){
			new_translation = next_key.GetTranslation();
			new_rotation = next_key.GetRotation();
			new_scale = next_key.GetScale();
		}else{
			new_translation = mix(current_key.GetTranslation(), next_key.GetTranslation(), alpha);
			new_rotation = mix(current_key.GetRotation(), next_key.GetRotation(), alpha);
			new_scale = mix(current_key.GetScale(), next_key.GetScale(), alpha);
		}

		float extra_y_rot = (extra_yaw / 180.0f * PI);
		new_rotation = new_rotation.opMul(quaternion(vec4(0,1,0,extra_y_rot)));

		ApplyTransform(new_translation, new_rotation, new_scale);
	}

	//Current time, start value, change in value, duration
	float sine_wave(float t, float b, float c, float d) {
		return -c/2 * (cos(PI*t/d) - 1) + b;
	}

	void DrawDebugMesh(Object@ object){
		mat4 mesh_transform;
		mesh_transform.SetTranslationPart(object.GetTranslation());
		mat4 rotation = Mat4FromQuaternion(object.GetRotation());
		mesh_transform.SetRotationPart(rotation);

		mat4 scale_mat;
		scale_mat[0] = object.GetScale().x;
		scale_mat[5] = object.GetScale().y;
		scale_mat[10] = object.GetScale().z;
		scale_mat[15] = 1.0f;
		mesh_transform = mesh_transform * scale_mat;

		vec4 color = object.IsSelected()?vec4(0.0f, 0.85f, 0.0f, 0.75f):vec4(0.0f, 0.35f, 0.0f, 0.75f);
		DebugDrawWireMesh("Data/Models/drika_hotspot_cube.obj", mesh_transform, color, _delete_on_draw);
	}

	void DrawEditing(){
		CameraPlaceholderCheck();
		if(animation_method == timeline_method){
			if(target_select.identifier_type == cam){
				DebugDrawLine(placeholder.GetTranslation(), this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
			}
			DrawTimeline();
			MoveAnimationKey();
		}else{
			UpdateAnimationKeys();
			int num_keys = max(0, key_ids.size() - 1);
			for(int i = 0; i < num_keys; i++){
				if(ObjectExists(key_ids[i]) && ObjectExists(key_ids[i+1])){
					Object@ current_key = ReadObjectFromID(key_ids[i]);
					Object@ next_key = ReadObjectFromID(key_ids[i+1]);
					if(i == 0){
						DrawDebugMesh(current_key);
					}
					DrawDebugMesh(next_key);
					DebugDrawLine(current_key.GetTranslation(), next_key.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
				}else if(!ObjectExists(key_ids[i])){
					key_ids.removeAt(i);
					WriteAnimationKeyParams();
					return;
				}else if(!ObjectExists(key_ids[i+1])){
					key_ids.removeAt(i+1);
					WriteAnimationKeyParams();
					return;
				}
			}
			if(target_select.identifier_type == cam){
				DebugDrawLine(placeholder.GetTranslation(), this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);

				for(uint i = 0; i < key_ids.size(); i++){
					Object@ key = ReadObjectFromID(key_ids[i]);
					if(key.IsSelected()){
						placeholder.SetTranslation(key.GetTranslation());
						placeholder.SetRotation(key.GetRotation());
						if(animate_scale){
							placeholder.SetScale(key.GetScale());
						}
					}
				}

			}
		}

		if(target_select.identifier_type != cam){
			array<Object@> targets = target_select.GetTargetObjects();
			for(uint i = 0; i < targets.size(); i++){
				DebugDrawLine(targets[i].GetTranslation(), this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
			}
		}
	}

	void ApplyActorTransform(){
		if(preview_animation){return;}
		array<MovementObject@> chars = target_select.GetTargetMovementObjects();

		for(uint i = 0; i < chars.size(); i++){
			MovementObject@ char = chars[i];
			Object@ obj = ReadObjectFromID(char.GetID());
			if(obj.IsSelected()){
				vec3 translation = obj.GetTranslation();
				vec3 direction = SingularityFix(obj.GetRotation());
				char.ReceiveScriptMessage("set_rotation " + direction.y);
				char.ReceiveScriptMessage("set_dialogue_position " + translation.x + " " + translation.y + " " + translation.z);
				char.Execute("this_mo.velocity = vec3(0.0, 0.0, 0.0);");
			}
		}
	}

	void Update(){
		ApplyActorTransform();
		if(moving_animation_key){return;}
		UpdateTimelineEditing();
	}

	void UpdateTimelineEditing(){
		if(preview_animation){
			if(GetInputPressed(0, "space")){
				preview_animation = false;
			}else{
				UpdateAnimation();
				timeline_position = animation_timer;
				if(animation_finished){
					preview_animation = false;
				}
			}
		}else{
			//Don't move/insert/delete keys when the modifier keys are pressed.
			if(GetInputDown(0, "lctrl") || GetInputDown(0, "lalt")){
				return;
			}else{
				if(GetInputPressed(0, "i")){
					//Make sure to delete any key that's currently at this position before adding a new one.
					DeleteAnimationKey();
					InsertAnimationKey();
				}else if(GetInputPressed(0, "x")){
					DeleteAnimationKey();
				}else if(GetInputPressed(0, "g")){
					for(uint i = 0, len = key_data.size(); i < len; i++){
						if(key_data[i].time == timeline_position){
							@target_key = key_data[i];
							target_key.moving = true;
							target_key.moving_time = target_key.time;
							moving_animation_key = true;
						}
					}
				}else if(GetInputPressed(0, "c")){
					for(uint i = 0, len = key_data.size(); i < len; i++){
						if(key_data[i].time == timeline_position){
							AnimationKey new_key = key_data[i];

							@target_key = new_key;
							target_key.moving = true;
							target_key.moving_time = target_key.time;
							moving_animation_key = true;

							key_data.insertLast(@new_key);
						}
					}
				}else if(GetInputPressed(0, "space")){
					if(animation_finished){
						Reset();
					}
					animation_started = true;
					preview_animation = true;
				}
			}
		}
	}

	void MoveAnimationKey(){
		if(!moving_animation_key){return;}
		target_key.moving_time = min(timeline_duration, max(0.0, ImGui_GetMousePos().x - margin / 2.0) * timeline_duration / timeline_width);
		if(timeline_snap){
			float lowest = floor(target_key.moving_time * 10.0) / 10.0;
			float highest = ceil(target_key.moving_time * 10.0) / 10.0;
			if(abs(target_key.moving_time - lowest) < abs(highest - target_key.moving_time)){
				target_key.moving_time = lowest;
			}else{
				target_key.moving_time = highest;
			}
		}
		//Apply the movement to the keyframe.
		if(ImGui_IsMouseClicked(0)){
			//Remove any keyframe that is currently at this position.
			for(uint i = 0; i < key_data.size(); i++){
				if(key_data[i] !is target_key && key_data[i].time == target_key.moving_time){
					key_data.removeAt(i);
					i--;
				}
			}
			target_key.moving = false;
			target_key.time = target_key.moving_time;
			moving_animation_key = false;
		//Cancel the movement to the keyframe.
		}else if(ImGui_IsMouseClicked(1)){
			target_key.moving = false;
			moving_animation_key = false;
		}
	}

	void EditDone(){
		preview_animation = false;
		for(uint i = 0; i < key_ids.size(); i++){
			if(key_ids[i] != -1 && ObjectExists(key_ids[i])){
				Object@ current_key = ReadObjectFromID(key_ids[i]);
				current_key.SetSelected(false);
				current_key.SetSelectable(false);
			}
		}
		if(target_select.identifier_type == cam){
			placeholder.Remove();
		}
		DrikaElement::EditDone();
	}

	void ApplySettings(){
		Reset();
	}

	void StartEdit(){
		CameraPlaceholderCheck();
		DrikaElement::StartEdit();
		for(uint i = 0; i < key_ids.size(); i++){
			if(key_ids[i] != -1 && ObjectExists(key_ids[i])){
				Object@ current_key = ReadObjectFromID(key_ids[i]);
				current_key.SetSelectable(true);
			}
		}
		Reset();
	}

	void CreateKey(){
		int new_key_id = CreateObject("Data/Objects/drika_hotspot_cube.xml", false);
		key_ids.insertLast(new_key_id);
		Object@ new_key = ReadObjectFromID(new_key_id);
		new_key.SetName("Animation Key");
		new_key.SetDeletable(true);
		new_key.SetCopyable(true);
		new_key.SetSelectable(true);
		new_key.SetTranslatable(true);
		new_key.SetScalable(true);
		new_key.SetRotatable(true);
		new_key.SetScale(vec3(1.0));
		new_key.SetTranslation(this_hotspot.GetTranslation() + vec3(0, key_ids.size(), 0));
	}

	void InsertAnimationKey(){
		AnimationKey new_key;
		Object@ target;
		if(target_select.identifier_type == cam){
			@target = placeholder.cube_object;
		}else{
			array<Object@> targets = target_select.GetTargetObjects();
			if(targets.size() == 0){
				return;
			}
			@target = targets[0];
		}
		new_key.time = timeline_position;
		new_key.translation = target.GetTranslation();
		new_key.rotation = target.GetRotation();
		new_key.scale = target.GetScale();
		key_data.insertLast(@new_key);
	}

	void DeleteAnimationKey(){
		for(uint i = 0; i < key_data.size(); i++){
			if(key_data[i].time == timeline_position){
				key_data.removeAt(i);
				i--;
			}
		}
	}

	void LeftClick(){
		if(animation_method == timeline_method){
			DrikaElement::LeftClick();
		}else if(animation_method == placeholder_method){
			int selected_index = -1;
			for(uint i = 0; i < key_ids.size(); i++){
				Object@ next_key = ReadObjectFromID(key_ids[i]);
				if(next_key.IsSelected()){
					selected_index = i;
					break;
				}
			}
			if(selected_index != -1){
				selected_index += 1;
				if(selected_index >= int(key_ids.size())){
					selected_index = 0;
				}
				for(uint i = 0; i < key_ids.size(); i++){
					Object@ next_key = ReadObjectFromID(key_ids[i]);
					if(selected_index == int(i)){
						next_key.SetSelected(true);
					}else{
						next_key.SetSelected(false);
					}
				}
			}
		}
	}

	void DrawTimeline(){
		timeline_width = GetScreenWidth() - margin;
		timeline_height = GetScreenHeight() - margin;
		if(duration > 0.0){
			timeline_duration = duration;
		}

		if(ImGui_Begin("Animation Timeline", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing)){
			vec2 current_position = ImGui_GetWindowPos() + vec2(margin / 2.0);
			float step_size = 10.0f;
			if(timeline_duration > 50.0){
				step_size = 1.0f;
			}
			float line_separation = timeline_width / (timeline_duration * step_size);
			//Add one more line for the 0 keyframe.
			int nr_lines = int(timeline_duration * step_size) + 1;
			for(int i = 0; i < nr_lines; i++){
				ImDrawList_AddLine(current_position + ((i%10==0?vec2():vec2(0.0, 20.0))), current_position + vec2(0, timeline_height), ImGui_GetColorU32(vec4(1.0, 1.0, 1.0, 1.0)), 1.0f);

				string frame_label = formatFloat((i / step_size), '0l', 2, 1);
				if(line_separation > 30.0 || i%10==0){
					ImDrawList_AddText(current_position - vec2(10.0, 10.0), ImGui_GetColorU32(i%10==0?vec4(1.0, 1.0, 1.0, 0.5):vec4(1.0, 1.0, 1.0, 0.25)), frame_label);
				}

				current_position += vec2(line_separation, 0.0);
			}
			if(ImGui_IsWindowHovered() && ImGui_IsMouseDown(0)){
				timeline_position = min(timeline_duration, max(0.0, ImGui_GetMousePos().x - margin / 2.0) * timeline_duration / timeline_width);
				if(timeline_snap){
					float lowest = floor(timeline_position * 10.0) / 10.0;
					float highest = ceil(timeline_position * 10.0) / 10.0;
					if(abs(timeline_position - lowest) < abs(highest - timeline_position)){
						timeline_position = lowest;
					}else{
						timeline_position = highest;
					}
				}
				animation_timer = timeline_position;
				SetCurrentTransform();
			}
		}
		//Draw the current position on the timeline.
		vec2 cursor_position = ImGui_GetWindowPos() + vec2(margin / 2.0, 0.0);
		//Convert the time in msec to a x position on the timeline.
		cursor_position += vec2(timeline_position * timeline_width / timeline_duration, 0.0);
		ImDrawList_AddLine(cursor_position, cursor_position + vec2(0, timeline_height), ImGui_GetColorU32(vec4(0.0, 1.0, 0.0, 1.0)), 4.0f);

		for(uint i = 0; i < key_data.size(); i++){
			vec2 key_position = ImGui_GetWindowPos() + vec2(margin / 2.0, 0.0);
			//Convert the time in msec to a x position on the timeline.
			float keyframe_time = key_data[i].moving?key_data[i].moving_time:key_data[i].time;
			key_position += vec2(keyframe_time * timeline_width / timeline_duration, 0.0);
			ImDrawList_AddLine(key_position + vec2(0.0, 20.0), key_position + vec2(0, timeline_height), ImGui_GetColorU32(vec4(1.0, 0.75, 0.0, 0.85)), 4.0f);
		}

		ImGui_SetWindowSize("Animation Timeline", vec2(GetScreenWidth(), GetScreenHeight() / 8.0));
		ImGui_SetWindowPos("Animation Timeline", vec2(0.0f, GetScreenHeight() - (GetScreenHeight() / 8.0)));
		ImGui_End();
	}

	void Reset(){
		if(animation_type == forward){
			animation_timer = 0.0;
			alpha = 0.0;
		}else if(animation_type == backward){
			animation_timer = duration;
			alpha = 1.0;
		}else if(animation_type == looping_forwards){
			animation_timer = 0.0;
			alpha = 0.0;
		}else if(animation_type == looping_backwards){
			animation_timer = duration;
			alpha = 1.0;
		}else if(animation_type == looping_forwards_and_backwards){
			animation_timer = 0.0;
			alpha = 0.0;
		}

		previous_translation = vec3();
		timeline_position = 0.0;
		animation_started = false;
		loop_direction = 1.0;
		done = false;
		animation_finished = false;
		preview_animation = false;

		if(target_select.identifier_type == cam){
			level.SendMessage("animating_camera false " + hotspot.GetID());
		}

		if(animation_method == placeholder_method){
			UpdateAnimationKeys();
			if(animation_type == looping_forwards || animation_type == looping_forwards_and_backwards || animation_type == forward){
				key_index = 0;
				@current_key = ReadObjectFromID(key_ids[key_index]);
				@next_key = ReadObjectFromID(key_ids[key_index + 1]);
			}else{
				key_index = key_ids.size() - 2;
				@current_key = ReadObjectFromID(key_ids[key_index]);
				@next_key = ReadObjectFromID(key_ids[key_index + 1]);
			}
		}
		SetCurrentTransform();
	}
}
