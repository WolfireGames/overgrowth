//-----------------------------------------------------------------------------
//           Name: ledgegrab.as
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

const float _ledge_move_speed = 10.0f;   // Multiplier for horz and vert movement speed on ledges
const float _ledge_move_damping = 0.95f; // Multiplier that damps velocity every timestep
const float _height_under_ledge = 1.05f; // Target vertical distance between leg sphere origin and ledge height
        
class MovingGrip {  // This class handles grip positions (IK targets) as they move from point to point
                    // A timer is constantly going from 0.0 to 1.0 on a timeline, and each MovingGrip
                    // class has a set window of time to get from the old position to the new one
    vec3 start_pos;
    vec3 end_pos;
    float start_time;
    float end_time;
    float move_time;
    vec3 offset;
    float time;
    float speed;
    int id;

    void Move(vec3 _start_pos, 
              vec3 _end_pos, 
              float _start_time,
              float _end_time,
              vec3 _offset)
    {
        start_pos = _start_pos;             // Where is grip moving from
        end_pos = _end_pos;                 // Where is grip moving to
        start_time = _start_time;           // When does movement start
        end_time = _end_time;               // When does movement end
        move_time = end_time - start_time;  // How long does movement last
        time = 0.0f;                        // Current time on timeline
        offset = _offset;                   // Which way does gripper move away from the surface while in transit
        speed = 2.0f;                       // How fast does timer go
    }

    void SetSpeed(float _speed){
        speed = _speed;
    }

    void SetEndPos(vec3 _pos) {
        if(time < end_time){
            end_pos = _pos + vec3(this_mo.velocity.x,                           // Target grip position leads character movement
                                  0.0f,
                                  this_mo.velocity.z)* 0.1f;
        }
    }

    void Update(const Timestep &in ts) {
        time += speed * ts.step();                                 // Timer increments based on speed
        if(time >= end_time){
            if(id < 2 && distance(start_pos, end_pos)>0.1f){                    // Hands play grip sound at the end of their movements
                if(id==0){
                    this_mo.MaterialEvent("edge_crawl", this_mo.rigged_object().GetIKTargetPosition("leftarm"));
                } else {
                    this_mo.MaterialEvent("edge_crawl", this_mo.rigged_object().GetIKTargetPosition("rightarm"));
                }
            }
            start_pos = end_pos;
            time -= 1.0f;
        }
    }

    float GetWeight(){                                                          // How much weight is placed on this grip?
        if(time <= start_time){                                                 // Grip is fully weight-bearing when not moving
            return 1.0f;
        }
        if(time >= end_time){
            return 1.0f;
        }
        float progress = (time - start_time)/move_time;                         // Weight-bearing follows a sin valley while moving
        float weight = 1.0f - sin(progress * 3.14f);
        return weight * weight;
    }

    vec3 GetPos() {
        if(time <= start_time){                                                 // If not moving, just return start or end pos
            return start_pos;
        }
        if(time >= end_time){
            return end_pos;
        }
        float progress = (time - start_time)/move_time;                         // If moving, position is straight line from start to end, plus 
        vec3 pos = mix(start_pos, end_pos, progress);                           // offset sin arc
        return pos;
    }

    vec3 GetOffset() {
        float progress = (time - start_time)/move_time;
        return offset * sin(progress * 3.14f) * distance(start_pos, end_pos);
    }

}

class ShimmyAnimation {     // Shimmy animation is composed of a MovingGrip for each hand and foot, as well as a body offset
    MovingGrip[] hand_pos;  // Animated hand grips
    MovingGrip[] foot_pos;  // Animated foot grips
    vec3 lag_vel;           // Lagged clone of character velocity, used to detect accelerations
    vec3 ledge_dir;         // Which direction is the ledge we are climbing relative to the character
    vec3 ik_shift;
    vec3[] last_grip_pos;
    vec3[] old_ik_offset;
    vec3 target_pos;

    ShimmyAnimation() {
        hand_pos.resize(2);
        foot_pos.resize(2);
        hand_pos[0].id = 0;
        hand_pos[1].id = 1;
        foot_pos[0].id = 2;
        foot_pos[1].id = 3;
        lag_vel = vec3(0.0f);
        ik_shift = vec3(0.0f);
        last_grip_pos.resize(2);
        old_ik_offset.resize(2);
    }

    void Start(vec3 pos, vec3 dir, vec3 vel){                                   // Set up the rhythm of the hand and foot grippers
        ledge_dir = dir;
        hand_pos[0].Move(pos, pos, 0.0f, 0.4f, vec3(0.0f,0.1f,0.0f));           // Initialize hands to offset up when moving
        hand_pos[1].Move(pos, pos, 0.5f, 0.9f, vec3(0.0f,0.1f,0.0f));
        foot_pos[0].Move(pos, pos, 0.1f, 0.5f, dir * -0.1f);                    // Initialize feet to offset away from wall when moving
        foot_pos[1].Move(pos, pos, 0.6f, 1.0f, dir * -0.1f);
        ik_shift = vec3(0.0f);
        lag_vel = vel;
        last_grip_pos[0] = vec3(0.0f);
        last_grip_pos[1] = vec3(0.0f);
        old_ik_offset[0] = vec3(0.0f);
        old_ik_offset[1] = vec3(0.0f);
        target_pos = pos;
    }

