//-----------------------------------------------------------------------------
//           Name: placecollision.as
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

class Plane {
    vec3 normal;
    float plane_d;
};

Plane GetPlane(const CollisionPoint cp) {
    Plane plane;
    plane.normal = cp.normal;
    plane.plane_d = dot(plane.normal,cp.position);
    return plane;
}

class Line {
    vec3 point;
    vec3 dir;
}

vec3 GetTriPlaneIntersection(Plane a, Plane b, Plane c){
    vec3 point;
    point.x = -(c.plane_d * a.normal.z * b.normal.y - 
                     c.plane_d * a.normal.y * b.normal.z - 
                     b.plane_d * a.normal.z * c.normal.y + 
                     a.plane_d * b.normal.z * c.normal.y + 
                     b.plane_d * a.normal.y * c.normal.z - 
                     a.plane_d * b.normal.y * c.normal.z) /
                   (-a.normal.z * b.normal.y * c.normal.x +
                     a.normal.y * b.normal.z * c.normal.x + 
                     a.normal.z * b.normal.x * c.normal.y - 
                     a.normal.x * b.normal.z * c.normal.y - 
                     a.normal.y * b.normal.x * c.normal.z + 
                     a.normal.x * b.normal.y * c.normal.z);
    point.y = -(-c.plane_d * a.normal.z * b.normal.x + 
                     c.plane_d * a.normal.x * b.normal.z + 
                     b.plane_d * a.normal.z * c.normal.x - 
                     a.plane_d * b.normal.z * c.normal.x - 
                     b.plane_d * a.normal.x * c.normal.z + 
                     a.plane_d * b.normal.x * c.normal.z) /
                   (-a.normal.z * b.normal.y * c.normal.x +
                     a.normal.y * b.normal.z * c.normal.x + 
                     a.normal.z * b.normal.x * c.normal.y - 
                     a.normal.x * b.normal.z * c.normal.y - 
                     a.normal.y * b.normal.x * c.normal.z + 
                     a.normal.x * b.normal.y * c.normal.z);
    point.z = -(-c.plane_d * a.normal.y * b.normal.x + 
                     c.plane_d * a.normal.x * b.normal.y + 
                     b.plane_d * a.normal.y * c.normal.x - 
                     a.plane_d * b.normal.y * c.normal.x - 
                     b.plane_d * a.normal.x * c.normal.y + 
                     a.plane_d * b.normal.x * c.normal.y) /
                    (a.normal.z * b.normal.y * c.normal.x - 
                     a.normal.y * b.normal.z * c.normal.x - 
                     a.normal.z * b.normal.x * c.normal.y + 
                     a.normal.x * b.normal.z * c.normal.y + 
                     a.normal.y * b.normal.x * c.normal.z - 
                     a.normal.x * b.normal.y * c.normal.z);
    return point;
}

Line GetPlaneIntersection(Plane a, Plane b, vec3 point){
    Line line;
    line.dir = vec3(0.0f);
    line.point = point;
    if(dot(a.normal,b.normal)>0.99f){
        return line;
    }

    line.dir = normalize(cross(a.normal, b.normal));
    Plane c;
    c.normal = line.dir;
    c.plane_d = dot(c.normal, point);

    line.point = GetTriPlaneIntersection(a, b, c);

    return line;
}

void DrawPlane(Plane plane, vec3 point){
    vec3 perp1 = normalize(cross(vec3(0.0f,1.0f,0.0f), plane.normal))*0.5f;
    vec3 perp2 = normalize(cross(perp1, plane.normal))*0.5f;
    for(int i=-5; i<6; i++){
        DebugDrawLine(point + perp1 * i - perp2 * 5,
                      point + perp1 * i + perp2 * 5,
                      vec3(0.0f,1.0f,0.0f),
                      _delete_on_update);
    }
    for(int i=-5; i<6; i++){
        DebugDrawLine(point + perp2 * i - perp1 * 5,
                      point + perp2 * i + perp1 * 5,
                      vec3(0.0f,1.0f,0.0f),
                      _delete_on_update);
    }
}

float square(float val) {
    return val*val;
}

