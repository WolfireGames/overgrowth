Object@ this_hotspot = ReadObjectFromID(hotspot.GetID());
array<Firefly@> fireflies = {};
bool player_in_proximity = false;
float proximity_check_timer = 0.0f;
int fire_object_id = -1;
float light_brightness = 0.0f;
float random_brightness_timer = 0.0f;
float target_light_brightness = 0.0f;

// These are the variables that can be changed to get the desired effect.
const uint NR_FIREFLIES = 38;
const float MOVEMENT_SPEED = 1.0;
const float TURN_SPEED = 0.25;
const float BRIGHTNESS_MAXIMUM = 50.0f;
const float BRIGHTNESS_FADE_SPEED = 10.0f;
const float FIREFLY_SIZE = 0.05f;
const float FIREFLY_OPACITY = 0.5;
const float PLAYER_PROXIMITY_THRESHOLD = 25.0;
const float LIGHT_DISTANCE = 20.0;
const float LIGHT_BRIGHTNESS_MINIMUM = 2.0f;
const float LIGHT_BRIGHTNESS_MAXIMUM = 5.0f;

class Firefly{
	vec3 position;
	// Use a random direction so that all the fireflies do not go in the same direction for the first few updates.
	vec3 direction = normalize(vec3(	RangedRandomFloat(-1.0, 1.0),
										RangedRandomFloat(-1.0, 1.0),
										RangedRandomFloat(-1.0, 1.0)));
	float brightness = 0.0;
	vec3 random_position;
	float random_position_timer = 0.0;
	float random_tint_timer = 0.0;

	Firefly(){
		vec3 hotspot_scale = this_hotspot.GetScale() * 2.0;
		position = 	this_hotspot.GetTranslation() +
					vec3(	RangedRandomFloat(-hotspot_scale.x, hotspot_scale.x),
							RangedRandomFloat(-hotspot_scale.y, hotspot_scale.y),
							RangedRandomFloat(-hotspot_scale.z, hotspot_scale.z));
	}

	void Update(){
		random_position_timer -= time_step;
		random_tint_timer -= time_step;

		// Pick a random position within the hotspot to move towards.
		if(random_position_timer <= 0.0){
			vec3 hotspot_scale = this_hotspot.GetScale() * 2.0;
			random_position = this_hotspot.GetTranslation() +
								vec3(	RangedRandomFloat(-hotspot_scale.x, hotspot_scale.x),
										RangedRandomFloat(-hotspot_scale.y, hotspot_scale.y),
										RangedRandomFloat(-hotspot_scale.z, hotspot_scale.z));
			// Also randomize how often the random position is picked.
			random_position_timer = RangedRandomFloat(0.5, 3.0);
		}

		// Turn the brightness all the way up at random intervals to make them blink.
		if(random_tint_timer <= 0.0){
			random_tint_timer = RangedRandomFloat(0.5, 3.0);
			brightness = BRIGHTNESS_MAXIMUM;
		}

		// Slowly fade out the brightness.
		brightness = mix(brightness, 0.0, time_step * BRIGHTNESS_FADE_SPEED);

		// Slowly change the direction the fly is moving towards so that it smoothly swoops around.
		vec3 target_direction = normalize(random_position - position);
		direction = mix(direction, target_direction, time_step * TURN_SPEED);
		position += direction * time_step * MOVEMENT_SPEED;
	}

	void Draw(){
		vec4 tint = vec4(brightness, brightness, brightness, FIREFLY_OPACITY);
		DebugDrawBillboard("Data/Textures/spark.tga", position, FIREFLY_SIZE, tint, _delete_on_draw);
	}
}

void CheckPlayerProximity(){
	proximity_check_timer -= time_step;
	if(proximity_check_timer <= 0.0){
		// Use a random time to wait between proximity checks so that not all firefly check at the same time.
		proximity_check_timer = RangedRandomFloat(1.0, 2.0);
		player_in_proximity = false;

		for(int i = 0; i < GetNumCharacters(); i++){
			MovementObject@ char = ReadCharacter(i);
			if(char.is_player && distance(char.position, this_hotspot.GetTranslation()) < PLAYER_PROXIMITY_THRESHOLD){
				player_in_proximity = true;
			}
		}
	}
}

void UpdateLight(){
	if(fire_object_id == -1){
		fire_object_id = CreateObject("Data/Objects/default_light.xml", true);
		Object@ fire_obj = ReadObjectFromID(fire_object_id);
		fire_obj.SetTranslation(this_hotspot.GetTranslation());
	}

	uint active_fireflies = fireflies.size();

	// Do not update the light when there are no fireflies or when there are still fireflies to create.
	if(active_fireflies == 0 || active_fireflies != NR_FIREFLIES){return;}

	// Take the first, last and middel firefly to get an average position.
	vec3 average = fireflies[0].position + fireflies[active_fireflies - 1].position + fireflies[active_fireflies / 2].position;
	average /= 3.0f;

	Object@ fire_obj = ReadObjectFromID(fire_object_id);

	random_brightness_timer -= time_step;
	if(random_brightness_timer < 0.0f){
		random_brightness_timer = RangedRandomFloat(0.1, 0.75);
		target_light_brightness = RangedRandomFloat(LIGHT_BRIGHTNESS_MINIMUM, LIGHT_BRIGHTNESS_MAXIMUM);
	}

	light_brightness = mix(light_brightness, target_light_brightness, time_step * 5.0f);

	fire_obj.SetTranslation(average);
	fire_obj.SetTint(light_brightness);
	fire_obj.SetScale(vec3(LIGHT_DISTANCE));
}

void Init(){

}

void Update(){
	CheckPlayerProximity();
	// Stop updating the light and fireflies when the player is too far away from the hotspot.
	if(!player_in_proximity){return;}
	UpdateLight();
	// Add a new firefly when the number of fireflies has been increased.
	if(NR_FIREFLIES > fireflies.size()){
		Firefly@ new_firefly = Firefly();
		fireflies.insertLast(@new_firefly);
	// Delete the last firefly in the array if the number of fireflies has descreased.
	}else if(NR_FIREFLIES < fireflies.size()){
		fireflies.removeLast();
	}

	for(uint i = 0; i < fireflies.size(); i++){
		fireflies[i].Update();
	}
}

void Draw(){
	// Since rendering seems to be hardest on performance, skip rendering the fireflies when the player is too far.
	if(!player_in_proximity){return;}
	for(uint i = 0; i < fireflies.size(); i++){
		fireflies[i].Draw();
	}
}