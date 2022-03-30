//-----------------------------------------------------------------------------
//           Name: tutorial_assignment_checks.as
//      Developer: Wolfire Games LLC
//    Script Type:
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

class AssignmentCallback
{
	float time;
	float delay;
	bool timerStarted = false;
	bool completed = false;
	bool disabled = false;
	MovementObject@ player = ReadCharacter(GetPlayerCharacter());

    void Completed(){}
    //Every assignment has some functions that are the same.
    //Handling them in the baseclass saves some duplicate code.
    void Init(){}
    bool CheckCompleted(){
		//Don't start the timer if it's already triggered.
		if(GetInputPressed(0, "tab")){
			return true;
        }
		if(!disabled){
			if(timerStarted){
	    		UpdateTimer();
	    	}else if(!timerStarted && !completed){
	    		Completed();
	    	}
	    	if(completed){
	    		disabled = true;
	    	}
	    	//When the assignment is completed it only needs to send back ONE true.
	    	return completed;
		}else{
			return false;
		}
    }
	void OnCompleted(){}
    void StartTimer(float _delay){
    	if(!timerStarted){
    		//Print("THE ASSIGNMENT IS COMPLETED!-----------\n");
    		PlaySound("Data/Sounds/lugaru/consolesuccess.ogg");
			delay = _delay;
			timerStarted = true;
		}
    }
    void StartTimerNoSound(float _delay){
    	if(!timerStarted){
			delay = _delay;
			timerStarted = true;
		}
    }
    void UpdateTimer(){
		time += time_step;
		if(time > delay){
			completed = true;
		}
    }
    void Reset(){
		time = 0;
		//delay = 0;
		timerStarted = false;
		completed = false;
		disabled = false;
		LocalReset();
    }

    void LocalReset(){

    }

    int GetPlayerCharacter() {
	    int num = GetNumCharacters();
	    for(int i=0; i<num; ++i){
	        MovementObject@ char = ReadCharacter(i);
	        if(char.controlled){
	        	return char.GetID();
	        }
	    }
	    return 0;
	}
	void LevelExecute(string command){
		SendCommand("level_execute", command);
	}
	void EnemyExecute(string command){
		SendCommand("enemy_execute", command);
	}
	void PlayerExecute(string command){
		SendCommand("player_execute", command);
	}
	void SendCommand(string firstCommand, string secondCommand) {
		//Remove all the spaces from the actual command or else the command will be split up in the ReceiveMessage
		level.SendMessage(firstCommand + " " + join(secondCommand.split( " " ), "" ));
	}
	void SendCommand(string singleCommand) {
		level.SendMessage(singleCommand);
	}
	void ReceiveAchievementEvent(string _achievement){}
	void ReceiveAchievementEventFloat(string _achievement, float _value){}
}

class Delay : AssignmentCallback
{
	Delay(float _delay){delay = _delay;}
	void Completed()
	{
		//Wait for n seconds and then continue.
		StartTimerNoSound(delay);
	}
}

class TabToContinue : AssignmentCallback
{
	TabToContinue(){}
	void Init(){
		SendCommand("extra_assignment_text " + "Press @slow@ to continue." );
	}
	void Completed(){
	}
}

class MouseMove : AssignmentCallback
{
	float negX;
	float posX;
	float negY;
	float posY;
	float threshold;
	float prevXAxis;
	float prevYAxis;

	void Init(){
		negX = 0;
		posX = 0;
		negY = 0;
		posY = 0;
		threshold = 100.0f;
		prevXAxis = 0;
		prevYAxis = 0;
	}

    void Completed()
    {
    	//To make sure the player has looked in every direction add the movement
    	float diffXAxis = GetLookXAxis(player.controller_id) - prevXAxis;
    	float diffYAxis = GetLookYAxis(player.controller_id) - prevYAxis;
    	if(diffXAxis < 0){
    		negX += (diffXAxis * -1);
    	}else if(diffXAxis > 0){
    		posX += diffXAxis;
    	}
    	if(diffYAxis < 0){
    		negY += (diffYAxis * -1);
    	}else if(diffYAxis > 0){
    		posY += diffYAxis;
    	}
    	float success = ((negX + negY + posX + posY) / threshold) * 0.25f;
    	DebugText("Success", "Success: "+success, 0.5f);
    	//Checking for all mousemovements are large enough
    	if(success >= 1.0){
    		StartTimer(1.0f);
    	}
    }
}

class WASDMove : AssignmentCallback
{

	float success;

	void Init(){
		success = 0.0f;
	}