    void Update(vec3 _target_pos, vec3 dir, const Timestep &in ts){
        hand_pos[0].SetEndPos(_target_pos);                                     // Update gripper targets
        hand_pos[1].SetEndPos(_target_pos);
        foot_pos[0].SetEndPos(_target_pos);
        foot_pos[1].SetEndPos(_target_pos);
        hand_pos[0].Update(ts);                                                   // Update gripper movement
        hand_pos[1].Update(ts);
        foot_pos[0].Update(ts);
        foot_pos[1].Update(ts);

        float speed = sqrt(this_mo.velocity.x * this_mo.velocity.x              // Gripper speed is proportional to sqrt of
                          +this_mo.velocity.z * this_mo.velocity.z)*3.0f;       // character's horizontal velocity
        speed = min(3.0f,max(1.0f,speed));
        
        hand_pos[0].SetSpeed(speed);                                            // Update gripper speed
        hand_pos[1].SetSpeed(speed);
        foot_pos[0].SetSpeed(speed);
        foot_pos[1].SetSpeed(speed);

        lag_vel = mix(this_mo.velocity, lag_vel, pow(0.95f,ts.frames()));        // Update lagged velocity clone

        ledge_dir = dir;                                                        // Update ledge direction
        target_pos = _target_pos;
    }

    void GetIKOffsets(const PassLedgeState &in pls, vec3 &out body, vec3 &out left_hand, vec3 &out right_hand, vec3 &out left_foot, vec3 &out right_foot) {
        const float _dist_limit = 0.4f;
        const float _dist_limit_squared = _dist_limit*_dist_limit;

        vec3 transition_offset = pls.ledge_grab_origin - this_mo.position;          // Offset used for transitioning into ledge climb
        vec3[] offset(4);
        offset[0] = hand_pos[0].GetPos() - this_mo.position;                    // Get gripper offsets relative to leg_sphere origin
        offset[1] = hand_pos[1].GetPos() - this_mo.position;
        offset[2] = foot_pos[0].GetPos() - this_mo.position;
        offset[3] = foot_pos[1].GetPos() - this_mo.position;


        offset[2].y *= pls.leg_ik_mult;                                             // Flatten out foot positions if needed
        offset[3].y *= pls.leg_ik_mult;

        vec3[] old_offset(2);
        old_offset[0] = offset[0];
        old_offset[1] = offset[1];

        vec3 check_pos = this_mo.rigged_object().GetIKTargetPosition("leftarm")+offset[0]+ledge_dir*0.1f;    
        vec3 check_dir = normalize(vec3(0.0f,-1.0f,0.0f)+ledge_dir*0.5f);
        col.GetSweptSphereCollision(check_pos-check_dir*0.2f,
                                        check_pos+check_dir*0.4f,
                                        0.05f);
        if(sphere_col.NumContacts() != 0){
            last_grip_pos[0] = sphere_col.position + vec3(0.0f,-0.15f,0.0f);
            offset[0] += sphere_col.position - check_pos;
            offset[0].y -= 0.10f;
        } else {
            if(last_grip_pos[0] != vec3(0.0f)){
                offset[0] = last_grip_pos[0] - this_mo.rigged_object().GetIKTargetPosition("leftarm");   
            }
        }

        check_pos = this_mo.rigged_object().GetIKTargetPosition("rightarm")+offset[1]+ledge_dir*0.1f;    
        col.GetSweptSphereCollision(check_pos-check_dir*0.2f,
                                        check_pos+check_dir*0.4f,
                                        0.05f); 
        if(sphere_col.NumContacts() != 0){
            last_grip_pos[1] = sphere_col.position + vec3(0.0f,-0.15f,0.0f);
            offset[1] += sphere_col.position - check_pos;
            offset[1].y -= 0.10f;
        } else {
            if(last_grip_pos[1] != vec3(0.0f)){
                offset[1] = last_grip_pos[1] - this_mo.rigged_object().GetIKTargetPosition("rightarm");   
            }
        }

        vec3 ik_offset;

        ik_offset = offset[0] - old_offset[0];
        old_ik_offset[0] = mix(ik_offset, old_ik_offset[0], 0.8f);
        offset[0] = old_offset[0] + old_ik_offset[0];

        ik_offset = offset[1] - old_offset[1];
        old_ik_offset[1] = mix(ik_offset, old_ik_offset[1], 0.8f);
        offset[1] = old_offset[1] + old_ik_offset[1];

        vec3 new_ik_shift = (offset[0]-old_offset[0] + offset[1]-old_offset[1])*0.5f;
        ik_shift = mix(new_ik_shift, ik_shift, 0.9f);
        offset[2].y += ik_shift.y;
        offset[3].y += ik_shift.y;
        
        offset[0] += hand_pos[0].GetOffset();
        offset[1] += hand_pos[1].GetOffset();
        offset[0] += foot_pos[0].GetOffset();
        offset[1] += foot_pos[1].GetOffset();

        float[] weight(4);
        weight[0] = hand_pos[0].GetWeight();                                    // Get weight bearing for each gripper
        weight[1] = hand_pos[1].GetWeight();
        weight[2] = foot_pos[0].GetWeight();
        weight[3] = foot_pos[1].GetWeight();
        float total_weight = weight[0] + weight[1] + weight[2] + weight[3];     
        vec3 weight_offset = vec3(0.0f);
        weight_offset += this_mo.rigged_object().GetIKTargetPosition("leftarm") * (weight[0] / total_weight);   // Get weighted average of gripper offsets
        weight_offset += this_mo.rigged_object().GetIKTargetPosition("rightarm") * (weight[1] / total_weight);
        weight_offset += this_mo.rigged_object().GetIKTargetPosition("left_leg") * (weight[2] / total_weight);
        weight_offset += this_mo.rigged_object().GetIKTargetPosition("right_leg") * (weight[3] / total_weight);
        weight_offset -= this_mo.position;
        weight_offset -= dot(ledge_dir,weight_offset) * ledge_dir;              // Flatten weighted offset against ledge plane
        float move_amount = sqrt(this_mo.velocity.x * this_mo.velocity.x
                                +this_mo.velocity.z * this_mo.velocity.z) * 0.4f; // Get constant multiplier for weight_offset based on movement
        weight_offset *= max(0.0f,min(1.0f,move_amount-0.1f))*0.5f;
        weight_offset.y = 0.0f;
        weight_offset += (lag_vel - this_mo.velocity) * 0.1f;                   // Add acceleration lag to weight offset
        //weight_offset += this_mo.velocity * 0.05f;
        weight_offset.y += 0.3f * move_amount;
        weight_offset += ledge_dir * move_amount * 0.3f;
        weight_offset.y += ik_shift.y;
        weight_offset.x += ik_shift.x*0.5f;
        weight_offset.z += ik_shift.z*0.5f;
        weight_offset -= ledge_dir * 0.1f;

        weight_offset *= pls.ik_mult;     
        weight_offset.y += pls.height_offset * (1.0f-pls.ik_mult);
        
        for(int i=0; i<4; i++){
            offset[i] -= weight_offset;
            if(length_squared(offset[i]) > _dist_limit_squared){
                offset[i] = normalize(offset[i])*_dist_limit;
            }
            offset[i] += weight_offset;
        }

        for(int i=0; i<4; ++i){  
            offset[i] *= pls.ik_mult;
            offset[i].y += pls.height_offset * (1.0f-pls.ik_mult);
        }

        body = mix(transition_offset,weight_offset,pls.transition_progress);
        left_hand = mix(transition_offset,offset[0],pls.transition_progress);
        right_hand = mix(transition_offset,offset[1],pls.transition_progress);
        left_foot = mix(transition_offset,offset[2],pls.transition_progress);
        right_foot = mix(transition_offset,offset[3],pls.transition_progress);
    }
}

