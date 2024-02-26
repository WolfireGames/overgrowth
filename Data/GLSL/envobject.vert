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
#version 150
#extension GL_ARB_shading_language_420pack : enable

#include "lighting150.glsl"

in vec3 vertex_attrib;
in vec2 tex_coord_attrib;

#if defined(PARTICLE)
    #ifndef INSTANCED
        in vec3 normal_attrib;
        in vec3 tangent_attrib;
    #endif
#elif defined(DETAIL_OBJECT)
    in vec3 tangent_attrib;
    in vec3 bitangent_attrib;
    in vec3 normal_attrib;
#elif defined(TERRAIN)
    in vec3 tangent_attrib;
    in vec2 detail_tex_coord;
#elif defined(ITEM)
    in vec3 normal_attrib;
#elif defined(SKY)
#elif defined(CHARACTER)
    #ifdef TANGENT
        in vec3 tangent_attrib;
        in vec3 bitangent_attrib;
        in vec3 normal_attrib;
    #endif

    in vec2 morph_tex_offset_attrib;
    in vec2 fur_tex_coord_attrib;

    #if defined(GPU_SKINNING)

        in vec4 bone_weights;
        in vec4 bone_ids;
        in vec3 morph_offsets;

    #else  // GPU_SKINNING

        in vec4 transform_mat_column_a;
        in vec4 transform_mat_column_b;
        in vec4 transform_mat_column_c;

    #endif  // GPU_SKINNING

    in vec3 vel_attrib;
#else // static object

    in vec3 model_translation_attrib;  // set per-instance. separate from rest because it's not needed in the fragment shader, so is not slow on low-end GPUs

    #if defined(ATTRIB_ENVOBJ_INSTANCING)
        in vec3 model_scale_attrib;
        in vec4 model_rotation_quat_attrib;
        in vec4 color_tint_attrib;
        in vec4 detail_scale_attrib;
    #endif

    #ifdef TANGENT
        in vec3 tangent_attrib;
        in vec3 bitangent_attrib;
        in vec3 normal_attrib;
    #endif

    #ifdef PLANT
        in vec3 plant_stability_attrib;
    #endif
#endif

#if defined(GPU_PARTICLE_FIELD)
    uniform mat4 mvp;
    uniform mat4 projection_matrix;
    uniform mat4 view_matrix;
    uniform vec3 cam_pos;
    uniform float time;
    uniform vec2 viewport_dims;
    uniform vec3 cam_dir;
    uniform float time_scale;
#elif defined(PARTICLE)
    uniform mat4 mvp;
    const int kMaxInstances = 100;

    #ifdef INSTANCED
        uniform InstanceInfo {
            vec4 instance_color[kMaxInstances];
            mat4 instance_transform[kMaxInstances];
        };
    #endif
#elif defined(DETAIL_OBJECT)
    uniform vec3 cam_pos;
    uniform vec3 ws_light;
    uniform float time;

    #if defined(UBO_BATCH_SIZE_8X)
        const int kMaxInstances = 200 * 8;
    #elif defined(UBO_BATCH_SIZE_4X)
        const int kMaxInstances = 200 * 4;
    #elif defined(UBO_BATCH_SIZE_2X)
        const int kMaxInstances = 200 * 2;
    #else
        const int kMaxInstances = 200 * 1;  // 200 * 4 * 4 * 5 < 16384 , which is lowest possible support uniform block size
    #endif

    uniform InstanceInfo {
        mat4 transforms[kMaxInstances];
        vec4 texcoords2[kMaxInstances];
    };

    uniform float height;
    uniform float max_distance;
    uniform mat4 shadow_matrix[4];
    uniform mat4 projection_view_mat;
#elif defined(ITEM)
    uniform mat4 projection_view_mat;
    uniform mat4 model_mat;
    uniform mat4 old_vel_mat;
    uniform mat4 new_vel_mat;
    uniform vec3 cam_pos;
    uniform mat4 shadow_matrix[4];
#elif defined(TERRAIN)
    uniform vec3 cam_pos;
    uniform mat4 projection_view_mat;
    uniform vec3 ws_light;
    uniform mat4 shadow_matrix[4];
