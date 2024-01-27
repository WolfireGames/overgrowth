#include "aschar_aux.as"
// --------------------------------------------------------
// Configurable variables for the FPS character.
float aiming_camera_fov = 70.0f;
float default_camera_fov = 90.0f;
float muzzle_flash_duration = 0.1f;
float muzzle_flash_brightness = 8.0f;
vec3 muzzle_flash_color = vec3(1.0f, 1.0f, 0.85f);
float footstep_camera_shake = 0.075f;
float shoot_camera_shake = 0.85f;
float land_camera_shake = 0.1f;
float hit_camera_shake = 0.75f;
float camera_shake_multiplier = 3.0f;
float firing_duration = 0.15f;
float firing_empty_duration = 0.1f;
float velocity_camera_tilt = 1.0f;
float horizontal_camera_lead_amount = 0.02f;
float facing_camera_lead_amount = -0.02f;
float recoil_amount = 0.3;
int smoke_particle_amount = 5;
int fire_particle_amount = 10;
int magazine_size = 7;
float aiming_look_speed = 15.0f;
float not_aiming_look_speed = 5.0f;
float look_sensitivity = 0.5f;
float camera_distance = -0.15f;
float aiming_movement_speed = 10.0f;
float walking_movement_speed = 25.0f;
float full_health = 3.0f;
float dof_strength = 0.45f;
float sideways_aim_multiplier = 0.85f;
float sideways_aim_interpolation_speed = 20.0f;
float jump_speed = 5.0f;
float jump_wait_amount = 0.25f;
float character_height_offset = -0.75f;
const float QUIET_SOUND_RADIUS = 5.0f;
const float LOUD_SOUND_RADIUS = 10.0f;
const float VERY_LOUD_SOUND_RADIUS = 20.0f;
// --------------------------------------------------------
// Variables used by the FPS script, not to be changed.
float fov = 90.0f;
float recoil = 0.0f;
float aim_height = 0.0f;
int main_id = -1;
int slide_open_id = -1;
int magazine_id = -1;
bool setup_done = false;
array<int> temporary_item_ids;
array<float> temporary_item_timers;
bool on_ground = true;
float in_air_timer = 0.0f;
bool has_control = false;
bool aiming = false;
float aiming_amount = 0.0;
int muzzle_flash_id = -1;
Object@ muzzle_flash_obj;
float muzzle_flash_timer = 0.0f;
int gun_state = Ready;
int rounds = 0;
float step_counter = 0.0f;
float camera_shake = 0.0f;
float target_rotation = 0.0f;
float jump_wait = 0.1f;
float firing_timer = 0.0f;
float dof_far = 0.0f;
float dof_distance = 0.0f;
float cam_rotation_x = 0.0f;
float cam_rotation_y = 0.0f;
float cam_rotation_z = 0.0f;
float target_cam_rotation_x = 0.0f;
float target_cam_rotation_y = 0.0f;
float target_cam_rotation_z = 0.0f;
float sideways_aim = 1.0f;
float flat_movement_velocity = 0.0f;
float vertical_movement_velocity = 0.0f;
float health = full_health;
int fps_weapon_id = -1;
ItemObject@ fps_weapon_item;
Object@ fps_weapon_obj;
int fps_weapon_slide_open_id = -1;
ItemObject@ fps_weapon_slide_open_item;
Object@ fps_weapon_slide_open_obj;
int fps_weapon_magazine_id = -1;
ItemObject@ fps_weapon_magazine_item;
Object@ fps_weapon_magazine_obj;
bool queue_fix_discontinuity = false;
float last_game_time = 0.0f;
vec3 horizontal_offset = vec3(0.0f);
vec3 facing_offset = vec3(0.0f);

enum GunStates{
	Ready = 0,
	Firing = 1,
	Empty = 2,
	FiringEmpty = 3,
	ReloadStart = 4,
	ReloadOldMagazineOut = 5,
	ReloadGrabNewMagazine = 6,
	ReloadNewMagazineIn = 7,
	ReloadPullSlide = 8
}

enum SoundType {
    _sound_type_foley,
    _sound_type_loud_foley,
    _sound_type_voice,
    _sound_type_combat
}
// --------------------------------------------------------
// Variables from aschar.as that are mostly unused, but needed to make the character script work.
int target_id = -1;
float startle_time;
bool has_jump_target = false;
vec3 jump_target_vel;
float awake_time = 0.0f;
const float AWAKE_NOTICE_THRESHOLD = 1.0f;
float enemy_seen = 0.0f;
bool hostile = true;
bool listening = true;
bool ai_attacking = false;
bool hostile_switchable = true;
int waypoint_target_id = -1;
int old_waypoint_target_id = -1;
const float _throw_counter_probability = 0.2f;
bool will_throw_counter;
int ground_punish_decision = -1;
float body_bob_freq = 0.0f;
float body_bob_time_offset;
float notice_target_aggression_delay = 0.0f;
int notice_target_aggression_id = 0.0f;

float target_attack_range = 0.0f;
float strafe_vel = 0.0f;
const float _block_reflex_delay_min = 0.1f;
const float _block_reflex_delay_max = 0.2f;
float block_delay;
bool going_to_block = false;
float dodge_delay;
bool going_to_dodge = false;
float roll_after_ragdoll_delay;
bool throw_after_active_block;
bool allow_active_block = true;
bool always_unaware = false;
bool always_active_block = false;

bool combat_allowed = true;
bool chase_allowed = false;

const float _ground_normal_y_threshold = 0.5f;
const float _leg_sphere_size = 0.45f;  // affects the size of a sphere collider used for leg collisions
const float _bumper_size = 0.5f;

class InvestigatePoint {
	vec3 pos;
	float seen_time;
};
array<InvestigatePoint> investigate_points;

const float kGetWeaponDelay = 0.4f;
float get_weapon_delay = kGetWeaponDelay;
const float kWalkSpeed = 0.5f;

enum AIGoal {_patrol, _attack, _investigate, _get_help, _escort, _get_weapon, _navigate, _struggle, _hold_still};
AIGoal goal = _patrol;

enum AISubGoal {_unknown = -1, _punish_fall, _provoke_attack, _avoid_jump_kick, _wait_and_attack, _rush_and_attack, _defend, _surround_target, _escape_surround,
	_investigate_slow, _investigate_urgent, _investigate_body, _investigate_around};
AISubGoal sub_goal = _wait_and_attack;

AIGoal old_goal;
AISubGoal old_sub_goal;

int investigate_target_id = -1;
vec3 nav_target;
int ally_id = -1;
int escort_id = -1;
int chase_target_id = -1;
int weapon_target_id = -1;

float investigate_body_time;
float patrol_wait_until = 0.0f;

enum PathFindType {_pft_nav_mesh, _pft_climb, _pft_drop, _pft_jump};
PathFindType path_find_type = _pft_nav_mesh;
vec3 path_find_point;
float path_find_give_up_time;

enum ClimbStage {_nothing, _jump, _wallrun, _grab, _climb_up};
ClimbStage trying_to_climb = _nothing;
vec3 climb_dir;

int num_ribbons = 1;
int fire_object_id = -1;
bool on_fire = false;
float flame_effect = 0.0f;

