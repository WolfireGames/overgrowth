// This is a lightweight hotspot for sending level messages and setting DHS variables on characters entering
bool played;

void Reset() {
    played = false;
}

void Init() {
    Reset();
}

void SetParameters() {
    params.AddIntCheckbox("Activate Once", true);
    params.AddIntCheckbox("Activate Player Only", true);
    // Sends a level message
	params.AddIntCheckbox("Level Message Send", false);
    params.AddString("Level Message Value", "level_message");
    // Sets a variable
	params.AddIntCheckbox("Var Set", false);
    params.AddString("Var Set Name", "variable_name");
    params.AddString("Var Set Value", "variable_value");
    // Adds to a variable
	params.AddIntCheckbox("Var Add", false);
    params.AddString("Var Add Name", "variable_name");
    params.AddString("Var Add Value", "0");
    // Var(iable) Add Var(iable) adds one variable (the name) to the other (the value)
	params.AddIntCheckbox("Var Add Var", false);
    params.AddString("Var Add Var Name", "variable_name");
    params.AddString("Var Add Var Value", "variable_value");
}

void SaveDataToFile(string variable_name, string value){ 	//This custom function works like the Write mode in DHS ReadWriteSaveFile
	SavedLevel@ data = save_file.GetSavedLevel("drika_data"); //Here we define where we are saving the data - DHS uses drika_data
	
	data.SetValue(variable_name, value); //First we set the variable and give it a value
	data.SetValue(("[" + variable_name + "]"), "true"); //This additional variable is what DHS uses to check if the variable exists
	
	save_file.WriteInPlace();
}

void AddVariableToVariable(string variable1, string variable2){ //Take two variables, convert them to integers, add them together and write the result to variable2
	SavedLevel@ data = save_file.GetSavedLevel("drika_data");
	int value1 = atoi(data.GetValue(variable1));
	int value2 = atoi(data.GetValue(variable2));
	string value_to_save = "" + (value1 + value2);
	SaveDataToFile(variable2, value_to_save);
}

void AddValueToVariable(int integer1, string variable2){
    SavedLevel@ data = save_file.GetSavedLevel("drika_data");
    int value1 = integer1;
    int value2 = atoi(data.GetValue(variable2));
    string value_to_save = "" + (value1 + value2);
    SaveDataToFile(variable2, value_to_save);
}

void HandleEvent(string event, MovementObject @mo){
    if(event == "enter"){
        OnEnter(mo);
    } else if(event == "exit"){
        OnExit(mo);
    }
}

void OnEnter(MovementObject @mo) {
    if((!played || params.GetInt("Activate Once") == 0)
    && (mo.controlled || params.GetInt("Activate Player Only") == 0)){
        if(params.GetInt("Var Set") == 1){
            SaveDataToFile(params.GetString("Var Set Name"), params.GetString("Var Set Value"));
        }
        if(params.GetInt("Var Add") == 1){
            AddValueToVariable(params.GetInt("Var Add Value"), params.GetString("Var Add Name"));
        }
        if(params.GetInt("Var Add Var") == 1){
            AddVariableToVariable(params.GetString("Var Add Var Name"), params.GetString("Var Add Var Value"));
        }
        if(params.GetInt("Level Message Send") == 1){
            level.SendMessage(params.GetString("Level Message Value"));
        }
    }
    played = true;
}

void OnExit(MovementObject @mo) {
}