#elif defined(CHARACTER)
    #ifndef DEPTH_ONLY
        uniform vec3 cam_pos;
        uniform mat4 shadow_matrix[4];
    #endif

    uniform mat4 mvp;

    #if defined(GPU_SKINNING)
        const int kMaxBones = 200;
        uniform mat4 bone_mats[kMaxBones];
    #endif  // GPU_SKINNING
#elif defined(SKY)
    uniform mat4 projection_view_mat;
    uniform vec3 cam_pos;
#else // static object

    #if !defined(ATTRIB_ENVOBJ_INSTANCING)
        #if defined(UBO_BATCH_SIZE_8X)
            const int kMaxInstances = 256 * 8;
        #elif defined(UBO_BATCH_SIZE_4X)
            const int kMaxInstances = 256 * 4;
        #elif defined(UBO_BATCH_SIZE_2X)
            const int kMaxInstances = 256 * 2;
        #else
            const int kMaxInstances = 256 * 1;
        #endif

        struct Instance {
            vec3 model_scale;
            vec4 model_rotation_quat;
            vec4 color_tint;
            vec4 detail_scale;  // TODO: DETAILMAP4 only?
        };

        uniform InstanceInfo {
            Instance instances[kMaxInstances];
        };
    #endif

    vec3 GetInstancedModelScale(int instance_id) {
        #if defined(ATTRIB_ENVOBJ_INSTANCING)
            return model_scale_attrib;
        #else
            return instances[instance_id].model_scale;
        #endif
    }

    vec4 GetInstancedModelRotationQuat(int instance_id) {
        #if defined(ATTRIB_ENVOBJ_INSTANCING)
            return model_rotation_quat_attrib;
        #else
            return instances[instance_id].model_rotation_quat;
        #endif
    }

    vec4 GetInstancedColorTint(int instance_id) {
        #if defined(ATTRIB_ENVOBJ_INSTANCING)
            return color_tint_attrib;
        #else
            return instances[instance_id].color_tint;
        #endif
    }

    #if defined(DETAILMAP4)
        vec4 GetInstancedDetailScale(int instance_id) {
            #if defined(ATTRIB_ENVOBJ_INSTANCING)
                return detail_scale_attrib;
            #else
                return instances[instance_id].detail_scale;
            #endif
        }
    #endif

    uniform mat4 projection_view_mat;
    uniform vec3 cam_pos;
    uniform mat4 shadow_matrix[4];
    uniform float time;
#endif


#ifdef SHADOW_CASCADE
    //#define frag_tex_coords geom_tex_coords
    //#define world_vert      geom_world_vert

    #ifdef CHARACTER
        //#define fur_tex_coord   geom_fur_tex_coord
    #endif  // CHARACTER

#endif  // SHADOW_CASCADE


out vec3 world_vert;

#if defined(PARTICLE)
    out vec2 tex_coord;

    #if defined(NORMAL_MAP_TRANSLUCENT) || defined(WATER) || defined(SPLAT)
        out vec3 tangent_to_world1;
        out vec3 tangent_to_world2;
        out vec3 tangent_to_world3;
    #endif

    flat out int instance_id;
#elif defined(GPU_PARTICLE_FIELD)
    out vec2 tex_coord;
    out float alpha;
    flat out int instance_id;
#elif defined(DETAIL_OBJECT)
    out vec2 frag_tex_coords;
    out vec2 base_tex_coord;
    out mat3 tangent_to_world;

    #define TERRAIN_LIGHT_OFFSET vec2(0.0);//vec2(0.0005)+ws_light.xz*0.0005
#elif defined(ITEM)
    #ifndef DEPTH_ONLY
        #ifndef NO_VELOCITY_BUF
            out vec3 vel;
        #endif

        #ifdef TANGENT
            out vec3 frag_normal;
        #endif
    #endif

    out vec2 frag_tex_coords;
#elif defined(SKY)
#elif defined(TERRAIN)
    #if defined(DETAILMAP4)
        out vec3 frag_tangent;
    #endif

    #if !defined(SIMPLE_SHADOW)
        //out float alpha;
    #endif

    out vec4 frag_tex_coords;

    const float terrain_size = 500.0;
    const float fade_distance = 50.0;
    const float fade_mult = 1.0 / fade_distance;

    #define TERRAIN_LIGHT_OFFSET vec2(0.0);//vec2(0.0005)+ws_light.xz*0.0005