int shadow_id = -1;
int lf_shadow_id = -1;
int rf_shadow_id = -1;

// Parameter values
float p_aggression;
float p_ground_aggression;
float p_damage_multiplier;
float p_block_skill;
float p_block_followup;
float p_attack_speed_mult;
float p_speed_mult;
float p_fat;
float p_muscle;
float p_ear_size;
int p_lives;
int lives;

const float _base_run_speed = 8.0f; // used to calculate movement and jump velocities, change this instead of max_speed
const float _base_true_max_speed = 12.0f; // speed can never exceed this amount
float run_speed = _base_run_speed;
float true_max_speed = _base_true_max_speed;
float max_speed = run_speed; // this is recalculated constantly because actual max speed is affected by slopes

int tether_id = -1;

float breath_amount = 0.0f;
float breath_time = 0.0f;
float breath_speed = 0.9f;
array<float> resting_mouth_pose;
array<float> target_resting_mouth_pose;
float resting_mouth_pose_time = 0.0f;

float old_time = 0.0f;

vec3 ground_normal(0.0f, 1.0f, 0.0f);
vec3 flip_modifier_axis;
float flip_modifier_rotation;
vec3 tilt_modifier;
const float collision_radius = 1.0f;
enum IdleType{_stand, _active, _combat};
IdleType idle_type = _active;

bool idle_stance = false;
float idle_stance_amount = 0.0f;

// The main timer of the script, used whenever anything has to know how much time has passed since something else happened.
float time = 0;

vec3 head_look;
vec3 torso_look;

string[] legs = { "left_leg", "right_leg" };
string[] arms = { "leftarm", "rightarm" };

string dialogue_anim = "Data/Animations/r_sweep.anm";
int knocked_out = _awake;

// States are used to differentiate between various widely different situations
const int _movement_state = 0; // character is moving on the ground
const int _ground_state = 1; // character has fallen down or is raising up, ATM ragdolls handle most of this
const int _attack_state = 2; // character is performing an attack
const int _hit_reaction_state = 3; // character was hit or dealt damage to and has to react to it in some manner
const int _ragdoll_state = 4; // character is falling in ragdoll mode
int state = _movement_state;

vec3 last_col_pos;
float duck_amount = 0.5f;

bool balancing = false;
vec3 balance_pos;

enum WeaponSlot {
    _held_left = 0,
    _held_right = 1,
    _sheathed_left = 2,
    _sheathed_right = 3,
    _sheathed_left_sheathe = 4,
    _sheathed_right_sheathe = 5,
};

const int _num_weap_slots = 6;
bool show_debug = false;
bool dialogue_control = false;
bool static_char = false;
int invisible_when_stationary = 0;
int species = 0;
float threat_amount = 0.0f;
float target_threat_amount = 0.0f;
float threat_vel = 0.0f;
int primary_weapon_slot = _held_right;
int secondary_weapon_slot = _held_left;
array<int> weapon_slots = {-1, -1};
int knife_layer_id = -1;
int throw_knife_layer_id = -1;
float land_magnitude = 0.0f;
float character_scale = 2.0f;
AttackScriptGetter attack_attacker;
float block_stunned = 1.0f;
int block_stunned_by_id = -1;

array<BoneTransform> skeleton_bind_transforms;
array<BoneTransform> inv_skeleton_bind_transforms;
array<int> ik_chain_elements;
enum IKLabel {kLeftArmIK, kRightArmIK, kLeftLegIK, kRightLegIK,
			  kHeadIK, kLeftEarIK, kRightEarIK, kTorsoIK,
			  kTailIK, kNumIK };
array<int> ik_chain_start_index;
array<int> ik_chain_length;
array<float> ik_chain_bone_lengths;
array<int> bone_children;
array<int> bone_children_index;
array<vec3> convex_hull_points;
array<int> convex_hull_points_index;

// Key transform enums
const int kHeadKey = 0;
const int kLeftArmKey = 1;
const int kRightArmKey = 2;
const int kLeftLegKey = 3;
const int kRightLegKey = 4;
const int kChestKey = 5;
const int kHipKey = 6;
const int kNumKeys = 7;

array<float> key_masses;
array<int> root_bone;

array<int> flash_obj_ids;

float last_changed_com = 0.0f;
vec3 com_offset;
vec3 com_offset_vel;
vec3 target_com_offset;

array<int> roll_check_bones;
array<BoneTransform> key_transforms;
array<float> target_leg_length;

vec3 push_velocity;

array<vec3> temp_old_ear_points;
array<vec3> old_ear_points;
array<vec3> ear_points;

array<float> target_ear_rotation;
array<float> ear_rotation;
array<float> ear_rotation_time;

int skip_ear_physics_counter = 0;

array<vec3> temp_old_tail_points;
array<vec3> old_tail_points;
array<vec3> tail_points;
array<vec3> tail_correction;
array<float> tail_section_length;

// Verlet integration for arm physics
array<vec3> temp_old_arm_points;
array<vec3> old_arm_points;
array<vec3> arm_points;
enum ChainPointLabels {kHandPoint, kWristPoint, kElbowPoint, kShoulderPoint, kCollarTipPoint, kCollarPoint, kNumArmPoints};

vec3 old_com;
vec3 old_com_vel;
vec3 old_hip_offset;
array<float> old_foot_offset;
array<quaternion> old_foot_rotate;

vec3 old_head_facing;
vec2 old_angle;
vec2 head_angle;
vec2 target_head_angle;
vec2 head_angle_vel;
vec2 head_angle_accel;
float old_head_angle;

vec3 old_chest_facing;
vec2 old_chest_angle_vec;
vec2 chest_angle;
vec2 target_chest_angle;
vec2 chest_angle_vel;
float old_chest_angle;
float ragdoll_fade_speed = 1000.0f;
float preserve_angle_strength = 0.0f;

quaternion total_body_rotation;

array<vec3> temp_old_weap_points;
array<vec3> old_weap_points;
array<vec3> weap_points;

bool asleep = false;
bool sitting = false;
int max_ko_shield = 0;
int ko_shield = max_ko_shield;
bool g_is_throw_trainer;
float g_throw_counter_probability;
int group_leader = -1;
int last_num_frames = 1;

void Update(int num_frames) {
	Timestep ts(time_step, num_frames);
	time += ts.step();
	// The functions need to run even though the character isn't controlled at the moment.
	Setup();
	UpdateTemporaryItems();
	UpdateMuzzleFlash();
	// Make sure the user hasn't gone into editor mode when updating the character.
	if(has_control && !EditorModeActive()){
		UpdateCamera(ts);
		HandleGroundCollisions(ts);
		ApplyPhysics(ts);
		UpdateMovement();
		UpdateGunState();
		UpdateLook();
		UpdateJumping();
		UpdateAiming();
		UpdateFootsteps();
		UpdateAimAnimation();
		UpdateDebugKeys();
	}else{
		// Just stop any movement when this character isn't controlled.
		// This makes inspecting in editor mode easier.
		this_mo.velocity = vec3(0.0f);
	}

	last_col_pos = this_mo.position;
	last_num_frames = num_frames;
}

