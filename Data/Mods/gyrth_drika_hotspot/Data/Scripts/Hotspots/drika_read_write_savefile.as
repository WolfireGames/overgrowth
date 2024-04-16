enum read_write_modes	{
									read = 0,
									write = 1
						};

enum operator_modes	{
									operator_mode_set = 0,
									operator_mode_reference = 1,
									operator_mode_add = 2,
									operator_mode_subtract = 3,
									operator_mode_divide = 4,
									operator_mode_multiply = 5,
									operator_mode_addvariable = 6,
									operator_mode_subtractvariable = 7,
									operator_mode_dividevariable = 8,
									operator_mode_multiplyvariable = 9
					};

enum error_states	{
									error_state_nothing = 0,
									error_state_opcheckstring = 1,
									error_state_dividebyzero = 2,
									error_state_floatoverflow = 3,
									error_state_emptyvariable = 4
					};

class DrikaReadWriteSaveFile : DrikaElement{
	//The variables that can be changed by the user don't have a default value, instead this is done in the constructor.

	~DrikaReadWriteSaveFile(){
		save_file.WriteInPlace();
	}

	read_write_modes read_write_mode;

	int current_read_write_mode;
	int current_operator_mode;
	int current_check_mode;

	//These arrays store the data for each condition.
	array<string> parameters;
	array<string> values;
	array<check_modes> checkmodes;
	array<operator_modes> operatormodes;

	//These three variables are for the debugging options at the bottom of the function. Their values don't get saved after we leave the level or copy the hotspot.
	string direct_set_param = "";
	string direct_set_value = "";
	bool debug_display_values = false;

	int current_error_state = error_state_nothing;

	//This is referred to a LOT to determine which condition we are talking about.
	int condition_count;

	//This is used in the Trigger function to catch errors and pass them to DisplayString.

	DrikaGoToLineSelect@ continue_element;
	bool continue_if_false;
	bool if_any_are_true;
	//These strings are not changed by the user, but simply to display readable text in the dropdown menus.
	array<string> mode_choices = {"Read", "Write"};
	array<string> operator_mode_choices = 	{
												"Set To",
												"Reference Variable",
												"Add",
												"Subtract",
												"Divide By",
												"Multiply By",
												"Add Variable",
												"Subtract Variable",
												"Divide By Variable",
												"Multiply By Variable"
											};
	//These are versions of the above strings that are used in DisplayString.
	array<string> check_mode_display = {"=", "not =", ">", "<", "= Variable", "not = Variable", "> Variable", "< Variable"};
	//There are two copies here because of how Display String splits up the statement to make it easier to read for non coders.
	array<string> operator_mode_display1 = {"Set", "Set", "Add", "Subtract", "Divide", "Multiply", "Add", "Subtract", "Divide", "Multiply"};
	array<string> operator_mode_display2 = {"To", "Variable", "To", "From", "By", "By", "To Variable", "From Variable", "By Variable", "By Variable"};
	//These are the error messages the user might see.
	array<string> error_state_display = {"NO ERROR: The user should never see this message",
										 "ERROR: Attempting a mathematical check or operation on a string",
										 "ERROR: Attempting to divide by zero",
										 "ERROR: Overflow - number too big or string too long",
										 "ERROR: Attempting to access a variable that doesn't exist"
										};

	string saved_parameter;
	string saved_value;
	string local_parameter;
	string local_value;

	float max_float_range = 3.402823466e+38;

