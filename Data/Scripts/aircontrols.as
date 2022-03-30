//-----------------------------------------------------------------------------
//           Name: aircontrol.as
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

#include "fliproll.as"
#include "ledgegrab.as"
#include "aircontrols_params.as"

const float kJUMP_THRESHOLD_TIME = 0.1f; // time that must pass before a new jump, used in aschar.as
const float kJUMP_LAUNCH_DECAY = 2.0f;  // used for animating the initial jump stretch pose
// const float kWALL_RUN_FRICTION = 0.1f; // used to let wall running pull the character up, uncomment code in UpdateWallRun() to enable. Logic is currently commented out

// TODO: Remove these in OG 1.6 (if ever there is such a thing)
//       Just here for backwards compatibility with aschar 1.4 scripts
const float _jump_fuel_burn = 10.0f; // multiplier for amount of fuel lost per time_step
const float _jump_fuel = 5.0f; // used to set the amount of fuel available at the start of a jump
const float _air_control = 3.0f; // multiplier for the amount of directional control available while in the air
const float _jump_vel = 5.0f; // y-axis (up) velocity of jump used in GetJumpVelocity()
const float _jump_threshold_time = 0.1f; // time that must pass before a new jump, used in aschar.as
const float _jump_launch_decay = 2.0f;
const float _wall_run_friction = 0.1f; // used to let wall running pull the character up, uncomment code in UpdateWallRun() to enable

// at the end of this file, JumpInfo is instantiated as jump_info
// aschar.as uses jump_info to execute code in aircontrol.as

class JumpInfo {
    bool left_foot_jump;
    bool to_jump_with_left;
    bool no_rotate;

    array<vec3> jump_path;
    float jump_path_progress;
    bool follow_jump_path;
    vec3 jump_start_vel;

    float jetpack_fuel; // the amount of fuel available for acceleration
    float jump_launch; // used for the initial jump stretch pose

    bool has_hit_wall;
    bool hit_wall; // used to see if the character is wallrunning
    float wall_hit_time;
    vec3 wall_dir;
    vec3 wall_run_facing;

    float down_jetpack_fuel; // pushes the character down after upwards velocity of the jump has stopped
    float ledge_delay;

    JumpInfo() {
        jetpack_fuel = 0.0f;
        jump_launch = 0.0f;
        hit_wall = false;
        left_foot_jump = false;
        to_jump_with_left = false;
    }

    bool ClimbedUp() {
        bool val = ledge_info.climbed_up;
        ledge_info.climbed_up = false;
        return val;
    }

    void SetFacingFromWallDir() {
        vec3 flat_dir = wall_dir;
        flat_dir.y = 0.0f;
        if(length_squared(flat_dir) > 0.0f){
            flat_dir = normalize(flat_dir);
            this_mo.SetRotationFromFacing(flat_dir);
            //Print("Setting facing: " + wall_dir.x + " "+wall_dir.y+" "+wall_dir.z+"\n");
        }
    }

    void HitWall(vec3 dir) {
        float impact = dot(dir, this_mo.velocity);
        if(has_hit_wall || impact < 0.0f){
            return;
        }
        if(!ledge_info.on_ledge){
            if(character_getter.GetTag("species") == "cat"){
                this_mo.MaterialEvent("leftwallstep", this_mo.position+dir*_leg_sphere_size, 0.5f);
            } else {
                this_mo.MaterialEvent("leftwallstep", this_mo.position+dir*_leg_sphere_size);
            }
        }

        hit_wall = true;
        has_hit_wall = true;
        wall_hit_time = 0.0f;
        wall_dir = dir;
        //this_mo.velocity = vec3(0.0f);
        SetFacingFromWallDir();
    }

    void LostWallContact() {
        if(hit_wall){
            hit_wall = false;
            this_mo.SetRotationFromFacing(wall_run_facing);
        }
    }

