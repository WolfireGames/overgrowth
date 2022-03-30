//-----------------------------------------------------------------------------
//           Name: playercontrol.as
//      Developer: Wolfire Games LLC
//    Script Type: Movement Object Controller
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

#include "aircontrols_params.as"
#include "aschar.as"
#include "situationawareness.as"

float grab_key_time;
bool listening = false;
bool delay_jump;

const float kWalkSpeed = 0.2f;

// For pressing crouch to drop off ledges
bool crouch_pressed_on_ledge = false;
bool crouch_pressable_on_ledge = false;

Situation situation;
int got_hit_by_leg_cannon_count = 0;

int IsUnaware() {
    return 0;
}

enum DropKeyState {
    _dks_nothing,
    _dks_pick_up,
    _dks_drop,
    _dks_throw
};

DropKeyState drop_key_state = _dks_nothing;

enum ItemKeyState {
    _iks_nothing,
    _iks_sheathe,
    _iks_unsheathe
};

ItemKeyState item_key_state = _iks_nothing;

void AIMovementObjectDeleted(int id) {
}

string GetIdleOverride() {
    return "";
}

float last_noticed_time;

void DrawAIStateDebug() {
}

void DrawStealthDebug() {
}

bool DeflectWeapon() {
    return active_blocking;
}

int IsAggro() {
    return 1;
}

bool StuckToNavMesh() {
    return false;
}

void UpdateBrain(const Timestep &in ts) {
    EnterTelemetryZone("playercontrol.as UpdateBrain");
    startled = false;

    if(GetInputDown(this_mo.controller_id, "grab")) {
        grab_key_time += ts.step();
    } else {
        grab_key_time = 0.0f;
    }

    if(ledge_info.on_ledge && !GetInputDown(this_mo.controller_id, "crouch")) {
        crouch_pressable_on_ledge = true;
    } else if(!ledge_info.on_ledge) {
        crouch_pressable_on_ledge = false;
        crouch_pressed_on_ledge = false;
    }

    if(GetInputDown(this_mo.controller_id, "crouch") && crouch_pressable_on_ledge) {
        crouch_pressed_on_ledge = true;
    }

    if(time > last_noticed_time + 0.2f) {
        array<int> characters;
        GetVisibleCharacters(0, characters);

        for(uint i = 0; i < characters.size(); ++i) {
            situation.Notice(characters[i]);
        }

        last_noticed_time = time;
    }

    force_look_target_id = situation.GetForceLookTarget();

    if(!GetInputDown(this_mo.controller_id, "drop")) {
        drop_key_state = _dks_nothing;
    } else if (drop_key_state == _dks_nothing) {
        if((weapon_slots[primary_weapon_slot] == -1 || (weapon_slots[secondary_weapon_slot] == -1 && duck_amount < 0.5f)) &&
                GetNearestPickupableWeapon(this_mo.position, _pick_up_range) != -1) {
            drop_key_state = _dks_pick_up;
        } else {
            if(GetInputDown(this_mo.controller_id, "crouch") &&
                    duck_amount > 0.5f &&
                    on_ground &&
                    !flip_info.IsFlipping() &&
                    GetThrowTarget() == -1 &&
                    target_rotation2 < -60.0f) {
                drop_key_state = _dks_drop;
            } else if(GetInputDown(this_mo.controller_id, "grab") && tethered != _TETHERED_REARCHOKE ) {
                drop_key_state = _dks_drop;
            } else {
                drop_key_state = _dks_throw;
            }
        }
    }

    int primary_weapon_id = weapon_slots[primary_weapon_slot];
    string label = "";

    if(primary_weapon_id != -1) {
        label = ReadItemID(weapon_slots[primary_weapon_slot]).GetLabel();
    }

    if(!GetInputDown(this_mo.controller_id, "item") || label == "spear" || label == "big_sword" || label == "staff") {
        item_key_state = _iks_nothing;
    } else if (item_key_state == _iks_nothing) {
        if(primary_weapon_id == -1 || (weapon_slots[_sheathed_left] != -1 && weapon_slots[_sheathed_right] != -1)) {
            item_key_state = _iks_unsheathe;
        } else {  // if(held_weapon != -1 && sheathed_weapon == -1) {
            item_key_state = _iks_sheathe;
        }
    }

    if(delay_jump && !GetInputDown(this_mo.controller_id, "jump")) {
        delay_jump = false;
    }

    LeaveTelemetryZone();
}