	DrikaReadWriteSaveFile(JSONValue params = JSONValue()){
		//Every user editable variable is retrieved from the JSON data.
		//However when a variable isn't found the default value at the end is returned.
		//This makes sure older savedata is still valid when adding new functions.ar
		continue_if_false = GetJSONBool(params, "continue_if_false", false);
		if_any_are_true = GetJSONBool(params, "if_any_are_true", false);
		@continue_element = DrikaGoToLineSelect("continue_line", params);

		parameters = GetJSONStringArray(params, "parameters", array<string> = { "" });
		values = GetJSONStringArray(params, "values", array<string> = { "" });

		array<int> array_int_operatormodes = GetJSONIntArray(params, "operatormodes", array<int> = { operator_mode_set });
		operatormodes.resize(array_int_operatormodes.length());

		for(uint i = 0; i < array_int_operatormodes.length(); i++){
			operatormodes[i] = operator_modes(array_int_operatormodes[i]);
		}

		array<int> array_int_checkmodes = GetJSONIntArray(params, "checkmodes", array<int> = { check_mode_equals });
		checkmodes.resize(array_int_checkmodes.length());

		for(uint i = 0; i < array_int_checkmodes.length(); i++){
			checkmodes[i] = check_modes(array_int_checkmodes[i]);
		}

		condition_count = int(parameters.length());

		//The mode and count are enum values which can't be used by dropdown menus (combo).
		read_write_mode = read_write_modes(GetJSONInt(params, "read_write_mode", 0));
		//So we need to use an extra integer value to keep track of the currently selected dropdown item.
		current_read_write_mode = read_write_mode;

		//Every DHS function has it's own enum value that describes it's type.
		drika_element_type = drika_read_write_savefile;
		//Not sure why this was added. But the settings can be turned off if there are none to show.
		has_settings = true;
	}

	void PostInit(){
		//Objects in the scene are often not ready yet when you run your function in the constructor.
		//That's why we run this function on the first update only once, in which case all objects in the scene are added.
		continue_element.PostInit();
	}

	JSONValue GetSaveData(){
		//Every function returns it's savedata to DHS for saving separately.
		//The JSON library takes any variable type, and when writing to a file it saves it as strings.
		JSONValue data;

		data["parameters"] = JSONValue(JSONarrayValue);
		data["values"] = JSONValue(JSONarrayValue);
		data["operatormodes"] = JSONValue(JSONarrayValue);
		data["checkmodes"] = JSONValue(JSONarrayValue);

		for (int i = 0; i < condition_count; i++)
			(data["parameters"])[i] = JSONValue(parameters[i]);

		for (int i = 0; i < condition_count; i++)
			(data["values"])[i] = JSONValue(values[i]);

		for (int i = 0; i < condition_count; i++)
			(data["operatormodes"])[i] = JSONValue(operatormodes[i]);

		for (int i = 0; i < condition_count; i++)
			(data["checkmodes"])[i] = JSONValue(checkmodes[i]);

		data["read_write_mode"] = JSONValue(read_write_mode);
		data["continue_if_false"] = JSONValue(continue_if_false);
		data["if_any_are_true"] = JSONValue(if_any_are_true);

		if(continue_if_false){
			continue_element.SaveGoToLine(data);
		}

		return data;
	}

	string GetDisplayString(){
		//This creates a readable string for the UI to display.
		//Setting the text color and determinating text cutoff is done in the main DHS script.
		continue_element.CheckLineAvailable();
		string display_string;
		string display_value = IsFloat(values[0]) == true ? ReduceZeroes(values[0]) : values[0];

		if(current_error_state == error_state_nothing){ //This is determined in the Trigger function.
				switch(read_write_mode){
					case read:
						{
							if(condition_count == 1){
								display_string += "Check if " + parameters[0] + " " + check_mode_display[checkmodes[0]] +  " " + display_value;
								display_string += (continue_if_false?" else line " + continue_element.GetTargetLineIndex():"");
							}else{
								//Perhaps in the future this could show the first two or three conditions. This could get cluttered tho. Needs testing.
								display_string += "Check multiple conditions";
							}
						}
						break;
					case write:
						{
							if(condition_count == 1){
								//TODO - this should be rewritten into a switch statement or something
								if(operatormodes[0] == operator_mode_add || operatormodes[0] == operator_mode_subtract || operatormodes[0] == operator_mode_addvariable || operatormodes[0] == operator_mode_subtractvariable){
									//Reverse order of parameters and values if operation is Add or Subtract (better readability)
									display_string += operator_mode_display1[operatormodes[0]] +  " " + display_value + " " + operator_mode_display2[operatormodes[0]] +  " " +  parameters[0];
								}else{
									display_string += operator_mode_display1[operatormodes[0]] +  " " + parameters[0] + " " + operator_mode_display2[operatormodes[0]] +  " " +  display_value;
								}
							}else{
								display_string += "Set multiple variables";
							}
						}
						break;
					}
			//Because of how DHS works, this error will not appear while inside the DHS editor. DHS will have to try to run it first.
			//It would be nice to have this error show up immediately, but that would likely require a substantial change in how DHS works first.
			//Probably not worth it.
			}else{
				display_string += error_state_display[current_error_state];
			}

		return display_string;
	}