    float GetFlailingAmount() {
        //const float _flail_threshold = 0.5f;
        //return min(1.0f,max(0.0f,(-this_mo.velocity.y-_shock_damage_threshold*_flail_threshold)*_shock_damage_multiplier*(1.0f)));
        return max(0.3f, min(1.0f,max(0.0f,(length(this_mo.velocity)-10.0f)*0.05f)));
    }

    void UpdateFreeAirAnimation() {
        float up_coord = this_mo.velocity.y/p_jump_initial_velocity + 0.5f;
        up_coord = min(1.5f,up_coord)+jump_launch*0.5f;
        up_coord *= -0.5f;
        up_coord += 0.5f;
        float flailing = GetFlailingAmount();
        flailing = min(0.6f+sin(time*2.0f)*0.2f,flailing);
        this_mo.rigged_object().anim_client().SetBlendCoord("up_coord",up_coord);
        this_mo.rigged_object().anim_client().SetBlendCoord("tuck_coord",max(flip_info.GetTuck(), tuck_override));
        this_mo.rigged_object().anim_client().SetBlendCoord("flail_coord",flailing);
        int8 flags = 0;
        if(left_foot_jump){
            flags = _ANM_MIRRORED;
        }
        this_mo.SetCharAnimation("jump",20.0f,flags);
        this_mo.rigged_object().ik_enabled = false;
    }

    void UpdateWallRunAnimation() {
        vec3 wall_right = wall_dir;
        float temp = wall_dir.x;
        wall_right.x = -wall_dir.z;
        wall_right.z = temp;
        float speed = length(this_mo.velocity);
        this_mo.SetCharAnimation("wall",5.0f);
        this_mo.rigged_object().anim_client().SetBlendCoord("ground_speed",speed);
        this_mo.rigged_object().anim_client().SetBlendCoord("speed_coord",speed*0.25f);
        this_mo.rigged_object().anim_client().SetBlendCoord("dir_coord",dot(normalize(this_mo.velocity), wall_right));
        vec3 flat_vel = this_mo.velocity;
        flat_vel.y = 0.0f;
        wall_run_facing = normalize(this_mo.GetFacing() + flat_vel*0.25f);
        /*DebugDrawLine(this_mo.position,
                      this_mo.position + normalize(flat_vel),
                      vec3(1.0f),
                      _delete_on_update);
        */
    }

    void UpdateLedgeAnimation() {
        ApplyIdle(5.0f, true);
    }

    void UpdateAirAnimation() {
        if(ledge_info.on_ledge){
            ledge_info.UpdateLedgeAnimation();
        } else if(hit_wall && !flip_info.IsFlipping()){
            UpdateWallRunAnimation();
        } else {
            UpdateFreeAirAnimation();
        }
    }

    // returns ledge_dir rotated 90 degrees clockwise
    vec3 WallRight() {
        vec3 wall_right = wall_dir;
        float temp = wall_dir.x;
        wall_right.x = -wall_dir.z;
        wall_right.z = temp;
        return wall_right;
    }

    void UpdateWallRun(const Timestep &in ts) {
        wall_hit_time += ts.step();
        if(wall_hit_time > 0.1f && this_mo.velocity.y < -4.0f && !ledge_info.on_ledge){
            LostWallContact();
        }

        // lets wall-running pull the character farther up, as affected by the kWALL_RUN_FRICTION
        /*this_mo.velocity -= physics.gravity_vector *
                            time_step *
                            kWALL_RUN_FRICTION;
        */
        col.GetSlidingSphereCollision(this_mo.position,
                                          _leg_sphere_size * 1.05f);
        if(sphere_col.NumContacts() == 0){
            LostWallContact();
        } else {
            vec3 closest_point;
            float closest_dist = -1.0f;
            for(int i=0; i<sphere_col.NumContacts(); i++){
                const CollisionPoint contact = sphere_col.GetContact(i);
                float dist = distance_squared(contact.position, this_mo.position);
                if(closest_dist == -1.0f || dist < closest_dist){
                    closest_dist = dist;
                    closest_point = contact.position;
                }
            }
            wall_dir = normalize(closest_point -
                                 this_mo.position);
            /*DebugDrawWireSphere(this_mo.position,
                                _leg_sphere_size * 1.0f,
                                vec3(1.0f,0.0f,0.0f),
                                _delete_on_update);
            for(int i=0; i<sphere_col.NumContacts(); ++i){
                DebugDrawLine(this_mo.position,
                              sphere_col.GetContact(0).position,
                              vec3(0.0f,1.0f,0.0f),
                              _delete_on_update);
            }*/
            SetFacingFromWallDir();
        }
        if(WantsToJumpOffWall()){
            AchievementEvent("jump_off_wall");
            StartWallJump(wall_dir * -1.0f);
        }

        if(WantsToFlipOffWall()){
            AchievementEvent("wall_flip");
            StartWallJump(wall_dir * -1.0f);
            flip_info.StartWallFlip(wall_dir * -1.0f);
        }
    }