class LedgeDirInfo {
    bool success;
    vec3 ledge_dir;
    bool downwards_surface;
};

vec3 closest_contact;

LedgeDirInfo GetPossibleLedgeDir(vec3 &in pos) {
    LedgeDirInfo ledge_dir_info;
    col.GetSlidingSphereCollision(pos, _leg_sphere_size*1.5f);    
    if(sphere_col.NumContacts() == 0){    
        ledge_dir_info.success = false;
        return ledge_dir_info;
    }
    float closest_dist = 0.0f;
    float dist;
    int closest_point = -1;
    float closest_horz_dist = 0.0f;
    int closest_horz_point = -1;
    int num_contacts = sphere_col.NumContacts();
    for(int i=0; i<num_contacts; ++i){
        dist = distance_squared(sphere_col.GetContact(i).position, pos);
        if(closest_point == -1 || dist < closest_dist){
            closest_dist = dist;
            closest_point = i;
        }
        if(closest_horz_point == -1 || dist < closest_horz_dist){
            if(normalize(sphere_col.GetContact(i).position - pos).y >= -0.7f){
                closest_horz_dist = dist;
                closest_horz_point = i;
            }
        }
    }
    if(normalize(sphere_col.GetContact(closest_point).position - pos).y < -0.7f){
        ledge_dir_info.downwards_surface = true;
    } else {
        ledge_dir_info.downwards_surface = false;
    }
    if(closest_horz_point == -1){
        ledge_dir_info.ledge_dir = sphere_col.GetContact(closest_point).position - pos;
    } else {
        ledge_dir_info.ledge_dir = sphere_col.GetContact(closest_horz_point).position - pos;
    }
    closest_contact = sphere_col.GetContact(closest_point).position;
    ledge_dir_info.ledge_dir = normalize(ledge_dir_info.ledge_dir);
    bool _debug_draw_sphere_check = false;
    if(_debug_draw_sphere_check){
        DebugDrawWireSphere(pos, _leg_sphere_size*1.5f, vec3(1.0f), _delete_on_update);
        DebugDrawLine(pos, pos+ledge_dir_info.ledge_dir, vec3(1.0f,0.0f,0.0f), _delete_on_update);
    }
    ledge_dir_info.success = true;
    return ledge_dir_info;
}

class LedgeHeightInfo {
    bool success;
    float edge_height;
};

int ledge_obj = -1;
int ledge_segment = -1;
float ledge_progress = 0.0;

LedgeHeightInfo GetLedgeHeightInfo(vec3&in pos, vec3&in ledge_dir) {
    LedgeHeightInfo ledge_height_info;

    float cyl_height = 1.0f;                                                    // Get ledge height by sweeping a cylinder downwards onto the ledge
    vec3 test_start = pos+vec3(0.0f,2.5f,0.0f)+ledge_dir * 0.5f;
    vec3 test_end = pos+vec3(0.0f,0.5f,0.0f)+ledge_dir * 0.5f;
    
    col.GetSweptCylinderCollision(test_start,
                                  test_end,
                                  _leg_sphere_size,
                                  1.0f);

    bool _debug_draw_sweep_test = false;
    if(_debug_draw_sweep_test){
        DebugDrawWireCylinder(test_start,
                              _leg_sphere_size,
                              1.0f,
                              vec3(1.0f,0.0f,0.0f),
                              _delete_on_update);
        DebugDrawWireCylinder(test_end,
                              _leg_sphere_size,
                              1.0f,
                              vec3(0.0f,1.0f,0.0f),
                              _delete_on_update);
        
        DebugDrawWireCylinder(sphere_col.position,
                              _leg_sphere_size,
                              1.0f,
                              vec3(0.0f,0.0f,1.0f),
                              _delete_on_update);
    }

    if(sphere_col.NumContacts() == 0){
        ledge_height_info.success = false;
        return ledge_height_info;                                               // Return if there is no ledge detected
    }


    ledge_height_info.edge_height = sphere_col.position.y - cyl_height * 0.5f;
    vec3 surface_pos = sphere_col.position;

    col.GetCylinderCollisionDoubleSided(surface_pos + vec3(0.0f,0.07f,0.0f),               // Make sure that top surface is clear, i.e. player could stand on it
                                _leg_sphere_size,
                                1.0f);


    bool _debug_draw_top_surface_clear = false;
    if(sphere_col.NumContacts() != 0){
        if(_debug_draw_top_surface_clear){
            DebugDrawWireCylinder(surface_pos + vec3(0.0f,0.07f,0.0f),
                                  _leg_sphere_size,
                                  1.0f,
                                  vec3(1.0f,0.0f,0.0f),
                                  _delete_on_update);
        }
        ledge_height_info.success = false;
        return ledge_height_info;                                               // Return if surface is obstructed
    }

    if(_debug_draw_top_surface_clear){
        DebugDrawWireCylinder(surface_pos + vec3(0.0f,0.07f,0.0f),
                              _leg_sphere_size,
                              1.0f,
                              vec3(0.0f,1.0f,0.0f),
                              _delete_on_update);
    }
    ledge_height_info.success = true;
    return ledge_height_info;
}

