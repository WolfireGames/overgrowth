//-----------------------------------------------------------------------------
//           Name: fliproll.as
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

#include "interpdirection.as"

bool allow_rolling = true;

const float _flip_accel = 50.0f;
const float _flip_vel_inertia = 0.89f;
const float _flip_tuck_inertia = 0.7f;
const float _flip_axis_inertia = 0.9f;
const float _flip_facing_inertia = 0.08f;
const float _flip_speed = 2.5f; 
const float _wind_up_threshold = 0.1f;
const float _wall_flip_protection_time = 0.2f;    

class FlipInfo {
    bool flipping;
    float flip_angle;
    vec3 target_flip_axis;
    vec3 flip_axis;
    float flip_progress;
    bool flipped;
    float flip_vel;
    float target_flip_angle;
    float old_target_flip_angle;
    float target_flip_tuck;
    float flip_tuck;
    float wall_flip_protection;
    bool rolling;
    float start_roll_angle;
    float roll_face_angle;

    FlipInfo() {
        flip_progress = 0.0f;
        flip_angle = 1.0f;
        target_flip_angle = 1.0f;
        flip_vel = 0.0f;
        old_target_flip_angle = 0.0f;
        target_flip_tuck = 0.0f;
        flip_tuck = 0.0f;
        wall_flip_protection = 0.0f;
        flipping = false;
        flipped = false;
        rolling = false;
    }

    bool IsFlipping() {
        return flipping;
    }

    float PrepareFlipAngle(const float old_angle) {
        // Set flip angle to be close to 0.0 so it can target 1.0, and
        // chain smoothly with other flips
        float new_angle = old_angle - floor(old_angle);
        if(new_angle > 0.5f){
            new_angle -= 1.0f;
        }
        return new_angle;
    }

    vec3 GetFlipDir(vec3 target_velocity){
        // Flip direction is based on target velocity, or facing if
        // target velocity is too small
        if(length_squared(target_velocity)>0.2f){
            return normalize(target_velocity);
        } else {
            return normalize(this_mo.GetFacing());
        }
    }

    
    vec3 AxisFromDir(vec3 dir) {
        vec3 up = vec3(0.0f,1.0f,0.0f);
        return normalize(cross(up,dir));
    }

    vec3 ChooseFlipAxis() {
        return AxisFromDir(GetFlipDir(GetTargetVelocity()));
    }

    bool NeedWindup(){
        // If not rotating, need wind up anticipation before flipping
        return abs(flip_vel)<_wind_up_threshold;
    }

    void FlipRecover() {
        vec3 axis = this_mo.rigged_object().GetAvgAngularVelocity();
        axis.y = 0.0f;
        if(length(axis)>2.0f){
            axis = normalize(axis);
            flip_info.target_flip_axis = axis;
        } else {
            axis = flip_info.target_flip_axis;
        }
        quaternion rotation(this_mo.GetAvgRotationVec4());
        vec3 facing = Mult(rotation,vec3(0,0,1));
        vec3 up = Mult(rotation,vec3(0,1,0));
        
        vec3 going_up_vec = normalize(cross(axis,up));
        bool going_up = going_up_vec.y > 0.0f;

        float rotation_amount = acos(up.y)/6.283185f;
        if(going_up){
            rotation_amount = 1.0f-rotation_amount;
        }
        
        if(rotation_amount > 0.7f){
            rotation_amount = 0.0f;
        }

        flip_info.flip_progress = rotation_amount;
        flip_info.flip_angle = rotation_amount;

        vec3 flat_facing = vec3(facing.x,0.0f,facing.z);
        if(rotation_amount > 0.5f){
            flat_facing *= -1.0f; 
        }
        this_mo.SetRotationFromFacing(flat_facing);
    }

    
    void StartWallFlip(vec3 dir){
        flip_vel = _wind_up_threshold;
        StartFlip(dir);
        flip_angle += 0.1f;
        wall_flip_protection = _wall_flip_protection_time;
    }
    
    void StartLegCannonFlip(vec3 dir, float leg_cannon_flip){
        flip_vel = _wind_up_threshold;
        StartFlip(dir);
        flip_angle += leg_cannon_flip/-1.4f*0.25f;
        flip_tuck = 0.5f;
        wall_flip_protection = _wall_flip_protection_time;
    }