void AIEndAttack() {

}

vec3 GetTargetJumpVelocity() {
    return vec3(0.0f);
}

bool TargetedJump() {
    return false;
}

bool IsAware() {
    return true;
}

void ResetMind() {
    situation.clear();
    got_hit_by_leg_cannon_count = 0;
}

int IsIdle() {
    return 0;
}

void HandleAIEvent(AIEvent event) {
    if(event == _climbed_up) {
        delay_jump = true;
    }
}

void MindReceiveMessage(string msg) {
}

bool WantsToCrouch() {
    if(!this_mo.controlled) {
        return false;
    }

    return GetInputDown(this_mo.controller_id, "crouch");
}

bool WantsToRoll() {
    if(!this_mo.controlled) {
        return false;
    }

    return GetInputPressed(this_mo.controller_id, "crouch");
}

bool WantsToJump() {
    if(!this_mo.controlled) {
        return false;
    }

    return GetInputDown(this_mo.controller_id, "jump") && !delay_jump;
}

bool WantsToAttack() {
    if(!this_mo.controlled) {
        return false;
    }

    if(on_ground) {
        return GetInputDown(this_mo.controller_id, "attack");
    } else {
        return GetInputPressed(this_mo.controller_id, "attack");
    }
}

bool WantsToRollFromRagdoll() {
    if(game_difficulty <= 0.4 && on_ground) {
        return true;
    }

    if(!this_mo.controlled) {
        return false;
    }

    return GetInputPressed(this_mo.controller_id, "crouch");
}

void BrainSpeciesUpdate() {

}

bool ActiveDodging(int attacker_id) {
    bool knife_attack = false;
    MovementObject@ char = ReadCharacterID(attacker_id);
    int enemy_primary_weapon_id = GetCharPrimaryWeapon(char);

    if(enemy_primary_weapon_id != -1) {
        ItemObject@ weap = ReadItemID(enemy_primary_weapon_id);

        if(weap.GetLabel() == "knife") {
            knife_attack = true;
        }
    }

    if(attack_getter2.GetFleshUnblockable() == 1 && knife_attack) {
        return active_dodge_time > time - (HowLongDoesActiveDodgeLast() + 0.2);  // Player gets bonus to dodge vs knife attacks
    } else {
        return active_dodge_time > time - HowLongDoesActiveDodgeLast();
    }
}

bool ActiveBlocking() {
    return active_blocking;
}

bool WantsToFlip() {
    if(!this_mo.controlled) {
        return false;
    }

    return GetInputPressed(this_mo.controller_id, "crouch");
}

bool WantsToGrabLedge() {
    if(!this_mo.controlled) {
        return false;
    }

    if(GetConfigValueBool("auto_ledge_grab")) {
        return !crouch_pressed_on_ledge;
    } else {
        return GetInputDown(this_mo.controller_id, "grab");
    }
}

bool WantsToThrowEnemy() {
    if(!this_mo.controlled) {
        return false;
    }

    // if(holding_weapon) {
    //     return false;
    // }

    return grab_key_time > 0.2f;
}

void Startle() {
}

bool WantsToDragBody() {
    if(!this_mo.controlled) {
        return false;
    }

    return GetInputDown(this_mo.controller_id, "grab");
}

bool WantsToPickUpItem() {
    if(!this_mo.controlled) {
        return false;
    }

    if(species == _wolf) {
        return false;
    }

    return drop_key_state == _dks_pick_up;
}

bool WantsToDropItem() {
    if(!this_mo.controlled) {
        return false;
    }

    if(species == _wolf) {
        return true;
    }

    return drop_key_state == _dks_drop;
}

bool WantsToThrowItem() {
    if(!this_mo.controlled) {
        return false;
    }

    return drop_key_state == _dks_throw;
}

bool WantsToThroatCut() {
    if(!this_mo.controlled) {
        return false;
    }

    return GetInputDown(this_mo.controller_id, "attack") || drop_key_state != _dks_nothing;
}

bool WantsToSheatheItem() {
    if(!this_mo.controlled) {
        return false;
    }

    return item_key_state == _iks_sheathe;
}

