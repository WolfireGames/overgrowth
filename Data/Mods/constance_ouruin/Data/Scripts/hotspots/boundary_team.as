void Init() {
}

string placeholderPath = "Data/Objects/block.xml";
const int _ragdoll_state = 4;

void SetParameters() {
	params.AddString("Team", "Turner");
}

void Reset(){
}

void Update(){
	if(EditorModeActive()){
		ShowPlaceholder();
	}else if(ReadObjectFromID(hotspot.GetID()).GetEnabled()){
		array<int> charIDs;
		level.GetCollidingObjects(hotspot.GetID(), charIDs);

		for(uint i = 0; i < charIDs.size(); i++){
			if(ReadObjectFromID(charIDs[i]).GetType() == _movement_object){
				MovementObject@ this_mo = ReadCharacterID(charIDs[i]);
				Object@ mo_obj = ReadObjectFromID(charIDs[i]);
				string team = params.GetString("Team");
				ScriptParams @char_params = mo_obj.GetScriptParams();
				string char_teams = char_params.GetString("Teams");
				char_teams = join(char_teams.split(" "), "");
				array<string> teams_array = char_teams.split(",");

				if(!this_mo.static_char && teams_array.find(team) != -1){
					const float _push_force_mult = 0.5f;
					vec3 push_force;
					vec3 direction = ReadObjectFromID(hotspot.GetID()).GetRotation() * vec3(0,0,-1);// normalize(this_mo.position - oldPos);
					push_force.x -= direction.x;
					push_force.z -= direction.z;
					push_force *= _push_force_mult;
					if(length_squared(push_force) > 0.0f){
						this_mo.velocity += push_force;
						if(this_mo.GetIntVar("state") == _ragdoll_state){
							this_mo.rigged_object().ApplyForceToRagdoll(push_force * 500.0f, this_mo.rigged_object().skeleton().GetCenterOfMass());
						}
					}
				}
			}
		}
	}
}

void ShowPlaceholder(){
	//Object@ hotspotObj = ReadObjectFromID(hotspot.GetID());
	//PlaceholderObject@ placeholder_object = cast<PlaceholderObject@>(hotspotObj);
	//placeholder_object.SetPreview(path);
}