void UpdateDebugKeys(){
	if(!DebugKeysEnabled()){return;}
	// To change the model of the character we don't need to delete it and spawn a new one.
	// Just set the character model path, create the rigged object and set the aim animation again.
	// Doing this will make any ItemObjects attached to grip or sheathe hang in mid air,
	// so reattach them to the correct slots.
	if(GetInputPressed(0, "1")){
		DebugSwitchCharacter("Data/Characters/ghost.xml");
	}else if(GetInputPressed(0, "2")){
		DebugSwitchCharacter("Data/Characters/guard.xml");
	}else if(GetInputPressed(0, "3")){
		DebugSwitchCharacter("Data/Characters/raider_rabbit.xml");
	}else if(GetInputPressed(0, "4")){
		DebugSwitchCharacter("Data/Characters/pale_turner.xml");
	}else if(GetInputPressed(0, "5")){
		DebugSwitchCharacter("Data/Characters/male_rabbit_1.xml");
	}else if(GetInputPressed(0, "6")){
		DebugSwitchCharacter("Data/Characters/female_rabbit_1.xml");
	}else if(GetInputPressed(0, "7")){
		DebugSwitchCharacter("Data/Characters/pale_rabbit_civ.xml");
	}else if(GetInputPressed(0, "8")){
		DebugSwitchCharacter("Data/Characters/fancy_striped_cat.xml");
	}else if(GetInputPressed(0, "9")){
		DebugSwitchCharacter("Data/Characters/rabbot.xml");
	}
}

void DebugSwitchCharacter(string character_path){
	LoadCharacter(character_path);
	AttachWeaponItems();
	SetGunState(Ready);
}

void SetControl(bool control){
	// Reset the step counter variables so the character doesn't play a bunch of
	// footstep sounds when switching to enabled.
	last_col_pos = this_mo.position;
	step_counter = 0.0f;
	has_control = control;

	// The FPS Hotspot takes care of placing the character at the right spot and rotation
	// after which the character sets the correct models to be visible.
	if(control){
		// Show the character mesh and let the last gun state be used to enable the correct gun meshes.
		this_mo.visible = true;
		SetGunState(gun_state);
	}else if(setup_done){
		// Hide the weapon and character meshes when the control is disabled.
		this_mo.visible = false;
		fps_weapon_obj.SetEnabled(false);
		fps_weapon_slide_open_obj.SetEnabled(false);
		fps_weapon_magazine_obj.SetEnabled(false);
	}
}

void UpdateGunState(){
	// The gun uses a state machine to run certain functions.
	// This way the gun can only fire when ready or empty, and not when reloading for example.
	if(knocked_out == _dead){return;}
	switch(gun_state){
		case Ready:
			UpdateAttackPosition();
			CheckFiring();
			CheckReload();
			break;
		case Firing:
			UpdateAttackPosition();
			UpdateFiring();
			break;
		case Empty:
			CheckFiring();
			CheckReload();
			break;
		case FiringEmpty:
			UpdateFiringEmpty();
			break;
		case ReloadStart:
			break;
		case ReloadOldMagazineOut:
			break;
		case ReloadGrabNewMagazine:
			break;
		case ReloadNewMagazineIn:
			break;
		case ReloadPullSlide:
			break;
	}
}

void UpdateAttackPosition(){
	if(aiming_amount < 0.5){return;}
	// Because the weapon is an ItemObject and not a normal Object, we need to use the PhysicsTransform.
	mat4 trans = fps_weapon_item.GetPhysicsTransform();
	// In the weapon ItemObject XML file there are lines that are normally used for collision with characters.
	// The engine uses these lines to check if the weapon will stick in a body or terrain.
	// However we use this information to get the weapon barrel beginning and end.
	vec3 start = trans * fps_weapon_item.GetLineStart(0);
	vec3 end = trans * fps_weapon_item.GetLineEnd(0);

    vec3 a = trans.GetColumn(0);
    vec3 b = trans.GetColumn(1);
    vec3 c = trans.GetColumn(2);
	
	vec3 forward = normalize(end - start);
	vec3 bullet_spawn_point = end;

	vec3 ray_collision = col.GetRayCollision(bullet_spawn_point, bullet_spawn_point + (forward * 50.0f));
	// A particle is used to create a crosshair in the scene.
	MakeParticle("Data/Particles/fps_aim.xml", ray_collision, vec3(0.0));
	// Keep setting the dof strength while this function is called.
	// The script will slowly reduce this value in the UpdateCamera function when the player isn't aiming.
	// If the player is aiming at the sky, or somewhere very far away, then just skip this to make the DOF clear.
	if(distance(bullet_spawn_point, ray_collision) < 49.0f){
		dof_far = dof_strength;
	}

	dof_distance = distance(bullet_spawn_point, ray_collision);
	// Log(warning, "dof_distance " + dof_distance);
	// DebugDrawWireSphere(bullet_spawn_point, 0.15, vec3(1.0), _fade);
}

void UpdateAimAnimation(){
	float target_aim_height = target_cam_rotation_x * 3.1415 / 180.0f;
	// Add a bit of aim height based on the vertical velocity of the character.
	// So that the character responds to jumping or falling.
	// target_aim_height += vertical_movement_velocity * 0.02;
	// Multiply the aiming height by an arbitrary value to make the animation line up better with the camera angle.
	if(target_aim_height < 0.0){
		target_aim_height *= 1.6;
	}else{
		target_aim_height *= 0.9;
	}
	// Apply some interpolation to the aim height to prevent any sudden animation movement.
	aim_height = mix(aim_height, target_aim_height, time_step * 30.0f);
	// Log(warning, "aim_height " + aim_height);
	// The recoil should suddenly change in value, but slowly reduce as the character recovers.
	recoil = max(0.0, recoil - time_step * 1.3);
	// To get smooth looking animations we blend between a lot of different animations that
	// are driven by a combination of velocity, aim direction and recoil.
	this_mo.rigged_object().anim_client().SetBlendCoord("aim_distance", aiming_amount);
	this_mo.rigged_object().anim_client().SetBlendCoord("aim_far_left", aim_height + recoil);
	this_mo.rigged_object().anim_client().SetBlendCoord("aim_far_center", aim_height + recoil);
	this_mo.rigged_object().anim_client().SetBlendCoord("aim_far_right", aim_height + recoil);
	this_mo.rigged_object().anim_client().SetBlendCoord("aim_close_center", aim_height + recoil);
	// The hands of the character will turn sideways when moving into the right or left direction
	// of the camera. The amount of sideways movement and interpolation speed can be changed in the global variables.
	vec3 camera_right_direction = cross(camera.GetUpVector(), camera.GetFlatFacing());
	float sideways_movement_amount = dot(camera_right_direction, normalize(this_mo.velocity)) * (flat_movement_velocity / 3.0) * sideways_aim_multiplier;
	sideways_aim = mix(sideways_aim, sideways_movement_amount, time_step * sideways_aim_interpolation_speed);
	this_mo.rigged_object().anim_client().SetBlendCoord("aim_far_horizontal", sideways_aim);
}

void CheckReload(){
	if(GetInputPressed(this_mo.controller_id, "r")){
		SetGunState(ReloadStart);
		// Adding a layer of animation allows the character to do two animations at the same time.
		// For example a walking animation with a knife slash.
		this_mo.rigged_object().anim_client().AddLayer("Data/Animations/fps_reload.anm", 4.0f, _ANM_FROM_START);
	}
}