#elif defined(CHARACTER)
    out vec2 fur_tex_coord;

    #ifdef TANGENT
        out mat3 tan_to_obj;
    #endif

    #ifndef DEPTH_ONLY
        out vec3 concat_bone1;
        out vec3 concat_bone2;
        out vec2 tex_coord;
        out vec2 morphed_tex_coord;
        out vec3 orig_vert;

        #ifndef NO_VELOCITY_BUF
            out vec3 vel;
        #endif
    #endif
#else // static object

    #if defined(ATTRIB_ENVOBJ_INSTANCING)
        flat out vec3 model_scale_frag;
        flat out vec4 model_rotation_quat_frag;
        flat out vec4 color_tint_frag;

        #if defined(DETAILMAP4)
            flat out vec4 detail_scale_frag;
        #endif
    #endif

    #ifdef TANGENT
        out mat3 tan_to_obj;
    #endif

    out vec2 frag_tex_coords;

    #ifndef NO_INSTANCE_ID
        flat out int instance_id;
    #endif
#endif

const float water_height = 26.0;
const float refract_amount = 0.6;

void GenerateNormal() {

}

//#define GRASS_ESSENTIALS
#ifdef GRASS_ESSENTIALS
    layout (std140) uniform ClusterInfo {
        uvec3 grid_size;
        uint num_decals;
        uint num_lights;
        uint light_cluster_data_offset;
        uint light_data_offset;
        uint cluster_width;
        mat4 inv_proj_mat;
        vec4 viewport;
        float z_near;
        float z_mult;
        float pad3;
        float pad4;
    };


    #define NUM_GRID_COMPONENTS 2u
    #define ZCLUSTERFUNC(val) (log(-1.0 * (val) - z_near + 1.0) * z_mult)

    #define light_decal_data_buffer tex15
    #define cluster_buffer tex13

    const uint COUNT_BITS = 24u;
    const uint COUNT_MASK = ((1u << (32u - COUNT_BITS)) - 1u);
    const uint INDEX_MASK = ((1u << (COUNT_BITS)) - 1u);

    uniform samplerBuffer light_decal_data_buffer;
    uniform usamplerBuffer cluster_buffer;

    // this MUST match the one in source or bad things happen
    struct DecalData {
        vec3 scale;
        float spawn_time;
        vec4 rotation;  // quaternion
        vec3 position;
        float pad1;

        vec4 tint;
        vec4 uv;
        vec4 normal;
    };

    #define DECAL_SIZE_VEC4 6u

    DecalData FetchDecal(uint decal_index) {
        DecalData decal;

        vec4 temp        = texelFetch(light_decal_data_buffer, int(DECAL_SIZE_VEC4 * decal_index + 0u));
        decal.scale      = temp.xyz;
        decal.spawn_time = temp.w;
        decal.rotation   = texelFetch(light_decal_data_buffer, int(DECAL_SIZE_VEC4 * decal_index + 1u));
        decal.position   = texelFetch(light_decal_data_buffer, int(DECAL_SIZE_VEC4 * decal_index + 2u)).xyz;
        decal.pad1       = 0.0f;

        decal.tint = texelFetch(light_decal_data_buffer, int(DECAL_SIZE_VEC4 * decal_index + 3u));

        decal.uv = texelFetch(light_decal_data_buffer, int(DECAL_SIZE_VEC4 * decal_index + 4u));

        decal.normal = texelFetch(light_decal_data_buffer, int(DECAL_SIZE_VEC4 * decal_index + 5u));

        return decal;
    }

    #define decalShadow 1

    void CalculateDecals(uint decal_val, inout vec3 vert) {
        // number of decals in current cluster
        uint decal_count = (decal_val >> COUNT_BITS) & COUNT_MASK;

        //colormap.xyz = vec3(decal_count) / 16.0;
        //return;

        // debug option, uncomment to visualize clusters
        //colormap.xyz = vec3(min(decal_count, 63u) / 63.0);
        //colormap.xyz = vec3(g.z / grid_size.z);

        // index into cluster_decals
        uint first_decal_index = decal_val & INDEX_MASK;

        // decal list data is immediately after cluster lookup data
        uint num_clusters = grid_size.x * grid_size.y * grid_size.z;
        first_decal_index = first_decal_index + 2u * num_clusters;

        vec3 orig_vert = vert;

        for (uint i = 0u; i < decal_count; ++i) {
            // texelFetch takes int
            uint decal_index = texelFetch(cluster_buffer, int(first_decal_index + i)).x;

            DecalData decal = FetchDecal(decal_index);
            float spawn_time = decal.spawn_time;

            vec2 start_uv = decal.uv.xy;
            vec2 size_uv = decal.uv.zw;

            vec2 start_normal = decal.normal.xy;
            vec2 size_normal = decal.normal.zw;

            // We omit scale component because we want to keep normal unit length
            // We omit translation component because normal
            mat3 rotation_mat = mat_from_quat(decal.rotation);
            vec3 decal_ws_normal = rotation_mat * vec3(0.0, 1.0, 0.0);

            vec3 inv_scale = vec3(1.0f) / decal.scale;
            mat3 inv_rotation_mat = transpose(rotation_mat);
            vec3 temp = (inv_rotation_mat * (orig_vert - decal.position)) * inv_scale;

            if(abs(temp[0]) < 0.5 && abs(temp[1]) < 0.5 && abs(temp[2]) < 0.5){
                bool ambient_shadow = false;
                int type = int(decal.tint[3]);
                bool skip = false;
                vec3 dir;

                switch (type) {
                    case decalShadow:
                        dir = normalize(orig_vert - decal.position);
                        dir.y = -1.0;
                        dir = normalize(dir);
                        vert += dir * max(0.0, (0.8 - length(temp)*2.0))*0.2;
                        break;
                }
            }
        }
    }
