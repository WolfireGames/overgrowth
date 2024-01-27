// --------------------------------------------------------
// Description.
// This hotspot manages various tasks for the FPS Character. Such as adding and removing the character, 
// updating bullets and adding vignette screen effects.
// --------------------------------------------------------
// Variables that can be changed.
float bullet_speed = 433.0f;
float max_bullet_distance = 150.0f;
float black_vignette_base = 1.0f;
float red_fade_duration = 0.4f;
float red_fade_alpha = 0.25f;
vec3 red_fade_color  = vec3(1.0f, 0.0f, 0.0f);
int impact_spark_particle_amount = 10;
// --------------------------------------------------------
// Variables used by the script that should not be changed.
Object@ this_hotspot = ReadObjectFromID(hotspot.GetID());
bool show_red = false;
array<Bullet@> bullets;
int fps_character_id = -1;
MovementObject@ fps_character;
bool setup_done = false;
bool fps_character_enabled = false;
float red_fade_start;
float red_fade_end = -1.0f;
float black_vignette_added = 0.0f;
float red_vignette_amount = 0.0f;
bool post_reset = false;

class Bullet{
	int user_id;
	vec3 position;
	vec3 forward_direction;
	float distance_done = 0.0f;
	float timer = 0.0f;
	// Constructor for the bullet with all the information needed to start moving forward.
	Bullet(int _user_id, vec3 spawn_position, vec3 _forward_direction){
		user_id = _user_id;
		position = spawn_position;
		forward_direction = _forward_direction;
	}
	// When the bullet has moved, keep a total distance traveled.
	// This way we can delete the bullet when no collision has been found endlessly, like shooting in the air.
	void SetPosition(vec3 new_position){
		distance_done += distance(position, new_position);
		position = new_position;
	}

	bool Update(){
		// Calculate the new bullet position by multiplying the direction by the bullet speed
		// and the time difference since the last update.
		vec3 new_position = position + (forward_direction * bullet_speed * time_step);
		// DebugDrawLine(position, new_position, vec3(0.5), vec3(0.5), _fade);
		bool collided = CheckBulletCollisions(position, new_position);
		SetPosition(new_position);
		// When the distance limit has been reached or a collision is found, return true in this function.
		// This will make the other Update function remove this bullet from the list of bullets.
		return (distance_done > max_bullet_distance || collided);
	}