    void StartFlip(vec3 dir){
        if(allow_rolling && (species == _rabbit || species == _cat)){
            level.SendMessage("character_start_flip "+this_mo.getID());
            AchievementEvent("character_start_flip");
            rolling = false;
            flipping = true;
            wall_flip_protection = 0.0f;
            flip_progress = 0.0f;
            flip_angle = PrepareFlipAngle(flip_angle);
            target_flip_axis = AxisFromDir(GetFlipDir(dir));
            if(NeedWindup()){
                flip_axis = target_flip_axis;
                flip_vel = -2.0f;
            }
            this_mo.MaterialEvent("flip", this_mo.position);
        }
    }

    void StartFlip(){
        StartFlip(GetTargetVelocity());
    }

    void UpdateFlipProgress(const Timestep &in ts){
        if(flipping){
            flip_progress += _flip_speed * ts.step();
            if(flip_progress > 0.5f){
                flipped = true;
            }
            if(flip_progress > 1.0f){
                flipping = false;
            }
        }
    }

    void EndFlip(){
        flipping = false;
        flip_angle = 1.0f;
        flip_vel = 0.0f;
        flip_progress = 0.0f;
        flip_modifier_rotation = 0.0f;
    }

    void RotateTowardsTarget(const Timestep &in ts) {
        if(flipping){
            vec3 facing = InterpDirections(FlipFacing(),
                                           this_mo.GetFacing(),
                                           pow(1.0f-_flip_facing_inertia,ts.frames()));
            this_mo.SetRotationFromFacing(facing);
        }
    }

    void UpdateFlipTuckAmount() {
        target_flip_tuck = min(1.0f,max(0.0f,flip_vel));
        if(flipping){
            target_flip_tuck = max(sin(flip_progress*3.1417f),target_flip_tuck);
        }
        flip_tuck = mix(target_flip_tuck,flip_tuck,_flip_tuck_inertia);
    }

    void UpdateFlipAngle(const Timestep &in ts) {
        flip_vel += (target_flip_angle - flip_angle) * ts.step() * _flip_accel;
        flip_angle += flip_vel * ts.step() ;
        flip_vel *= pow(_flip_vel_inertia, ts.frames());
    }

    void UpdateFlip(const Timestep &in ts) {
        if(!flipping && !flipped){
            return;
        }
        UpdateFlipProgress(ts);
        RotateTowardsTarget(ts);
        UpdateFlipTuckAmount();
        UpdateFlipAngle(ts);
        
        flip_axis = InterpDirections(target_flip_axis,
                                     flip_axis,
                                     pow(_flip_axis_inertia,ts.frames()));

        flip_modifier_axis = flip_axis;
        flip_modifier_rotation = flip_angle*6.2832f;

        if(wall_flip_protection > 0.0f){
            wall_flip_protection -= time_step;
        }
    }

    bool RagdollOnImpact(){
        return flipping && wall_flip_protection <= 0.0f;
    }

    void StartedJump() {
        if(!flipping){
            flipped = false;
            flip_angle = 1.0f;
            flip_tuck = 0.0f;
            flip_modifier_axis = flip_axis;
            flip_modifier_rotation = flip_angle*6.2832f;
        } else {
            target_flip_angle = 1.0f;
        }
    }

    float GetTuck() {
        return flip_tuck;
    }

    bool IsRolling() {
        return flipping && rolling;
    }

