//-----------------------------------------------------------------------------
//           Name: fixedattribute.as
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

void Init() {
}

string displayText;
string changeToChar;
string recoverHealth;
string recoverAll;
float changeRunSpeedAmt;
float changeDmgResistanceAmt;
float changeAttackSpeedAmt;
float changeAttackDmgAmt;

void SetParameters() {
        
params.AddString("Display Text","false");
    displayText = params.GetString("Display Text");

params.AddString("Change to Character(Path)","false");
    changeToChar = params.GetString("Change to Character(Path)");

params.AddString("Recover Health","false");
    recoverHealth = params.GetString("Recover Health");

params.AddString("Recover All","false");
    recoverAll = params.GetString("Recover All");

params.AddString("Change Run Speed","0.0");
    changeRunSpeedAmt = params.GetFloat("Change Run Speed");

params.AddString("Change Damage Resistance","0.0");
    changeDmgResistanceAmt = params.GetFloat("Change Damage Resistance");

params.AddString("Change Attack Speed","0.0");
    changeAttackSpeedAmt = params.GetFloat("Change Attack Speed");

params.AddString("Change Attack Damage","0.0");
    changeAttackDmgAmt = params.GetFloat("Change Attack Damage");

}

void HandleEvent(string event, MovementObject @mo){
    if(event == "enter"){
        OnEnter(mo);
    } else if(event == "exit"){
        OnExit(mo);
    }
}

void OnEnter(MovementObject @mo) {
    if(mo.controlled){

//Display Text
if (displayText != "false")
    level.Execute("ReceiveMessage2(\"displaytext\",\""+displayText+"\")");

//Change to character 
if (changeToChar != "false")
    mo.Execute("SwitchCharacter(\""+changeToChar+"\");");

//Recover Health
if (recoverHealth == "true")
    mo.Execute("RecoverHealth();");

//Recover All 
if (recoverAll == "true")
    mo.Execute("Recover();");

//Change Player Run Speed
if (changeRunSpeedAmt > 0.0f){
    mo.Execute("run_speed = 8*"+changeRunSpeedAmt+";"); 
    mo.Execute("true_max_speed = 12*"+changeRunSpeedAmt+";");
}

//Change Damage Resistance
if (changeDmgResistanceAmt != 0.0f){
    float damageResistance = 1.0f / changeDmgResistanceAmt;
    mo.Execute("p_damage_multiplier = "+damageResistance+";"); 
}

//Change Attack Speed
if (changeAttackSpeedAmt != 0.0f){
    mo.Execute("p_attack_speed_mult = min(2.0f, max(0.1f, "+changeAttackSpeedAmt+"));"); 
}

//Change Attack Damage
if (changeAttackDmgAmt != 0.0f){
    mo.Execute("p_attack_damage_mult = max(0.0f, "+changeAttackDmgAmt+");"); 
}

}
else{
//NPCs

}
}

void OnExit(MovementObject @mo) {
    if(mo.controlled){
        level.Execute("ReceiveMessage(\"cleartext\")");
    }
}

