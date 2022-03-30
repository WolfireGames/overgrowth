#include "threatcheck.as"
#include "music_load.as"

float time = 0.0f;
vec3 old;
array<int> @object_ids = GetObjectIDs();
int num_objects = object_ids.length();

MusicLoad ml("Data/Music/challengelevel.xml");

void Update()
{

	for (int i=0; i<num_objects; ++i) //Loop
	{
	Object @obj = ReadObjectFromID(object_ids[i]);
	ScriptParams @pm = obj.GetScriptParams();


        if (pm.HasParam("Rotating Y")) //hawk rotation
	{
	obj.SetRotation(quaternion(vec4(0.0f, 0.4f, 0.0f, time_step)) * obj.GetRotation());
       	}

	}
	time += time_step;
}

void UpdateMusic() {
    int player_id = GetPlayerCharacterID();
    if(player_id != -1 && ReadCharacter(player_id).GetIntVar("knocked_out") != _awake){
        PlaySong("challengelevel_sad");
        return;
    }
    int threats_remaining = ThreatsRemaining();
    if(threats_remaining == 0){
        PlaySong("challengelevel_ambient-happy");
        return;
    }
    if(player_id != -1 && ReadCharacter(player_id).QueryIntFunction("int CombatSong()") == 1){
        PlaySong("challengelevel_combat");
        return;
    }
    PlaySong("challengelevel_ambient-tense");
}

void Init(string str) {
}

bool HasFocus(){
    return false;
}

void DrawGUI() {
}