void CreateMagazine(){
	mat4 trans = fps_weapon_item.GetPhysicsTransform();
	// Line number 1 is the magazine line that goes from the bottom of gun to the barrel.
	vec3 start = trans * fps_weapon_item.GetLineStart(1);
	vec3 end = trans * fps_weapon_item.GetLineEnd(1);
	// The center of these two points can be used for the spawn point of the empty magazine.
	vec3 magazine_spawn_point = (start + end) / 2.0f;

	int empty_magazine_id = CreateObject("Data/Items/m1911_magazine.xml");
	Object@ empty_magazine_id_obj = ReadObjectFromID(empty_magazine_id);
	ItemObject@ empty_magazine_item = ReadItemID(empty_magazine_id);
	empty_magazine_id_obj.SetTranslation(magazine_spawn_point);
	// We can reuse the rotation of the weapon held in the hand to apply to the empty magazine.
	empty_magazine_id_obj.SetRotation(QuaternionFromMat4(trans.GetRotationPart()));
	empty_magazine_item.SetLinearVelocity(this_mo.velocity);
	// Setting the item as "Safe" will stop characters from catching it, although the "misc" type
	// in the ItemObject XML file should stop AI from trying to pick it up.
	empty_magazine_item.SetSafe();
	// Add the empty magazine to the array of temporary items so that it can be deleted after a timer runs out.
	temporary_item_ids.insertLast(empty_magazine_id);
	temporary_item_timers.insertLast(0.0f);
}

void CheckFiring(){
	if(GetInputPressed(this_mo.controller_id, "attack")){
		// When the gun is empty we play a click sound, and wait for a while before allowing 
		// pulling of the trigger again.
		if(rounds <= 0){
			PlaySound("Data/Sounds/ouruin/fps/1911/dry_fire.wav", fps_weapon_item.GetPhysicsPosition());
			firing_timer = firing_empty_duration;
			SetGunState(FiringEmpty);
		}else{
			Shoot();
			firing_timer = firing_duration;
			SetGunState(Firing);
		}
	}
}

void UpdateFiring(){
	firing_timer -= time_step;

	if(firing_timer <= 0.0){
		// When the firing timer has run out go into the correct gun state
		// by checking how many rounds are still available.
		if(rounds > 0){
			SetGunState(Ready);
		}else{
			SetGunState(Empty);
		}
	}
}

void UpdateFiringEmpty(){
	firing_timer -= time_step;
	// This short timeout when firing empty is mainly to stop the user
	// from pulling the trigger very fast and the click sounds overlapping.
	if(firing_timer <= 0.0){
		SetGunState(Empty);
	}
}

void UpdateMuzzleFlash(){
	// The muzzle flash object is a dynamic light that changes color and brightness based on the tint.
	// Calculate the brightness based on the color and how long the flash should be shown.
	// Skip setting the tint every update when the muzzleflash timer is zero and thus not needed.
	if(muzzle_flash_timer <= 0.0){return;}
	muzzle_flash_timer = max(0.0, muzzle_flash_timer - time_step);
	vec3 muzzle_flash_tint = mix(vec3(0.0f), muzzle_flash_color * muzzle_flash_brightness, muzzle_flash_timer / muzzle_flash_duration);
	muzzle_flash_obj.SetTint(muzzle_flash_tint);
}

void UpdateTemporaryItems(){
	// Temporary items are spend shells and empty magazines.
	for(uint i = 0; i < temporary_item_ids.size(); i++){
		temporary_item_timers[i] += time_step;
		if(temporary_item_timers[i] > 3.0f){
			// Delete them when their induvidual timer has reached 3 seconds.
			QueueDeleteObjectID(temporary_item_ids[i]);
			temporary_item_timers.removeAt(i);
			temporary_item_ids.removeAt(i);
			i--;
		}
	}
}

void UpdateFootsteps(){
	if(!on_ground){
		return;
	}
	// The footsteps are based on the distance traveled. This means the frequency of
	// footsteps sounds is low when moving slowly and it increased when walking faster.
	step_counter += flat_movement_velocity * time_step;

	float step_threshold = 1.25f;
	if(step_counter > step_threshold){
		// Add a small amount of screen shake to each step to sell the effect of walking.
		camera_shake += footstep_camera_shake;
		step_counter -= step_threshold;
		string path;
		// Randomly select a footstep audio clip.
		switch(rand() % 5) {
			case 0:
				path = "Data/Sounds/fps_gun_fs_1.wav"; break;
			case 1:
				path = "Data/Sounds/fps_gun_fs_2.wav"; break;
			case 2:
				path = "Data/Sounds/fps_gun_fs_3.wav"; break;
			case 3:
				path = "Data/Sounds/fps_gun_fs_4.wav"; break;
			default:
				path = "Data/Sounds/fps_gun_fs_5.wav"; break;
		}
		// Place the sound underneath the character at ground level.
		vec3 sound_position = this_mo.position + vec3(0.0, -1.0, 0.0);
		int sound_id = PlaySound(path, sound_position);
		// Add some random pitch to make the footsteps sounds different each time.
		SetSoundPitch(sound_id, RangedRandomFloat(0.9f, 1.2f));
		// Notify any AI that are nearby that a step has been taken.
		AISound(sound_position, QUIET_SOUND_RADIUS, _sound_type_loud_foley);
	}
}

void UpdateAiming(){
	// Slowly aim down when this character is dead, as if slumped over.
	if(knocked_out == _dead){
		aiming_amount = mix(aiming_amount, 0.0, time_step * 7.0);
	}else{
		// Check if the player is holding the right mouse button, if so then
		// start aiming and move the gun away from the body.
		aiming = GetInputDown(this_mo.controller_id, "grab");
		aiming_amount = mix(aiming_amount, aiming?1.0:0.0, time_step * 7.0);
	}
}

void UpdateMovement(){
	// Use a slower movement speed when aiming so the player has to choose between
	// moving faster and being more agile or increased aiming accuracy.
	float movement_speed = aiming?aiming_movement_speed:walking_movement_speed;
	// The character can still slightly move when in air to make controlling easier.
	if(!on_ground){
		movement_speed = 3.0;
	}
	vec3 target_velocity = GetTargetVelocity();
	// Skip the movement velocity calculation if no input has been given.
	if(length(target_velocity) > 0.1){
		// Adjust the movement direction based on the normal of the ground.
		vec3 target_velocity_right = cross(normalize(target_velocity), vec3(0.0f, 1.0f, 0.0f));
		// Using the cross product of the right side to the normal direction will result in an angled movement direction.
		vec3 movement_direction_ground_adjusted = cross(ground_normal, target_velocity_right);
		// Get the original movement speed and apply it to this normal adjusted direction.
		float orig_length = length(target_velocity);
		vec3 new_target_velocity = movement_direction_ground_adjusted * orig_length;
		// Calculate the velocity based on adjusted direction and movement speed.
		this_mo.velocity += new_target_velocity * time_step * movement_speed;
	}
}