    void UpdateAirControls(const Timestep &in ts) {
        if(!follow_jump_path){
            if(WantsToAccelerateJump()){
                // if there's fuel left and character is not moving down, height can still be increased
                if(jetpack_fuel > 0.0 && this_mo.velocity.y > 0.0) {
                    jetpack_fuel -= p_jump_fuel_burn * ts.step();
                    this_mo.velocity.y += p_jump_fuel_burn * ts.step();
                }
            } else {
                jetpack_fuel = 0.0f; // Don't allow releasing jump and then pressing it again
                // the character is pushed downwards to allow for smaller, controlled jumps
                if(down_jetpack_fuel > 0.0){
                    down_jetpack_fuel -= p_jump_fuel_burn * ts.step();
                    this_mo.velocity.y -= p_jump_fuel_burn * ts.step();
                }
            }
        }

        if(!hit_wall){
            if(WantsToFlip()){
                if(!flip_info.IsFlipping()){
                    flip_info.StartFlip();
                    ++roll_count;
                }
            }
            if(!flip_info.IsFlipping() && !flip_info.HasFlipped() && !no_rotate && !has_hit_wall){
                vec3 target_facing = this_mo.velocity;
                target_facing.y = 0.0;
                target_facing = normalize(target_facing);
                vec3 facing = InterpDirections(target_facing,
                                               this_mo.GetFacing(),
                                               pow(0.85f,ts.frames()));
                this_mo.SetRotationFromFacing(facing);
            }
        }

        if(hit_wall){
            UpdateWallRun(ts);
        }

        if(this_mo.no_grab == 0 && WantsToGrabLedge() && !fall_death && (ledge_info.on_ledge || ledge_delay <= 0.0f) && !flip_info.IsFlipping() && this_mo.velocity.y > -15.0){
            ledge_info.CheckLedges();
            if(ledge_info.on_ledge){
                has_hit_wall = false;
                HitWall(ledge_info.ledge_dir);
                ledge_delay = 0.3f;
                //this_mo.position.x = ledge_info.ledge_grab_pos.x;
                //this_mo.position.z = ledge_info.ledge_grab_pos.z;
            }
        }

        // if not holding a ledge, the character is airborne and can get controlled by arrow keys
        if(!ledge_info.on_ledge){
            ledge_delay -= ts.step();
            if(!follow_jump_path){
                vec3 target_velocity = GetTargetVelocity();
                this_mo.velocity += target_velocity * p_jump_air_control * ts.step();
            }
        }

        jump_launch -= kJUMP_LAUNCH_DECAY * ts.step();
        jump_launch = max(0.0f, jump_launch);
    }

    void StartWallJump(vec3 target_velocity) {
        vec3 old_vel_flat = this_mo.velocity;
        old_vel_flat.y = 0.0f;

        LostWallContact();
        StartFall();

        vec3 jump_vel = GetJumpVelocity(target_velocity, vec3(0.0, 1.0, 0.0));
        this_mo.velocity = jump_vel * 0.5f;
        jetpack_fuel = p_jump_fuel;
        jump_launch = 1.0f;
        down_jetpack_fuel = p_jump_fuel * 0.5f;


        if(character_getter.GetTag("species") == "cat"){
            this_mo.MaterialEvent("jump",this_mo.position + wall_dir * _leg_sphere_size, 0.5f);
        } else {
            this_mo.MaterialEvent("jump",this_mo.position + wall_dir * _leg_sphere_size);
        }
        AISound(this_mo.position, QUIET_SOUND_RADIUS, _sound_type_foley);
        this_mo.velocity += old_vel_flat;
        tilt = this_mo.velocity * 5.0f;
    }