class LedgeGrabInfo {
    bool can_grab;
    bool scramble_grab;
    bool success;
};

LedgeGrabInfo GetLedgeGrabInfo(vec3 &in pos, vec3 &in ledge_dir, float ledge_height, float vert_vel) {
    LedgeGrabInfo ledge_grab_info;
    vec3 test_end = pos+ledge_dir;                                              // Use a swept cylinder to detect the distance
    test_end.y = ledge_height;                                                  // of the ledge from the player sphere origin
    vec3 test_start = test_end+ledge_dir*-1.0f;
    col.GetSweptCylinderCollision(test_start,
                                  test_end,
                                  _leg_sphere_size,
                                  1.0f);
    
    bool _debug_draw_depth_test = false;
    if(_debug_draw_depth_test){
        DebugDrawWireCylinder(sphere_col.position,
                              _leg_sphere_size,
                              1.0f,
                              vec3(0.0f,0.0f,1.0f),
                              _delete_on_update);
    }

    if(sphere_col.NumContacts() == 0){
        ledge_grab_info.success = false;
        return ledge_grab_info;                                                 // Return if swept cylinder detects no ledge
    }

    bool grabbable = false;
    int obj_id = -1;
    int tri_id;
    for(int i=0, len=sphere_col.NumContacts(); i<len; ++i){
        CollisionPoint contact = sphere_col.GetContact(i);
        int val = int(contact.custom_normal.y);
        if(val == 0 || val == 6){
            grabbable = true;
            obj_id = contact.id;
            tri_id = contact.tri;
        }
    }

    if(!grabbable){
        ledge_grab_info.success = false;
        return ledge_grab_info;           
    }

    const bool kUseLedgeLines = false;
    if(kUseLedgeLines){
        ledge_obj = -1;
        if(obj_id != -1 && ObjectExists(obj_id) && ReadObjectFromID(obj_id).GetType() == _env_object){
            EnvObject@ eo = ReadEnvObjectID(obj_id);
            Object@ obj = ReadObjectFromID(obj_id);
            int num_ledge_lines = eo.GetNumLedgeLines();
            DebugText("num_ledge_lines", "num_ledge_lines: "+num_ledge_lines, 0.1);
            float closest_dist = 0.0;
            int closest = -1;
            vec3 closest_point;
            float closest_progress = 0.0;
            for(int i=1; i<num_ledge_lines; i+=2){
                vec3 a = obj.GetTransform()*eo.GetCollisionVertex(eo.GetLedgeLine(i-1)/3);
                vec3 b = obj.GetTransform()*eo.GetCollisionVertex(eo.GetLedgeLine(i)/3);
                DebugDrawLine(a+vec3(0,1,0)*0.1,
                              b+vec3(0,1,0)*0.1, 
                              vec4(1.0), vec4(1.0), _delete_on_draw);
                vec3 point = ClosestPointOnSegment(this_mo.position, a, b);
                float dist = distance_squared(point, this_mo.position);
                if(closest == -1 || dist < closest_dist){
                    closest_dist = dist;
                    closest_point = point;
                    closest_progress = distance(a,point) / distance(a,b);
                    closest = i-1;
                }
            }
            if(closest != -1){
                ledge_obj = obj_id;
                ledge_segment = closest;
                ledge_progress = min(1.0, max(0.0, closest_progress));
                if(closest_dist > 1.5 * 1.5){
                    ledge_grab_info.success = false;
                    return ledge_grab_info;                          
                }
            }
        }
    }

    float char_height = pos.y;                                                  // Start checking if the ledge is within vertical
    const float _base_grab_height = 1.0f;                                       // reach of the player. Vertical reach is extended
    float grab_height = _base_grab_height;                                      // to allow one-armed scramble grabs if velocity is
    if(vert_vel < 1.5f){                                                        // below a threshold.
        if(vert_vel > 0.0f){
            grab_height += 1.5f;
        } else {
            grab_height += 0.5f;
        }
    }

    if(char_height > ledge_height - grab_height){                               // If ledge is within reach, grab it
        ledge_grab_info.can_grab = true;
        if(char_height < ledge_height - _base_grab_height){                     
            ledge_grab_info.scramble_grab = true;
        } else {
            ledge_grab_info.scramble_grab = false;
        }
    } else {
        ledge_grab_info.can_grab = false;        
    }
    ledge_grab_info.success = true;
    return ledge_grab_info;
}

class PassLedgeState {
    vec3 ledge_grab_origin;             // Where was leg sphere origin when ledge grab started
    float transition_speed;             // How fast to transition to new position
    float transition_progress;          // How far along the transition is (0-1)
    float leg_ik_mult;                  // How much we are allowing vertical leg IK displacement
    float ik_mult;
    float height_offset;
}

