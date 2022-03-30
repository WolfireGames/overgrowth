//-----------------------------------------------------------------------------
//           Name: particles.cpp
//      Developer: Wolfire Games LLC
//         Author: David Rosen
//    Description: This class handles particle animation and rendering
//        License: Read below
//-----------------------------------------------------------------------------
//
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
#pragma once

#include <Math/vec3.h>
#include <Math/vec4.h>

#include <Graphics/textureref.h>
#include <Graphics/vboringcontainer.h>

#include <Asset/assets.h>
#include <Asset/Asset/animationeffect.h>
#include <Asset/Asset/particletype.h>

#include <Scripting/angelscript/ascontext.h>

#include <map>
#include <vector>

class SceneGraph;
class AnimationEffectSystem;

class Particle {
public:
    enum DrawType {DEPTH, COLOR};
    vec3 position;
    vec3 old_position;
    vec3 interp_position;
    float size;
    float old_size;
    float interp_size;
    vec3 velocity;
    vec4 color;
    vec4 old_color;
    vec4 interp_color;
    float rotation;
    float old_rotation;
    float interp_rotation;
    AnimationEffectReader ae_reader;
    unsigned id;
    float alive_time;
    float rotate_speed;
    float cam_dist;
    float initial_size;
    float initial_opacity;
    ParticleTypeRef particle_type;
    typedef std::list<Particle*> ParticleList;
    ParticleList connected;
    ParticleList connected_from;
    vec3 last_connected_pos;
    bool has_last_connected;
    bool collided;

    void Draw(SceneGraph *scenegraph, DrawType draw_type, const mat4& proj_view_matrix);
    void Update(SceneGraph *scenegraph, float timestep, float curr_game_time);
};

class ParticleSystem {
public:
    AnimationEffectSystem* particle_types;
    ParticleSystem(const ASData& as_data);
    ~ParticleSystem();
    void deleteParticle(unsigned int which);
 	unsigned MakeParticle( SceneGraph *scenegraph, const std::string &path, const vec3 &pos, const vec3 &vel, const vec3 &tint);
    void Dispose();
    void Draw(SceneGraph *scenegraph);
    void Update(SceneGraph *scenegraph, float timestep, float curr_game_time);
    unsigned CreateID();
    void ConnectParticles( unsigned a, unsigned b );
	void TintParticle( unsigned id, const vec3& color );

	ASContext script_context_;
private:
    struct {
        ASFunctionHandle update;
    } as_funcs;

    unsigned last_id_created;
    typedef std::vector<Particle*> ParticleVector;
    ParticleVector particles;
    typedef std::map<unsigned, Particle*> ParticleMap;
    ParticleMap particle_map;
};

void DrawGPUParticleField(SceneGraph *scenegraph, const char* type);