	void Completed()
	{
		if(length(player.velocity) > 1.0f){
			success += time_step * 0.2;
		}
    	DebugText("Success", "Success: "+success, 0.5f);
		if(success >= 1.0){			
			StartTimer(1.0f);
		}
	}
}

class SpaceJump : AssignmentCallback
{
	float success;

	void Init(){
		success = 0.0f;
	}

	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "player_jumped"){
			success += 0.2f;
		}
    	DebugText("Success", "Success: "+success, 0.5f);
		if(success >= 1.0){			
			StartTimer(1.0f);
		}
	}
}

class ShiftCrouch : AssignmentCallback
{
	float success;

	void Init(){
		success = 0.0f;
	}

	void Completed()
	{
		if(GetInputDown(0, "crouch") && player.GetBoolVar("on_ground")){
			success += time_step * 0.25f;
		}
    	DebugText("Success", "Success: "+success, 0.5f);
		if(success >= 1.0){			
			StartTimer(1.0f);
		}
	}
}

class ShiftRoll : AssignmentCallback
{
	float success;

	void Init(){
		success = 0.0f;
	}

	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "character_start_roll"){
			success += 0.2f;
		}
    	DebugText("Success", "Success: "+success, 0.5f);
		if(success >= 1.0){			
			StartTimer(1.0f);
		}
	}
}

class ShiftFlip : AssignmentCallback
{
	float success;

	void Init(){
		success = 0.0f;
	}

	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "character_start_flip"){
			success += 0.2f;
		}
    	DebugText("Success", "Success: "+success, 0.5f);
		if(success >= 1.0){			
			StartTimer(1.0f);
		}
	}
}

class ShiftSneak : AssignmentCallback
{
	float success;

	void Init(){
		success = 0.0f;
	}

	void Completed()
	{
		if(GetInputDown(0, "crouch") && length(player.velocity) > 1.0f && player.GetBoolVar("on_ground")){
			success += time_step * 0.2;
		}
    	DebugText("Success", "Success: "+success, 0.5f);
		if(success >= 1.0){			
			StartTimer(1.0f);
		}
	}
}
class AnimalRun : AssignmentCallback
{
	void Completed()
	{
		if(GetInputDown(0, "crouch")){
			StartTimer(1.0f);
		}
	}
}
class WallJump : AssignmentCallback
{
	float success;

	void Init(){
		success = 0.0f;
	}

	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "jump_off_wall" || _achievement == "wall_flip"){
			success += 0.34;
		}
    	DebugText("Success", "Success: "+success, 0.5f);
		if(success >= 1.0){			
			StartTimer(1.0f);
		}
	}
}
class WallFlip : AssignmentCallback
{
	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "wall_flip"){
			StartTimer(1.0f);
		}
	}
}

class SendInEnemy : AssignmentCallback
{
    string type;
    SendInEnemy(string _type) {
        type = _type; 
    }

	void LocalReset(){
		level.SendMessage("delete_enemy");
	}

	void Init() {
		level.SendMessage("send_in_enemy " + type);
		StartTimerNoSound(3.0f);
	}
}
class AnyAttack : AssignmentCallback
{
	float success;

	void Init(){
		success = 0.0f;
	}

	void ReceiveAchievementEvent(string _achievement){
		if(	_achievement == "attack_stationary_close" ||
			_achievement == "attack_stationary_far" ||
			_achievement == "attack_moving_close" ||
			_achievement == "attack_moving_far" ||
			_achievement == "attack_low")
		{
			success += 0.1f;
		}
    	DebugText("Success", "Success: "+success, 0.5f);
		if(success >= 1.0f){			
			StartTimer(1.0f);
		}
	}
}
class AttackCloseStationary : AssignmentCallback
{
	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "attack_stationary_close"){
			StartTimer(1.0f);
		}
	}
}
class AttackFarStationary : AssignmentCallback
{
	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "attack_stationary_far"){
			StartTimer(1.0f);
		}
	}
}
class AttackCloseMoving : AssignmentCallback
{
	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "attack_moving_close"){
			StartTimer(1.0f);
		}
	}
}
class KneeStrike : AssignmentCallback
{
	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "attack_stationary_close"){
			StartTimer(1.0f);
		}
	}
}
class SpinKick : AssignmentCallback
{
	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "attack_moving_far"){
			StartTimer(1.0f);
		}
	}
}
class Sweep : AssignmentCallback
{
	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "attack_low"){
			StartTimer(1.0f);
		}
	}
}
class LegCannon : AssignmentCallback
{
	float success;

	void Init(){
		success = 0.0f;
	}

	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "leg_cannon_hit"){
			success += 0.34f;
		}
    	DebugText("Success", "Success: "+success, 0.5f);
		if(success >= 1.0f){			
			StartTimer(1.0f);
		}
	}
}