class LedgeInfo {
    bool on_ledge;                  // Grabbing ledge or not
    float ledge_height;             // Height of the ledge in world coords
    vec3 ledge_grab_pos;            // Where did we first grab the ledge
    vec3 ledge_dir;                 // Direction to the ledge from the character
    bool climbed_up;                // Did we just climb up something? Queried by main movement update
    bool playing_animation;         // Are we currently playing an animation?
    bool allow_ik;
    vec3 disp_ledge_dir;
    bool ghost_movement;
    PassLedgeState pls;

    ShimmyAnimation shimmy_anim;    // Hand and foot animation class

    LedgeInfo() {    
        on_ledge = false;
        climbed_up = false;
        playing_animation = false;
        allow_ik = true;
        ghost_movement = false;
    }

    void UpdateLedgeAnimation() {
        if(!playing_animation){     // If not playing a special animation, adopt ledge pose
            //this_mo.SetCharAnimation("ledge",5.0f);
            this_mo.SetAnimation("Data/Animations/r_ledge.xml",5.0f);
        }
    }

    vec3 WallRight() {              // Get the vector that points right when facing the ledge
        vec3 wall_right = ledge_dir;
        float temp = ledge_dir.x;
        wall_right.x = -ledge_dir.z;
        wall_right.z = temp;
        return wall_right;        
    }
    
    vec3 CalculateGrabPos() {       // Sweep a cylinder to get the closest position a cylinder can be to the ledge
        vec3 test_end = this_mo.position+ledge_dir*1.0f;
        vec3 test_start = test_end+ledge_dir*-2.0f;
        test_end.y = ledge_height;
        test_start.y = ledge_height;
        col.GetSweptCylinderCollision(test_start,
                                          test_end,
                                          _leg_sphere_size,
                                          1.0f);


        bool _debug_draw_sweep_test = false;
        if(_debug_draw_sweep_test){
            DebugDrawWireCylinder(test_start,
                                  _leg_sphere_size,
                                  1.0f,
                                  vec3(1.0f,0.0f,0.0f),
                                  _delete_on_update);

            DebugDrawWireCylinder(test_end,
                                  _leg_sphere_size,
                                  1.0f,
                                  vec3(0.0f,1.0f,0.0f),
                                  _delete_on_update);

            DebugDrawWireCylinder(sphere_col.position,
                                  _leg_sphere_size,
                                  1.0f,
                                  vec3(0.0f,0.0f,1.0f),
                                  _delete_on_update);
        }

        return sphere_col.position - vec3(0.0f, _height_under_ledge, 0.0f);
    }

