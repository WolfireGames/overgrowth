//-----------------------------------------------------------------------------
//           Name: cam.as
//      Developer: Wolfire Games LLC
//    Script Type: 
//    Description: Main player camera control script
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
    position = co.GetTranslation();
    vec3 facing = co.GetRotation() * vec3(0,0,1);
    vec3 flat_facing = normalize(vec3(facing.x, 0.0f, facing.z));
    target_rotation =  atan2(-flat_facing.x, flat_facing.z) / 3.1417f * 180.0f;
    target_rotation2 =  asin(facing.y) / 3.1417f * 180.0f;
    rotation = target_rotation;
    rotation2 = target_rotation2;
}

int controller_id = 0;

vec3 position;
vec3 old_position;
float speed = 0.0f;
float target_rotation = 0.0f;
float target_rotation2 = 0.0f;
float rotation = 0.0f;
float rotation2 = 0.0f;
float smooth_speed = 0.0f;

bool focus_on_selection = false;
bool focus_increased_distance = false;

const float _camera_rotation_inertia = 0.8f;
const float _camera_inertia = 0.8f;
const float _camera_media_mode_rotation_inertia = 0.99f;
const float _camera_media_mode_inertia = 0.99f;
const float _acceleration = 20.0f;
const float _base_speed = 5.0f;

const string _increase_current_camera_fov_zoom_key = "-";
const string _decrease_current_camera_fov_zoom_key = "=";
const string _reset_current_camera_fov_key = "0";
const float _minimum_camera_fov = 1.0f;
const float _default_camera_fov = 90.0f;
const float _maximum_camera_fov = 180.0f;
const float _fov_config_change_per_tick = 0.125f;

bool just_got_control = false;

float GetAndUpdateCurrentFov() {
    float current_fov = _default_camera_fov;
    float config_fov = GetConfigValueFloat("editor_camera_fov");
    bool did_fov_change = false;

    if (config_fov != 0.0f) {
        current_fov = config_fov;
    }

    if (GetInputDown(controller_id, _increase_current_camera_fov_zoom_key)) {
        current_fov += _fov_config_change_per_tick;
        did_fov_change = true;
    } else if (GetInputDown(controller_id, _decrease_current_camera_fov_zoom_key)) {
        current_fov -= _fov_config_change_per_tick;
        did_fov_change = true;
    } else if (GetInputPressed(controller_id, _reset_current_camera_fov_key)) {
        current_fov = _default_camera_fov;
        did_fov_change = true;
    }

    if (current_fov < _minimum_camera_fov) {
        current_fov = _minimum_camera_fov;
    }
    if (current_fov > _maximum_camera_fov) {
        current_fov = _maximum_camera_fov;
    }

    if (did_fov_change) {
        SetConfigValueFloat("editor_camera_fov", current_fov);
    }

    return current_fov;
}

vec3 ScaleNonUniform(const vec3 &lhs, const vec3 &rhs) {
    return vec3(
        lhs.x * rhs.x,
        lhs.y * rhs.y,
        lhs.z * rhs.z);
}

vec3 compMax(const vec3 &lhs, const vec3 &rhs) {
    return vec3(
        max(lhs.x, rhs.x),
        max(lhs.y, rhs.y),
        max(lhs.z, rhs.z));
}

vec3 compMin(const vec3 &lhs, const vec3 &rhs) {
    return vec3(
        min(lhs.x, rhs.x),
        min(lhs.y, rhs.y),
        min(lhs.z, rhs.z));
}

void FrameSelection( bool increased_distance ) {
    focus_on_selection = true;
    focus_increased_distance = increased_distance;
    
}

vec3 GetScaledBounds(Object @obj) {
    vec3 bounding_box;

    switch (obj.GetType()) {
        case _hotspot_object:
            bounding_box.x = bounding_box.y = bounding_box.z = 4.0f;
            break;

        // TODO: Anything other type incorrectly handled?

        default:
            bounding_box = obj.GetBoundingBox();
            if (bounding_box.x == 0.0f && bounding_box.y == 0.0f && bounding_box.z == 0.0f) {
                bounding_box.x = bounding_box.y = bounding_box.z = 1.0f;
            }
            break;
    }

    return ScaleNonUniform(bounding_box, obj.GetScale());
}

