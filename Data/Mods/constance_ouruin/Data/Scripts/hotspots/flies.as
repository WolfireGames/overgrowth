Object@ this_hotspot = ReadObjectFromID(hotspot.GetID());
array<Firefly@> fireflies = {};

// These are the variables that can be changed to get the desired effect.
uint nr_fireflies = 4;
const float movement_speed = 4.0;
const float turn_speed = 1.35;
const float max_brightness = 0.0f;
const float brightness_fade_speed = 10.0f;
const float size = 0.02f;
const float opacity = 1.0;

class Firefly{
	vec3 position;
	vec3 direction = vec3(0.0, 1.0, 0.0);
	float brightness = 0.0;
	vec3 random_position;
	float random_position_timer = 0.0;
	float random_tint_timer = 0.0;

	Firefly(){
		vec3 hotspot_scale = this_hotspot.GetScale() * 2.0;
		position = 	this_hotspot.GetTranslation() +
					vec3(RangedRandomFloat(-hotspot_scale.x, hotspot_scale.x),
						RangedRandomFloat(-hotspot_scale.y, hotspot_scale.y),
						RangedRandomFloat(-hotspot_scale.z, hotspot_scale.z));
	}

	void Update(){
		random_position_timer -= time_step;
		random_tint_timer -= time_step;

		// Pick a random position within the hotspot to move towards.
		if(random_position_timer <= 0.0){
			vec3 hotspot_scale = this_hotspot.GetScale() * 2.0;
			random_position = 	this_hotspot.GetTranslation() +
								vec3(RangedRandomFloat(-hotspot_scale.x, hotspot_scale.x),
									RangedRandomFloat(-hotspot_scale.y, hotspot_scale.y),
									RangedRandomFloat(-hotspot_scale.z, hotspot_scale.z));
			// Also randomize how often the random position is picked.
			random_position_timer = RangedRandomFloat(0.5, 3.0);
		}

		// Slowly change the direction the fly is moving towards so that it smoothly swoops around.
		vec3 target_direction = normalize(random_position - position);
		direction = mix(direction, target_direction, time_step * turn_speed);
		position += direction * time_step * movement_speed;
	}

	void Draw(){
		vec4 tint = vec4(brightness, brightness, brightness, opacity);
		DebugDrawBillboard("Data/Textures/spark.tga", position, size, tint, _delete_on_draw);
	}
}

void Init(){

}

void Update(){
	// Add a new firefly when the number of fireflies has been increased.
	if(nr_fireflies > fireflies.size()){
		Firefly@ new_firefly = Firefly();
		fireflies.insertLast(@new_firefly);
	// Delete the last firefly in the array if the number of fireflies has descreased.
	}else if(nr_fireflies < fireflies.size()){
		fireflies.removeLast();
	}

	for(uint i = 0; i < fireflies.size(); i++){
		fireflies[i].Update();
	}
}

void Draw(){
	for(uint i = 0; i < fireflies.size(); i++){
		fireflies[i].Draw();
	}
}