class ChokeHold : AssignmentCallback
{
	void Init(){
		EnemyExecute("always_unaware = true;");
	}

	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "choke_hold_kill"){
			StartTimer(1.0f);
		}
	}
}
class Dodge : AssignmentCallback
{
	float success;

	void Init(){
		success = 0.0f;
		level.SendMessage("delete_weapon");
        level.SendMessage("give_enemy_knife");
		SendCommand("set_combat true");
		EnemyExecute(
		   "SetHostile(true);
			always_unaware = false;
			combat_allowed = true;
			chase_allowed = false;
			allow_active_block = true;
			always_active_block = false;
			goal = _attack;");
        SendCommand("player_invincible");
	}
	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "active_dodging"){
			success += 0.2f;
		}
    	DebugText("Success", "Success: "+success, 0.5f);
		if(success >= 1.0f){			
			StartTimer(1.0f);
		}
	}
}
class ActivateEnemy : AssignmentCallback
{
	float success;

	void Init(){
		success = 0.0f;
		EnemyExecute("SetHostile(true);
				  	  always_unaware = false;
					  combat_allowed = false;
					  chase_allowed = true;
					  allow_active_block = true;
					  allow_throw = false;
					  goal = _attack;");
	}

	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "enemy_ko"){
			success += 0.34f;
		}
    	DebugText("Success", "Success: "+success, 0.5f);
		if(success >= 1.0f){			
			StartTimer(1.0f);
		}
	}
}
class ThrowEscape : AssignmentCallback
{
	float success;

	void Init(){
		success = 0.0f;
		EnemyExecute("always_active_block = true;
					  allow_throw = true;
					  goal = _attack;");
	}
	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "character_throw_escape"){
			success += 0.34f;
		}
    	DebugText("Success", "Success: "+success, 0.5f);
		if(success >= 1.0f){			
			StartTimer(1.0f);
		}
	}
}

class ThrowEnemy : AssignmentCallback
{
	float success;

	void Init(){
		success = 0.0f;
		EnemyExecute("always_active_block = false;
					  goal = _attack;");
	}
	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "character_throw_escape"){
			success += 0.34f;
		}
    	DebugText("Success", "Success: "+success, 0.5f);
		if(success >= 1.0f){			
			StartTimer(1.0f);
		}
	}
}

class TwoThrowEscape : AssignmentCallback
{
	int successfull;
	void Init(){
		successfull = 0;
	}
	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "character_throw_escape"){
			successfull++;
		}else if (_achievement == "player_damage"){
			successfull = 0;
		}
		if(successfull >= 2){
			StartTimer(1.0f);
		}
	}
}
class CountDown : AssignmentCallback
{
	int seconds;
	int lastTime;
	void Init(){
		seconds = 8;
		lastTime = -1;
	}
	void Completed(){
		time += time_step;
		if(floor(time) != lastTime){
			lastTime = int(floor(time));
			seconds -= 1;
			//Print("time " + seconds + "\n");
			SendCommand("update_text_variables " + seconds );
			SendCommand("set_combat true");
		}
		if(seconds == 0){
			StartTimerNoSound(0.0f);
		}
	}
	void OnCompleted(){
		level.SendMessage("set_highlight true");
	}
}
class BlockAttack : AssignmentCallback
{
	float success;

	void Init(){
		success = 0.0f;
		SendCommand("set_combat true");
		EnemyExecute("always_unaware = false;");
		EnemyExecute("combat_allowed = true;");
		EnemyExecute("chase_allowed = false;");
		EnemyExecute("allow_active_block = true;");
		EnemyExecute("always_active_block = false;");
		EnemyExecute("goal = _attack;");
		player.Execute("max_ko_shield = 9999; ko_shield = max_ko_shield;");
	}
	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "player_blocked"){
			success += 0.1f;
		}
    	DebugText("Success", "Success: "+success, 0.5f);
		if(success >= 1.0f){			
			StartTimer(1.0f);
		}
	}
}
class RollFromGround : AssignmentCallback
{
	float success;

	void Init(){
		success = 0.0f;
	}
	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "player_wake_roll"){
			success += 0.34f;
		}
    	DebugText("Success", "Success: "+success, 0.5f);
		if(success >= 1.0f){			
			StartTimer(1.0f);
		}
	}
}
class ReverseAttack : AssignmentCallback
{
	float success;