vec3 ApplyScaledSphereSlide(vec3 pos,
                            float radius,
                            vec3 scale)
{
    int num_contacts = sphere_col.NumContacts();
    if (num_contacts == 0) {
        return pos;
    }
    
    vec3 btpos = pos;
    int deepest = -1;
    float biggest_depth;
    Plane deepest_plane;
    for(int i=0; i<num_contacts; ++i){
        Plane plane = GetPlane(sphere_col.GetContact(i));
        float obj_d = dot(plane.normal, btpos);
        float scaled_radius = radius * sqrt(square(scale.x*plane.normal.x)+
                                            square(scale.y*plane.normal.y)+
                                            square(scale.z*plane.normal.z));
        float depth = plane.plane_d - obj_d + scaled_radius;
        if(depth < 0.0001f){
            continue;
        }
        if(deepest == -1 || depth > biggest_depth){
            deepest = i;
            biggest_depth = depth;
            deepest_plane = plane;
        }
    }
    if(deepest != -1){
        Plane plane = deepest_plane;
        float obj_d = dot(plane.normal, btpos);
        float scaled_radius = radius * sqrt(square(scale.x*plane.normal.x)+
                                            square(scale.y*plane.normal.y)+
                                            square(scale.z*plane.normal.z));
        float depth = plane.plane_d - obj_d + scaled_radius;
        btpos += plane.normal * depth;
        DrawPlane(deepest_plane, sphere_col.GetContact(deepest).position);
    }

    this_mo.GetSlidingScaledSphereCollision(btpos,
                                      1.0f,
                                      scale);
    num_contacts = sphere_col.NumContacts();
    if (num_contacts == 0) {
        return btpos;
    }

    int second_deepest = -1;
    Plane second_deepest_plane;
    for(int i=0; i<num_contacts; ++i){
        Plane plane = GetPlane(sphere_col.GetContact(i));
        float obj_d = dot(plane.normal, btpos);
        float scaled_radius = radius * sqrt(square(scale.x*plane.normal.x)+
                                            square(scale.y*plane.normal.y)+
                                            square(scale.z*plane.normal.z));
        float depth = plane.plane_d - obj_d + scaled_radius;
        if(depth < 0.00001f){
            continue;
        }
        if(second_deepest == -1 || depth > biggest_depth){
            second_deepest = i;
            biggest_depth = depth;
            second_deepest_plane = plane;
        }
    }
    if(second_deepest == -1){
        return btpos;
    }


    {
        float scaled_radius = radius * sqrt(square(scale.x*deepest_plane.normal.x)+
                                        square(scale.y*deepest_plane.normal.y)+
                                        square(scale.z*deepest_plane.normal.z));
        deepest_plane.plane_d += scaled_radius;
    }
    {
        float scaled_radius = radius * sqrt(square(scale.x*second_deepest_plane.normal.x)+
                                        square(scale.y*second_deepest_plane.normal.y)+
                                        square(scale.z*second_deepest_plane.normal.z));
        second_deepest_plane.plane_d += radius;
    }
    Line line = GetPlaneIntersection(deepest_plane,
                                     second_deepest_plane,
                                     btpos);
    btpos = line.point;
    DrawPlane(second_deepest_plane, sphere_col.GetContact(second_deepest).position);

    this_mo.GetSlidingScaledSphereCollision(btpos,
                                      1.0f,
                                      scale);
    num_contacts = sphere_col.NumContacts();
    if (num_contacts == 0) {
        return btpos;
    }

    int third_deepest = -1;
    Plane third_deepest_plane;
    for(int i=0; i<num_contacts; ++i){
        Plane plane = GetPlane(sphere_col.GetContact(i));
        float obj_d = dot(plane.normal, btpos);
        float scaled_radius = radius * sqrt(square(scale.x*plane.normal.x)+
                                            square(scale.y*plane.normal.y)+
                                            square(scale.z*plane.normal.z));
        float depth = plane.plane_d - obj_d + scaled_radius;
        if(depth < 0.00001f){
            continue;
        }
        if(third_deepest == -1 || depth > biggest_depth){
            third_deepest = i;
            biggest_depth = depth;
            third_deepest_plane = plane;
        }
    }
    if(third_deepest == -1){
        return btpos;
    }

    if(dot(third_deepest_plane.normal, second_deepest_plane.normal)>0.99f ||
       dot(third_deepest_plane.normal, deepest_plane.normal)>0.99f){
        return btpos;    
    }

    vec3 old_pos = btpos;
    {
        float scaled_radius = radius * sqrt(square(scale.x*third_deepest_plane.normal.x)+
                                            square(scale.y*third_deepest_plane.normal.y)+
                                            square(scale.z*third_deepest_plane.normal.z));
        third_deepest_plane.plane_d += scaled_radius;
    }
    btpos = GetTriPlaneIntersection(deepest_plane,
                                    second_deepest_plane,
                                    third_deepest_plane);
    if(distance_squared(old_pos, btpos) > radius * radius){
        btpos = normalize(btpos - old_pos)*radius + old_pos;
    }

    DrawPlane(third_deepest_plane, sphere_col.GetContact(third_deepest).position);
    return btpos;
}