	void DrawSettings(){
		//The settings are divided into two column by default. (Exceptions do exist)
		//Inside the first column a description/name of the setting is displayed.
		//To keep this visible at all times a static width is used.
		float option_name_width = 190.0;

		//You need to keep good track of the number of columns and how many times you use NextColumn.
		//Or else every UI element going forward is going to be in the wrong column.
		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		//This function makes the UI element vertically centered.
		//We need to do this a lot because going to a new column seems to undo this setting.
		ImGui_AlignTextToFramePadding();
		//By default we are on the first column, so add the first descriptor for the setting.
		ImGui_Text("Read Write Mode");
		//Now we are going to the second column where the actual setting is displayed.
		ImGui_NextColumn();
		//The window size can change so we need to calculate the maximum amount of width we have left.
		float second_column_width = ImGui_GetContentRegionAvailWidth();

		//This values is then used to make sure dropdown menus and other elements have the correct width.
		ImGui_PushItemWidth(second_column_width);
		//We use an enum for tracking the current read/write mode, but a combo can only use integers for the currently selected item.
		//So when the combo changes the ``current_`` value, we also change the enum value by casting it to the correct enum type.
		if(ImGui_Combo("##Read Write Mode", current_read_write_mode, mode_choices, mode_choices.size())){
			read_write_mode = read_write_modes(current_read_write_mode);
		}
		//The setting is now rendering so clear the width for the next item.
		ImGui_PopItemWidth();
		//Using NextColumn here goes from the second column to the first column because we only have two columns.
		ImGui_NextColumn();

		if(read_write_mode == read){
			//You can show/hide certain settings based on conditions.
			//However the settings still keep their values.
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Number of conditions");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			//Probably not a good idea to allow the user to create 5000 conditions and send the file size of the level xml through the roof
			//So the slider goes to 10 and the user can override, up to 25 conditions. Anything above that or below 1 gets set back to the permissible range
			if(ImGui_SliderInt("##Condition Count", condition_count, 1, 10, "%.0f"))
			{
				if(condition_count < 1) condition_count = 1;
				if(condition_count > 100) condition_count = 100;
			}

			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_Separator();
			ImGui_NextColumn();
			ImGui_NextColumn();

			//Let's make sure the arrays are the same length as the number of conditions.
			if(int(parameters.length()) != condition_count)
			{
				parameters.resize(condition_count);
				values.resize(condition_count);
				checkmodes.resize(condition_count);
				operatormodes.resize(condition_count);
			}

			//Display input boxes and dropdowns for as many conditions as defined in condition_count.
			for (int i = 0; i < condition_count; i++)
				{
				ImGui_AlignTextToFramePadding();
				ImGui_Text("Condition " + (i + 1));
				ImGui_NextColumn();
				ImGui_NextColumn();

				ImGui_AlignTextToFramePadding();
				ImGui_Text("Check if param");
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);
				if(ImGui_InputText("##Parameter" + (i + 1), parameters[i], 64))
					{parameters[i] = CleanString(parameters[i]);}

				ImGui_PopItemWidth();
				ImGui_NextColumn();

				ImGui_AlignTextToFramePadding();
				ImGui_PushItemWidth(option_name_width - 10);

				current_check_mode = checkmodes[i];
				if(ImGui_Combo("##Check Mode" + (i + 1), current_check_mode, check_mode_choices, check_mode_choices.size()))
					{checkmodes[i] = check_modes(current_check_mode);}
				ImGui_PopItemWidth();
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);

				//Here we want to only allow float inputs when the user is attempting certain checks where strings would never be used.
				//This doesn't guarantee that the user won't mess up anyway, but it decreases the number of conditions where an error could happen.
				if(checkmodes[i] == check_mode_greaterthan || checkmodes[i] == check_mode_lessthan){
					float buf = atof(values[i]);
					ImGui_DragFloat("##Operator Input" + (i + 1), buf, 0.1f, 0.0f, 100.0f, "%.3f", 1.0f);
					values[i] = formatFloat(buf,'', 0, 3);
				} else {
					ImGui_InputText("##Value " + (i + 1), values[i], 64);}
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					ImGui_Separator();
				}