	void Init(){
		success = 0.0f;
		
		EnemyExecute("always_unaware = false;");
		EnemyExecute("combat_allowed = true;");
		EnemyExecute("chase_allowed = false;");
		EnemyExecute("allow_active_block = true;");
		EnemyExecute("always_active_block = false;");
		EnemyExecute("goal = _attack;");
	}
	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "player_counter_attacked"){
			success += 0.34f;
		}
    	DebugText("Success", "Success: "+success, 0.5f);
		if(success >= 1.0f){			
			StartTimer(1.0f);
		}
	}
}
class ReverseAttackArmed : AssignmentCallback
{
	float success;

	void Init(){
		success = 0.0f;
        level.SendMessage("give_enemy_knife");
		EnemyExecute("SetHostile(true);");
		EnemyExecute("always_unaware = false;");
		EnemyExecute("combat_allowed = true;");
		EnemyExecute("chase_allowed = false;");
		EnemyExecute("allow_active_block = true;");
		EnemyExecute("always_active_block = false;");
		EnemyExecute("goal = _attack;");
        SendCommand("player_invincible");
	}
	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "player_counter_attacked"){
			success += 0.34f;
		}
    	DebugText("Success", "Success: "+success, 0.5f);
		if(success >= 1.0f){			
			StartTimer(1.0f);
		}
	}
}
class AttackCountDown : AssignmentCallback
{
	float success;

	void Init(){
		success = 0.0f;
		level.SendMessage("set_highlight false");
	}

	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "enemy_ko"){
			success += 0.2f;
		}
    	DebugText("Success", "Success: "+success, 0.5f);
		if(success >= 1.0f){			
			StartTimer(1.0f);
			SendCommand("set_combat false");
		}
	}
}
class PickUpKnife : AssignmentCallback
{
	void Init(){
		EnemyExecute("SetHostile(false);");
		EnemyExecute("hostile = false;");
		EnemyExecute("always_unaware = false;");
		EnemyExecute("combat_allowed = false;");
		EnemyExecute("chase_allowed = false;");
		EnemyExecute("allow_active_block = false;");
		EnemyExecute("always_active_block = false;");
		level.SendMessage("delete_weapon");
		level.SendMessage("send_in_weapon knife");
	}
	void Completed(){
		if(GetCharPrimaryWeapon(player) != -1){
			StartTimer(1.0f);
		}
	}
	void LocalReset(){
		level.SendMessage("delete_weapon");
	}
}

class PickUpSword : AssignmentCallback
{
	void Init() {
		EnemyExecute("SetHostile(false);");
		EnemyExecute("hostile = false;");
		EnemyExecute("always_unaware = false;");
		EnemyExecute("combat_allowed = false;");
		EnemyExecute("chase_allowed = false;");
		EnemyExecute("allow_active_block = false;");
		EnemyExecute("always_active_block = false;");
		level.SendMessage("delete_weapon");
		level.SendMessage("send_in_weapon sword");
	}
	void Completed(){
		if(GetCharPrimaryWeapon(player) != -1){
			StartTimer(1.0f);
		}
	}
	void LocalReset(){
		level.SendMessage("delete_weapon");
	}
}

class RemoveWeapon : AssignmentCallback
{
	RemoveWeapon(float _delay){delay = _delay;}
	void Init() {
		level.SendMessage("delete_weapon");
	}

	void Completed()
	{
		//Wait for n seconds and then continue.
		StartTimerNoSound(delay);
	}
}

class SheatheKnife : AssignmentCallback
{
	void Completed(){
		if(GetCharPrimarySheathedWeapon(player) != -1){
			StartTimer(1.0f);
		}
	}
}

class SharpDamage : AssignmentCallback
{
	float success;

	void Init(){
		success = 0.0f;
	}

	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "ai_took_sharp_damage"){
			success += 0.1f;
		}
    	DebugText("Success", "Success: "+success, 0.5f);
		if(success >= 1.0f){			
			StartTimer(1.0f);
		}
	}
}

class KillWolfSharp : AssignmentCallback
{
	float success;

	void Init(){
		success = 0.0f;
	}

	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "ai_took_sharp_damage"){
			success += 0.1f;
		}
		if(success >= 1.0f && _achievement == "enemy_died"){			
			StartTimer(1.0f);
		}
	}
}

class PullSword : AssignmentCallback
{
	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "ai_alive_weapon_removed_from_body"){
			StartTimer(1.0f);
		}
	}
}
class KnifeGrabExecution : AssignmentCallback
{
	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "choke_hold_knife_cut_kill"){
            StartTimer(1.0f);
        }
    }
}
class KnifeThrow : AssignmentCallback
{
	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "player_threw_knife"){
			StartTimer(1.0f);
		}
	}
}

