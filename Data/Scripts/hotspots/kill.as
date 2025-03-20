//-----------------------------------------------------------------------------
//           Name: kill.as
//      Developer: Wolfire Games LLC
//    Script Type: Hotspot
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
    // No initialization needed
}

class Victim {
    MovementObject@ char;
    Object@ charObj;
    float origSpeed;
    bool applyDamage = true;
    int soundHandle = -1;
    bool done = false;
    float TimeOfDeath;

    Victim(MovementObject@ _char) {
        @char = _char;
        origSpeed = char.GetFloatVar("p_speed_mult");
        SetCharSpeed(origSpeed * 0.3f);
        @charObj = ReadObjectFromID(char.GetID());
        if (params.GetInt("UseFire") == 1) {
            soundHandle = PlaySoundLoop("Data/Sounds/fire.wav", 1.0f);
            char.rigged_object().anim_client().AddLayer("Data/Animations/r_writhe.anm", 1.0f, 0);
        }
        if (params.GetInt("InstantKill") == 1) {
            // char.Execute("KillCharacter()");
        }
    }

    void ReActivate() {
        if (params.GetInt("UseFire") == 1) {
            soundHandle = PlaySoundLoop("Data/Sounds/fire.wav", 1.0f);
            char.rigged_object().anim_client().AddLayer("Data/Animations/r_writhe.anm", 1.0f, 0);
            SetCharSpeed(origSpeed * 0.3f);
        }
        if (params.GetInt("InstantKill") == 1) {
            // char.Execute("KillCharacter()");
        }
        applyDamage = true;
    }

    void Reset() {
        SetCharSpeed(origSpeed);
        StopSound(soundHandle);
    }

    void SetCharSpeed(float speed) {
        char.Execute(
            "p_speed_mult = " + speed + ";" +
            "run_speed = _base_run_speed * p_speed_mult;" +
            "true_max_speed = _base_true_max_speed * p_speed_mult;"
        );
    }
}

array<Victim@> victims;
float time = 0.0f;
int currentIndex = 0;
float timeAfterDeath = 5.0f;

void SetParameters() {
    params.AddIntCheckbox("UseFire", true);
    params.AddIntCheckbox("UseLights", false);
    params.AddIntCheckbox("InstantKill", true);
    params.AddFloatSlider("ParticleInterval", 0.03f, "min:0.01,max:0.1,step:0.001,text_mult:1000");
    params.AddFloatSlider("Damage", 0.05f, "min:0.01,max:0.5,step:0.01,text_mult:100");
}

void Reset() {
    for (uint i = 0; i < victims.length(); ++i) {
        victims[i].Reset();
    }
    victims.resize(0);
    currentIndex = 0;
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter") {
        OnEnter(mo);
    }
}

void OnEnter(MovementObject@ mo) {
    int index = FindVictimIndex(mo.GetID());
    if (index == -1) {
        victims.insertLast(Victim(mo));
    } else {
        victims[index].ReActivate();
    }
}

void Update() {
    if (victims.isEmpty()) {
        return;
    }

    time += time_step;
    Victim@ curVictim = victims[currentIndex];

    if (curVictim.done) {
        AdvanceCurrentIndex();
        return;
    }

    if (curVictim.applyDamage && curVictim.char.GetFloatVar("roll_ik_fade") == 1.0f || curVictim.char.GetBoolVar("pre_jump")) {
        curVictim.applyDamage = false;
        curVictim.char.rigged_object().anim_client().RemoveAllLayers();
        curVictim.char.Execute("ResetLayers();");
        curVictim.SetCharSpeed(curVictim.origSpeed);
        StopSound(curVictim.soundHandle);
        AdvanceCurrentIndex();
        return;
    }

    if (curVictim.applyDamage && time > params.GetFloat("ParticleInterval")) {
        ApplyDamageToVictim(curVictim);
        time = 0.0f;
    }

    AdvanceCurrentIndex();
}

void ApplyDamageToVictim(Victim@ victim) {
    if (victim.char.GetIntVar("knocked_out") == _awake) {
        victim.char.Execute("TakeBloodDamage(" + params.GetFloat("Damage") + ");");
        if (victim.char.GetIntVar("knocked_out") != _awake) {
            victim.char.Execute("Ragdoll(_RGDL_INJURED);");
            PlaySound("Data/Sounds/voice/animal2/voice_bunny_groan_3.wav", victim.char.position);
            StopSound(victim.soundHandle);
            if (params.GetInt("UseFire") == 1) {
                victim.TimeOfDeath = the_time;
            } else {
                victim.done = true;
            }
        }
    } else if (victim.TimeOfDeath + timeAfterDeath < the_time) {
        victim.done = true;
    }
}

void AdvanceCurrentIndex() {
    ++currentIndex;
    if (currentIndex >= int(victims.length())) {
        currentIndex = 0;
    }
}

int FindVictimIndex(int id) {
    for (uint i = 0; i < victims.length(); ++i) {
        if (victims[i].char.GetID() == id) {
            return i;
        }
    }
    return -1;
}