vec3 GetTargetVelocity(){
	vec3 target_velocity(0.0f);
	// When the character is dead, then return an empty input direction.
	// This will make the character slowly stop moving.
	if(knocked_out == _dead){
		return target_velocity;
	}
	vec3 right = camera.GetFlatFacing();

	float side = right.x;
	right.x = -right.z;
	right.z = side;

	target_velocity -= GetMoveYAxis(this_mo.controller_id) * camera.GetFlatFacing();
	target_velocity += GetMoveXAxis(this_mo.controller_id) * right;
	// Walk slower when the "walk" key is held down. This is LCTRL by default.
	if(GetInputDown(this_mo.controller_id, "walk")) {
		if(!GetInputDown(this_mo.controller_id, "grab")) {
			if(length_squared(target_velocity) > kWalkSpeed * kWalkSpeed) {
				target_velocity = normalize(target_velocity) * kWalkSpeed;
			}
		}
	} else {
		if(length_squared(target_velocity) > 1) {
			target_velocity = normalize(target_velocity);
		}
	}

	return target_velocity;
}

void UpdateFOV(float delta_time){
	// Smoothly change the FOV when this character is in control based on aiming or not aiming.
	if(has_control && !EditorModeActive()){
		fov = mix(fov, (aiming?aiming_camera_fov:default_camera_fov), delta_time * 5.0f);
		camera.SetFOV(fov);
	}
}

void UpdateCamera(const Timestep &in ts){
	// Remove the camera shake over time.
	for(int i = 0; i < ts.frames(); i++){
		camera_shake *= 0.95f;
	}

	SetGrabMouse(true);

	RiggedObject@ rigged_object = this_mo.rigged_object();
	Skeleton@ skeleton = rigged_object.skeleton();
	vec3 facing = this_mo.GetFacing();
	vec3 flat_facing = normalize(vec3(facing.x, 0.0f, facing.z));

	float camera_vibration = camera_shake * camera_shake_multiplier;
	float y_shake = RangedRandomFloat(-camera_vibration, camera_vibration);
	float x_shake = RangedRandomFloat(-camera_vibration, camera_vibration);

	float interpolation_speed = mix(not_aiming_look_speed, aiming_look_speed, aiming_amount);
	cam_rotation_x = mix(cam_rotation_x, target_cam_rotation_x, time_step * interpolation_speed);
	cam_rotation_y = mix(cam_rotation_y, target_cam_rotation_y, time_step * interpolation_speed);
	cam_rotation_z = mix(cam_rotation_z, target_cam_rotation_z, time_step * 50.0f);
	// Camera shake is applied as rotation to the camera in random directions.
	camera.SetYRotation(cam_rotation_y + y_shake);
	camera.SetXRotation(cam_rotation_x + x_shake);
	camera.SetZRotation(cam_rotation_z);
	camera.CalcFacing();
	// Camera bobbing is calculated with a sine wave based on the flat velocity.
	// This will make the character look like it's taking steps while on the ground.
	float bob_amount = sin(the_time * 15.0) / 50.0 * (on_ground?flat_movement_velocity / 4.0:0.0f);
	vec3 bob_offset = vec3(0.0, bob_amount, 0.0);
	// Arbitrary sideways offset to make the camera line up with the gun.
	vec3 target_horizontal_offset = cross(camera.GetFacing(), camera.GetUpVector()) * 0.01;
	vec3 right_direction = cross(this_mo.GetFacing(), vec3(0.0f, 1.0f, 0.0f));
	// Move the camera left and right based on the velocity of the character so it leads the gun.
	target_horizontal_offset += dot(right_direction, normalize(this_mo.velocity)) * flat_movement_velocity * right_direction * horizontal_camera_lead_amount;
	// Move the camera forward or backward based on velocity as well.
	vec3 target_facing_offset = dot(flat_facing, normalize(this_mo.velocity)) * flat_movement_velocity * flat_facing * facing_camera_lead_amount;
	// Interpolate the camera offsets so that sudden movement is smoothed out.
	horizontal_offset = mix(horizontal_offset, target_horizontal_offset, time_step * 20.0f);
	facing_offset = mix(facing_offset, target_facing_offset, time_step * 20.0f);

	camera.SetPos(this_mo.position + bob_offset + horizontal_offset + facing_offset);
	camera.SetDistance(camera_distance);
	UpdateListener(camera.GetPos(), vec3(0, 0, 0), camera.GetFacing(), camera.GetUpVector());
	camera.SetInterpSteps(ts.frames());

	camera.SetVignetteTint(vec3(0.0f));
	camera.SetTint(vec3(0.0f));

	if(knocked_out == _dead){
		// When the character is dead just add a very fuzzy DOF amount.
		camera.SetDOF(0, 0, 0, 1.0, 0, 0.2);
	}else{
		// The DOF is calculated based on attack position and applied to the camera here.
		camera.SetDOF(1.0f, min(0.0, dof_distance - 1.0f), 1.0f, dof_far, dof_distance + 1.0f, 2.0f);
		dof_far = mix(dof_far, 0.0f, time_step * 5.0);
	}
}

void UpdateLook(){
	// When the character is dead also make the camera slowly look down.
	if(knocked_out == _dead){
		target_cam_rotation_x = mix(target_cam_rotation_x, -40.0f, time_step * 2.0f);
	}else{
		target_cam_rotation_y -= GetLookXAxis(this_mo.controller_id) * look_sensitivity;
		target_cam_rotation_x -= GetLookYAxis(this_mo.controller_id) * look_sensitivity;
		// Clamp the vertical rotation between 40 degrees down and 70 degrees up.
		target_cam_rotation_x = max(-40.0f, min(target_cam_rotation_x, 70.0f));
		// Just like the sideways aim animation blending, we apply a camera tilt in the direction we're going.
		target_cam_rotation_z = dot(normalize(this_mo.velocity), cross(camera.GetUpVector(), camera.GetFlatFacing())) * flat_movement_velocity * velocity_camera_tilt;
	}
}

void Shoot(){
	mat4 trans = fps_weapon_item.GetPhysicsTransform();
	vec3 start = trans * fps_weapon_item.GetLineStart(0);
	vec3 end = trans * fps_weapon_item.GetLineEnd(0);

    vec3 a = trans.GetColumn(0);
    vec3 b = trans.GetColumn(1);
    vec3 c = trans.GetColumn(2);

	vec3 up = vec3(a.y, b.y, c.y);
	vec3 forward = normalize(end - start);
	vec3 right = cross(forward, up);
	vec3 bullet_spawn_point = end;

	CreateShell(up, right);
	CreateSmoke(bullet_spawn_point, forward);
	CreateFire(bullet_spawn_point, forward);
	MakeParticle("Data/Particles/fps_gun_bullet.xml", bullet_spawn_point, forward * 0.15);

//	string sound = "Data/Sounds/ouruin/fps/1911/shoot.xml";
//            PlaySoundGroup(sound, this_mo.position, _sound_priority_very_high);
	string path;
	// Randomly select a gunshot audio clip.
	switch(rand() % 5) {
		case 0:
			path = "Data/Sounds/ouruin/fps/1911/shoot_1.wav"; break;
		case 1:
			path = "Data/Sounds/ouruin/fps/1911/shoot_2.wav"; break;
		case 2:
			path = "Data/Sounds/ouruin/fps/1911/shoot_3.wav"; break;
		default:
			path = "Data/Sounds/ouruin/fps/1911/shoot_4.wav"; break;
	}
	vec3 sound_position = this_mo.position;
	int sound_id = PlaySound(path, sound_position);
	// Add some random pitch to make the shots sounds different each time.
	SetSoundPitch(sound_id, RangedRandomFloat(0.9f, 1.1f));
	// Send a very loud AI sound event to all nearby characters.
	AISound(bullet_spawn_point, VERY_LOUD_SOUND_RADIUS, _sound_type_combat);

	camera_shake += shoot_camera_shake;
	muzzle_flash_timer = muzzle_flash_duration;
	muzzle_flash_obj.SetTranslation(bullet_spawn_point + forward * 0.25);
	// The hotspot handles bullet updatin, so send a message to create a new bullet.
	SendBulletMessage(bullet_spawn_point, forward);
	recoil += recoil_amount;
	rounds -= 1;
}