    void StartJump(vec3 target_velocity, bool follow_path) {
        col.GetSlidingSphereCollision(this_mo.position, _leg_sphere_size);
        this_mo.position = sphere_col.adjusted_position;
        follow_jump_path = follow_path;

        StartFall();

        vec3 jump_vel;
        if(follow_path){
            jump_vel = target_velocity;
            target_velocity = vec3(target_velocity.x, 0.0f, target_velocity.z);
        } else {
            jump_vel = GetJumpVelocity(target_velocity, ground_normal);
        }
        this_mo.velocity = jump_vel;
		if(this_mo.controlled){
			AchievementEvent("player_jumped");
		}else{
			AchievementEvent("ai_jumped");
		}

        //Print("Start jump: "+this_mo.velocity.y+"\n");
        jetpack_fuel = p_jump_fuel;
        if(character_getter.GetTag("species") != "rabbit"){
            jetpack_fuel *= 0.2f;
            this_mo.velocity.y *= 0.6f;
        }
        jump_launch = 1.0f;
        down_jetpack_fuel = p_jump_fuel*0.5f;

        if(water_depth < 0.25){
            if(character_getter.GetTag("species") == "cat"){
                this_mo.MaterialEvent("jump",this_mo.position - vec3(0.0f, _leg_sphere_size, 0.0f), 0.5f);
            } else {
                this_mo.MaterialEvent("jump",this_mo.position - vec3(0.0f, _leg_sphere_size, 0.0f));
            }
        }
        AISound(this_mo.position, QUIET_SOUND_RADIUS, _sound_type_foley);

        if(length(target_velocity)>0.4f){
            no_rotate = false;
        } else {
            no_rotate = true;
        }

        left_foot_jump = to_jump_with_left;
        to_jump_with_left = !to_jump_with_left;
    }

    void StartFall() {
        jetpack_fuel = 0.0f;
        jump_launch = 0.0f;
        hit_wall = false;
        has_hit_wall = false;
        flip_info.StartedJump();
        ledge_delay = 0.0f;
        tuck_override = 0.0f;
    }

    // adjusts the velocity of jumps and wall jumps based on ground_normal
    vec3 GetJumpVelocity(vec3 target_velocity, vec3 temp_ground_normal){
        vec3 jump_vel = target_velocity * run_speed;
        jump_vel.y = p_jump_initial_velocity;

        vec3 jump_dir = normalize(jump_vel);
        if(dot(jump_dir, temp_ground_normal) < 0.3f){
            vec3 ground_up = temp_ground_normal;
            vec3 ground_front = target_velocity;
            if(length_squared(ground_front) == 0){
                ground_front = vec3(0,0,1);
            }
            vec3 ground_right = normalize(flatten(cross(ground_up, ground_front)));
            ground_front = normalize(cross(ground_right, ground_up));
            ground_up = normalize(cross(ground_front,ground_right));

            vec3 ground_space;
            ground_space.x = dot(ground_right, jump_vel);
            ground_space.y = dot(ground_up, jump_vel);
            ground_space.z = dot(ground_front, jump_vel);

            vec3 corrected_ground_space = vec3(0,p_jump_initial_velocity,length(target_velocity)*run_speed);
            ground_space = corrected_ground_space;

            jump_vel = ground_space.x * ground_right +
                       ground_space.y * ground_up +
                       ground_space.z * ground_front;
        }

        if(jump_vel.y > p_jump_initial_velocity){
            jump_vel.y = p_jump_initial_velocity;
        }

        return jump_vel;
    }
};