	bool CheckBulletCollisions(vec3 start, vec3 &inout end){
		// Checking raycast collisions with characters is most important, so start with that.
		col.CheckRayCollisionCharacters(start, end);
		// DebugDrawWireSphere(start, 0.01, vec3(1.0, 0.0, 0.0), _persistent);
		// DebugDrawWireSphere(end, 0.015, vec3(0.0, 0.0, 1.0), _persistent);

		for(int i = 0; i < sphere_col.NumContacts(); i++){
			CollisionPoint collision_point = sphere_col.GetContact(i);
			vec3 collision_position = collision_point.position;
			int char_id = collision_point.id;
			
			if(char_id != -1 && char_id != user_id){
				MovementObject@ char = ReadCharacterID(char_id);
				char.rigged_object().Stab(collision_position, forward_direction, 1, 0);
				vec3 force = forward_direction * 5000.0f;
				float damage = 0.25;

				char.Execute("vec3 impulse = vec3(" + force.x + ", " + force.y + ", " + force.z + ");" +
							"vec3 pos = vec3(" + collision_position.x + ", " + collision_position.y + ", " + collision_position.z + ");" +
							"HandleRagdollImpactImpulse(impulse, pos, " + damage + ");");
				
				end = collision_position;

				PlaySoundGroup("Data/Sounds/weapon_foley/cut/flesh_hit.xml", sphere_col.GetContact(0).position);
				// If a collision with a valid character has been found, then just skip checking other characters
				// and also skip checking collisions with static objects, since the bullet is already done.
				return true;
			}
		}

		col.GetObjRayCollision(start, end);
		// The sphere_col variable actually holds the raycast results, not just sphere collision results.
		for(int i = 0; i < sphere_col.NumContacts(); i++){
			CollisionPoint collision_point = sphere_col.GetContact(i);
			// The collision point also contains the normal direction of the raycast.
			// This can be used to change the direction of the bullet, creating a ricochet effect.
			vec3 collision_position = collision_point.position;
			// You can get the id of the static object the raycast is colliding with to change
			// the impact sounds or create a different particle effect.
			int obj_id = collision_point.id;
			
			if(obj_id != -1){
				Object@ obj = ReadObjectFromID(obj_id);
				// Log(warning, "Object label " + obj.GetLabel());
				// Create the particles at the collision position and leave a decal on the surface.
				CreateImpactSparks(collision_position);
				MakeParticle("Data/Particles/fps_gun_decal.xml", collision_position - forward_direction, forward_direction * 10.0f);
				// Randomly select a ricochet sound effect.
				string path;
				switch(rand() % 17) {
					case 0:
						path = "Data/Sounds/ouruin/fps/rico/ricochet_1.wav"; break;
					case 1:
						path = "Data/Sounds/ouruin/fps/rico/ricochet_2.wav"; break;
					case 2:
						path = "Data/Sounds/ouruin/fps/rico/ricochet_3.wav"; break;
					case 3:
						path = "Data/Sounds/ouruin/fps/rico/ricochet_4.wav"; break;
					case 4:
						path = "Data/Sounds/ouruin/fps/rico/ricochet_5.wav"; break;
					case 5:
						path = "Data/Sounds/ouruin/fps/rico/ricochet_6.wav"; break;
					case 6:
						path = "Data/Sounds/ouruin/fps/rico/ricochet_7.wav"; break;
					case 7:
						path = "Data/Sounds/ouruin/fps/rico/ricochet_8.wav"; break;
					case 8:
						path = "Data/Sounds/ouruin/fps/rico/ricochet_9.wav"; break;
					case 9:
						path = "Data/Sounds/ouruin/fps/rico/ricochet_10.wav"; break;
					case 10:
						path = "Data/Sounds/ouruin/fps/rico/ricochet_11.wav"; break;
					case 12:
						path = "Data/Sounds/ouruin/fps/rico/ricochet_12.wav"; break;
					case 13:
						path = "Data/Sounds/ouruin/fps/rico/ricochet_13.wav"; break;
					case 14:
						path = "Data/Sounds/ouruin/fps/rico/ricochet_14.wav"; break;
					case 15:
						path = "Data/Sounds/ouruin/fps/rico/ricochet_15.wav"; break;
					case 16:
						path = "Data/Sounds/ouruin/fps/rico/ricochet_16.wav"; break;
					default:
						path = "Data/Sounds/ouruin/fps/rico/ricochet_17.wav"; break;
				}

				int sound_id = PlaySound(path, collision_position);
				// Change the gain and randomize the pitch to make it sound different every time.
				SetSoundGain(sound_id, RangedRandomFloat(0.4f, 1.2f));
				SetSoundPitch(sound_id, RangedRandomFloat(0.9f, 1.2f));
				// DebugDrawWireSphere(collision_position, 0.5, vec3(1.0), _fade);
				end = collision_position;
				return true;
			}
		}
		// No collision between characters or static objects have been found.
		return false;
	}
}
// The listening to level events needs to specifically enabled and disabled in hotspots.
void Init(){
	level.ReceiveLevelEvents(hotspot.GetID());
}

void Dispose() {
	level.StopReceivingLevelEvents(hotspot.GetID());
}
// The global variables do not automatically reset when a level reset is triggered.
// So we need to reset these values in this Reset function.
void Reset(){
	post_reset = true;
	show_red = false;
	red_fade_end = -1.0f;
}

void PostReset(){
	if(!post_reset){return;}
	SetActiveCharacter();
	post_reset = false;
}

void Draw(){
	// Only show the floating text on the hotspot when in editor mode and skip drawing the vignette textures.
	if(EditorModeActive()){
		DebugDrawText(this_hotspot.GetTranslation(), "FPS Level Hotspot", 1.0, true, _delete_on_draw);
		return;
	}else if(!fps_character_enabled){
		return;
	}

	float width = GetScreenWidth();
	float height = GetScreenHeight();

	if(red_fade_end != -1.0f){
		// Slowly blend from a value of 0.0 to 1.0 to create a smooth red death overlay.
		float blend_progress = min(1.0, 1.0 - ((red_fade_end - the_time) / (red_fade_end - red_fade_start)));
		HUDImage @diffuse_image = hud.AddImage();
		// This texture is completely white and can be tinted red or any color for that matter.
		diffuse_image.SetImageFromPath("Data/Textures/diffuse.tga");
		diffuse_image.position.y = 0.0f;
		diffuse_image.position.x = 0.0f;
		diffuse_image.position.z = -2.0f;
		// Scale the image so it covers the whole screen.
		diffuse_image.scale = vec3(width / diffuse_image.GetWidth(), height / diffuse_image.GetHeight(), 1.0);
		diffuse_image.color = vec4(red_fade_color.x, red_fade_color.y, red_fade_color.z, blend_progress * red_fade_alpha);
	}
	
	float black_vignette_amount = black_vignette_base + black_vignette_added;

	HUDImage @black_vignette_image = hud.AddImage();
	black_vignette_image.SetImageFromPath("Data/Textures/fps_vignette_black.tga");
	black_vignette_image.position.y = 0.0f;
	black_vignette_image.position.x = 0.0f;
	black_vignette_image.position.z = -5.0f;
	black_vignette_image.scale = vec3(width / black_vignette_image.GetWidth(), height / black_vignette_image.GetHeight(), 1.0);
	black_vignette_image.color = vec4(0.0f, 0.0f, 0.0f, black_vignette_amount);

	HUDImage @red_vignette_image = hud.AddImage();
	red_vignette_image.SetImageFromPath("Data/Textures/fps_vignette_white.tga");
	red_vignette_image.position.y = 0.0f;
	red_vignette_image.position.x = 0.0f;
	red_vignette_image.position.z = -4.0f;
	red_vignette_image.scale = vec3(width / red_vignette_image.GetWidth(), height / red_vignette_image.GetHeight(), 1.0);
	red_vignette_image.color = vec4(1.0f, 0.0f, 0.0f, red_vignette_amount);
}