class EnemyKnife : AssignmentCallback
{
	void Init() {
        level.SendMessage("give_enemy_knife");
        StartTimerNoSound(5.0f);
    }
	void ReceiveAchievementEvent(string _achievement) {
	}
}

class LedgeGrab : AssignmentCallback
{
	int pillarID = -1;
	float offsetY = 3.0f;
	float speed = 0.01f;
	float movedY = 0.0f;
	float success;

	void Init(){
		success = 0.0f;
        array<int> @object_ids = GetObjectIDs();
        int num_objects = object_ids.length();
        for(int i=0; i<num_objects; ++i){
            Object @obj = ReadObjectFromID(object_ids[i]);
            ScriptParams@ params = obj.GetScriptParams();
            if(params.HasParam("Name")){
                string name_str = params.GetString("Name");
                if("pillar_spawn" == name_str){
					pillarID = CreateObject("Data/Objects/Buildings/pillar1.xml");
			        Object@ pillarObj = ReadObjectFromID(pillarID);
                    pillarObj.SetTranslation(obj.GetTranslation() - vec3(0,offsetY,0));
                    pillarObj.SetScale(obj.GetScale());
                    pillarObj.SetRotation(obj.GetRotation());
                    break;
                }
            }
        }
	}
	void Completed(){
		if(pillarID != -1){
			if(movedY < offsetY){
				Object@ pillarObj = ReadObjectFromID(pillarID);
				pillarObj.SetTranslation(pillarObj.GetTranslation() + vec3(0,speed,0));
				movedY += speed;
			}
		}
	}
	void OnCompleted(){
		if(pillarID != -1){
			if(movedY < offsetY){
				Object@ pillarObj = ReadObjectFromID(pillarID);
				pillarObj.SetTranslation(pillarObj.GetTranslation() + vec3(0,(offsetY - movedY),0));
			}
		}
	}
	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "climbed_up"){
			success += 0.34f;
		}
    	DebugText("Success", "Success: "+success, 0.5f);
		if(success >= 1.0){			
			StartTimer(1.0f);
		}
	}
}
class Plants : AssignmentCallback
{
	int bushID = -1;
	void Init(){
        array<int> @object_ids = GetObjectIDs();
        int num_objects = object_ids.length();
        for(int i=0; i<num_objects; ++i){
            Object @obj = ReadObjectFromID(object_ids[i]);
            ScriptParams@ params = obj.GetScriptParams();
            if(params.HasParam("Name")){
                string name_str = params.GetString("Name");
                if("bush_spawn" == name_str){
					bushID = CreateObject("Data/Objects/Plants/Trees/temperate/green_bush.xml");
			        Object@ bushObj = ReadObjectFromID(bushID);
                    bushObj.SetTranslation(obj.GetTranslation());
                    bushObj.SetScale(obj.GetScale());
                    bushObj.SetRotation(obj.GetRotation());
                    break;
                }
            }
        }
	}
	void Completed()
	{
		if(player.GetFloatVar("in_plant") > 0.5f){
			StartTimer(1.0f);
		}
	}
	void OnCompleted(){
		if(bushID != -1){
			DeleteObjectID(bushID);
		}
	}
}

class Fire : AssignmentCallback
{
	int fireID = -1;
	bool player_on_fire = false;
	void Init(){
        array<int> @object_ids = GetObjectIDs();
        int num_objects = object_ids.length();
        for(int i=0; i<num_objects; ++i){
            Object @obj = ReadObjectFromID(object_ids[i]);
            ScriptParams@ params = obj.GetScriptParams();
            if(params.HasParam("Name")){
                string name_str = params.GetString("Name");
                if("fire_spawn" == name_str){
					fireID = CreateObject("Data/Objects/Hotspots/fire_test.xml");
			        Object@ fireObj = ReadObjectFromID(fireID);
                    fireObj.SetTranslation(obj.GetTranslation());
                    fireObj.SetScale(obj.GetScale());
                    fireObj.SetRotation(obj.GetRotation());
                    break;
                }
            }
        }
	}
	void Completed()
	{
		if(player.GetBoolVar("on_fire")){
			player_on_fire = true;
		}else{
			player_on_fire = false;
		}
	}
	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "character_start_roll" && player_on_fire){
			StartTimer(1.0f);
		}
	}
	void OnCompleted(){
		level.SendMessage("revive_all");
		if(fireID != -1){
			DeleteObjectID(fireID);
		}
	}
}

class EndLevel : AssignmentCallback
{
	void ReceiveAchievementEvent(string _achievement){
		if(_achievement == "character_reset_hotspot"){
			level.SendMessage("go_to_main_menu");
		}
	}
}
