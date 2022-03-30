//-----------------------------------------------------------------------------
//           Name: kill.as
//      Developer: Wolfire Games LLC
//    Script Type: Hotspot
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
//   Copyright 2022 Wolfire Games LLC
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//-----------------------------------------------------------------------------

void Init() {
}

void SetParameters() {
	params.AddIntCheckbox("UseFire", true);
	params.AddIntCheckbox("UseLights", false);
    params.AddIntCheckbox("InstantKill", true);
	params.AddFloatSlider("ParticleInterval",0.03,"min:0.01,max:0.1,step:0.001,text_mult:1000");
	params.AddFloatSlider("Damage",0.05f,"min:0.01,max:0.5,step:0.01,text_mult:100");
}

class light{
    //The light class is used to create multiple point lights when flame particles spawn.
    Object@ lightObj;
    float spawnTime;
    light(Object@ _lightObj, float _spawnTime){
        //Use @ to make the script use handles in stead of new objects.
        @lightObj = @_lightObj;
        spawnTime = _spawnTime;
    }
}

class flame{
    int id;
    float spawnTime;
    flame(int _id, float _spawnTime){
        id = _id;
        spawnTime = _spawnTime;
    }
}

class victim{
    MovementObject@ char;
    Object@ charObj;
    array<light@> lights;
    array<flame@> flames;
    float origSpeed;
    array<vec3> paletteColors(5);
    bool burned = false;
    bool applyDamage = true;
	int soundHandle = -1;
	bool done = false;
	float TimeOfDeath;
    victim(MovementObject@ _char) {
        @char = @_char;
        //Get the current speed multiplier to be able to reset it when the character exits.
        origSpeed = char.GetFloatVar("p_speed_mult");
        //Limit the speed while the character is inside, like the char is stuck.
        SetCharSpeed(_char, origSpeed * 0.3f);
        //Again use handles to avoid using ReadOjectFromID in each update.
        @charObj = ReadObjectFromID(char.GetID());
        //Store the original palette colors to be able to reset.
        for(int i = 0; i < charObj.GetNumPaletteColors(); i++){
            paletteColors[i] = charObj.GetPaletteColor(i);
        }
		if(params.GetInt("UseFire") == 1){
			soundHandle = PlaySoundLoop("Data/Sounds/fire.wav", 1.0f);
		    char.rigged_object().anim_client().AddLayer("Data/Animations/r_writhe.anm",1.0f,0 );
		}
        if( params.GetInt("InstantKill") == 1 )
        {
//            char.Execute("KillCharacter()");
        }
    }
	void ReActivate(){
		if(params.GetInt("UseFire") == 1){
			soundHandle = PlaySoundLoop("Data/Sounds/fire.wav", 1.0f);
			char.rigged_object().anim_client().AddLayer("Data/Animations/r_writhe.anm",1.0f,0 );
			SetCharSpeed(char, origSpeed * 0.3f);
		}
        if( params.GetInt("InstantKill") == 1 )
        {
//            char.Execute("KillCharacter()");
        }
		applyDamage = true;
	}

    void Reset(){
        SetCharSpeed(char, origSpeed);
        /*for(int i = 0; i < 4; i++){
            charObj.SetPaletteColor(i, paletteColors[i]);
        }*/
        for(uint i = 0;i < lights.size(); i++){
            DeleteObjectID(lights[i].lightObj.GetID());
        }
		StopSound(soundHandle);
        burned = false;
    }
}

array<victim@> victims;
float time;
float particle_timer;
float interval = 0.1f;
Object@ thisHotspot = ReadObjectFromID(hotspot.GetID());
int currentIndex = 0;
float timeAfterDeath = 5.0f;

void Reset(){
    for(uint i = 0; i < victims.size(); i++){
        victims[i].Reset();
    }
    victims.resize(0);
    currentIndex = 0;
}

void HandleEvent(string event, MovementObject @mo){
    if(event == "enter"){
        OnEnter(mo);
    } else if(event == "exit"){
        OnExit(mo);
    }
}

void OnEnter(MovementObject @mo) {
    int index = InsideArray(mo.GetID());
    if(index == -1){
        victims.insertLast(victim(mo));
    }else{
		victims[index].ReActivate();
    }
}

void OnExit(MovementObject @mo) {
    //Find on which index the character is.
    int index = InsideArray(mo.GetID());
    //If the character is not in the array (somehow) don't do anything.
    if(index != -1){

    }
}

void Update(){
	CheckKeyPresses();
    //Now the update gets into hurting the characters inside.
    if(victims.size() != 0){
        //This time is used to update the script every interval. For performance.
        time += time_step;
		particle_timer += time_step;
        //Get the victim object from the list and use it for the rest of the loop.
        victim@ curVictim = victims[currentIndex];
		//Even if the character enters and exits the hotspot it will still be in the victims array,
        //this way the light keep getting handled and the palette colors are kept.
		if(curVictim.done){
			return;
		}
        if(curVictim.applyDamage){
            if(curVictim.char.GetFloatVar("roll_ik_fade") == 1.0f || curVictim.char.GetBoolVar("pre_jump")){
                curVictim.applyDamage = false;
                curVictim.char.rigged_object().anim_client().RemoveAllLayers();
                curVictim.char.Execute("ResetLayers();");
                SetCharSpeed(curVictim.char, curVictim.origSpeed);
				StopSound(curVictim.soundHandle);
				return;
            }
        }
        if(curVictim.applyDamage){
			if(params.GetInt("UseFire") == 1){
                curVictim.char.ReceiveScriptMessage("ignite");
			}
            //Update some stuff that is not time crucial.
            if(time > interval){
                if(curVictim.char.GetIntVar("knocked_out") == _awake){
                    //Use TakeBloodDamage so that the character will hold his stomach when escaping the hotspot.
                    curVictim.char.Execute("TakeBloodDamage("+ params.GetFloat("Damage") +");");
                    if(curVictim.char.GetIntVar("knocked_out") != _awake){
                        //Ragdoll when the character is not _awake anymore, thus dead.
                        curVictim.char.Execute("Ragdoll(_RGDL_INJURED);");
                        //Scream to sell the effect.
                        PlaySound("Data/Sounds/voice/animal2/voice_bunny_groan_3.wav", curVictim.char.position);
						StopSound(curVictim.soundHandle);
						if(params.GetInt("UseFire") == 1){
							curVictim.TimeOfDeath = the_time;
						}else{
							curVictim.done = true;
						}
                    }
                }else{
					//Character is Dead
					if(curVictim.TimeOfDeath + timeAfterDeath < the_time){
						curVictim.done = true;
					}
				}
                //Reset the timer to let the interval start again.
                time = 0.0f;
            }
        }
		currentIndex++;
		if(uint(currentIndex) >= victims.size()){
	        currentIndex = 0;
	    }
    }
}
void CheckKeyPresses(){
	if(GetInputPressed(0, "x")){
		Reset();
	}
}

int InsideArray(int id){
    int inside = -1;
    for(uint i = 0; i < victims.size(); i++){
        if(victims[i].char.GetID() == id){
            inside = i;
        }
    }
    return inside;
}

void SetCharSpeed(MovementObject@ char, float speed){
    char.Execute("p_speed_mult = " + speed + ";" +
                    "run_speed = _base_run_speed * p_speed_mult;" +
                    "true_max_speed = _base_true_max_speed * p_speed_mult;");
}
