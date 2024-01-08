class DrikaSlowMotion : DrikaElement{
	float target_time_scale;
	float duration;
	float delay;
	float timer;
	bool wait;

	DrikaSlowMotion(JSONValue params = JSONValue()){
		target_time_scale = GetJSONFloat(params, "target_time_scale", 0.5);
		duration = GetJSONFloat(params, "duration", 2.0);
		delay = GetJSONFloat(params, "delay", 0.25);
		wait = GetJSONBool(params, "wait", true);
		SetTimer();

		drika_element_type = drika_slow_motion;
		has_settings = true;
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["target_time_scale"] = JSONValue(target_time_scale);
		data["duration"] = JSONValue(duration);
		data["delay"] = JSONValue(delay);
		data["wait"] = JSONValue(wait);
		return data;
	}

	string GetDisplayString(){
		return "SlowMotion " + duration;
	}

	void ApplySettings(){
		SetTimer();
	}

	void SetTimer(){
		timer = (delay + duration) * target_time_scale;
	}

	void DrawSettings(){

		float option_name_width = 150.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Wait until finished");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_Checkbox("###Wait until finished", wait);
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Target Time Scale");
		ImGui_NextColumn();
		ImGui_PushItemWidth(second_column_width);
		ImGui_SliderFloat("###Target Time Scale", target_time_scale, 0.0f, 1.0f, "%.2f");
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Duration");
		ImGui_NextColumn();
		ImGui_PushItemWidth(second_column_width);
		ImGui_SliderFloat("###Duration", duration, 0.0f, 10.0f, "%.2f");
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Delay");
		ImGui_NextColumn();
		ImGui_PushItemWidth(second_column_width);
		ImGui_SliderFloat("###Delay", delay, 0.0f, 10.0f, "%.2f");
		ImGui_PopItemWidth();
		ImGui_NextColumn();
	}

	void Reset(){
		triggered = false;
	}

	bool Trigger(){
		if(!triggered){
			triggered = true;
			TimedSlowMotion(target_time_scale, duration, delay);
		}
		if(timer <= 0.0 || !wait){
			triggered = false;
			SetTimer();
			return true;
		}else{
			timer -= time_step;
			return false;
		}
	}
}