void Update() {
	Setup();
	PostReset();
	CheckFPSCharacterDied();
	CheckFPSToggle();
	ReduceVignette();
	UpdateBullets();
}

void Setup(){
	if(setup_done){return;}
	setup_done = true;

	fps_character_id = CreateObject("Data/Characters/fps_character_actor.xml");
	@fps_character = ReadCharacterID(fps_character_id);
	fps_character.position = vec3(0.0f, -1000.0f, 0.0f);

	int player_id = GetPlayerCharacterID();
	if(player_id != -1){
		MovementObject@ player = ReadCharacterID(player_id);
		if(fps_character.char_path != player.char_path){
			fps_character.Execute("LoadCharacter(\"" + player.char_path + "\");");
		}
	}
}
// Slowly reduce both vignette effects to make damage and tunnel vision disappear.
void ReduceVignette(){
	black_vignette_added = max(0.0, black_vignette_added - time_step);
	red_vignette_amount = max(0.0, red_vignette_amount - time_step);
}

void CheckFPSCharacterDied(){
	if(!fps_character_enabled || EditorModeActive()){return;}
	// Switch on the red overlay and set the timer duration when the FPS Character dies.
	if(fps_character.GetIntVar("knocked_out") != _awake && !show_red){
		show_red = true;
		red_fade_start = the_time;
		red_fade_end = the_time + red_fade_duration;
	}
}

void CheckFPSToggle(){
	// Switch between the FPS Character and active player haracter when pressing F5 in with
	// the debug keys enabled.
	if(DebugKeysEnabled() && GetInputPressed(0, "f5")){
		fps_character_enabled = !fps_character_enabled;
		SetActiveCharacter();
	}
}

void SetActiveCharacter(){
	int player_id = GetPlayerCharacterID();
	MovementObject@ player = ReadCharacterID(player_id);
	Object@ player_obj = ReadObjectFromID(player_id);
	Object@ fps_character_obj = ReadObjectFromID(fps_character_id);

	if(fps_character_enabled){
		// To enable the FPS Character we don't actually set the "controlled" variable on the MovementObject.
		// This can cause issues when the engine notices there isn't a player character active and
		// spawn a new Turner. To solve this we just set player character in dialogue mode and let the
		// FPS Character script take control.
		float target_rotation = player.GetFloatVar("target_rotation");
		// Copy the camera rotation from the player to the FPS Character so that it
		// faces the same way for a smoother experience.
		fps_character.Execute("target_rotation = " + target_rotation + ";");
		fps_character.Execute("target_cam_rotation_y = " + target_rotation + ";");
		fps_character.Execute("cam_rotation_y = " + target_rotation + ";");

		player.velocity = vec3(0.0);
		fps_character.position = player.position + vec3(0.0f, 0.75f, 0.0f);
		vec3 player_position = player.position + vec3(0.0f, -50.0f, 0.0f);
		player.ReceiveMessage("set_dialogue_control true");
		player.ReceiveMessage("set_dialogue_position " + player_position.x + " " + player_position.y + " " + player_position.z);
		// Using ""SetEnabled" on the player character will freeze the character at the current position and hide it.
		// However the character can still be targeted and shot. To work around this we just set the visibility
		// to false, turn on the dialogue mode to keep it at the same position, and make it static just to make sure.
		player.static_char = true;
		player.visible = false;

		ScriptParams@ player_script_params = player_obj.GetScriptParams();
		// To make AI respond in the same way as to the player character we transfer over some parameters like Teams and Species.
		if(player_script_params.HasParam("Teams")){
			string player_teams = player_script_params.GetString("Teams");
			string player_species = player_script_params.GetString("Species");
			int player_species_value = player.GetIntVar("species");
			fps_character.Execute("SetTeams(\"" + player_teams + "\");");
			fps_character.Execute("SetSpecies(\"" + player_species + "\", " + player_species_value + ");");
		}

		fps_character.Execute("SetControl(true);");
		ForgetCharacter(player_id);
	}else{
		// To enable the player character again, we need to do this in reverse.
		if(!post_reset){
			player.visible = true;
			player.static_char = false;
			player.ReceiveMessage("set_dialogue_control false");
			player.position = fps_character.position - vec3(0.0f, 0.75f, 0.0f);
			player.Execute("FixDiscontinuity();");
			
			float target_rotation = fps_character.GetFloatVar("target_rotation");
			player.Execute("target_rotation = " + target_rotation + ";");
			player.SetRotationFromFacing(camera.GetFacing());
		}

		fps_character.position = fps_character.position + vec3(0.0f, -50.0f, 0.0f);
		fps_character.Execute("SetControl(false);");
		ForgetCharacter(fps_character_id);
	}
}