void CreateShell(vec3 up, vec3 right){
	mat4 trans = fps_weapon_item.GetPhysicsTransform();
	// The lines for the shell are stored in line 2.
	vec3 start = trans * fps_weapon_item.GetLineStart(2);
	vec3 end = trans * fps_weapon_item.GetLineEnd(2);
	vec3 shell_spawn_point = (start + end) / 2.0f;

	// DebugDrawLine(start, end, vec3(1.0), _persistent);

	int shell_id = CreateObject("Data/Items/m1911_shell.xml");
	Object@ shell_obj = ReadObjectFromID(shell_id);
	ItemObject@ shell_item = ReadItemID(shell_id);
	shell_obj.SetTranslation(shell_spawn_point);
	shell_obj.SetRotation(QuaternionFromMat4(trans.GetRotationPart()));
	shell_item.SetLinearVelocity((right + up) * RangedRandomFloat(1.0, 3.0) + this_mo.velocity);
	shell_item.SetAngularVelocity(vec3(RangedRandomFloat(-1.0, 1.0), RangedRandomFloat(-1.0, 1.0), RangedRandomFloat(-1.0, 1.0)) * 15.0f);
	shell_item.SetSafe();
	temporary_item_ids.insertLast(shell_id);
	temporary_item_timers.insertLast(0.0f);
}

void CreateFire(vec3 location, vec3 normal_direction){
	float spark_speed = 5.0f;
	vec3 force = normal_direction * 10.0f;
	for(int j = 0; j < fire_particle_amount; j++){
		vec3 random_force = vec3(	RangedRandomFloat(-spark_speed, spark_speed),
									RangedRandomFloat(-spark_speed, spark_speed),
									RangedRandomFloat(-spark_speed, spark_speed));
		MakeParticle("Data/Particles/fps_gun_fire.xml", location, force + random_force);
		MakeParticle("Data/Particles/metalspark.xml", location, force + random_force);
	}
}

void CreateSmoke(vec3 location, vec3 normal_direction){
	vec3 force = normal_direction * 5.0f;
	for(int i = 0; i < smoke_particle_amount; i++){
		vec3 random_force = vec3(	RangedRandomFloat(-1.0, 1.0),
									RangedRandomFloat(-1.0, 1.0),
									RangedRandomFloat(-1.0, 1.0));
		MakeParticle("Data/Particles/fps_gun_smoke.xml", location, force + random_force);
	}
}

void SendBulletMessage(vec3 spawn_position, vec3 move_direction){
	// The FPS Hotspot is listening to level messages, so send a message to add a bullet
	// with the given user id, position and direction.
	string msg = "add_bullet ";
	msg += this_mo.GetID() + " ";
	msg += spawn_position.x + " " + spawn_position.y + " " + spawn_position.z + " ";
	msg += move_direction.x + " " + move_direction.y + " " + move_direction.z;
	level.SendMessage(msg);
}

void SetScale(float new_character_scale){
	character_scale = new_character_scale;
	vec3 old_facing = this_mo.GetFacing();
	params.SetFloat("Character Scale", character_scale);
	this_mo.RecreateRiggedObject(this_mo.char_path);
	this_mo.SetRotationFromFacing(old_facing);
	FixDiscontinuity();
}

void HandleAnimationEvent(string event, vec3 world_pos){
	// The reload animation contains events that passed to this function.
	// Each event progresses the reload state one step and playes the correct sound effect.
	// Log(error, "Received " + event);
	if(gun_state == ReloadStart){
		PlaySound("Data/Sounds/fps_gun_remove_mag.wav", fps_weapon_item.GetPhysicsPosition());
		CreateMagazine();
		SetGunState(ReloadOldMagazineOut);
	}else if(gun_state == ReloadOldMagazineOut){
		SetGunState(ReloadGrabNewMagazine);
	}else if(gun_state == ReloadGrabNewMagazine){
		PlaySound("Data/Sounds/fps_gun_insert_mag.wav", fps_weapon_item.GetPhysicsPosition());
		SetGunState(ReloadNewMagazineIn);
	}else if(gun_state == ReloadNewMagazineIn){
		SetGunState(ReloadPullSlide);
		PlaySound("Data/Sounds/fps_gun_click.wav", fps_weapon_item.GetPhysicsPosition());
		rounds = magazine_size;
	}else if(gun_state == ReloadPullSlide){
		SetGunState(Ready);
	}
}

void Reset(){
	// When a character gets reset, the global variables lose their values.
	// So to make sure all the objects are available, just delete them and
	// let the Setup function create them again.
	QueueDeleteObjectID(muzzle_flash_id);
	QueueDeleteObjectID(fps_weapon_id);
	QueueDeleteObjectID(fps_weapon_slide_open_id);
	QueueDeleteObjectID(fps_weapon_magazine_id);
	// Also delete any temporary items before reset has finished.
	for(uint i = 0; i < temporary_item_ids.size(); i++){
		QueueDeleteObjectID(temporary_item_ids[i]);
	}
	temporary_item_ids.resize(0);
	temporary_item_timers.resize(0);

	this_mo.rigged_object().anim_client().RemoveAllLayers();
	this_mo.DetachAllItems();
	this_mo.rigged_object().CleanBlood();
	this_mo.rigged_object().SetWet(0.0);
	this_mo.rigged_object().Extinguish();
	ClearTemporaryDecals();
	this_mo.rigged_object().ClearBoneConstraints();
}

void PostReset(){
	CacheSkeletonInfo();
	fov = default_camera_fov;
	last_col_pos = this_mo.position;
	step_counter = 0.0f;
}

void Setup(){
	if(setup_done){return;}
	setup_done = true;

	CreateMuzzleFlashDynamicLight();
	CreateWeaponItems();
	AttachWeaponItems();
	rounds = magazine_size;
	// Make sure the character is disabled by default.
	SetControl(false);
}

void CreateWeaponItems(){
	// The weapon items are ItemObjects. They use a "grip" animation to place them in the correct
	// location based on an offset from the hand bone. So no location and rotation calculations are needed.
	fps_weapon_id = CreateObject("Data/Items/m1911.xml");
	@fps_weapon_item = ReadItemID(fps_weapon_id);
	@fps_weapon_obj = ReadObjectFromID(fps_weapon_id);

	fps_weapon_slide_open_id = CreateObject("Data/Items/m1911_slide_open.xml");
	@fps_weapon_slide_open_item = ReadItemID(fps_weapon_slide_open_id);
	@fps_weapon_slide_open_obj = ReadObjectFromID(fps_weapon_slide_open_id);

	fps_weapon_magazine_id = CreateObject("Data/Items/m1911_magazine.xml");
	@fps_weapon_magazine_item = ReadItemID(fps_weapon_magazine_id);
	@fps_weapon_magazine_obj = ReadObjectFromID(fps_weapon_magazine_id);
}

