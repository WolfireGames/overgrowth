array<int> list_ids;
array<int> list_states;

void Init()
{
	
}

void Update()
{
	// Read every character state in each Update iteration.
	// You can also write your savedata, as long as its inmemory
	// there should be no performance impacts.
	// If you cannot call your SaveDataToFile function here on each tick
	// then we can work out a different solution.

	list_ids.resize(0);
	list_states.resize(0);

	// Adds all inlevel characters into the array (does not empty list beforehand).
	GetCharacters(list_ids);
	list_states.resize(list_ids.length());

	for (int i = 0; i < int(list_ids.length()); ++i)
	{
		MovementObject@ character_mo = ReadCharacterID(list_ids[i]);
		int ko_state = character_mo.GetIntVar("knocked_out");
		
		// ko_state can have one of these values
		// _awake = 0, _unconscious = 1, _dead = 2
		
		list_states[i] = ko_state;
	}
}

void ReceiveMessage(string message)
{
	
}

// Just some fancy stuff to display the states ingame,
// useless for your usecase.
void Draw()
{
	string text = "Player States:";
	
	for (int i = 0; i < int(list_ids.length()); ++i)
	{
		text += "\n[" + list_ids[i] + "] = ";
	
		if (list_states[i] == _awake) text += "awake (" + _awake + ")";
		else if (list_states[i] == _unconscious) text += "unconscious (" + _unconscious + ")";
		else if (list_states[i] == _dead) text += "dead (" + _dead + ")";
	}
	
	
	Object@ obj = ReadObjectFromID(hotspot.GetID());
	DebugDrawText(obj.GetTranslation(), text, 1.0f, true, _delete_on_draw);
}