void Update() {
    if(!co.controlled) {
        just_got_control = true;
        return;
    }

    if( just_got_control ) {
        if( co.has_position_initialized == false ) {

            co.has_position_initialized = true; 

            int num_chars = GetNumCharacters();
            if( num_chars > 0 )
            {
                 MovementObject@ char = ReadCharacter(0);
                 Object@ char_obj = ReadObjectFromID(char.GetID());
                 position = char_obj.GetTranslation() + vec3(0,1,0);
            }

        }
        just_got_control = false; 
    }

    if(level.QueryIntFunction("int HasCameraControl()") == 1){
        return;
    }
    
    camera.SetInterpSteps(1);

    old_position = position;
    vec3 vel;
    if(!co.frozen){
        vec3 target_velocity;
        bool moving = false;
        vec3 flat_facing = camera.GetFlatFacing();
        vec3 flat_right = vec3(-flat_facing.z, 0.0f, flat_facing.x);
        target_velocity += GetMoveXAxis(controller_id)*flat_right;
        if(!GetInputDown(controller_id, "crouch")){
            target_velocity -= GetMoveYAxis(controller_id)*camera.GetFacing();
        } else {
            target_velocity -= GetMoveYAxis(controller_id)*camera.GetUpVector();
        }
        if(length_squared(target_velocity) > 0.0f){
            moving = true;
        }
        if (moving) {
            speed += time_step * _acceleration;
        } else {
            speed = 1.0f;
        }
        speed = max(0.0f, speed);
        target_velocity = normalize(target_velocity);
        target_velocity *= sqrt(speed) * _base_speed;
        if(GetInputDown(controller_id, "space")){
            target_velocity *= 0.1f;   
        }
        float inertia;
        if(MediaMode()){
            inertia = _camera_media_mode_inertia;
        } else {
            inertia = _camera_inertia;
        }
        co.velocity = co.velocity * inertia + target_velocity * (1.0f - inertia);
        position += co.velocity * time_step;
        if(GetInputDown(controller_id, "mouse0") && !co.ignore_mouse_input){
            target_rotation -= GetLookXAxis(controller_id);
            target_rotation2 -= GetLookYAxis(controller_id);
        }
        if(!MediaMode()){
            SetGrabMouse(false);
        }
    }

    float current_fov = GetAndUpdateCurrentFov();

    if (focus_on_selection) {
        focus_on_selection = false;
        // Get bounding extents of selected objects
        bool is_any_object_selected = false;
        array<int> @object_ids = GetObjectIDs();
        int num_objects = object_ids.length();

        // Initialize with "unset" value - TODO: ideally would be infinite
        vec3 min_object_extents(100000000.0f, 100000000.0f, 100000000.0f);
        vec3 max_object_extents(-100000000.0f, -100000000.0f, -100000000.0f);

        for (int i = 0; i < num_objects; ++i) {
            Object @obj = ReadObjectFromID(object_ids[i]);

            if (obj.IsSelected()) {
                is_any_object_selected = true;
                vec3 scaled_bounds = GetScaledBounds(obj);
                vec3 translation = obj.GetTranslation();
                min_object_extents = compMin(min_object_extents, translation - scaled_bounds);
                max_object_extents = compMax(max_object_extents, translation + scaled_bounds);
            }
        }

        if (is_any_object_selected) {
            // Set new position, centering selected objects on screen
            vec3 selected_objects_origin = (max_object_extents + min_object_extents) * 0.5f;
            position = selected_objects_origin;

            // Back away from the objects a bit, so not always directly inside them
            float framing_distance;

            if (focus_increased_distance) {
                // Back up so loose bounding sphere of selected objects fill half of FOV
                min_object_extents -= selected_objects_origin;
                max_object_extents -= selected_objects_origin;
                float selected_objects_radius = sqrt(
                    max(length_squared(max_object_extents), length_squared(min_object_extents)));

                framing_distance = selected_objects_radius / sin(3.1415926535f / 180.0f * current_fov * 0.5f);
            } else {
                // Get relatively close to the object, don't bother framing nicely
                // Useful for teleporting to be close to objects
                framing_distance = 10.0f;
            }

            position -= framing_distance * normalize(camera.GetFacing());
        }
    }

    float _camera_collision_radius = 0.4f;
    vec3 old_new_position = position;
    //position = col.GetSlidingCapsuleCollision(old_position, position, _camera_collision_radius) ;
    //co.velocity += (position - old_new_position)/time_step;

    float rot_inertia;
    if(MediaMode()){
        rot_inertia = _camera_media_mode_rotation_inertia;
    } else {
        rot_inertia = _camera_rotation_inertia;
    }
    rotation = rotation * rot_inertia + 
               target_rotation * (1.0f - rot_inertia);
    rotation2 = rotation2 * rot_inertia + 
               target_rotation2 * (1.0f - rot_inertia);

    float smooth_inertia = 0.9f;
    smooth_speed = mix(length(co.velocity), smooth_speed, smooth_inertia);
    
    float move_speed = smooth_speed;
    SetAirWhoosh(move_speed*0.01f,min(2.0f,move_speed*0.01f+0.5f));
    
    float camera_vibration_mult = 0.001f;
    float camera_vibration = move_speed * camera_vibration_mult;
    rotation += RangedRandomFloat(-camera_vibration, camera_vibration);
    rotation2 += RangedRandomFloat(-camera_vibration, camera_vibration);

    quaternion rot = quaternion(vec4(0.0f, -1.0f, 0.0f, rotation  * 3.1417f / 180.0f)) *
                     quaternion(vec4(-1.0f, 0.0f, 0.0f, rotation2 * 3.1417f / 180.0f));
    co.SetRotation(rot);

    camera.SetYRotation(rotation);    
    camera.SetXRotation(rotation2);
    camera.SetZRotation(0.0f);
    camera.SetPos(position);

    camera.SetDistance(0);
    camera.SetVelocity(co.velocity); 
    camera.SetFOV(current_fov);
    camera.SetDOF(0,0,0,0,0,0);    
        

    camera.CalcFacing();
    camera.CalcUp();

    UpdateListener(position,vel,camera.GetFacing(),camera.GetUpVector());
    
    co.SetTranslation(position);
}