#endif  // GRASS_ESSENTIALS


mat3 mat_from_quat(vec4 q) {
    float qxx = q.x * q.x;
    float qyy = q.y * q.y;
    float qzz = q.z * q.z;
    float qxz = q.x * q.z;
    float qxy = q.x * q.y;
    float qyz = q.y * q.z;
    float qwx = q.w * q.x;
    float qwy = q.w * q.y;
    float qwz = q.w * q.z;

    mat3 m;
    m[0][0] = 1.0f - 2.0f * (qyy +  qzz);
    m[0][1] = 2.0f * (qxy + qwz);
    m[0][2] = 2.0f * (qxz - qwy);

    m[1][0] = 2.0f * (qxy - qwz);
    m[1][1] = 1.0f - 2.0f * (qxx +  qzz);
    m[1][2] = 2.0f * (qyz + qwx);

    m[2][0] = 2.0f * (qxz + qwy);
    m[2][1] = 2.0f * (qyz - qwx);
    m[2][2] = 1.0f - 2.0f * (qxx +  qyy);

    return m;
}

vec3 quat_mul_vec3(vec4 q, vec3 v) {
    // Adapted from https://github.com/g-truc/glm/blob/master/glm/detail/type_quat.inl
    // Also from Fabien Giesen, according to - https://blog.molecular-matters.com/2013/05/24/a-faster-quaternion-vector-multiplication/
    vec3 quat_vector = q.xyz;
    vec3 uv = cross(quat_vector, v);
    vec3 uuv = cross(quat_vector, uv);
    return v + ((uv * q.w) + uuv) * 2;
}

vec3 transform_vec3(vec3 scale, vec4 rotation_quat, vec3 translation, vec3 value) {
    vec3 result = scale * value;
    result = quat_mul_vec3(rotation_quat, result);
    result += translation;
    return result;
}


