array<CoinPickupEvent@> coin_pickup_events = {};
array<int> coins_picked_up = {};
bool reset_progress = false;

class CoinPickupEvent{
	int character_id = -1;
	int item_id = -1;
	float hold_timer = 0.0f;
	float sheathe_timer = 0.0f;

	float HOLD_DURATION = 0.3;
	float SHEATHE_DURATION = 0.4;

	CoinPickupEvent(int _character_id, int _item_id){
		character_id = _character_id;
		item_id = _item_id;
	}

	bool Update(){
		// Wait for the character to hold the item for a little bit, giving them a chance to stand up.
		if(hold_timer < HOLD_DURATION){
			hold_timer += time_step;
			if(hold_timer >= HOLD_DURATION){
				// Start sheathing the held item.
				MovementObject@ character = ReadCharacterID(character_id);
				string command = 	"if(weapon_slots[primary_weapon_slot] == " + item_id + "){StartSheathing(primary_weapon_slot);}" + 
									"if(weapon_slots[secondary_weapon_slot] == " + item_id + "){StartSheathing(secondary_weapon_slot);}";
				character.Execute(command);
			}
		}else if(sheathe_timer < SHEATHE_DURATION){
			// Wait for the sheathing animation to finish.
			sheathe_timer += time_step;
			if(sheathe_timer >= SHEATHE_DURATION){
				DisableItemObject();
				return true;
			}
		}

		return false;
	}

	void DisableItemObject(){
		Object@ obj = ReadObjectFromID(item_id);
		ItemObject@ item = ReadItemID(item_id);
		obj.SetEnabled(false);
		MovementObject@ character = ReadCharacterID(character_id);
		string command = 	"if(weapon_slots[_sheathed_right] == " + item_id + "){this_mo.DetachItem(weapon_slots[_sheathed_right]);weapon_slots[_sheathed_right] = -1;}" +
							"if(weapon_slots[_sheathed_left] == " + item_id + "){this_mo.DetachItem(weapon_slots[_sheathed_left]);weapon_slots[_sheathed_left] = -1;}";
		character.Execute(command);
		// After the item has been detached from the character, move it somewhere so the impact sound can't be heard.
		mat4 transform;
		transform.SetTranslationPart(vec3(0.0, 0.0, 0.0));
		item.SetPhysicsTransform(transform);
	}
}

void Init(){
	level.ReceiveLevelEvents(hotspot.GetID());
	ReadSavedData();
	SetCoinsState();
}

void Dispose() {
	level.StopReceivingLevelEvents(hotspot.GetID());
}

void Reset(){
	// ResetProgress();
}

void ReadSavedData(){
	JSON data;
	JSONValue root;

	// Get the saved data for this specific level, so every level can have it's own coins to pick up.
	SavedLevel@ saved_level = save_file.GetSavedLevel(GetCurrLevelName());
	string saved_data = saved_level.GetValue("coins_picked_up");
	
	// Check if the saved json is parseble.
	if(saved_data == "" || !data.parseString(saved_data)){
		Log(warning, "Couldn't find the coins picked up data!");
		return;
	}

	// Check if the existing saved data has the relevant data.
	root = data.getRoot();
	// Make sure the root is an array.
	if(!root.isArray()){return;}

	// Since we can't keep JSON objects as global variables without OG crashing, we convert it to a normal int array.
	coins_picked_up.resize(0);
	for(uint i = 0; i < root.size(); i++){
		coins_picked_up.insertLast(root[i].asInt());
	}
}

void WriteSavedData(){
	JSON data;
	JSONValue root;

	for(uint i = 0; i < coins_picked_up.size(); i++){
		root.append(coins_picked_up[i]);
	}

	data.getRoot() = root;
	SavedLevel@ saved_level = save_file.GetSavedLevel(GetCurrLevelName());
	saved_level.SetValue("coins_picked_up", data.writeString(false));
	save_file.WriteInPlace();
}

void SetCoinsState(){
	array<int> item_object_ids = GetObjectIDsType(_item_object);

	for(uint i = 0; i < item_object_ids.size(); i++){
		// First check if the object id actually exists in this scene.
		if(!ObjectExists(item_object_ids[i])){continue;}
		Object@ obj = ReadObjectFromID(item_object_ids[i]);
		// Then check if the object is in fact an ItemObject.
		if(obj.GetType() != _item_object){continue;}
		ItemObject@ item = ReadItemID(item_object_ids[i]);
		ScriptParams@ item_params = item.GetScriptParams();
		// Check if the item is a collectable ItemObject.
		if(!item_params.HasParam("Coin Pickup")){continue;}
		// If this ItemObject ID is inside the collected array then disable it.
		if(coins_picked_up.find(item_object_ids[i]) != -1){
			obj.SetEnabled(false);
		}else{
			obj.SetEnabled(true);
		}
	}
}

void Update(){
	if(reset_progress){
		reset_progress = false;
		params.SetInt("Reset Progress", 0);
		ResetProgress();
	}

	for(uint i = 0; i < coin_pickup_events.size(); i++){
		if(coin_pickup_events[i].Update()){
			// Add the ItemObject ID to the collected coins.
			coins_picked_up.insertLast(coin_pickup_events[i].item_id);
			SendGlobalMessage("wallet_gain");
			coin_pickup_events.removeAt(i);
			i--;
			WriteSavedData();
		}
	}
}

void ResetProgress(){
	coins_picked_up.resize(0);
	WriteSavedData();
	SetCoinsState();
}

void ReceiveMessage(string msg) {
	TokenIterator token_iter;
	token_iter.Init();

	if(!token_iter.FindNextToken(msg)) {
		return;
	}

	string token = token_iter.GetToken(msg);
	if(token == "level_event"){
		token_iter.FindNextToken(msg);
		string event = token_iter.GetToken(msg);
		if(event == "character_item_pickup"){
			token_iter.FindNextToken(msg);
			int character_id = atoi(token_iter.GetToken(msg));
			token_iter.FindNextToken(msg);
			int item_id = atoi(token_iter.GetToken(msg));
			// Log(warning, "character_id " + character_id + " item_id " + item_id);

			ItemObject@ item = ReadItemID(item_id);
			ScriptParams@ item_params = item.GetScriptParams();
			if(item_params.HasParam("Coin Pickup")){
				CoinPickupEvent coin_pickup_event(character_id, item_id);
				coin_pickup_events.insertLast(coin_pickup_event);
			}
		}else if(event == "reset_coin_progress"){
			ResetProgress();
		}
	}
}

void SetParameters() {
	params.AddInt("Reset Progress", 0);
	if(params.GetInt("Reset Progress") != 0){
		reset_progress = true;
	}
}