			ImGui_AlignTextToFramePadding();
			ImGui_Text("If not, go to line");
			ImGui_NextColumn();
			ImGui_Checkbox("###If not, go to line", continue_if_false);
			ImGui_NextColumn();

			if(condition_count > 1 && continue_if_false){
				ImGui_AlignTextToFramePadding();
				ImGui_Text("Continue if any true");
				ImGui_NextColumn();
				ImGui_Checkbox("###Continue if any true", if_any_are_true);
				ImGui_NextColumn();
			}

			if(continue_if_false){
				continue_element.DrawGoToLineUI();
			}
			//For Write mode:
		}else{
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Number of parameters");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			//Although using same array(condition_count),"Parameter" makes more sense for this mode
			if(ImGui_SliderInt("##Parameter count", condition_count, 1, 10, "%.0f"))
			{
				if(condition_count < 1) condition_count = 1;
				if(condition_count > 100) condition_count = 100;
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_Separator();
			ImGui_NextColumn();
			ImGui_NextColumn();

			if(int(parameters.length()) != condition_count)
			{
				parameters.resize(condition_count);
				values.resize(condition_count);
				checkmodes.resize(condition_count);
				operatormodes.resize(condition_count);
			}

			for (int i = 0; i < condition_count; i++)
			{
				ImGui_AlignTextToFramePadding();
				ImGui_Text("Variable " + (i + 1));
				ImGui_NextColumn();
				ImGui_NextColumn();
				ImGui_Text("Parameter name");
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);

				if(ImGui_InputText("##Parameter" + (i + 1), parameters[i], 64))
					{parameters[i] = CleanString(parameters[i]);}

				ImGui_PopItemWidth();
				ImGui_NextColumn();

				ImGui_AlignTextToFramePadding();
				ImGui_PushItemWidth(option_name_width - 10);

				current_operator_mode = operatormodes[i];

				if(ImGui_Combo("##Operator Mode " + (i + 1), current_operator_mode, operator_mode_choices, operator_mode_choices.size()))
					{operatormodes[i] = operator_modes(current_operator_mode);}

				ImGui_PopItemWidth();

				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);

				//Here we are also checking for cases where the user should only ever input a float, and changing the input box accordingly.
				if(operatormodes[i] < operator_mode_subtract || operatormodes[i] > operator_mode_multiply){
					ImGui_InputText("##Value" + (i + 1), values[i], 64);

				//The user is limited to two decimal positions. This is just to makes things easier to read and keep the strings shorter in DisplayString.
				} else {
					float buf = atof(values[i]);
					ImGui_DragFloat("##Operator Input" + (i + 1), buf, 0.1f, 0.0f, 100.0f, "%.3f", 1.0f);
					values[i] = formatFloat(buf,'', 0, 3);
				}

				ImGui_PopItemWidth();
				ImGui_NextColumn();
				ImGui_Separator();
			}

			//There may be cases where it's useful to directly set a parameter/value combo for testing purposes. This allows us to do that without running the hotspot.
			if(ImGui_Button("Debug Set Value"))	{WriteParamValue(0,false,true);}
			float text_width = ImGui_CalcTextSize("Parameter: Value: ").x;
			ImGui_NextColumn();
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Parameter: ");
			ImGui_SameLine();
			ImGui_PushItemWidth((second_column_width - text_width)/2 - 10);
			if(ImGui_InputText("###DebugSetParameter", direct_set_param, 64)){
				direct_set_param = CleanString(direct_set_param);
			}
			ImGui_PopItemWidth();
			ImGui_SameLine();
			ImGui_Text("Value: ");
			ImGui_SameLine();
			ImGui_PushItemWidth((second_column_width - text_width)/2 - 12);
			ImGui_InputText("###DebugSetValue", direct_set_value, 64);
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}

		//It could also be very useful to know the current saved value of the parameters you are working with. This next bit provides that option.
		//Keep in mind that both of these debug functions are for testing, so we don't bother to save their states.
		ImGui_AlignTextToFramePadding();
		ImGui_Text("Display values?");
		ImGui_NextColumn();
		ImGui_Checkbox("   ", debug_display_values);
		ImGui_NextColumn();
		ImGui_NextColumn();
		ImGui_PushItemWidth (ImGui_GetContentRegionAvailWidth());
		if(debug_display_values == true){
			for (int i = 0; i < condition_count; i++){
				if(CheckParamExists(parameters[i]) == true){
					ImGui_Text(parameters[i] + " = " + ReadParamValue(parameters[i]));
				}else{
					ImGui_Text("This variable does not exist!");}
					ImGui_NextColumn();
					ImGui_NextColumn();
				}
			}
		ImGui_PopItemWidth();
	}

	string CleanString(string my_string){
		for (int i = int(my_string.length()) - 1; i >= 0; i--){
			if(my_string[i] == "["[0] || my_string[i] == "]"[0]|| my_string[i] == "{"[0]|| my_string[i] == "}"[0]){
				my_string.erase(i, 1);
			}
		}
		return my_string;
	}

	//In this function, array_pos is the important bit. It's taken from the current condition_count in the Trigger loop.
	//There are a few cases where we want to pass extra info. is_float is used for add and addvariable. set_direct is for the debug set value button.
	void WriteParamValue(int array_pos, bool is_float = false, bool set_direct = false){
		SavedLevel@ data = save_file.GetSavedLevel("drika_data");

			string value_to_save;

			ParseParameters(array_pos);

			switch (operatormodes[array_pos])
				{
				case operator_mode_set: 				value_to_save = local_value; break;
				case operator_mode_reference: 			value_to_save = saved_value; break;

				//In the case of add and addvariable, we do different operations depending on whether both the values are floats or not.
				//If they are floats we do a normal addition operation. If either or both values are strings, we concatenate the values instead.
				case operator_mode_add: 				if(is_float == true) {value_to_save = ReduceZeroes(formatFloat((atof(saved_parameter) + atof(local_value)),'', 0, 3));}
														else  {value_to_save = saved_parameter + local_value;} break;
				case operator_mode_subtract: 			value_to_save = ReduceZeroes(formatFloat((atof(saved_parameter) - atof(local_value)),'', 0, 3)); break;
				case operator_mode_divide: 				value_to_save = ReduceZeroes(formatFloat((atof(saved_parameter) / atof(local_value)),'', 0, 3)); break;
				case operator_mode_multiply: 			value_to_save = ReduceZeroes(formatFloat((atof(saved_parameter) * atof(local_value)),'', 0, 3)); break;

				case operator_mode_addvariable: 		if(is_float == true) {value_to_save = ReduceZeroes(formatFloat((atof(saved_parameter) + atof(saved_value)),'', 0, 3));}
														else  {value_to_save = saved_parameter + saved_value;} break;

				case operator_mode_subtractvariable: 	value_to_save = ReduceZeroes(formatFloat((atof(saved_parameter) - atof(saved_value)),'', 0, 3)); break;
				case operator_mode_dividevariable: 		value_to_save = ReduceZeroes(formatFloat((atof(saved_parameter) / atof(saved_value)),'', 0, 3)); break;
				case operator_mode_multiplyvariable: 	value_to_save = ReduceZeroes(formatFloat((atof(saved_parameter) * atof(saved_value)),'', 0, 3)); break;
				}
		//If the Debug Set Value button is pressed, we want to ignore the usual way of doing things and take our values directly from those two input boxes.
		if(set_direct == true)
			{
			data.SetValue(direct_set_param, direct_set_value);
			data.SetValue("[" + direct_set_param + "]", "true");

			}
		//Otherwise we base everything on what the Trigger function passed along.
		else
			{
			data.SetValue(local_parameter, value_to_save);
			data.SetValue(("[" + local_parameter + "]"), "true"); //This additional var is used to check if the var exists
			}
		//save_file.WriteInPlace();
		level.SendMessage("drika_variable_changed");
	}

	string ReduceZeroes(string input){
		if(input.findFirst(".") >= 0){
			//Go over each character backwards.
			for(int i = input.length() - 1; i >= 0; i--){
				//If you find the decimal point, remove it and return the string.
				if(input[i] == "."[0]){
					input.erase(i, 1);
					break;
				//Remove any zero.
				}else if(input[i] == "0"[0]){
					input.erase(i, 1);
				}else{
					//Once we encounter a number, stop removing zeros.
					break;
				}
			}
		}
		return input;
	}

	//The Trigger function does all the heavy lifting for this bit.
	string ReadParamValue(string key){
		SavedLevel@ data = save_file.GetSavedLevel("drika_data");
		return data.GetValue(key);
	}

	bool CheckParamExists(string key){
		return ReadParamValue("[" + key + "]") == "true";
	}

	void ParseParameters(int index)
	{
		saved_parameter = ReadParamValue(parameters[index]);
		saved_value = ReadParamValue(values[index]);
		local_parameter = parameters[index];
		local_value = values[index];
	}

	bool Trigger()
	{
		//When DHS arrives at this function it will keep running Trigger on every update until true is returned.
		//In which case this function is done. Every function needs to handle it's own reset on trigger if it supports multiple triggers.
		int continue_line = continue_element.GetTargetLineIndex();
		current_error_state = error_state_nothing;

		if(read_write_mode == read) { //READ
			//For the Read mode we need to know if either all, some, or none of the conditions are true. An easy way to do this is to count upwards every time a condition is true.
			//If count reads 0, no condition is true. If count = condition_count, all the conditions are true. If < condition_count but > 0, some are true.

			int count = 0;

			//There are certain cases where the user has told DHS to perform an illegal operation.
			//When this happens we flag it and stop the hotspot from going to the next line.
			//When the user investigates why the hotspot isn't working, they should see an error message for that line.
			current_error_state = error_state_nothing;

			for (int i = 0; i < condition_count; i++){

				ParseParameters(i);

				switch (checkmodes[i])
					{
					case check_mode_equals:
						if(CheckExisting(local_parameter, local_parameter) == false)			{current_error_state = error_state_emptyvariable;}
						//Because floats aren't exact, we simply check if they are very very close.
						if(IsFloat(saved_parameter) == true) 									{count += CheckEpsilon(saved_parameter, local_value);}
						else																	{count += (saved_parameter == local_value)? 1:0;}
						break;

					case check_mode_notequals:
						if(CheckExisting(local_parameter, local_parameter) == false)			{current_error_state = error_state_emptyvariable;}
						if(IsFloat(saved_parameter) == true) 									{count += CheckEpsilon(saved_parameter, local_value);}
						else																	{count += (saved_parameter != local_value)? 1:0;}
						break;

					case check_mode_greaterthan:
						if(CheckExisting(local_parameter, local_parameter) == false)			{current_error_state = error_state_emptyvariable;}
						if(IsFloat(saved_parameter) == true)									{count += (atof(saved_parameter) > atof(local_value))? 1:0;}
						else 																	{current_error_state = error_state_opcheckstring;}
						break;

					case check_mode_lessthan:
						if(CheckExisting(local_parameter, local_parameter) == false)			{current_error_state = error_state_emptyvariable;}
						if(IsFloat(saved_parameter) == true)									{count += (atof(saved_parameter) < atof(local_value))? 1:0;}
						else 																	{current_error_state = error_state_opcheckstring;}
						break;

					case check_mode_variable:
						if(CheckExisting(local_parameter, local_value) == false)				{current_error_state = error_state_emptyvariable;}
						if(IsFloat(saved_parameter) == true)									{count += CheckEpsilon(saved_parameter, saved_value);}
						else																	{count += (saved_parameter == saved_value)? 1:0;}
						break;

					case check_mode_notvariable:
						if(CheckExisting(local_parameter, local_value) == false)				{current_error_state = error_state_emptyvariable;}
						if(IsFloat(saved_parameter) == true)									{count += CheckEpsilon(saved_parameter, saved_value);}
						else																	{count += (saved_parameter != saved_value)? 1:0;}
						break;

					case check_mode_greaterthanvariable:
						if(CheckExisting(local_parameter, local_value) == false)				{current_error_state = error_state_emptyvariable;}
						if(IsFloat(saved_parameter) == true && IsFloat(saved_value) == true)	{count += (atof(saved_parameter) > atof(saved_value))? 1:0;}
						else 																	{current_error_state = error_state_opcheckstring;} break;

					case check_mode_lessthanvariable:
						if(CheckExisting(local_parameter, local_value) == false)				{current_error_state = error_state_emptyvariable;}
						if(IsFloat(saved_parameter) == true && IsFloat(saved_value) == true)	{count += (atof(saved_parameter) < atof(saved_value))? 1:0;}
						else 																	{current_error_state = error_state_opcheckstring;} break;
					}
			}
				//Now that we've completed the count of what returned true, let's compare that to condition count and decide what to do.
				if(count == condition_count || (count > 0 && if_any_are_true))	return current_error_state == error_state_nothing;
				else if(continue_if_false == true and continue_line < int(drika_elements.size())){
					current_line = continue_line;
					display_index = drika_indexes[continue_line];}
				return false;
		}

		else //In the Write mode, the WriteParamValue function does the heavy lifting. Our main concern here is ensuring the user doesn't attempt an illegal operation.
			{
				current_error_state = error_state_nothing;

				for (int i = 0; i < condition_count; i++)
				{

				ParseParameters(i);

				bool thisoperation_float = false;

					switch (operatormodes[i])
					{
						case operator_mode_set:
							if(CheckOverflow(local_value, "set") == true)							{current_error_state = error_state_floatoverflow;}
							break;

						case operator_mode_reference:
							if(CheckExisting(local_value, local_value) == false)					{current_error_state = error_state_emptyvariable;}
							if(CheckOverflow(saved_value, "set") == true)							{current_error_state = error_state_floatoverflow;}
							break;

						case operator_mode_add:
							if(CheckExisting(local_parameter, local_parameter) == false)				{current_error_state = error_state_emptyvariable;}
							if 	(IsFloat(saved_parameter) == true && IsFloat(local_value) == true) 	{thisoperation_float = true;}
							if(CheckOverflow(saved_parameter, thisoperation_float ? "+" : "concat", local_value) == true)			{current_error_state = error_state_floatoverflow;}
							break;

						case operator_mode_subtract:
							if(CheckExisting(local_parameter, local_parameter) == false)				{current_error_state = error_state_emptyvariable;}
							if(IsFloat(saved_parameter) == true && IsFloat(local_value) == true)	{thisoperation_float = true;}
							if(thisoperation_float == false)										{current_error_state = error_state_opcheckstring;}
							if(CheckOverflow(saved_parameter, "-", local_value) == true)			{current_error_state = error_state_floatoverflow;}
							break;

						case operator_mode_divide:
							if(CheckExisting(local_parameter, local_parameter) == false)				{current_error_state = error_state_emptyvariable;}
							if(IsFloat(saved_parameter) == true && IsFloat(local_value) == true) 	{thisoperation_float = true;}
							if(atof(local_value) == 0.0)											{current_error_state = error_state_dividebyzero;}
							if(thisoperation_float == false)										{current_error_state = error_state_opcheckstring;}
							if(CheckOverflow(saved_parameter, "/", local_value) == true)			{current_error_state = error_state_floatoverflow;}
							break;

						case operator_mode_multiply:
							if(CheckExisting(local_parameter, local_parameter) == false)				{current_error_state = error_state_emptyvariable;}
							if(IsFloat(saved_parameter) == true && IsFloat(local_value) == true) 	{thisoperation_float = true;}
							if(thisoperation_float == false)										{current_error_state = error_state_opcheckstring;}
							if(CheckOverflow(saved_parameter, "*", local_value) == true)			{current_error_state = error_state_floatoverflow;}
							break;

						case operator_mode_addvariable:
							if(CheckExisting(local_parameter, local_value) == false)				{current_error_state = error_state_emptyvariable;}
							if 	(IsFloat(saved_parameter) == true && IsFloat(saved_value) == true) 	{thisoperation_float = true;}
							if(CheckOverflow(saved_parameter, thisoperation_float ? "+" : "concat", saved_value) == true)			{current_error_state = error_state_floatoverflow;}
							break;

						case operator_mode_subtractvariable:
							if(CheckExisting(local_parameter, local_value) == false)				{current_error_state = error_state_emptyvariable;}
							if(IsFloat(saved_parameter) == true && IsFloat(saved_value) == true) 	{thisoperation_float = true;}
							if(thisoperation_float == false)										{current_error_state = error_state_opcheckstring;}
							if(CheckOverflow(saved_parameter, "-", saved_value) == true)			{current_error_state = error_state_floatoverflow;}
							break;

						case operator_mode_dividevariable:
							if(CheckExisting(local_parameter, local_value) == false)				{current_error_state = error_state_emptyvariable;}
							if(IsFloat(saved_parameter) == true && IsFloat(saved_value) == true) 	{thisoperation_float = true;}
							if(atof(saved_value) == 0.0)											{current_error_state = error_state_dividebyzero;}
							if(thisoperation_float == false)										{current_error_state = error_state_opcheckstring;}
							if(CheckOverflow(saved_parameter, "/", saved_value) == true)			{current_error_state = error_state_floatoverflow;}
							break;

						case operator_mode_multiplyvariable:
							if(CheckExisting(local_parameter, local_value) == false)				{current_error_state = error_state_emptyvariable;}
							if(IsFloat(saved_parameter) == true && IsFloat(saved_value) == true) 	{thisoperation_float = true;}
							if(thisoperation_float == false)										{current_error_state = error_state_opcheckstring;}
							if(CheckOverflow(saved_parameter, "*", saved_value) == true)			{current_error_state = error_state_floatoverflow;}
							break;
					}

				if(current_error_state == error_state_nothing){
					WriteParamValue(i,thisoperation_float);
				}
			}
			return current_error_state == error_state_nothing;
		}
	}

	bool CheckOverflow(string operand1, string operation, string operand2 = ""){
		float result;

		if(operation == "concat")
		{
			return (operand1 + operand2).length() >= 1024 * 1024;
		}

		float op1 = atof(operand1);
		float op2 = atof(operand2);

		if 	(operation == "set") result = op1;
		else if(operation == "+") result = op1 + op2;
		else if(operation == "-") result = op1 - op2;
		else if(operation == "*") result = op1 * op2;
		else if(operation == "/") result = op1 / op2;

		return abs(result) > max_float_range;
	}

	bool CheckExisting(string operand1, string operand2){
		return (CheckParamExists(operand1) == true && CheckParamExists(operand2) == true);
	}
}