void AttachWeaponItems(){
	this_mo.AttachItemToSlot(fps_weapon_id, _at_grip, false);
	this_mo.AttachItemToSlot(fps_weapon_slide_open_id, _at_grip, false);
	this_mo.AttachItemToSlot(fps_weapon_magazine_id, _at_grip, true);
}

void SetGunState(int state){
	// Based on the gunstate we can enable and disable the models. This will make it look like
	// the gun is animated by using a slide open version of the gun.
	// Log(warning, "Gun state " + state);
	gun_state = state;

	fps_weapon_obj.SetEnabled(	gun_state == Ready ||
								gun_state == ReloadPullSlide);
	fps_weapon_slide_open_obj.SetEnabled(	gun_state == Firing ||
											gun_state == Empty ||
											gun_state == FiringEmpty ||
											gun_state == ReloadStart ||
											gun_state == ReloadOldMagazineOut ||
											gun_state == ReloadGrabNewMagazine ||
											gun_state == ReloadNewMagazineIn);
	fps_weapon_magazine_obj.SetEnabled(gun_state == ReloadGrabNewMagazine);
}

bool Init(string character_path){
	return LoadCharacter(character_path);
}

bool LoadCharacter(string character_path){
	// Log(error, "Load " + character_path);
	this_mo.char_path = character_path;
	bool success = character_getter.Load(this_mo.char_path);
	if(success){
		this_mo.RecreateRiggedObject(this_mo.char_path);
		this_mo.SetScriptUpdatePeriod(1);
		CacheSkeletonInfo();
		// Load the FPS Aim animation as the main character animation. This file contains references
		// to all the other animation that will be blended between.
		this_mo.SetAnimAndCharAnim("Data/Animations/fps_aim.xml", 20.0, _ANM_FROM_START, "aim_distance");
		this_mo.SetCharAnimation("aim_distance", 20.0, _ANM_FROM_START);
		Log(info, "Successfully loaded character : " + character_path);
		return true;
	}else{
		Log(error, "Failed at loading character : " + character_path);
		return false;
	}
}

void CreateMuzzleFlashDynamicLight(){
	// A dynamic light is used to illuminate the surroundings in front of the gun.
	muzzle_flash_id = CreateObject("Data/Objects/default_light.xml", true);
	@muzzle_flash_obj = ReadObjectFromID(muzzle_flash_id);
	// Set the default tint to black to make it not show any light at first.
	muzzle_flash_obj.SetTint(vec3(0.0f));
	// A scale of 60 will make the light visible over a large area and very bright in the center.
	muzzle_flash_obj.SetScale(vec3(60.0f));
}

int WasHit(string type, string attack_path, vec3 dir, vec3 pos, int attacker_id, float attack_damage_mult, float attack_knockback_mult){
	if(type == "attackimpact"){
		// The character can be hit and damaged until death occurs.
		PlaySoundGroup("Data/Sounds/hit/hit_hard.xml", pos, _sound_priority_high);
		// This achievement event is used to add a red damage vignette in the FPS Hotspot.
		AchievementEvent("player_was_hit");
		camera_shake += hit_camera_shake;
		health -= attack_damage_mult;
		if(health <= 0.0){
			Died();
		}
	}
	return 2;
}

void FixDiscontinuity() {
	queue_fix_discontinuity = true;
}

void PreDrawCameraNoCull(float curr_game_time){
	if(queue_fix_discontinuity){
		this_mo.FixDiscontinuity();
		FinalAnimationMatrixUpdate(1);
		queue_fix_discontinuity = false;
	}
	// When the game is in slowmotion, the Update function not called as frequent.
	// This means the FOV calculation would be choppy, so to fix this we do it in the
	// PreDraw function which updates every frame regardless of slowmotion.
	float delta_time = curr_game_time - last_game_time;
	UpdateFOV(delta_time);
	last_game_time = curr_game_time;
}

void FinalAnimationMatrixUpdate(int num_frames){
	RiggedObject@ rigged_object = this_mo.rigged_object();
	BoneTransform local_to_world;

	vec3 location = this_mo.position;
	vec3 facing = this_mo.GetFacing();

	facing = normalize(facing);
	target_rotation = mix(target_rotation, target_cam_rotation_y, time_step * 30.0f);
	
	quaternion rot = quaternion(vec4(0.0f, 1.0f, 0.0f, (target_rotation + 180.0f) * 3.1417f / 180.0f));
	this_mo.SetRotationFromFacing(rot * vec3(0.0, 0.0, 1.0));

	local_to_world.rotation = rot;
	// The center of the character is calculated a bit different than in aschar.
	// While in aschar the origin is at the feet of the character, we use the camera
	// as the center of the character. So add an offset to the model to make everything
	// line up again.
	local_to_world.origin = location + vec3(0.0f, -1.25f, 0.0);
	rigged_object.TransformAllFrameMats(local_to_world);
}

void SetParameters(){
	string team_str;
	character_getter.GetTeamString(team_str);

	params.AddString("Teams", team_str);
	params.AddFloatSlider("Character Scale",1.0,"min:0.25,max:2.0,step:0.02,text_mult:100");
	string species_name_from_tag = character_getter.GetTag("species");
    params.AddString("Species", species_name_from_tag);

	character_scale = params.GetFloat("Character Scale");

	if(character_scale != this_mo.rigged_object().GetRelativeCharScale()){
		this_mo.RecreateRiggedObject(this_mo.char_path);
		FixDiscontinuity();
		CacheSkeletonInfo();
	}
}

void SetTeams(string teams){
	// When the player character gets replaced by this FPS Character the Teams parameter
	// gets transfered by the FPS Hotspot to make AI react the same way to this character.
	params.SetString("Teams", teams);
}

void SetSpecies(string species_tag, int species_value){
	// There are some character specie specific reaction and attack modifiers in aschar.
	// So copy over the species ScriptParameter and enum value to this FPS Character.
	params.SetString("Species", species_tag);
	species = species_value;
}