bool WantsToUnSheatheItem(int &out src) {
    if(!this_mo.controlled) {
        return false;
    }

    if(item_key_state != _iks_unsheathe && throw_weapon_time < time - 0.2) {
        return false;
    }

    src = -1;

    if(weapon_slots[_sheathed_left] != -1 && weapon_slots[_sheathed_right] != -1) {
        // If we have two weapons, draw better one
        string label1 = ReadItemID(weapon_slots[_sheathed_left]).GetLabel();
        string label2 = ReadItemID(weapon_slots[_sheathed_right]).GetLabel();

        if((label1 == "sword" || label1 == "rapier") && label2 == "knife") {
            src = _sheathed_left;
        } else {
            src = _sheathed_right;
        }
    } else if(weapon_slots[_sheathed_right] != -1) {
        src = _sheathed_right;
    } else if(weapon_slots[_sheathed_left] != -1) {
        src = _sheathed_left;
    }

    return true;
}


bool WantsToStartActiveBlock(const Timestep &in ts) {
    if(!this_mo.controlled) {
        return false;
    }

    return GetInputPressed(this_mo.controller_id, "grab");
}

bool WantsToFeint() {
    if(!this_mo.controlled || game_difficulty <= 0.5) {
        return false;
    } else {
        return GetInputDown(this_mo.controller_id, "grab");
    }
}

bool WantsToCounterThrow() {
    if(!this_mo.controlled) {
        return false;
    }

    return GetInputDown(this_mo.controller_id, "grab") && !GetInputDown(this_mo.controller_id, "attack");
}

bool WantsToJumpOffWall() {
    if(!this_mo.controlled) {
        return false;
    }

    return GetInputPressed(this_mo.controller_id, "jump");
}

bool WantsToFlipOffWall() {
    if(!this_mo.controlled) {
        return false;
    }

    return GetInputPressed(this_mo.controller_id, "crouch");
}

bool WantsToAccelerateJump() {
    if(!this_mo.controlled) {
        return false;
    }

    return GetInputDown(this_mo.controller_id, "jump");
}

vec3 GetDodgeDirection() {
    return GetTargetVelocity();
}

bool WantsToDodge(const Timestep &in ts) {
    if(!this_mo.controlled) {
        return false;
    }

    vec3 targ_vel = GetTargetVelocity();
    bool movement_key_down = false;

    if(length_squared(targ_vel) > 0.1f) {
        movement_key_down = true;
    }

    return movement_key_down;
}

bool WantsToCancelAnimation() {
    return GetInputDown(this_mo.controller_id, "jump") ||
           GetInputDown(this_mo.controller_id, "crouch") ||
           GetInputDown(this_mo.controller_id, "grab") ||
           GetInputDown(this_mo.controller_id, "attack") ||
           GetInputDown(this_mo.controller_id, "move_up") ||
           GetInputDown(this_mo.controller_id, "move_left") ||
           GetInputDown(this_mo.controller_id, "move_right") ||
           GetInputDown(this_mo.controller_id, "move_down");
}

// Converts the keyboard controls into a target velocity that is used for movement calculations in aschar.as and aircontrol.as.
vec3 GetTargetVelocity() {
    vec3 target_velocity(0.0f);

    if(!this_mo.controlled) {
        return target_velocity;
    }

    vec3 right;

    {
        right = camera.GetFlatFacing();
        float side = right.x;
        right.x = -right .z;
        right.z = side;
    }

    target_velocity -= GetMoveYAxis(this_mo.controller_id) * camera.GetFlatFacing();
    target_velocity += GetMoveXAxis(this_mo.controller_id) * right;

    if(GetInputDown(this_mo.controller_id, "walk")) {
        if(length_squared(target_velocity)>kWalkSpeed * kWalkSpeed) {
            target_velocity = normalize(target_velocity) * kWalkSpeed;
        }
    } else {
        if(length_squared(target_velocity)>1) {
            target_velocity = normalize(target_velocity);
        }
    }

    if(trying_to_get_weapon > 0) {
        target_velocity = get_weapon_dir;
    }

    return target_velocity;
}

// Called from aschar.as, bool front tells if the character is standing still. Only characters that are standing still may perform a front kick.
void ChooseAttack(bool front, string &out attack_str) {
    attack_str = "";

    if(on_ground) {
        if(!WantsToCrouch()) {
            if(front) {
                attack_str = "stationary";
            } else {
                attack_str = "moving";
            }
        } else {
            attack_str = "low";
        }
    } else {
        attack_str = "air";
    }
}

WalkDir WantsToWalkBackwards() {
    return FORWARDS;
}

bool WantsReadyStance() {
    return true;
}

int CombatSong() {
    return situation.PlayCombatSong() ? 1 : 0;
}

int IsAggressive() {
    return 0;
}