void main() {
    #if defined(PARTICLE)
        #ifdef INSTANCED
            vec3 transformed_vertex = (instance_transform[gl_InstanceID] * vec4(vertex_attrib, 1.0)).xyz;

            #if defined(NORMAL_MAP_TRANSLUCENT) || defined(WATER) || defined(SPLAT)
                tangent_to_world3 = normalize((instance_transform[gl_InstanceID] * vec4(0,0,-1,0)).xyz);
                tangent_to_world1 = normalize((instance_transform[gl_InstanceID] * vec4(1,0,0,0)).xyz);
                tangent_to_world2 = normalize(cross(tangent_to_world1,tangent_to_world3));
            #endif
        #else
            vec3 transformed_vertex = vertex_attrib;

            #if defined(NORMAL_MAP_TRANSLUCENT) || defined(WATER) || defined(SPLAT)
                tangent_to_world3 = normalize(normal_attrib * -1.0);
                tangent_to_world1 = normalize(tangent_attrib);
                tangent_to_world2 = normalize(cross(tangent_to_world1,tangent_to_world3));
            #endif
        #endif

        mat4 projection_view_mat = mvp;
        tex_coord = tex_coord_attrib;
        tex_coord[1] = 1.0 - tex_coord[1];

        #ifdef INSTANCED
            instance_id = gl_InstanceID;
        #endif
    #elif defined(GPU_PARTICLE_FIELD)
        instance_id = gl_InstanceID*1000 + (gl_VertexID/4);
        float speed = 0.0;
        float fall_speed = 0.0;
        float size = 0.005 * (sin(instance_id*1.327)*0.5+1.0);
        float max_dist = 16.13;

        #ifdef ASH

            int type = instance_id % 83;

            if(type == 0){
                max_dist *= 2.0;
                size *= 4.0;
                fall_speed = -2.0;
                speed = 0.3;
            } else if(type >= 1 && type < 3){
                max_dist *= 2.0;
                size *= 4.0;
                fall_speed = 1.0;
                speed = 0.2;
            } else {
                fall_speed = -0.5;
                speed = 0.1;
                size *= 80.0;
            }

        #elif defined(SANDSTORM)

            int type = instance_id % 83;

            if(type >= 0 && type < 3){
                max_dist *= 2.0;
                size *= 2.0;
                fall_speed = 1.0;
                speed = 0.2;
            } else { // cloud
                fall_speed = 0;
                speed = 0.1;
                size *= 80.0;
            }

        #elif defined(RAIN)

            speed = 0.0;
            fall_speed = 9.0;
            //max_dist *= 0.25;
            int type = instance_id % 83;

        #elif defined(SNOW)

            speed = 0.1;
            fall_speed = 1.0;

            #ifdef MED
                int type = instance_id % 83;

                if(type == 10){
                    size *= 80.0;
                }

                if(type > 10){
                    size = 0.0;
                }

                fall_speed *= 0.5;
            #endif

        #elif defined(BUGS)

            speed = 0.2;
            fall_speed = 0.0;
            size *= 0.5;
            max_dist *= 1.0;

            #ifdef FIREFLY
                max_dist *= 4.0;
                speed *= 0.5;

                if(instance_id % 8 != 0){
                    size = 0.0;
                }
            #elif defined(MOREBUGS)
                if(instance_id % 4 != 0){
                    size = 0.0;
                }
            #else
                if(instance_id % 10 != 0){
                    size = 0.0;
                }
            #endif

        #endif

        mat4 projection_view_mat = mvp;
        vec3 pos = vec3(instance_id%100, (instance_id/100)%100, instance_id/100/100);
        pos += vec3(-50, 0, -50);
        pos *= (8,4,8);
        vec3 offset = vec3(sin(instance_id*1.1), sin(instance_id*1.3), sin(instance_id*1.7)) * 5.0;
        pos += offset;
        float dist = distance(pos, cam_pos);
        pos.y -= time*speed*3.0;

        #ifdef BUGS
            pos *= 3.0;
        #endif

        pos.x += sin(time*1.7*speed+pos.x)+sin(time*speed*1.3+pos.z)+sin(time*speed*2.1+pos.y);
        pos.y += sin(time*1.1*speed+pos.x)+sin(time*speed*1.9+pos.z)+sin(time*speed*1.4+pos.y);
        pos.z += sin(time*0.7*speed+pos.x)+sin(time*speed*2.3+pos.z)+sin(time*speed*1.2+pos.y);

        #ifdef BUGS
            pos /= 3.0;
        #endif

        pos.y += time*speed*3.0;
        pos.y -= time*fall_speed;
        pos.x += sin(instance_id);

        #ifdef SANDSTORM
            pos.x -= time*15.;
        #endif

        pos += vec3(1000);

        for(int i=0; i<3; ++i){
            if(abs(pos[i] - cam_pos[i]) > max_dist){
                pos[i] -= cam_pos[i];
                pos[i] /= max_dist * 2.0;
                pos[i] = fract(pos[i]);
                pos[i] *= max_dist * 2.0;
                pos[i] -= max_dist;
                pos[i] += cam_pos[i];
            }
        }

        #if defined(SNOW) && !defined(MED)
            if(pos.y < -4.1){
                size = max(0.0, size*(1.0 + (pos.y+4.1)*3.0));
                pos.y = -4.1;
            }
        #endif

        #if defined(FIREFLY) && defined(WATER_DELETE)
            if(pos.y < 82.7){
                size = 0.0;
            }
        #endif

        dist = distance(pos, cam_pos);

        #ifdef ASH

            if(type == 0){
                float max_spark_dist = 16.0;

                if(dist > max_spark_dist){
                    size = 0.0;
                }
                alpha = (1.0 - dist/max_spark_dist)*4.0;
            } else if(type >= 1 && type < 3){
                alpha = 4.0;
            } else {
                alpha = 0.04;

                if(dist > 16){
                    size = 0.0;
                }
            }

            if(type > 15){
                size = 0.0;
            }

        #elif defined(SANDSTORM)

            if(type == 0){
                float max_spark_dist = 16.0;

                if(dist > max_spark_dist){
                    size = 0.0;
                }

                alpha = (1.0 - dist/max_spark_dist)*4.0;
            } else if(type >= 1 && type < 3){
                alpha = 4.0;
            } else {
                alpha = 0.04;

                if(dist > 16){
                    size = 0.0;
                }
            }

            if(type > 15){
                size = 0.0;
            }

        #elif defined(RAIN)

            float rain_stretch = max(1.0, 20.0 * time_scale);
            float size_mult = max(1.0, dist*0.5 - 1.0);
            size *= size_mult;
            alpha = min(0.2, 0.07/pow(size_mult,1.6)*20.0/rain_stretch)*3.0;

        #elif defined(FIREFLY)

            float size_mult = max(1.0, dist*0.7 - 1.0);
            size *= size_mult;
            alpha = 1.0/pow(size_mult, 1.5);

        #elif defined(BUGS)

            float size_mult = max(1.0, dist*0.7 - 1.0);
            size *= size_mult;
            alpha = 1.0/size_mult/size_mult;

        #else

            float size_mult = max(1.0, dist*0.5 - 1.0);
            size *= size_mult;
            alpha = 1.0/size_mult/size_mult;

            #if defined(SNOW) && defined(MED)
                //alpha *= 0.2;

                if(type == 10){
                    alpha = 0.001;
                }
            #endif

        #endif

        tex_coord = tex_coord_attrib;

        #ifdef RAIN
            vec3 transformed_vertex = size*(inverse(view_matrix) * vec4(vertex_attrib, 0.0)).xyz * vec3(1,rain_stretch,1) + pos;
        #else
            vec3 transformed_vertex = size*(inverse(view_matrix) * vec4(vertex_attrib, 0.0)).xyz + pos;
        #endif

    #elif defined(SKY)

        vec3 transformed_vertex = cam_pos + vertex_attrib * 10000;

    #elif defined(DETAIL_OBJECT)

        mat4 obj2world = transforms[gl_InstanceID];
        vec3 transformed_vertex = (obj2world*vec4(vertex_attrib, 1.0)).xyz;

        #ifdef PLANT
            float plant_shake = 0.0;
            vec3 vertex_offset = CalcVertexOffset(transformed_vertex, vertex_attrib.y*2.0, time, plant_shake);
            vertex_offset.y *= 0.2;

            #ifdef LESS_PLANT_MOVEMENT
                vertex_offset *= 0.1;
            #endif
        #endif

        mat3 obj2worldmat3 = mat3(normalize(obj2world[0].xyz),
                                  normalize(obj2world[1].xyz),
                                  normalize(obj2world[2].xyz));
        mat3 tan_to_obj = mat3(tangent_attrib, bitangent_attrib, normal_attrib);
        tangent_to_world = obj2worldmat3 * tan_to_obj;


        vec4 aux = texcoords2[gl_InstanceID];

        float embed = aux.z;
        float height_scale = aux.a;
        transformed_vertex.y -= max(embed,length(transformed_vertex.xyz - cam_pos)/max_distance)*height*height_scale;

        #ifdef PLANT
            transformed_vertex += (obj2world * vec4(vertex_offset,0.0)).xyz;
        #endif

        vec3 temp = transformed_vertex.xyz;
        transformed_vertex.xyz = temp;

        frag_tex_coords = tex_coord_attrib;
        base_tex_coord = aux.xy+TERRAIN_LIGHT_OFFSET;

        frag_tex_coords[1] = 1.0 - frag_tex_coords[1];
        base_tex_coord[1] = 1.0 - base_tex_coord[1];

    #elif defined(ITEM)

        vec3 transformed_vertex = (model_mat * vec4(vertex_attrib, 1.0)).xyz;

        frag_tex_coords = tex_coord_attrib;
        frag_tex_coords[1] = 1.0 - frag_tex_coords[1];

        #ifndef DEPTH_ONLY
            #ifndef NO_VELOCITY_BUF
                vec4 old_vel = old_vel_mat * vec4(vertex_attrib, 1.0);
                vec4 new_vel = new_vel_mat * vec4(vertex_attrib, 1.0);
                vel = (new_vel - old_vel).xyz;
            #endif

            #ifdef TANGENT
                frag_normal = normal_attrib;
            #endif
        #endif

    #elif defined(TERRAIN)

        vec3 transformed_vertex = vertex_attrib;

        #if defined(DETAILMAP4)
            frag_tangent = tangent_attrib;
        #endif

        #if !defined(SIMPLE_SHADOW)
            //alpha = min(1.0,(terrain_size-vertex_attrib.x)*fade_mult)*
            //    min(1.0,(vertex_attrib.x+500.0)*fade_mult)*
            //    min(1.0,(terrain_size-vertex_attrib.z)*fade_mult)*
            //    min(1.0,(vertex_attrib.z+500.0)*fade_mult);
            //alpha = max(0.0,alpha);
        #endif

        frag_tex_coords.xy = tex_coord_attrib+TERRAIN_LIGHT_OFFSET;
        frag_tex_coords.zw = detail_tex_coord*0.1;
        frag_tex_coords[1] = 1.0 - frag_tex_coords[1];
        frag_tex_coords[3] = 1.0 - frag_tex_coords[3];

    #elif defined(CHARACTER)

        #if defined(GPU_SKINNING)

            mat4 concat_bone = bone_mats[int(bone_ids[0])];
            concat_bone *= bone_weights[0];

            for (int i = 1; i < 4; i++) {
                concat_bone += bone_weights[i] * bone_mats[int(bone_ids[i])];
            }

            vec3 offset = (concat_bone * vec4(morph_offsets, 0.0)).xyz;
            concat_bone[3] += vec4(offset, 0.0);

        #else  // GPU_SKINNING

            mat4 concat_bone;
            concat_bone[0] = vec4(transform_mat_column_a[0], transform_mat_column_b[0], transform_mat_column_c[0], 0.0);
            concat_bone[1] = vec4(transform_mat_column_a[1], transform_mat_column_b[1], transform_mat_column_c[1], 0.0);
            concat_bone[2] = vec4(transform_mat_column_a[2], transform_mat_column_b[2], transform_mat_column_c[2], 0.0);
            concat_bone[3] = vec4(transform_mat_column_a[3], transform_mat_column_b[3], transform_mat_column_c[3], 1.0);

        #endif  // GPU_SKINNING

        #if defined(TANGENT)
            tan_to_obj = mat3(tangent_attrib, bitangent_attrib, normal_attrib);
        #endif

        vec3 transformed_vertex = (concat_bone * vec4(vertex_attrib, 1.0)).xyz;

        mat4 projection_view_mat = mvp;

        // Set up varyings to pass bone matrix to fragment shader
        #ifndef DEPTH_ONLY
            orig_vert = vertex_attrib;
            concat_bone1 = concat_bone[0].xyz;
            concat_bone2 = concat_bone[1].xyz;

            tex_coord = tex_coord_attrib;
            morphed_tex_coord = tex_coord_attrib + morph_tex_offset_attrib;

            #ifndef NO_VELOCITY_BUF
                vel = vel_attrib;
            #endif

            tex_coord[1] = 1.0 - tex_coord[1];
            morphed_tex_coord[1] = 1.0 - morphed_tex_coord[1];
        #endif

        fur_tex_coord = fur_tex_coord_attrib;
        fur_tex_coord[1] = 1.0 - fur_tex_coord[1];

    #else  // static object

        #ifdef NO_INSTANCE_ID
            int instance_id;
        #endif

        instance_id = gl_InstanceID;

        #if defined(TANGENT)
            tan_to_obj = mat3(tangent_attrib, bitangent_attrib, normal_attrib);
        #endif

        #if defined(ATTRIB_ENVOBJ_INSTANCING)
            model_scale_frag = model_scale_attrib;
            model_rotation_quat_frag = model_rotation_quat_attrib;
            color_tint_frag = color_tint_attrib;

            #if defined(DETAILMAP4)
                detail_scale_frag = detail_scale_attrib;
            #endif
        #endif

        vec3 transformed_vertex = transform_vec3(GetInstancedModelScale(instance_id), GetInstancedModelRotationQuat(instance_id), model_translation_attrib, vertex_attrib);

        #ifdef PLANT
            float plant_shake = max(0.0, GetInstancedColorTint(instance_id)[3]);
            float stability = plant_stability_attrib.r;

            #ifdef GRASS_ESSENTIALS
                stability = -0.4 - vertex_attrib.y * 5.0;


                vec4 temp_vec = projection_view_mat * vec4(transformed_vertex, 1.0);
                vec4 eyePos = inv_proj_mat * temp_vec;

                float zVal = ZCLUSTERFUNC(eyePos.z);

                zVal = max(0u, min(zVal, grid_size.z - 1u));

                uvec3 g = uvec3(uvec2((temp_vec.xy / temp_vec.w * 0.5 + vec2(0.5)) * (viewport.zw - viewport.xy)) / cluster_width, zVal);

                uint decal_cluster_index = NUM_GRID_COMPONENTS * ((g.y * grid_size.x + g.x) * grid_size.z + g.z);
                uint decal_val = texelFetch(cluster_buffer, int(decal_cluster_index)).x;
                uint decal_count = (decal_val >> COUNT_BITS) & COUNT_MASK;

                if(decal_count > 0){
                    CalculateDecals(decal_val, transformed_vertex);
                }
            #endif

            vec3 vertex_offset = CalcVertexOffset(transformed_vertex, stability, time, plant_shake);
            transformed_vertex.xyz += quat_mul_vec3(GetInstancedModelRotationQuat(instance_id), vertex_offset);
        #endif

        #ifdef WATERFALL
            transformed_vertex.xyz += CalcVertexOffset(transformed_vertex, 0.1, time, 1.0);
        #endif

        #if defined(WATER) && defined(BEACH)
            transformed_vertex.y += sin(time*0.5)*0.1;
        #endif

        vec3 temp = transformed_vertex.xyz;
        transformed_vertex.xyz = temp;
        frag_tex_coords = tex_coord_attrib;

        #ifdef SCROLL_MEDIUM
            frag_tex_coords.y -= time;
        #endif

        #ifdef SCROLL_FAST
            frag_tex_coords.y -= time * 3.0;
        #endif

        #ifdef SCROLL_SLOW
            frag_tex_coords.y -= time * 0.3;
        #endif

        #ifdef SCROLL_VERY_SLOW
            frag_tex_coords.y -= time * 0.2;
        #endif

        frag_tex_coords[1] = 1.0 - frag_tex_coords[1];

    #endif

    world_vert = transformed_vertex;

    #ifndef SHADOW_CASCADE
	   gl_Position = projection_view_mat * vec4(transformed_vertex, 1.0);
    #endif  // SHADOW_CASCADE
}
