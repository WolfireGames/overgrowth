/* relevant aschar.as lines 363 onwards
	
	enum WeaponSlot {
		_held_left = 0,
		_held_right = 1,
		_sheathed_left = 2,
		_sheathed_right = 3,
		_sheathed_left_sheathe = 4,
		_sheathed_right_sheathe = 5,
	};

	int primary_weapon_slot = _held_right;
	int secondary_weapon_slot = _held_left;
	const int _num_weap_slots = 6;
	array<int> weapon_slots;
*/

// I'm deliberately hardcoding some values because
// they will most likely not change with a future OG update.
// If you were to make everything "correct" you should parse that.
// But fuck it.

array<int> list_ids;
array<array<int>> list_holding_weapons;

void Init()
{
	
}

void Update()
{
	list_ids.resize(0);
	list_holding_weapons.resize(0);
	
	GetCharacters(list_ids);
	
	for (int i = 0; i < int(list_ids.length()); ++i)
	{
		MovementObject@ mo = ReadCharacterID(list_ids[i]);
		
		array<int> holding_ids;
		
		for (int slot_index = 0; slot_index < 6; ++slot_index)
			holding_ids.insertLast(mo.GetArrayIntVar("weapon_slots", slot_index));
			
		list_holding_weapons.insertLast(holding_ids);
	}
}

void Draw()
{
	string text = "Character Weapons:";
	
	for (int i = 0; i < int(list_ids.length()); ++i)
	{		
		Object@ character_obj = ReadObjectFromID(list_ids[i]);
		
		text += "\n[" + list_ids[i] + "] = ";
		
		for (int slot_index = 0; slot_index < 6; ++slot_index)
		{
			int weapon_id = list_holding_weapons[i][slot_index];	
			if (weapon_id == -1) continue;
			
			ItemObject@ weapon_io = ReadItemID(weapon_id);
			Object@ weapon_obj = ReadObjectFromID(weapon_id);
			
			// Object has .GetLabel() aswell but for some reason
			// it doesn't display what ItemObject does.
			
			string label = weapon_io.GetLabel();
			string name = weapon_obj.GetName();
			
			ScriptParams@ sp = weapon_obj.GetScriptParams();
			
			string custom_parameter = "";
			if (sp.HasParam("Weapon XML Path"))
				custom_parameter = sp.GetString("Weapon XML Path");
				
			text += "\n       Slot: " + slot_index
				+ " - " + "[" + weapon_id + "]"
				+ " - " + label
				+ " - " + name
				+ " - " + custom_parameter
			;
			
		}
	}
	
	Object@ obj = ReadObjectFromID(hotspot.GetID());
	DebugDrawText(obj.GetTranslation(), text, 1.0f, true, _delete_on_draw);
}