    void StartRoll(vec3 target_velocity) {
        if(allow_rolling){
            level.SendMessage("character_start_roll "+this_mo.getID());
			AchievementEvent("character_start_roll");
            if(water_depth < 0.1){
                if(character_getter.GetTag("species") == "cat"){
                    this_mo.MaterialEvent("roll", this_mo.position - vec3(0.0f, _leg_sphere_size, 0.0f), 0.5f);
                } else {
                    this_mo.MaterialEvent("roll", this_mo.position - vec3(0.0f, _leg_sphere_size, 0.0f));
                    AISound(this_mo.position, QUIET_SOUND_RADIUS, _sound_type_foley);
                }
            }
            if(water_depth > 0.0 && water_depth < 0.4){
                this_mo.PlaySoundGroupAttached("Data/Sounds/water_foley/mud_slide_roll.xml", this_mo.position);
                AISound(this_mo.position, QUIET_SOUND_RADIUS, _sound_type_foley);
            }

            flipping = true;
            flip_progress = 0.0f;
            flip_angle = PrepareFlipAngle(flip_angle);
            flip_vel = 0.0f;

            roll_direction = GetFlipDir(target_velocity);
            flip_axis = AxisFromDir(roll_direction);
            target_flip_axis = AxisFromDir(roll_direction);

            start_roll_angle = atan2(roll_direction.z, roll_direction.x);
            vec3 roll_facing = this_mo.GetFacing();
            roll_face_angle = atan2(roll_facing.z, roll_facing.x);
                
            feet_moving = false;
            rolling = true;
            if(this_mo.controlled){
                SetOnFire(false);
            }
        }
    }

    void UpdateRollProgress(const Timestep &in ts){
        flip_progress += _roll_speed * ts.step();
        if(flip_progress > 1.0f){
            flipping = false;
        }
    }

    void UpdateRollVelocity(const Timestep &in ts){
        if(flip_progress < 0.95f){
            vec3 adjusted_vel = WorldToGroundSpace(roll_direction);
            vec3 flat_ground_normal = ground_normal;
            flat_ground_normal.y = 0.0f;
            roll_direction = InterpDirections(roll_direction,normalize(flat_ground_normal),mix(0.0f,0.1f,min(1.0f,(1.0f-ground_normal.y)*5.0f)));
            flip_axis = AxisFromDir(roll_direction);
            float roll_angle = atan2(roll_direction.z, roll_direction.x);
            float angle = roll_face_angle;// + roll_angle - start_roll_angle;
            this_mo.SetRotationFromFacing(vec3(cos(angle), 0.0f, sin(angle)));
            this_mo.velocity = mix(adjusted_vel * _roll_ground_speed,
                                this_mo.velocity, 
                                pow(0.95f,ts.frames()));
        }
    }

    void UpdateRollDuckAmount(){
        if(flip_progress < 0.8f){
            target_duck_amount = 1.0f;
        }
        if(flip_progress < 0.95f){
            duck_vel = 2.5f;
        }
        duck_amount = min(1.25f, duck_amount);
    }

    void UpdateRollAngle(const Timestep &in ts) {
        float old_flip_angle = flip_angle;
        flip_angle = mix(target_flip_angle, flip_angle, pow(0.8f,ts.frames()));
        flip_vel = (flip_angle - old_flip_angle)/(ts.step());
    }

    void UpdateRoll(const Timestep &in ts) {
        if(flipping){
            UpdateRollProgress(ts);
            UpdateRollVelocity(ts);
            UpdateRollDuckAmount();
            target_flip_angle = flip_progress;
        } else {
            target_flip_angle = 1.0f;
        }

        UpdateRollAngle(ts);        

        flip_modifier_axis = flip_axis;
        flip_modifier_rotation = flip_angle*6.2832f;
    }

    bool HasControl() {
        return flipping && flip_progress < 0.7f;
    }

    bool UseRollAnimation() {
        return flipping && flip_progress < 0.8f;
    }

    float WhooshAmount() {
        if(!rolling){
            return abs(flip_vel)*0.1f;
        } else {
            return 0.0f;
        }
    }

    void Land() {
        flip_angle = 1.0f;
        flip_vel = 0.0f;
        flip_modifier_axis = flip_axis;
        flip_modifier_rotation = flip_angle*6.2832f;
        flipping = false;
    }

    bool HasFlipped(){
        return flipped;
    }

    bool ShouldRagdollOnLanding(){
        return abs(flip_info.flip_angle - 0.5f)<0.3f;
    }

    bool ShouldRagdollIntoWall(){
        return flip_info.flipping && flip_info.flip_progress > 0.1f;
    }
    
    bool ShouldRagdollIntoSteepGround() {
        return flip_info.flipping && flip_info.flip_progress > 0.4f;
    }

    vec3 GetAxis() {
        return flip_axis;
    }
};

FlipInfo flip_info;