void HandleCollisionsBetweenTwoCharacters(MovementObject @other){
	float distance_threshold = character_scale * 0.7f;
	vec3 this_com = this_mo.rigged_object().skeleton().GetCenterOfMass();
	vec3 other_com = other.rigged_object().skeleton().GetCenterOfMass();
	this_com.y = this_mo.position.y + character_height_offset;
	other_com.y = other.position.y;
	
	// DebugDrawWireSphere(this_com, 0.5, vec3(1.0), _fade);
	// DebugDrawWireSphere(other_com, 0.5, vec3(1.0), _fade);

	if(distance_squared(this_com, other_com) < distance_threshold * distance_threshold){
		vec3 dir = other_com - this_com;
		float dist = length(dir);
		dir /= dist;
		dir *= distance_threshold - dist;
		vec3 other_push = dir * 0.5f / (time_step) * 0.15f;

		// this_mo.position -= dir * 0.5f;
		this_mo.velocity -= other_push;

		other.Execute("
		if(!this_mo.static_char) {
			this_mo.position += vec3(" + dir.x + ", " + dir.y + ", " + dir.z + ") * 0.5f;
			push_velocity += vec3(" + other_push.x + ", " + other_push.y + ", " + other_push.z + ");
			MindReceiveMessage(\"collided " + this_mo.GetID() + "\");
		}");
	}
}

void ApplyPhysics(const Timestep &in ts) {
	bool collision_below = false;

	vec3 flat_current_position = this_mo.position;
	vec3 flat_last_position = last_col_pos;
	vec3 last_normal = vec3(0.0f, 1.0f, 0.0f);
	flat_current_position.y = 0.0;
	flat_last_position.y = 0.0;
	// The vertical and flat movement velocity is used later on to drive the animations and offset the camera.
	vertical_movement_velocity = (this_mo.position.y - last_col_pos.y) / time_step;
	flat_movement_velocity = distance(flat_current_position, flat_last_position) / time_step;
	// Check every collision to see if the character is standing on the ground.
	for(int i = 0; i < sphere_col.NumContacts(); i++){
		vec3 collision_point = sphere_col.GetContact(i).position;
		if(collision_point == vec3(0.0, 0.0, 0.0)){
			continue;
		}
		// The dot product would be 1.0 if the collision was directly below the character.
		vec3 collision_direction = normalize(collision_point - this_mo.position);
		float dot_product = dot(collision_direction, vec3(0.0, -1.0, 0.0));
		// But to allow some margin of error, a higher value than 0.9 should be enough.
		if(dot_product > 0.9){
			collision_below = true;
			last_normal = sphere_col.GetContact(i).normal;
			break;
		}
	}

	ground_normal = mix(ground_normal, last_normal, time_step * 20.0f);

	if(collision_below){
		// When the character has found a collision below and was previously off the ground
		// then start landing. Keeping an air timer prevents small bumps from triggering a land.
		if(!on_ground && in_air_timer > 0.5){
			Land();
		}
		on_ground = true;
		in_air_timer = 0.0f;
		// Reduce the velocity when standing on the ground, as if affected by friction.
		this_mo.velocity *= pow(0.95f, ts.frames());

	}else{
		on_ground = false;
		in_air_timer += time_step;
		// Keep adding velocity in the gravity direction to speed up faling.
		this_mo.velocity += physics.gravity_vector * ts.step();
	}
}

void Land(){
	// Create a landing sound effect at the feet of the character.
	vec3 sound_position = this_mo.position + vec3(0.0f, -1.0f, 0.0);
	PlaySound("Data/Sounds/fps_gun_land.wav", sound_position);
	// Add a little bit of screen shake that bigger than a normal footstep.
	camera_shake += land_camera_shake;
	// Inform the AI that a loud landing noise has been triggered.
	AISound(sound_position, LOUD_SOUND_RADIUS, _sound_type_loud_foley);
	// DebugDrawWireSphere(this_mo.position + vec3(0.0f, -1.0f, 0.0), 0.5, vec3(1.0), _fade);
}

void HandleGroundCollisions(const Timestep &in ts){
	vec3 offset = vec3(0.0, character_height_offset, 0.0);
	vec3 scale = vec3(1.0);
	float size = 0.5f;

    col.GetSlidingScaledSphereCollision(this_mo.position + offset, size, scale);
	this_mo.position = sphere_col.adjusted_position - offset;
}

void UpdateJumping(){
	if(knocked_out == _dead){return;}
	// Wait a while when the character has just jumped and standing on the ground.
	// This will make it look like the character has to land first to continue jumping.
	if(on_ground && jump_wait > 0.0f){
		jump_wait -= time_step;
	}

	if(on_ground && GetInputDown(this_mo.controller_id, "jump")){
		if(jump_wait < 0.0f){
			jump_wait = jump_wait_amount;
			vec3 jump_direction = vec3(0.0, 1.0, 0.0);
			this_mo.velocity += jump_direction * jump_speed;
		}
	}
}

void CacheSkeletonInfo() {
	RiggedObject@ rigged_object = this_mo.rigged_object();
	Skeleton@ skeleton = rigged_object.skeleton();
	int num_bones = skeleton.NumBones();
	skeleton_bind_transforms.resize(num_bones);
	inv_skeleton_bind_transforms.resize(num_bones);
	for(int i = 0; i < num_bones; i++) {
		skeleton_bind_transforms[i] = BoneTransform(skeleton.GetBindMatrix(i));
		inv_skeleton_bind_transforms[i] = invert(skeleton_bind_transforms[i]);
	}
}

void Died(){
	knocked_out = _dead;
	this_mo.DetachItem(fps_weapon_id);
}

void AchievementEvent(string event_str){
	level.SendMessage("achievement_event " + event_str);
}

void AISound(vec3 pos, float max_range, SoundType type){
	// Send a message to all the characters that are in range that a sound has been played
	// and they might want to investigate it.
    string msg = "nearby_sound " + pos.x + " " + pos.y + " " + pos.z + " " + max_range + " " + this_mo.getID() + " " + type;
    array<int> nearby_characters;
    GetCharactersInSphere(pos, max_range * 2.0, nearby_characters);
    // DebugDrawWireSphere(pos, max_range, vec3(1.0f), _fade);
    int num_chars = nearby_characters.size();

    for(int i = 0; i < num_chars; ++i) {
        ReadCharacterID(nearby_characters[i]).ReceiveScriptMessage(msg);
    }
}

// These functions are not used, but might be called externally by enemycontrol.as, playercontrol.as or aschar.as.
// So keep these available just in case, so the game doesn't crash.

int IsUnaware() {
	return 0;
}

int IsIdle() {
	if(goal == _patrol){
		return 1;
	} else {
		return 0;
	}
}

int IsAggressive() {
	return 0;
}

bool IsAware(){
	return hostile;
}

int NeedsAnimFrames(){
	return 0;
}

int IsPassive() {
	return 0;
}

int IsOnLedge(){
	return 0;
}

int IsDodging(){
	return 0;
}

int IsAggro(){
	return 1;
}

int AboutToBeHitByItem(int id){
	return 1;
}

void HandleRagdollImpactImpulse(const vec3 &in impulse, const vec3 &in pos, float damage){}
void NotifyItemDetach(int idex){}
void FinalAttachedItemUpdate(int num_frames){}
void HandleEditorAttachment(int x, int y, bool mirror){}
void Contact(){}
void Collided(float x, float y, float z, float o, float p){}
void ScriptSwap(){}
void SetEnabled(bool enabled){}
void ForceApplied(vec3 force){}
float GetTempHealth(){return 1.0f;}
void AttachWeapon(int id){}
void UpdatePaused(){}
void LayerRemoved(int id){}
void MindReceiveMessage(string msg){}
void ReceiveMessage(string msg){}
void Notice(int character_id){}
void ResetMind(){}
void HitByItem(string material, vec3 point, int id, int type){}
void ResetLayers(){}
void Dispose(){}
void DisplayMatrixUpdate(){}
void MovementObjectDeleted(int id){}