    bool CheckLedges(bool pure_check = false) {
        // Find the direction of the nearest ledge candidate
        vec3 possible_ledge_dir;
        if(on_ledge){
            // Check at hand level if on ledge already for smooth shimmying around corners
            vec3 test_pos = vec3(this_mo.position.x, ledge_height, this_mo.position.z);
            LedgeDirInfo ledge_dir_info = GetPossibleLedgeDir(test_pos);
            if(ledge_dir_info.success){
                possible_ledge_dir = ledge_dir_info.ledge_dir;
            } else {
                if(!pure_check){
                    on_ledge = false;
                }
                return false;
            }
        } else {
            // Check at various levels otherwise to catch high bars and low ledges
            bool success = false;
            int counter = 0;
            while(!success && counter < 3){
                vec3 test_pos = this_mo.position + vec3(0.0f, counter * 0.3f, 0.0f);
                LedgeDirInfo ledge_dir_info = GetPossibleLedgeDir(test_pos);
                possible_ledge_dir = ledge_dir_info.ledge_dir;
                if(ledge_dir_info.success){
                    success = true;
                }    
                ++counter;
            }
            if(!success){
                if(!pure_check){
                    on_ledge = false;
                }
                return false;
            }
        }

        if(GetConfigValueBool("auto_ledge_grab") && this_mo.velocity.y <= 0.0 && this_mo.controlled && dot(possible_ledge_dir, GetTargetVelocity()) <= 0) {
            return false;
        }
    
        // Get the height of the candidate ledge        
        possible_ledge_dir.y = 0.0f;
        possible_ledge_dir = normalize(possible_ledge_dir);
        LedgeHeightInfo ledge_height_info = GetLedgeHeightInfo(this_mo.position, possible_ledge_dir);
        if(ledge_height_info.success == false){
            if(!on_ledge){
                // If failed, check for the height of the second nearest candidate ledge
                bool success = false;
                int counter = 0;
                vec3 old_possible_ledge_dir = possible_ledge_dir;
                while(!success && counter < 3){
                    vec3 test_pos = this_mo.position + vec3(0.0f, counter * 0.3f, 0.0f);
                    test_pos -= possible_ledge_dir * 0.1f;
                    LedgeDirInfo ledge_dir_info = GetPossibleLedgeDir(test_pos);
                    possible_ledge_dir = ledge_dir_info.ledge_dir;
                    if(ledge_dir_info.success){
                        success = true;
                    }    
                    ++counter;
                }
                if(success){
                    possible_ledge_dir.y = 0.0f;
                    possible_ledge_dir = normalize(possible_ledge_dir);
                    ledge_height_info = GetLedgeHeightInfo(this_mo.position - old_possible_ledge_dir*0.2, possible_ledge_dir);
                    if(ledge_height_info.success == false) {
                        if(!pure_check){
                            if(on_ledge){
                                //Log(warning,"Letting go because of GetLedgeHeightInfo\n");
                            }
                            on_ledge = false;
                        }
                        return false;
                    } else {
                        this_mo.position -= old_possible_ledge_dir * 0.1f;
                    }
                }
            } else {
                if(!pure_check){
                    if(on_ledge){
                        //Log(warning,"Letting go because of GetLedgeHeightInfo\n");
                    }
                    on_ledge = false;
                }
                return false;
            }
        }
        
        // Check if we are trying to transition to a much higher ledge
        if(on_ledge && ledge_height_info.edge_height > ledge_height + 0.5f){
            if(!pure_check){
                if(on_ledge){
                    //Log(warning, "Letting go because of edge_height\n");
                }
                on_ledge = false;
            }
            return false;
        } else {
            if(!pure_check){
                ledge_height = ledge_height_info.edge_height;
            }
        }

        // Check if we are going to penetrate the ground
        {
            vec3 test_pos = vec3(this_mo.position.x, ledge_height, this_mo.position.z);
            test_pos -= possible_ledge_dir * 0.1f;
            col.GetSweptSphereCollision(test_pos,
                                        test_pos + vec3(0.0f, -0.5f, 0.0f),
                                        _leg_sphere_size);
            if(sphere_col.NumContacts() > 0) {
                if(!pure_check && on_ledge){
                    //Log(warning, "Letting go because nearest surface is downwards\n");
                    on_ledge = false;
                }
                return false;
            }
        }

        ledge_dir = possible_ledge_dir;

        // The remainder of this function is just for grabbing onto new ledges
        if(on_ledge){
            return false;                                                             // The rest of this function is only useful for
        }                                                                       // determining whether or not we can grab the ledge

        LedgeGrabInfo ledge_grab_info = 
            GetLedgeGrabInfo(this_mo.position, ledge_dir, ledge_height_info.edge_height, this_mo.velocity.y);        

        if(ledge_grab_info.success == false){
            return false;
        }

        /*DebugDrawWireCylinder(vec3(this_mo.position.x, ledge_height_info.edge_height + 0.5f, this_mo.position.z) + possible_ledge_dir * _leg_sphere_size,
                              _leg_sphere_size,
                              1.0f,
                              vec3(1.0f,1.0f,1.0f),
                              _delete_on_update);*/

        if(pure_check){
            return true;
        }

        // Set last_update far away so we don't use it
        last_update = this_mo.position + vec3(99,99,99);

        if(!pure_check && ledge_grab_info.can_grab){                            // If ledge is within reach, grab it
            if(ledge_grab_info.scramble_grab){                                  // If height difference requires the scramble grab, then
                playing_animation = true;                                       // play the animation
                int flags = _ANM_FROM_START;
                if(rand()%2 == 0){
                    flags = _ANM_MIRRORED;
                }
                this_mo.SetAnimation("Data/Animations/r_ledge_barely_reach.anm",8.0f,flags);
                this_mo.rigged_object().anim_client().SetAnimationCallback("void EndClimbAnim()");
                pls.leg_ik_mult = 0.0f;
            } else {
                playing_animation = false;                                      // Otherwise go straight into ledge pose
                pls.leg_ik_mult = 1.0f;
            }
            pls.ik_mult = 1.0f;

            pls.ledge_grab_origin = this_mo.position;                               // Record current position for smooth transition
            pls.transition_progress = 0.0f;
            pls.transition_speed = 10.0f/(abs(ledge_height - this_mo.position.y)+0.05f);
            
            allow_ik = true;
            on_ledge = true;                                                    // Set up ledge grab starting conditions
            pls.height_offset = 0.0f;
            disp_ledge_dir = this_mo.GetFacing();
            climbed_up = false;

            col.GetSlidingSphereCollision(ledge_grab_pos, _leg_sphere_size*1.2);
            float impact = dot(this_mo.velocity, ledge_dir);
            ImpactSound(impact, closest_contact);

            this_mo.velocity = vec3(0.0f);
            //ledge_grab_pos = CalculateGrabPos();            
            ledge_grab_pos = vec3(this_mo.position.x, ledge_height - _height_under_ledge, this_mo.position.z);
            shimmy_anim.Start(ledge_grab_pos, possible_ledge_dir, this_mo.velocity);
            ghost_movement = false;


            this_mo.MaterialEvent("edge_grab", this_mo.position + ledge_dir * _leg_sphere_size);
			AchievementEvent("edge_grab");
            HandleAIEvent(_grabbed_ledge);
        }

        return true;
    }

    void EndClimbAnim(){
        if(ghost_movement){
            on_ledge = false;    
            climbed_up = true;
			AchievementEvent("climbed_up");
            jump_info.hit_wall = false;
            this_mo.position += ledge_dir*0.13;//vec3(0.0f);
        }
        playing_animation = false; 
        allow_ik = true;
        ghost_movement = false;
    }

    vec3 last_update;
    