void ForgetCharacter(int id){
	// The AI will know there are in fact two characters that are used by the player.
	// So to keep them focussed on the active character, make them forget the inactive one.
	array<int> movement_objects = GetObjectIDsType(_movement_object);
	for(uint i = 0; i < movement_objects.size(); i++){
		if(MovementObjectExists(movement_objects[i])){
			MovementObject@ char = ReadCharacterID(movement_objects[i]);
			char.Execute("situation.MovementObjectDeleted(" + id + ");");
		}
	}
}

void UpdateBullets(){
	// Every bullet needs to be updated and when a collision has been detected, true will be
	// returned. After which the bullet can be removed from collection of bullets.
	for(uint i = 0; i < bullets.size(); i++){
		if(bullets[i].Update()){
			bullets.removeAt(i);
			i--;
		}
	}
}

void CreateImpactSparks(vec3 pos){
	float spark_speed = 5.0f;
	for(int i = 0; i < impact_spark_particle_amount; ++i) {
		MakeParticle("Data/Particles/metalspark.xml", pos, vec3(RangedRandomFloat(-spark_speed, spark_speed),
																RangedRandomFloat(-spark_speed, spark_speed),
																RangedRandomFloat(-spark_speed, spark_speed)));

		MakeParticle("Data/Particles/metalflash.xml", pos, vec3(RangedRandomFloat(-spark_speed, spark_speed),
																RangedRandomFloat(-spark_speed, spark_speed),
																RangedRandomFloat(-spark_speed, spark_speed)));
	}
}

int GetPlayerCharacterID() {
	int num = GetNumCharacters();
	for(int i = 0; i < num; i++){
		MovementObject@ char = ReadCharacter(i);
		if(char.is_player){
			return char.GetID();
		}
	}
	return -1;
}

void ReceiveMessage(string msg){
	TokenIterator token_iter;
	token_iter.Init();

	if(!token_iter.FindNextToken(msg)){
		return;
	}

	string token = token_iter.GetToken(msg);

	if(token == "level_event"){
		token_iter.FindNextToken(msg);
		token = token_iter.GetToken(msg);
		// Log(warning, token);
		// The FPS Character can be toggled using level messages via DHS.
		if(token == "toggle_fps_character"){
			fps_character_enabled = !fps_character_enabled;
			SetActiveCharacter();
		}else if(token == "add_bullet"){
			black_vignette_added += 0.2;
			int user_id;
			vec3 spawn_position;
			vec3 forward_direction;

			token_iter.FindNextToken(msg);
			user_id = atoi(token_iter.GetToken(msg));

			token_iter.FindNextToken(msg);
			spawn_position.x = atof(token_iter.GetToken(msg));
			token_iter.FindNextToken(msg);
			spawn_position.y = atof(token_iter.GetToken(msg));
			token_iter.FindNextToken(msg);
			spawn_position.z = atof(token_iter.GetToken(msg));

			token_iter.FindNextToken(msg);
			forward_direction.x = atof(token_iter.GetToken(msg));
			token_iter.FindNextToken(msg);
			forward_direction.y = atof(token_iter.GetToken(msg));
			token_iter.FindNextToken(msg);
			forward_direction.z = atof(token_iter.GetToken(msg));

			bullets.insertLast(Bullet(user_id, spawn_position, forward_direction));
		}else if(token == "achievement_event"){
			if(!fps_character_enabled || EditorModeActive()){return;}
			token_iter.FindNextToken(msg);
			token = token_iter.GetToken(msg);
			if(token == "enemy_ko"){
				TimedSlowMotion(0.25f, 0.75f, 0.0f);
			}else if(token == "player_was_hit"){
				red_vignette_amount = 0.5;
			}
		}
	}
}