    void UpdateLedge(const Timestep &in ts) {
        if(playing_animation){
            if(this_mo.rigged_object().GetStatusKeyValue("cancel")>=1.0f && 
               WantsToCancelAnimation())
            {
                EndClimbAnim();
            }
        }
        if(playing_animation){
            this_mo.position.y += pls.height_offset * 0.02f * ts.frames();
            pls.height_offset *= pow(0.98f, ts.frames());
            //allow_ik = true;
            target_duck_amount = 1.0f;
        }

        if(allow_ik){
            pls.ik_mult = min(1.0f, pls.ik_mult + ts.step() * 5.0f);
        } else {
            pls.ik_mult = max(0.0f, pls.ik_mult - ts.step() * 5.0f);
        }

        if(pls.transition_progress < 1.0f){
            pls.transition_progress += ts.step() * pls.transition_speed;   // Update transition to ledge grab
            pls.transition_progress = min(1.0f, pls.transition_progress); 
            cam_pos_offset = (pls.ledge_grab_origin - this_mo.position)*(1.0f-pls.transition_progress);
        }

        if(ghost_movement){
            this_mo.velocity = vec3(0.0f,0.1f,0.0f);
            /*DebugDrawWireSphere(this_mo.position,
                                _leg_sphere_size,
                                vec3(1.0f),
                                _delete_on_update);*/
            return;
        }

        /*if(on_ledge && weapon_slots[primary_weapon_slot] != -1){
            ItemObject@ item_obj = ReadItemID(weapon_slots[primary_weapon_slot]);
            if(item_obj.GetMass() > 1.0f){
                DropWeapon();
            }
        }*/
        if(on_ledge && ledge_obj != -1){
            EnvObject@ eo = ReadEnvObjectID(ledge_obj);
            Object@ obj = ReadObjectFromID(ledge_obj);
            int num_ledge_lines = eo.GetNumLedgeLines();
            DebugText("num_ledge_lines", "num_ledge_lines: "+num_ledge_lines, 0.1);
            
            vec3 pos;
            vec3 perp_vec;
            
            {
                vec3 a = obj.GetTransform()*eo.GetCollisionVertex(eo.GetLedgeLine(ledge_segment)/3);
                vec3 b = obj.GetTransform()*eo.GetCollisionVertex(eo.GetLedgeLine(ledge_segment+1)/3);
                perp_vec = normalize(cross(vec3(0,1,0), b-a));
                pos = mix(a,b,ledge_progress);
                pos += this_mo.velocity * ts.step();
            }

            int obj_id = ledge_obj;
            ledge_obj= -1;
            if(obj_id != -1 && ObjectExists(obj_id) && ReadObjectFromID(obj_id).GetType() == _env_object){
                DebugText("num_ledge_lines", "num_ledge_lines: "+num_ledge_lines, 0.1);
                float closest_dist = 0.0;
                int closest = -1;
                vec3 closest_point;
                float closest_progress = 0.0;
                for(int i=1; i<num_ledge_lines; i+=2){
                    vec3 a = obj.GetTransform()*eo.GetCollisionVertex(eo.GetLedgeLine(i-1)/3);
                    vec3 b = obj.GetTransform()*eo.GetCollisionVertex(eo.GetLedgeLine(i)/3);
                    DebugDrawLine(a+vec3(0,1,0)*0.1,
                                  b+vec3(0,1,0)*0.1, 
                                  vec4(1.0), vec4(1.0), _delete_on_draw);
                    vec3 point = ClosestPointOnSegment(pos, a, b);
                    float dist = distance_squared(point, pos);
                    if(closest == -1 || dist < closest_dist){
                        closest_dist = dist;
                        closest_point = point;
                        closest_progress = distance(a,point) / distance(a,b);
                        closest = i-1;
                    }
                }
                if(closest != -1){
                    ledge_obj = obj_id;
                    ledge_segment = closest;
                    ledge_progress = min(1.0, max(0.0, closest_progress));
                }
            }

            float old_height = this_mo.position.y;
            this_mo.position = pos + vec3(0,-1,0) + perp_vec * 0.5;
            this_mo.position.y = old_height;
            ledge_dir = perp_vec * -1.0;
            ledge_height = pos[1];
            
            if(!WantsToGrabLedge()){
                on_ledge = false;    // If let go or not in contact with wall, 
                                     // not on ledge
            }

            //this_mo.velocity += ledge_dir * 0.1f * ts.frames();                      // Pull towards wall

            float target_height = ledge_height - _height_under_ledge;
            this_mo.velocity.y += (target_height - this_mo.position.y) * 0.8f * ts.frames();      // Move height towards ledge height
            this_mo.velocity.y *= pow(0.92f, ts.frames());
            
            this_mo.position.y = min(this_mo.position.y, target_height + 0.5f);
            this_mo.position.y = max(this_mo.position.y, target_height - 0.1f);
            
            if(!playing_animation){
                pls.leg_ik_mult = min(1.0f, pls.leg_ik_mult + ts.step() * 5.0f);
            }

            vec3 target_velocity = GetTargetVelocity();
            float ledge_dir_dot = dot(target_velocity, ledge_dir);
            vec3 horz_vel = target_velocity - (ledge_dir * ledge_dir_dot);
            vec3 real_velocity = horz_vel;
            if(ledge_dir_dot > 0.0f){                                               // Climb up if moving towards ledge
                real_velocity.y += ledge_dir_dot * time_step * _ledge_move_speed * 70.0f;
            }    

            if(playing_animation){
                real_velocity.y = 0.0f;
            }

            this_mo.velocity += real_velocity * ts.step() * _ledge_move_speed; // Apply target velocity
            this_mo.velocity.x *= pow(_ledge_move_damping, ts.frames());             // Damp horizontal movement
            this_mo.velocity.z *= pow(_ledge_move_damping, ts.frames());

            //vec3 new_ledge_grab_pos = CalculateGrabPos();         
            vec3 new_ledge_grab_pos = vec3(this_mo.position.x, target_height, this_mo.position.z);
            shimmy_anim.Update(new_ledge_grab_pos, ledge_dir, ts);                      // Update hand and foot animation

            //DebugDrawWireSphere(this_mo.position, _leg_sphere_size, vec3(1.0f), _delete_on_update); 

            /*if(dot(disp_ledge_dir, ledge_dir) > 0.90f){
                disp_ledge_dir = ledge_dir;
            }*/
            float val = dot(ledge_dir, disp_ledge_dir)*0.5f+0.5f;
            float inertia = mix(0.95f,0.8f,pow(val,4.0));
            disp_ledge_dir= InterpDirections(ledge_dir,
                             disp_ledge_dir,
                             pow(inertia,ts.frames()));
            this_mo.SetRotationFromFacing(disp_ledge_dir); 
            if((this_mo.velocity.y >= 0.0f && this_mo.position.y > target_height + 0.4f) || !this_mo.controlled){ // Climb up ledge if above threshold
                this_mo.SetRotationFromFacing(ledge_dir); 
                if(this_mo.velocity.y >= 3.0f + ts.frames() * 0.25f){
                    on_ledge = false;    
                    climbed_up = true;
                    jump_info.hit_wall = false;
                    this_mo.velocity = vec3(0.0f);
                    this_mo.position.y = ledge_height + _leg_sphere_size * 0.7f;
                    this_mo.position += ledge_dir * 0.7f;
                } else {
                    pls.height_offset = target_height - this_mo.position.y;
                    //this_mo.position.y = target_height;
                    playing_animation = true;
                    allow_ik = false;
                    int flags = _ANM_SUPER_MOBILE | _ANM_FROM_START;
                    this_mo.SetAnimation("Data/Animations/r_ledge_climb_fast.anm",8.0f,flags);
                    this_mo.rigged_object().anim_client().SetAnimationCallback("void EndClimbAnim()");
                    ghost_movement = true;
                }
                HandleAIEvent(_climbed_up);
            }
            return;
        }

        CheckLedges();
        if(on_ledge){
            last_update = this_mo.position;
        } else {
            last_update.y = this_mo.position.y;
            if(distance_squared(last_update, this_mo.position) < 0.1f){
                this_mo.velocity += (last_update - this_mo.position) / ts.step();
                this_mo.position = last_update;
            }
            on_ledge = true;
        }
        /*DebugDrawWireSphere(this_mo.position,
                            _leg_sphere_size,
                            vec3(1.0f),
                            _delete_on_update);*/

        if(!WantsToGrabLedge()){
            on_ledge = false;    // If let go or not in contact with wall, 
                                 // not on ledge
        }

        this_mo.velocity += ledge_dir * 0.1f * ts.frames();                      // Pull towards wall

        float target_height = ledge_height - _height_under_ledge;
        this_mo.velocity.y += (target_height - this_mo.position.y) * 0.8f * ts.frames();      // Move height towards ledge height
        this_mo.velocity.y *= pow(0.92f, ts.frames());
        
        this_mo.position.y = min(this_mo.position.y, target_height + 0.5f);
        this_mo.position.y = max(this_mo.position.y, target_height - 0.1f);
        
        if(!playing_animation){
            pls.leg_ik_mult = min(1.0f, pls.leg_ik_mult + ts.step() * 5.0f);
        }

        vec3 target_velocity = GetTargetVelocity();
        if(WantsToJump()){
            target_velocity = ledge_dir;
        }
        float ledge_dir_dot = dot(target_velocity, ledge_dir);
        vec3 horz_vel = target_velocity - (ledge_dir * ledge_dir_dot);
        vec3 real_velocity = horz_vel;
        if(ledge_dir_dot > 0.0f){                                               // Climb up if moving towards ledge
            real_velocity.y += ledge_dir_dot * time_step * _ledge_move_speed * 70.0f;
        }    

        if(playing_animation){
            real_velocity.y = 0.0f;
        }

        this_mo.velocity += real_velocity * ts.step() * _ledge_move_speed; // Apply target velocity
        this_mo.velocity.x *= pow(_ledge_move_damping, ts.frames());             // Damp horizontal movement
        this_mo.velocity.z *= pow(_ledge_move_damping, ts.frames());

        //vec3 new_ledge_grab_pos = CalculateGrabPos();         
        vec3 new_ledge_grab_pos = vec3(this_mo.position.x, target_height, this_mo.position.z);
        shimmy_anim.Update(new_ledge_grab_pos, ledge_dir, ts);                      // Update hand and foot animation

        //DebugDrawWireSphere(this_mo.position, _leg_sphere_size, vec3(1.0f), _delete_on_update); 

        /*if(dot(disp_ledge_dir, ledge_dir) > 0.90f){
            disp_ledge_dir = ledge_dir;
        }*/
        float val = dot(ledge_dir, disp_ledge_dir)*0.5f+0.5f;
        float inertia = mix(0.95f,0.8f,pow(val,4.0));
        disp_ledge_dir= InterpDirections(ledge_dir,
                         disp_ledge_dir,
                         pow(inertia,ts.frames()));
        this_mo.SetRotationFromFacing(disp_ledge_dir); 
        if((this_mo.velocity.y >= 0.0f && this_mo.position.y > target_height + 0.4f) || !this_mo.controlled){ // Climb up ledge if above threshold
            this_mo.SetRotationFromFacing(ledge_dir); 
            if(this_mo.velocity.y >= 3.0f + ts.frames() * 0.25f){
                on_ledge = false;    
                climbed_up = true;
                jump_info.hit_wall = false;
                this_mo.velocity = vec3(0.0f);
                this_mo.position.y = ledge_height + _leg_sphere_size * 0.7f;
                this_mo.position += ledge_dir * 0.7f;
            } else {
                pls.height_offset = target_height - this_mo.position.y;
                //this_mo.position.y = target_height;
                playing_animation = true;
                allow_ik = false;
                int flags = _ANM_SUPER_MOBILE | _ANM_FROM_START;
                this_mo.SetAnimation("Data/Animations/r_ledge_climb_fast.anm",8.0f,flags);
                this_mo.rigged_object().anim_client().SetAnimationCallback("void EndClimbAnim()");
                ghost_movement = true;
            }
            HandleAIEvent(_climbed_up);
        }
    }
};

void EndClimbAnim(){
    ledge_info.EndClimbAnim();
}
