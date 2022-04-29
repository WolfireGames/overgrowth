//-----------------------------------------------------------------------------
//           Name: envobject.cpp
//      Developer: Wolfire Games LLC
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

#include <Physics/bulletobject.h>
#include <Physics/bulletworld.h>
#include <Physics/physics.h>

#include <Utility/assert.h>
#include <Utility/compiler_macros.h>

#include <Graphics/graphics.h>
#include <Graphics/particles.h>
#include <Graphics/camera.h>
#include <Graphics/shaders.h>
#include <Graphics/textures.h>
#include <Graphics/models.h>
#include <Graphics/particles.h>
#include <Graphics/sky.h>
#include <Graphics/pxdebugdraw.h>
#include <Graphics/detailobjectsurface.h>

#include <Objects/movementobject.h>
#include <Objects/riggedobject.h>
#include <Objects/decalobject.h>
#include <Objects/lightvolume.h>
#include <Objects/envobject.h>
#include <Objects/group.h>

#include <Internal/datemodified.h>
#include <Internal/collisiondetection.h>
#include <Internal/timer.h>
#include <Internal/filesystem.h>
#include <Internal/memwrite.h>
#include <Internal/profiler.h>
#include <Internal/common.h>
#include <Internal/config.h>
#include <Internal/message.h>

#include <Math/vec2math.h>
#include <Math/vec3math.h>
#include <Math/vec4math.h>
#include <Math/quaternions.h>

#include <Timing/timingevent.h>
#include <Timing/intel_gl_perf.h>

#include <Asset/Asset/material.h>
#include <Asset/Asset/image_sampler.h>
#include <Asset/Asset/averagecolorasset.h>

#include <Main/scenegraph.h>
#include <Main/engine.h>

#include <Sound/sound.h>
#include <Editors/map_editor.h>
#include <Compat/fileio.h>
#include <Scripting/angelscript/ascontext.h>
#include <Online/online.h>

#include <tinyxml.h>

#include <cmath>
#include <sstream>

extern Timer game_timer;
extern char* global_shader_suffix;
extern bool g_simple_shadows;
extern bool g_level_shadows;
extern bool g_simple_water;
extern bool g_no_decals;
const int _shadow_update_delay = 50;
extern bool shadow_cache_dirty;
extern bool g_draw_collision;
extern bool g_no_reflection_capture;
extern bool g_make_invisible_visible;
extern bool g_attrib_envobj_intancing_support;
extern bool g_attrib_envobj_intancing_enabled;
extern bool g_ubo_batch_multiplier_force_1x;

extern bool g_debug_runtime_disable_env_object_draw;
extern bool g_debug_runtime_disable_env_object_draw_depth_map;
extern bool g_debug_runtime_disable_env_object_draw_detail_object_instances;
extern bool g_debug_runtime_disable_env_object_draw_instances;
extern bool g_debug_runtime_disable_env_object_draw_instances_transparent;
extern bool g_debug_runtime_disable_env_object_pre_draw_camera;

bool last_ofr_is_valid = false;
std::string last_ofr_shader_name;
int last_shader;

struct EnvObjectGLState {
    GLState gl_state;
    EnvObjectGLState() {
        gl_state.depth_test = true;
        gl_state.cull_face = true;
        gl_state.depth_write = true;
        gl_state.blend = false;
    }
};

static const EnvObjectGLState env_object_gl_state;

static void UpdateDetailObjectSurfaces(EnvObject::DOSList *detail_object_surfaces,
                                       const ObjectFileRef& ofr,
                                       const TextureAssetRef& base_color,
                                       const TextureAssetRef& base_normal,
                                       const mat4 transform,
                                       int model_id)
{
    //TODO: Make sure we aren't duplicating code from Terrain.cpp
    for(int i=0, len=detail_object_surfaces->size(); i<len; ++i){
        delete detail_object_surfaces->at(i);
    }
    detail_object_surfaces->clear();
    detail_object_surfaces->resize(ofr->m_detail_object_layers.size());
    int counter = 0;
    for(EnvObject::DOSList::iterator iter = detail_object_surfaces->begin();
        iter != detail_object_surfaces->end(); ++iter)
    {
        DetailObjectSurface*& dos = (*iter);
        dos = new DetailObjectSurface();
        DetailObjectLayer& dol = ofr->m_detail_object_layers[counter];
        dos->AttachTo(Models::Instance()->GetModel(model_id), transform);
        dos->GetTrisInPatches(transform);
        dos->LoadDetailModel(dol.obj_path);
        dos->LoadWeightMap(dol.weight_path);
        dos->SetDensity(dol.density);
        dos->SetNormalConform(dol.normal_conform);
        dos->SetMinEmbed(dol.min_embed);
        dos->tint_weight = dol.tint_weight;
        dos->SetMaxEmbed(dol.max_embed);
        dos->SetMinScale(dol.min_scale);
        dos->SetMaxScale(dol.max_scale);
        dos->SetViewDist(dol.view_dist);
        dos->SetJitterDegrees(dol.jitter_degrees);
        dos->SetOverbright(dol.overbright);
        dos->SetCollisionType(dol.collision_type);
        dos->SetBaseTextures(base_color, base_normal);
        ++counter;
    }
}

EnvObject::EnvObject():
    bullet_object_(NULL),
    csg_modified_(false),
    model_id_(-1),
    attached_(NULL),
    placeholder_(false),
    base_color_tint(1.0f),
    normal_override_buffer_dirty(true),
    no_navmesh(false)
{
    added_to_physics_scene_ = false;
    collidable = true;
}

void EnvObject::UpdateParentHierarchy() {
    bool found_movement_object = false;
    Object* obj = this;
    while(obj->parent){
        obj = obj->parent;
        if(obj->GetType() == _movement_object){
            found_movement_object = true;
            break;
        }
    }
    if(found_movement_object && !attached_){
        // object was not attached to a character and now is
        RemovePhysicsShape();
        attached_ = (MovementObject*)obj;
    } else if(!found_movement_object && attached_){
        // object was attached to a character and now is not
        CreatePhysicsShape();
        attached_ = NULL;
    }
}

void EnvObject::HandleMaterialEvent( const std::string &the_event, const vec3 &event_pos ) {
    MaterialRef material = ofr_material;
    material->HandleEvent(the_event, event_pos);
}

const MaterialEvent& EnvObject::GetMaterialEvent( const std::string &the_event, const vec3 &event_pos, int *tri ) {
    MaterialRef material = ofr_material;
    return material->GetEvent(the_event);
}

const MaterialEvent& EnvObject::GetMaterialEvent( const std::string &the_event, const vec3 &event_pos, const std::string &mod, int *tri) {
    MaterialRef material = ofr_material;
    return material->GetEvent(the_event, mod);
}

EnvObject::~EnvObject() {
    RemovePhysicsShape();
    for(int i=0, len=detail_object_surfaces.size(); i<len; ++i){
        delete detail_object_surfaces[i];
    }
}

void EnvObject::Draw() {
    if (g_debug_runtime_disable_env_object_draw) {
        return;
    }

    EnvObject* instances[1];
    instances[0] = this;
    Camera* camera = ActiveCameras::Get();
    Graphics* graphics = Graphics::Instance();
    mat4 proj_view_mat = camera->GetProjMatrix() * camera->GetViewMatrix();
    mat4 prev_proj_view_mat = camera->GetProjMatrix() * camera->prev_view_mat;
    vec3 cam_pos = camera->GetPos();
    std::vector<mat4> shadow_matrix;
    shadow_matrix.resize(4);
    for(int i=0; i<4; ++i){
        shadow_matrix[i] = camera->biasMatrix * graphics->cascade_shadow_mat[i];
    }
    last_ofr_is_valid = false;
    DrawInstances(instances, 1, proj_view_mat, prev_proj_view_mat, &shadow_matrix, cam_pos, Object::kFullDraw);
    DrawDetailObjectInstances(instances, 1, Object::kFullDraw);
}

static void UpdateNormalOverride(EnvObject* obj, Model* model){
    PROFILER_ZONE(g_profiler_ctx, "UpdateNormalOverride");
    obj->normal_override.resize(model->faces.size()/3);
    obj->normal_override_custom.resize(model->faces.size()/3, vec4(0.0f));
    for(int i=0, len=obj->normal_override.size(); i<len; ++i){
        vec3 vert[3];
        for(int vert_index=0; vert_index<3; ++vert_index){
            int start = model->faces[i*3+vert_index]*3;
            for(int axis=0; axis<3; ++axis){
                vert[vert_index][axis] = model->vertices[start+axis];
            }
            vert[vert_index] = obj->GetTransform() * vert[vert_index];
        }
        vec3 normal = normalize(cross(vert[1] - vert[0], vert[2] - vert[0]));
        obj->normal_override[i] = mix(normal, obj->normal_override_custom[i].xyz(), obj->normal_override_custom[i][3]);//model->face_normals[i];
    }
}

const size_t kAttribIdCountVboInstancing = 11;
const size_t kAttribIdCountUboInstancing = 7;
static int attrib_ids[kAttribIdCountVboInstancing];

static void SetupAttribPointers(bool shader_changed, Model* model, VBORingContainer& env_object_model_translation_instance_vbo, VBORingContainer& env_object_model_scale_instance_vbo, VBORingContainer& env_object_model_rotation_quat_instance_vbo, VBORingContainer& env_object_color_tint_instance_vbo, VBORingContainer& env_object_detail_scale_instance_vbo, Shaders* shaders, int the_shader, Graphics* graphics) {
    bool attrib_envobj_instancing = g_attrib_envobj_intancing_support && g_attrib_envobj_intancing_enabled;
    int attrib_count = attrib_envobj_instancing ? kAttribIdCountVboInstancing : kAttribIdCountUboInstancing;
    if(shader_changed){
        for(int i=0; i<attrib_count; ++i){
            const char* attrib_str;
            int num_el;
            VBOContainer* vbo = NULL;
            VBORingContainer* vboRing = NULL;
            bool instanced;
            switch(i){
            case 0:
                attrib_str = "vertex_attrib";
                num_el = 3;
                vbo = &model->VBO_vertices;
                instanced = false;
                break;
            case 1:
                attrib_str = "tangent_attrib";
                num_el = 3;
                vbo = &model->VBO_tangents;
                instanced = false;
                break;
            case 2:
                attrib_str = "bitangent_attrib";
                num_el = 3;
                vbo = &model->VBO_bitangents;
                instanced = false;
                break;
            case 3:
                attrib_str = "normal_attrib";
                num_el = 3;
                vbo = &model->VBO_normals;
                instanced = false;
                break;
            case 4:
                attrib_str = "tex_coord_attrib";
                num_el = 2;
                vbo = &model->VBO_tex_coords;
                instanced = false;
                break;
            case 5:
                attrib_str = "plant_stability_attrib";
                num_el = 3;
                vbo = &model->VBO_aux;
                instanced = false;
                break;
            case 6:
                attrib_str = "model_translation_attrib";
                num_el = 3;
                vboRing = &env_object_model_translation_instance_vbo;
                instanced = true;
                break;
            case 7:
                attrib_str = "model_scale_attrib";
                num_el = 3;
                vboRing = &env_object_model_scale_instance_vbo;
                instanced = true;
                break;
            case 8:
                attrib_str = "model_rotation_quat_attrib";
                num_el = 4;
                vboRing = &env_object_model_rotation_quat_instance_vbo;
                instanced = true;
                break;
            case 9:
                attrib_str = "color_tint_attrib";
                num_el = 4;
                vboRing = &env_object_color_tint_instance_vbo;
                instanced = true;
                break;
            case 10:
                attrib_str = "detail_scale_attrib";
                num_el = 4;
                vboRing = &env_object_detail_scale_instance_vbo;
                instanced = true;
                break;
            default:
                __builtin_unreachable();
                break;
            }
            CHECK_GL_ERROR();
            attrib_ids[i] = shaders->returnShaderAttrib(attrib_str, the_shader);
            CHECK_GL_ERROR();
            if(attrib_ids[i] != -1 && ((vbo && vbo->valid()) || (vboRing && vboRing->valid()))){
                uintptr_t buffer_offset = 0;
                if(vbo) { vbo->Bind(); buffer_offset = 0; }
                if(vboRing) { vboRing->Bind(); buffer_offset = vboRing->offset(); }
                graphics->EnableVertexAttribArray(attrib_ids[i]);
                CHECK_GL_ERROR();
                glVertexAttribPointer(attrib_ids[i], num_el, GL_FLOAT, false, num_el*sizeof(GLfloat), (void*)buffer_offset);
                CHECK_GL_ERROR();
                glVertexAttribDivisorARB(attrib_ids[i], instanced ? 1 : 0);
                CHECK_GL_ERROR();
            }
        }
    } else {
        for(int i=0; i<attrib_count; ++i){
            if(attrib_ids[i] != -1){
                int num_el;
                VBOContainer* vbo = NULL;
                VBORingContainer* vboRing = NULL;
                bool instanced;
                switch(i){
                case 0:
                    num_el = 3;
                    vbo = &model->VBO_vertices;
                    instanced = false;
                    break;
                case 1:
                    num_el = 3;
                    vbo = &model->VBO_tangents;
                    instanced = false;
                    break;
                case 2:
                    num_el = 3;
                    vbo = &model->VBO_bitangents;
                    instanced = false;
                    break;
                case 3:
                    num_el = 3;
                    vbo = &model->VBO_normals;
                    instanced = false;
                    break;
                case 4:
                    num_el = 2;
                    vbo = &model->VBO_tex_coords;
                    instanced = false;
                    break;
                case 5:
                    num_el = 3;
                    vbo = &model->VBO_aux;
                    instanced = false;
                    break;
                case 6:
                    num_el = 3;
                    vboRing = &env_object_model_translation_instance_vbo;
                    instanced = true;
                    break;
                case 7:
                    num_el = 3;
                    vboRing = &env_object_model_scale_instance_vbo;
                    instanced = true;
                    break;
                case 8:
                    num_el = 4;
                    vboRing = &env_object_model_rotation_quat_instance_vbo;
                    instanced = true;
                    break;
                case 9:
                    num_el = 4;
                    vboRing = &env_object_color_tint_instance_vbo;
                    instanced = true;
                    break;
                case 10:
                    num_el = 4;
                    vboRing = &env_object_detail_scale_instance_vbo;
                    instanced = true;
                    break;
                default:
                    __builtin_unreachable();
                    break;
                }
                if((vbo && vbo->valid()) || (vboRing && vboRing->valid())){
                    uintptr_t buffer_offset = 0;
                    if(vbo) { vbo->Bind(); buffer_offset = 0; }
                    if(vboRing) { vboRing->Bind(); buffer_offset = vboRing->offset(); }
                    graphics->EnableVertexAttribArray(attrib_ids[i]);
                    glVertexAttribPointer(attrib_ids[i], num_el, GL_FLOAT, false, num_el*sizeof(GLfloat), (void*)buffer_offset);
                    glVertexAttribDivisorARB(attrib_ids[i], instanced ? 1 : 0);
                }
            }
        }
    }
}

void EnvObject::DrawInstances(EnvObject** instance_array, int num_instances, const mat4& proj_view_matrix, const mat4& prev_proj_view_matrix, const std::vector<mat4>* shadow_matrix, const vec3& cam_pos, Object::DrawType type) {
    if (g_debug_runtime_disable_env_object_draw_instances) {
        return;
    }

    if (g_debug_runtime_disable_env_object_draw_instances_transparent && transparent) {
        return;
    }

    if (type == Object::DrawType::kDrawDepthOnly && transparent) {
        return;
    }

    PROFILER_GPU_ZONE(g_profiler_ctx, "EnvObject::DrawInstances");

    if(g_draw_collision){
        if(GetCollisionModelID() == -1 || ofr->no_collision){
            return;
        }
    }
    PROFILER_ENTER(g_profiler_ctx, "Setup");

    // Tessellation is ignored when preloading shaders right now because the use-case isn't obvious.
    // Is it set once? Is it per-object? Where would it be set?
    bool use_tesselation = false;// GLEW_ARB_tessellation_shader; 
    Shaders* shaders = Shaders::Instance();
    Models* models = Models::Instance();
    Graphics* graphics = Graphics::Instance();
    Textures* textures = Textures::Instance();
    Timer* timer = &game_timer;
    Model* model = &models->GetModel(model_id_);
    if(g_draw_collision){
        model = &models->GetModel(GetCollisionModelID());
    }
    Camera* cam = ActiveCameras::Get();

    PROFILER_ENTER(g_profiler_ctx, "GL State");
    GLState gl_state;
    gl_state.cull_face = !ofr->double_sided;;
    gl_state.depth_test = true;
    if(type == Object::kDecal){
        gl_state.blend = true;
        gl_state.depth_write = false;
    } else if(transparent) {
        gl_state.blend = false;
        gl_state.depth_write = false;
        if(g_simple_water){
            gl_state.depth_write = true;
        }
    } else {
        gl_state.blend = false;
        gl_state.depth_write = true;
    }
    graphics->setGLState(gl_state);

    if(graphics->use_sample_alpha_to_coverage && !transparent){
        glEnable( GL_SAMPLE_ALPHA_TO_COVERAGE );
    }
    PROFILER_LEAVE(g_profiler_ctx); // GL State

    static int ubo_batch_size_multiplier = 1;
    static GLint max_ubo_size = -1;
    if(max_ubo_size == -1) {
        glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &max_ubo_size);

        if(max_ubo_size >= 131072) {
            ubo_batch_size_multiplier = 8;
        }
        else if(max_ubo_size >= 65536) {
            ubo_batch_size_multiplier = 4;
        }
        else if(max_ubo_size >= 32768) {
            ubo_batch_size_multiplier = 2;
        }
    }

    int the_shader = last_shader;

    bool use_textures = true;
    if(type == Object::kDrawDepthOnly || type == Object::kDrawDepthNoAA || type == Object::kDrawAllShadowCascades){
        use_textures = false;
    }

    bool shader_changed = true;
    if(last_ofr_is_valid && ofr->shader_name == last_ofr_shader_name){
        shader_changed = false;
    }

    if(shader_changed){
        PROFILER_ENTER(g_profiler_ctx, "Shader");

        PROFILER_ENTER(g_profiler_ctx, "Create shader string");
        const int kShaderStrSize = 1024;
        char buf[2][kShaderStrSize];
        char* shader_str[2] = {buf[0], buf[1]};
        if(g_make_invisible_visible && ofr->shader_name.find("#INVISIBLE") != ofr->shader_name.npos && type == Object::kFullDraw) {
            size_t index = ofr->shader_name.find("#INVISIBLE");
            std::string name = ofr->shader_name.substr(0, index);
            name += ofr->shader_name.substr(index + strlen("#INVISIBLE"));

            strcpy(shader_str[0], name.c_str());
            placeholder_ = true;
        } else {
            strcpy(shader_str[0], ofr->shader_name.c_str());
        }
        if(type == Object::kDrawDepthOnly || type == Object::kDrawDepthNoAA || type == Object::kDrawAllShadowCascades || type == Object::kWireframe){
        } else if(type == Object::kDecal){
            FormatString(shader_str[1], kShaderStrSize, "%s #DECAL", shader_str[0]);
            std::swap(shader_str[0], shader_str[1]);
        }
        if (config["decal_normals"].toNumber<bool>()){
            FormatString(shader_str[1], kShaderStrSize, "%s #DECAL_NORMALS", shader_str[0]);
            std::swap(shader_str[0], shader_str[1]);
        }
        if (g_draw_collision){
            FormatString(shader_str[1], kShaderStrSize, "%s #COLLISION", shader_str[0]);
            std::swap(shader_str[0], shader_str[1]);
        }
        if (placeholder_){
            FormatString(shader_str[1], kShaderStrSize, "%s #HALFTONE_STIPPLE", shader_str[0]);
            std::swap(shader_str[0], shader_str[1]);
        }
        if (type == Object::kWireframe){
            FormatString(shader_str[1], kShaderStrSize, "%s #WIREFRAME", shader_str[0]);
            std::swap(shader_str[0], shader_str[1]);
        }

        FormatString(shader_str[1], kShaderStrSize, "%s %s", shader_str[0], global_shader_suffix);
        PROFILER_LEAVE(g_profiler_ctx); // Create shader string
        PROFILER_ENTER(g_profiler_ctx, "returnProgram");
        the_shader = shaders->returnProgram(shader_str[1], use_tesselation?Shaders::kTesselation:Shaders::kNone);
        last_shader = the_shader;
        PROFILER_LEAVE(g_profiler_ctx); // returnProgram

        PROFILER_ENTER(g_profiler_ctx, "setProgram");
        shaders->setProgram(the_shader);
        PROFILER_LEAVE(g_profiler_ctx); // setProgram
        PROFILER_ENTER(g_profiler_ctx, "Misc uniforms");
        shaders->SetUniformMat4("projection_view_mat", proj_view_matrix);
        shaders->SetUniformMat4("prev_projection_view_mat", prev_proj_view_matrix);
        shaders->SetUniformFloat("time",timer->GetRenderTime());
        shaders->SetUniformVec3("cam_pos", cam_pos);
        if(type == Object::kFullDraw){
            if(shadow_matrix){
                std::vector<mat4> temp_shadow_matrix = *shadow_matrix;
                if(g_simple_shadows || !g_level_shadows){
                    temp_shadow_matrix[3] = cam->biasMatrix * graphics->simple_shadow_mat;
                }
                shaders->SetUniformMat4Array("shadow_matrix", temp_shadow_matrix);
            }
            shaders->SetUniformVec3("ws_light", scenegraph_->primary_light.pos);
            shaders->SetUniformVec4("primary_light_color",vec4(scenegraph_->primary_light.color,
                                                               scenegraph_->primary_light.intensity));
        }
        PROFILER_LEAVE(g_profiler_ctx); // Misc uniforms

        PROFILER_LEAVE(g_profiler_ctx); // Shader
    }
    PROFILER_ENTER(g_profiler_ctx, "Textures");
    textures->bindTexture(texture_ref_[0], TEX_COLOR);
    if (use_textures) {
        textures->bindTexture(normal_texture_ref_[0], TEX_NORMAL);
        if(!translucency_texture_ref_.empty()){
            textures->bindTexture(translucency_texture_ref_[0], TEX_TRANSLUCENCY);
        }
        if(weight_map_ref_.valid()){
            textures->bindTexture(weight_map_ref_, 5);

            vec4 temp(0.0f, 0.0f, 0.0f, 0.0f);
            for (unsigned int i = 0; i < detail_texture_color_indices_.size(); i++) {
                temp[i] = (float)detail_texture_color_indices_[i];
            }
            shaders->SetUniformVec4("detail_color_indices", temp);

            for (unsigned int i = 0; i < detail_texture_normal_indices_.size(); i++) {
                temp[i] = (float)detail_texture_normal_indices_[i];
            }
            shaders->SetUniformVec4("detail_normal_indices", temp);

            for(unsigned i=0; i<detail_texture_color_srgb_.size(); ++i){
                const int kBufSize = 256;
                char buf[kBufSize];
                FormatString(buf, kBufSize, "avg_color%d",i);
                shaders->SetUniformVec4(buf, detail_texture_color_srgb_[i]);
            }
        }
        if(shader_changed){
            textures->bindTexture(scenegraph_->sky->GetSpecularCubeMapTexture(), TEX_SPEC_CUBEMAP);
            if(g_simple_shadows || !g_level_shadows){
                textures->bindTexture(graphics->static_shadow_depth_ref, TEX_SHADOW);
            } else {
                textures->bindTexture(graphics->cascade_shadow_depth_ref, TEX_SHADOW);
            }
            if(scenegraph_->light_probe_collection.light_volume_enabled && scenegraph_->light_probe_collection.ambient_3d_tex.valid()){
                textures->bindTexture(scenegraph_->light_probe_collection.ambient_3d_tex, 16);
            }
            if(weight_map_ref_.valid()){
                textures->bindTexture(textures->getDetailColorArray(), 6);
                textures->bindTexture(textures->getDetailNormalArray(), 7);
            }
        }
    }
    PROFILER_LEAVE(g_profiler_ctx); // Textures

    if((type == Object::kFullDraw || type == Object::kWireframe) && shader_changed){
        PROFILER_ZONE(g_profiler_ctx, "kFullDraw uniforms");
        shaders->SetUniformFloat("haze_mult", scenegraph_->haze_mult);
        shaders->SetUniformInt("reflection_capture_num", scenegraph_->ref_cap_matrix.size());
        if(!scenegraph_->ref_cap_matrix.empty()){
            assert(!scenegraph_->ref_cap_matrix_inverse.empty());
            shaders->SetUniformMat4Array("reflection_capture_matrix", scenegraph_->ref_cap_matrix);
            shaders->SetUniformMat4Array("reflection_capture_matrix_inverse", scenegraph_->ref_cap_matrix_inverse);
        }

        if(scenegraph_->light_probe_collection.ShaderNumLightProbes() == 0){
            shaders->SetUniformInt("light_volume_num", 0);
            shaders->SetUniformInt("num_tetrahedra", 0);
            shaders->SetUniformInt("num_light_probes", 0);
        } else {
            std::vector<mat4> light_volume_matrix;
            std::vector<mat4> light_volume_matrix_inverse;
            for(int i=0, len=scenegraph_->light_volume_objects_.size(); i<len; ++i){
                Object* obj = scenegraph_->light_volume_objects_[i];
                const mat4 &mat = obj->GetTransform();
                light_volume_matrix.push_back(mat);
                light_volume_matrix_inverse.push_back(invert(mat));
            }
            shaders->SetUniformInt("light_volume_num", light_volume_matrix.size());
            if(!light_volume_matrix.empty()){
                assert(!light_volume_matrix_inverse.empty());
                shaders->SetUniformMat4Array("light_volume_matrix", light_volume_matrix);
                shaders->SetUniformMat4Array("light_volume_matrix_inverse", light_volume_matrix_inverse);
            }

            shaders->SetUniformInt("num_tetrahedra", scenegraph_->light_probe_collection.ShaderNumTetrahedra());
            shaders->SetUniformInt("num_light_probes", scenegraph_->light_probe_collection.ShaderNumLightProbes());
            shaders->SetUniformVec3("grid_bounds_min", scenegraph_->light_probe_collection.grid_lookup.bounds[0]);
            shaders->SetUniformVec3("grid_bounds_max", scenegraph_->light_probe_collection.grid_lookup.bounds[1]);
            shaders->SetUniformInt("subdivisions_x", scenegraph_->light_probe_collection.grid_lookup.subdivisions[0]);
            shaders->SetUniformInt("subdivisions_y", scenegraph_->light_probe_collection.grid_lookup.subdivisions[1]);
            shaders->SetUniformInt("subdivisions_z", scenegraph_->light_probe_collection.grid_lookup.subdivisions[2]);
            shaders->SetUniformInt(shaders->GetTexUniform(TEX_AMBIENT_COLOR_BUFFER), TEX_AMBIENT_COLOR_BUFFER);
            shaders->SetUniformInt(shaders->GetTexUniform(TEX_AMBIENT_GRID_DATA), TEX_AMBIENT_GRID_DATA);
            if(scenegraph_->light_probe_collection.light_probe_buffer_object_id != -1){
                glBindBuffer(GL_TEXTURE_BUFFER, scenegraph_->light_probe_collection.light_probe_buffer_object_id);
            }
        }

        if( g_no_reflection_capture == false) {
            textures->bindTexture(scenegraph_->cubemaps, 19);
        }

        if(use_textures){
            PROFILER_ZONE(g_profiler_ctx, "Decals and lights");
            scenegraph_->BindDecals(the_shader);
            scenegraph_->BindLights(the_shader);
        }
        //textures->bindTexture(graphics->screen_color_tex, 17);
        textures->bindTexture(graphics->post_effects.temp_screen_tex, 17);
        textures->bindTexture(graphics->screen_depth_tex, 18);
    }

    PROFILER_ENTER(g_profiler_ctx, "Attribs");
    if(!model->vbo_loaded){
        model->createVBO();
    }
    model->VBO_faces.Bind();

    int kBatchSize = 256 * (!g_ubo_batch_multiplier_force_1x ? ubo_batch_size_multiplier : 1);
    const bool ignore_multiplier = true;
    static VBORingContainer env_object_model_translation_instance_vbo(sizeof(vec3) * kBatchSize * 16, kVBOFloat | kVBOStream, ignore_multiplier);
    static VBORingContainer env_object_model_scale_instance_vbo(sizeof(vec3) * kBatchSize * 15, kVBOFloat | kVBOStream, ignore_multiplier);
    static VBORingContainer env_object_model_rotation_quat_instance_vbo(sizeof(vec4) * kBatchSize * 17, kVBOFloat | kVBOStream, ignore_multiplier);
    static VBORingContainer env_object_color_tint_instance_vbo(sizeof(vec4) * kBatchSize * 14, kVBOFloat | kVBOStream, ignore_multiplier);
    static VBORingContainer env_object_detail_scale_instance_vbo(sizeof(vec4) * kBatchSize * 18, kVBOFloat | kVBOStream, ignore_multiplier);

    PROFILER_LEAVE(g_profiler_ctx); // Attribs
    PROFILER_LEAVE(g_profiler_ctx); // Setup

    bool attrib_envobj_instancing = g_attrib_envobj_intancing_support && g_attrib_envobj_intancing_enabled;

    int instance_block_index = shaders->GetUBOBindIndex(the_shader, "InstanceInfo");
    if (attrib_envobj_instancing || (unsigned)instance_block_index != GL_INVALID_INDEX)
    {
        PROFILER_ENTER(g_profiler_ctx, "Setup uniform block");
        GLint block_size = !attrib_envobj_instancing ? shaders->returnShaderBlockSize(instance_block_index, the_shader) : -1;

        static UniformRingBuffer env_object_instance_buffer;
        if(!attrib_envobj_instancing && env_object_instance_buffer.gl_id == -1){
            env_object_instance_buffer.Create(2 * 1024 * 1024);
        }
        PROFILER_LEAVE(g_profiler_ctx); // Setup uniform block

        if(g_draw_collision){
            kBatchSize = 1;
        }

        static GLubyte blockBuffer[131072];  // Big enough for 8x the OpenGL guaranteed minimum size. Max supported by shader flags currently. 16x or higher could be added if new platforms end up having > 128k frequently

        static std::vector<vec3> model_translation_buffer;
        model_translation_buffer.resize(kBatchSize);

        static std::vector<vec3> model_scale_buffer;
        static std::vector<vec4> model_rotation_quat_buffer;
        static std::vector<vec4> color_tint_buffer;
        static std::vector<vec4> detail_scale_buffer;

        if(attrib_envobj_instancing) {
            model_scale_buffer.resize(kBatchSize);
            model_rotation_quat_buffer.resize(kBatchSize);
            color_tint_buffer.resize(kBatchSize);
            detail_scale_buffer.resize(kBatchSize);
        }

        for(int i=0; i<num_instances; i+=kBatchSize){
            vec3* model_translation_attrib = &model_translation_buffer[0];
            vec3* model_scale = attrib_envobj_instancing ? &model_scale_buffer[0] : (vec3*)((uintptr_t)blockBuffer + 0*sizeof(float)*4);
            vec4* model_rotation_quat = attrib_envobj_instancing ? &model_rotation_quat_buffer[0] : (vec4*)((uintptr_t)blockBuffer + 1*sizeof(float)*4);
            vec4* color_tint = attrib_envobj_instancing ? &color_tint_buffer[0] : (vec4*)((uintptr_t)blockBuffer + 2*sizeof(float)*4);
            vec4* detail_scale = attrib_envobj_instancing ? &detail_scale_buffer[0] : (vec4*)((uintptr_t)blockBuffer + 3*sizeof(float)*4);
            if(!attrib_envobj_instancing) {
                glUniformBlockBinding(shaders->programs[the_shader].gl_program, instance_block_index, 0);
            }
            int to_draw = min(kBatchSize, num_instances-i);
            PROFILER_ENTER(g_profiler_ctx, "Setup batch data");
            for(int j=0; j<to_draw; ++j){
                EnvObject* obj = instance_array[i+j];
                PlantComponent* plant_component = obj->plant_component_.get();
                {
                    *model_scale = obj->scale_;
                    *model_rotation_quat = *(vec4*)&obj->GetRotation();
                    *model_translation_attrib = obj->translation_;
                    if(plant_component && plant_component->IsActive()){
                        // TODO: This used to calculate a pivot. Is that important anymore, or can it just rotate around its origin?
                        //if(!plant_component->IsPivotCalculated()){
                        //    plant_component->SetPivot(*scenegraph_->bullet_world_,
                        //        obj->sphere_center_, obj->sphere_radius_);
                        //}
                        quaternion plant_rotation = plant_component->GetQuaternion(obj->sphere_radius_) * obj->GetRotation();
                        *model_rotation_quat = *(vec4*)&plant_rotation;
                    }
                }
                {
                    *color_tint = obj->GetDisplayTint();
                    if(plant_component){
                        (*color_tint)[3] = plant_component->GetPlantShake(obj->sphere_radius_);
                    } else {
                        (*color_tint)[3] = 0.0f;
                    }
                    if(attached_ != NULL){
                        (*color_tint)[3] = -1.0f;
                    }
                }
                {
                    vec4 scale = obj->GetScale();
                    float scaled_up = max(scale[0],max(scale[1],scale[2]));
                    float texel_scale = scaled_up/model->texel_density * 0.15f;
                    vec4 scale_vec(1.0f);
                    for(int k=0; k<(int)ofr->m_detail_map_scale.size(); ++k){
                        scale_vec[k] = texel_scale * ofr->m_detail_map_scale[k];
                    }
                    *detail_scale = scale_vec;
                }

                ++model_translation_attrib;
                if(attrib_envobj_instancing) {
                    ++model_scale;
                    ++model_rotation_quat;
                    ++color_tint;
                    ++detail_scale;
                } else {
                    model_scale = (vec3*)((uintptr_t)model_scale+64);
                    model_rotation_quat = (vec4*)((uintptr_t)model_rotation_quat+64);
                    color_tint = (vec4*)((uintptr_t)color_tint+64);
                    detail_scale = (vec4*)((uintptr_t)detail_scale+64);
                }

                if(g_draw_collision){
                    if(obj->normal_override_buffer_dirty){
                        if(!obj->normal_override_buffer.valid()){
                            obj->normal_override_buffer = Textures::Instance()->makeBufferTexture(obj->normal_override.size()*4*3, GL_RGB32F);
                        }
                        UpdateNormalOverride(obj, model);
                        PROFILER_ZONE(g_profiler_ctx, "UpdateNormalOverride buffer texture");
                        Textures::Instance()->updateBufferTexture(obj->normal_override_buffer, obj->normal_override.size()*4*3, &obj->normal_override[0]);
                        obj->normal_override_buffer_dirty = false;
                    }
                    textures->bindTexture(obj->normal_override_buffer, TEX_LIGHT_DECAL_DATA_BUFFER);
                }
            }
            PROFILER_LEAVE(g_profiler_ctx); // Setup batch data
            {
                env_object_model_translation_instance_vbo.Fill(sizeof(vec3) * to_draw, &model_translation_buffer[0]);

                if(attrib_envobj_instancing) {
                    env_object_model_scale_instance_vbo.Fill(sizeof(vec3) * to_draw, &model_scale_buffer[0]);
                    env_object_model_rotation_quat_instance_vbo.Fill(sizeof(vec4) * to_draw, &model_rotation_quat_buffer[0]);
                    env_object_color_tint_instance_vbo.Fill(sizeof(vec4) * to_draw, &color_tint_buffer[0]);
                    env_object_detail_scale_instance_vbo.Fill(sizeof(vec4) * to_draw, &detail_scale_buffer[0]);
                }

                SetupAttribPointers(shader_changed, model, env_object_model_translation_instance_vbo, env_object_model_scale_instance_vbo, env_object_model_rotation_quat_instance_vbo, env_object_color_tint_instance_vbo, env_object_detail_scale_instance_vbo, shaders, the_shader, graphics);
                shader_changed = false;

                if(!attrib_envobj_instancing) {
                    env_object_instance_buffer.Fill(block_size / kBatchSize * to_draw, blockBuffer);
                    glBindBufferRange( GL_UNIFORM_BUFFER, 0, env_object_instance_buffer.gl_id, env_object_instance_buffer.offset, env_object_instance_buffer.next_offset - env_object_instance_buffer.offset);
                }
            }
            CHECK_GL_ERROR();
            {
                PROFILER_ZONE(g_profiler_ctx, "glDrawElementsInstanced");
                graphics->DrawElementsInstanced(use_tesselation?GL_PATCHES:GL_TRIANGLES, model->faces.size(), GL_UNSIGNED_INT, 0, to_draw);
            }
            CHECK_GL_ERROR();
        }
    }

    graphics->ResetVertexAttribArrays();
    graphics->BindArrayVBO(0);
    graphics->BindElementVBO(0);

    int attrib_count = attrib_envobj_instancing ? kAttribIdCountVboInstancing : kAttribIdCountUboInstancing;
    for(int i=0; i<attrib_count; ++i){
        if(attrib_ids[i] != -1){
            glVertexAttribDivisorARB(attrib_ids[i], 0);
        }
    }

    if(graphics->use_sample_alpha_to_coverage){
        glDisable( GL_SAMPLE_ALPHA_TO_COVERAGE );
    }

    last_ofr_is_valid = ofr.valid();
    if(last_ofr_is_valid) {
        last_ofr_shader_name = ofr->shader_name;
    }
}

void EnvObject::DrawDetailObjectInstances(EnvObject** instance_array, int num_instances, Object::DrawType type) {
    if (g_debug_runtime_disable_env_object_draw_detail_object_instances) {
        return;
    }

    // Note: Should never get called with Object::kDrawDepthOnly. When this comment was written, that was true

    if(!detail_object_surfaces.empty() && type == Object::kFullDraw){
        for(int i=0; i<num_instances; ++i){
            EnvObject* obj = instance_array[i];
            for(DOSList::iterator iter = obj->detail_object_surfaces.begin();
                iter != obj->detail_object_surfaces.end(); ++iter)
            {
                (*iter)->Draw(obj->GetTransform(), DetailObjectSurface::ENVOBJECT, obj->GetDisplayTint(), scenegraph_->sky->GetSpecularCubeMapTexture(), &scenegraph_->light_probe_collection,scenegraph_);
            }
        }
    }
}

void EnvObject::PreDrawCamera(float curr_game_time) {
    if (g_debug_runtime_disable_env_object_pre_draw_camera) {
        return;
    }

    for(DOSList::iterator iter = detail_object_surfaces.begin();
        iter != detail_object_surfaces.end(); ++iter)
    {
        (*iter)->PreDrawCamera(transform_);
    }
}

void EnvObject::UpdateDetailScale() {
    if(!detail_texture_color_.empty()){
        vec4 scale = GetScale();
        float scaled_up = max(scale[0],max(scale[1],scale[2]));
        Model* model = &Models::Instance()->GetModel(model_id_);
        float texel_scale = scaled_up/model->texel_density * 0.15f;
        vec4 scale_vec(1.0f);
        for(int i=0; i<(int)ofr->m_detail_map_scale.size(); ++i){
            scale_vec[i] = texel_scale * ofr->m_detail_map_scale[i];
        }
    }
}

void EnvObject::GetObj2World(float *obj2world) {
    const float *gl_matrix = transform_;
    memcpy(obj2world,gl_matrix,sizeof(float) * 16);
}



void EnvObject::EnvInterpolate(uint16_t pending_updates) {
	/*delta_time_between_interpolation_frames = (next_time - current_time) / 1000.0;

	interpolation_step += (1.0 / (delta_time_between_interpolation_frames / game_timer.GetFrameTime()));
	double catchup = (pending_updates > 2) * pending_updates * pending_updates / 100.0;
	interpolation_step += catchup;

	if (interpolation_step > 1.0 || interpolation_time > delta_time_between_interpolation_frames) {
		interpolation_step = 0.0;
		interpolation_time = 0.0;
		interp = false;
	}
	else {
		vec3 new_scale = lerp(curr_scale, next_scale, interpolation_step);
		vec3 new_translation = lerp(curr_translation, next_translation, interpolation_step);
		quaternion new_rot = Slerp(curr_rot, next_rot, interpolation_step);

		SetTranslation(new_translation);
		SetScale(new_scale);
		SetRotation(new_rot);
	}


	interpolation_time += game_timer.GetFrameTime();
    */

	//LOGI << "first_frame" << first_frame << " Interp_step: " << interpolation_step << " delta: " << delta << " Frame time: " << game_timer.GetFrameTime() << " Interp time: " << interpolation_time << " pending transformations:" << std::endl;

}

void EnvObject::DrawDepthMap(const mat4& proj_view_matrix, const vec4* cull_planes, int num_cull_planes, Object::DrawType draw_type) {
    if (g_debug_runtime_disable_env_object_draw_depth_map) {
        return;
    }

    EnvObject* instances[1];
    instances[0] = this;
    Camera* camera = ActiveCameras::Get();
    Graphics* graphics = Graphics::Instance();
    vec3 cam_pos = camera->GetPos();
    std::vector<mat4> shadow_matrix;
    shadow_matrix.resize(4);
    for(int i=0; i<4; ++i){
        shadow_matrix[i] = camera->biasMatrix * graphics->cascade_shadow_mat[i];
    }
    last_ofr_is_valid = false;
    DrawInstances(instances, 1, proj_view_matrix, proj_view_matrix, &shadow_matrix, cam_pos, draw_type);
}

void EnvObject::SetEnabled(bool val) {
    if(val != enabled_){
        Object::SetEnabled(val);
        if(!enabled_){
            RemovePhysicsShape();
        } else {
            if(!attached_) {
                CreatePhysicsShape();
            }
        }
        shadow_cache_dirty = true;
    }
}

void EnvObject::SetCollisionEnabled(bool val) {
    if(val && !bullet_object_) {
        if(!attached_) {
            CreatePhysicsShape();
        }
    } else if(!val && bullet_object_) {
        RemovePhysicsShape();
    }
}

struct DistanceSorter {
    float distance;
    int id;
};

class DistanceSorterCompare {
public:
    bool operator()(const DistanceSorter &a, const DistanceSorter &b) {
        return a.distance < b.distance;
    }
};

void CreatePointCloud(std::vector<vec3> &points, const Model &model, const ImageSamplerRef &image) {
    float total_triangle_area = 0.0f;;
    std::vector<float> triangle_area(model.faces.size()/3);
    {
        vec3 verts[3];
        int face_index = 0;
        int vert_index;
        for(unsigned i=0; i<triangle_area.size(); ++i){
            for(unsigned j=0; j<3; ++j){
                vert_index = model.faces[face_index+j]*3;
                verts[j] = vec3(model.vertices[vert_index+0],
                                model.vertices[vert_index+1],
                                model.vertices[vert_index+2]);
            }
            triangle_area[i] = length(cross(verts[2]-verts[0], verts[1]-verts[0]))*0.5f;
            total_triangle_area += triangle_area[i];
            face_index += 3;
        }
    }

    float density = 100.0f;
    if(total_triangle_area * density > 10000.0f){
        density = 10000.0f / total_triangle_area;
    }
    int vert_index;
    int tex_index;
    vec3 verts[3];
    vec2 tex_coords[3];
    std::vector<vec3> temp_points;
    for(unsigned face_index=0; face_index<model.faces.size(); face_index+=3){
        for(unsigned j=0; j<3; ++j){
            vert_index = model.faces[face_index+j]*3;
            tex_index = model.faces[face_index+j]*2;
            verts[j] = vec3(model.vertices[vert_index+0],
                            model.vertices[vert_index+1],
                            model.vertices[vert_index+2]);
            tex_coords[j] = vec2(model.tex_coords[tex_index+0],
                                 model.tex_coords[tex_index+1]);
        }
        unsigned num_dots = max((unsigned)5,(unsigned)(triangle_area[face_index/3]*density));
        for(unsigned j=0; j<num_dots; ++j){
            vec2 coord(RangedRandomFloat(0.0f,1.0f),
                       RangedRandomFloat(0.0f,1.0f));
            if(coord[0] + coord[1] > 1.0f){
                std::swap(coord[0], coord[1]);
                coord[0] = 1.0f - coord[0];
                coord[1] = 1.0f - coord[1];
            }
            vec3 pos = (verts[0] + (verts[1]-verts[0])*coord[0] + (verts[2]-verts[0])*coord[1]);
            vec2 tex_coord;
            tex_coord = (tex_coords[0] + (tex_coords[1]-tex_coords[0])*coord[0] + (tex_coords[2]-tex_coords[0])*coord[1]);
            if(image->GetInterpolatedColorUV(tex_coord[0], tex_coord[1]).a()>0.0f){
                temp_points.push_back(pos);
            }
        }
    }

    vec3 center;
    for(unsigned i=0; i<temp_points.size(); ++i){
        center += temp_points[i];
    }
    center /= (float)temp_points.size();

    std::vector<DistanceSorter> ds(temp_points.size());
    for(unsigned i=0; i<temp_points.size(); ++i){
        ds[i].id = i;
        ds[i].distance = distance_squared(temp_points[i], center);
    }
    std::sort(ds.begin(), ds.end(), DistanceSorterCompare());
    //ds.resize(ds.size()*0.9f);
    points.resize(ds.size());
    for(unsigned i=0; i<points.size(); ++i){
        points[i] = temp_points[ds[i].id];
    }
}

BulletWorld* EnvObject::GetBulletWorld() {
    return ofr->bush_collision ? scenegraph_->plant_bullet_world_ : scenegraph_->bullet_world_;
}

void ShortName(const std::string &input, std::string &output){
    int slash = input.rfind('/');
    int dot = input.rfind('.');
    output = input.substr(slash+1, dot-slash-1);
}

typedef std::map<std::string, bool> HullExistsCache;
static HullExistsCache hull_exists_cache;

bool DoesHullFileExist(const std::string& hull_path){
    HullExistsCache::const_iterator iter = hull_exists_cache.find(hull_path);
    bool hull_file_exists = false;
    if(iter != hull_exists_cache.end()){
        hull_file_exists = iter->second;
    } else {
        hull_file_exists = FileExists(hull_path.c_str(), kDataPaths | kModPaths);
        hull_exists_cache[hull_path] = hull_file_exists;
    }
    return hull_file_exists;
}

int EnvObject::GetCollisionModelID() {
    if(ofr->bush_collision){
        return -1;
    } else {
        std::string collision_model_path = ofr->model_name.substr(0, ofr->model_name.size()-4)+"_col.obj";
        if(DoesHullFileExist(collision_model_path)){
            int collision_model_id = Models::Instance()->loadModel(collision_model_path.c_str());
            Model& collision_model = Models::Instance()->GetModel(collision_model_id);
            if(collision_model.old_center == collision_model.center_coords){
                Model& model = Models::Instance()->GetModel(model_id_);
                collision_model.center_coords = model.old_center;
                collision_model.CenterModel();
            }
            return collision_model_id;
        } else {
            return model_id_;
        }
    }
}

void EnvObject::CreatePhysicsShape() {
    BWFlags flags = BW_NO_FLAGS;
    if(ofr->no_collision){
        flags |= BW_DECALS_ONLY;
    }
    BulletWorld* bw = GetBulletWorld();
    if(ofr->bush_collision){
        CreateBushPhysicsShape();
    } else {
        int id = GetCollisionModelID();
        Model* model = &Models::Instance()->GetModel(id);
        normal_override_buffer_dirty = true;
        bullet_object_ = bw->CreateStaticMesh(model, id, flags);
    }
    if(bullet_object_){
        bullet_object_->owner_object = this;
        bullet_object_->SetTransform(GetTranslation(), Mat4FromQuaternion(GetRotation()), GetScale());
        bullet_object_->UpdateTransform();
        bullet_object_->UpdateTransform();
        bw->UpdateSingleAABB(bullet_object_);
    }
}

void EnvObject::RemovePhysicsShape() {
    if(bullet_object_){
        GetBulletWorld()->RemoveStaticObject(&bullet_object_);
    }
}

bool EnvObject::Initialize() {
    //int test_shader_id = Shaders::Instance()->returnProgram(ofr->shader_name);
    //transparent = Shaders::Instance()->IsProgramTransparent(test_shader_id);
    transparent = false;
    if(ofr->transparent) {
        transparent = true;
    }
    if(ofr->terrain_fixed){
        SetTranslation(Models::Instance()->GetModel(model_id_).old_center);
        permission_flags &= ~(Object::CAN_ROTATE |
            Object::CAN_SCALE | Object::CAN_TRANSLATE);
    }
    if(scenegraph_ && !added_to_physics_scene_ && !attached_){
        if(!ofr->bush_collision) {
            int id = GetCollisionModelID();
            Model* model = &Models::Instance()->GetModel(id);
            if(model->faces.empty()) {
                DisplayError("Couldn't find model", "The given envobject doesn't have a valid collision mesh. Make sure the model/collision mesh exists. The object will not be spawned", ErrorType::_ok);
                return false;
            }
        }

        CreatePhysicsShape();
        added_to_physics_scene_ = true;
        UpdatePhysicsTransform();
    }
    UpdateBoundingSphere();
    m_transform_starting_sphere_center = sphere_center_;
    m_transform_starting_sphere_radius = sphere_radius_;
    return true;
}

void EnvObject::GetShaderNames(std::map<std::string, int>& shaders) {
    shaders[ofr->shader_name] = SceneGraph::kPreloadTypeAll;
}

void EnvObject::Update(float timestep) {
    Online* online = Online::Instance();

    if(plant_component_.get()){
        if(plant_component_->IsActive()){
            plant_component_->Update(timestep);
        } else {
            if(scenegraph_ && update_list_entry != -1){
                scenegraph_->UnlinkUpdateObject(this, update_list_entry);
                update_list_entry = -1;
            }
        }
    }

    if (online->IsClient()) { 
        list<OnlineMessageRef>& frames = incoming_online_env_update;
        if(frames.size() > 0) {
            EnvObjectUpdate* eou = static_cast<EnvObjectUpdate*>(frames.begin()->GetData());
            if(eou != nullptr) {
                SetTransformationMatrix(eou->transform);
            }
            frames.pop_front();
        }
    }
}

void EnvObject::UpdateBoundingSphere() {
    vec3 radius_vec = box_.dims*0.5f;
    sphere_radius_ = length(transform_.GetRotatedvec3(radius_vec));
    sphere_center_ = GetTranslation();
}

int EnvObject::lineCheck(const vec3 &start, const vec3 &end, vec3 *point, vec3 *normal) {
    if(selected_){
        if(!sphere_line_intersection( start, end, sphere_center_, sphere_radius_ ) ){  // Much faster than the OBB check, and most objects probably won't fall under cursor
            return -1;
        }
        return LineCheckEditorCube(start, end, point, normal);
    }
    if(bullet_object_){
        return GetBulletWorld()->CheckRayCollisionObj(start, end, *bullet_object_, point, normal);
    }
    if (model_id_ == -1 ||
        !sphere_line_intersection( start, end, sphere_center_, sphere_radius_ ) )
    {
        return -1;
    }
    const Model& model = Models::Instance()->GetModel(model_id_);
    int intersecting = model.lineCheckNoBackface(invert(transform_)*start,
                                                 invert(transform_)*end,
                                                 point,
                                                 normal);
    if(intersecting!=-1){
        if(point){
            *point = transform_ * *point;
        }
        if(normal){
            *normal = GetRotation() * *normal;
        }
    }
    return intersecting;
}

void EnvObject::Moved(Object::MoveType type) {
    Object::Moved(type);
    if(type & Object::kScale) {
        UpdateDetailScale();
    }

    UpdateDetailScale();
    UpdateBoundingSphere();

    LoadModel();
    if(plant_component_.get()){
        plant_component_->ClearPivot();
    }
    Graphics::Instance()->nav_mesh_out_of_date = true;

    if(ofr.valid()){
        UpdateDetailObjectSurfaces(&detail_object_surfaces, ofr, texture_ref_[0],
            normal_texture_ref_[0], transform_, model_id_);
        if(!ofr->dynamic && !attached_ && !placeholder_){
            shadow_cache_dirty = true;
            if(scenegraph_) {
                auto it = std::find(scenegraph_->visible_static_meshes_.begin(), scenegraph_->visible_static_meshes_.end(), this);
                // Dirty shadow cache bounds for current object
                if(it != scenegraph_->visible_static_meshes_.end()){
                    auto& shadow_cache_bounds = *(scenegraph_->visible_static_meshes_shadow_cache_bounds_.begin() + std::distance(scenegraph_->visible_static_meshes_.begin(), it));
                    shadow_cache_bounds.is_calculated = false;
                }
            }
            normal_override_buffer_dirty = true;
        }
    }
}

// returns true iff an update occured
bool EnvObject::UpdatePhysicsTransform() {
    if (!bullet_object_) {
        return false;
    }

    bullet_object_->SetTransform(GetTranslation(), Mat4FromQuaternion(GetRotation()), GetScale());
    bullet_object_->UpdateTransform();
    bullet_object_->UpdateTransform();
    GetBulletWorld()->UpdateSingleAABB(bullet_object_);
    return true;
}

void EnvObject::GetDisplayName(char* buf, int buf_size) {
    if( GetName().empty() ) {
        FormatString(buf, buf_size, "%d, Static Mesh from %s", GetID(), obj_file.c_str());
    } else {
        FormatString(buf, buf_size, "%s, Static Mesh from %s", GetName().c_str(), obj_file.c_str());
    }
}

const Model * EnvObject::GetModel() const {
    return &Models::Instance()->GetModel(model_id_);
}

const MaterialDecal& EnvObject::GetMaterialDecal( const std::string &type, const vec3 &pos ) {
    MaterialRef material = ofr_material;
    return material->GetDecal(type);
}

const MaterialParticle& EnvObject::GetMaterialParticle( const std::string &type, const vec3 &pos ) {
    MaterialRef material = ofr_material;
    return material->GetParticle(type);
}

bool EnvObject::Load( const std::string& type_file ) {
    bool ret = true;
    Textures* textures = Textures::Instance();
    textures->setWrap(GL_REPEAT);

    ofr = Engine::Instance()->GetAssetManager()->LoadSync<ObjectFile>(type_file);

    if( ofr.valid() ) {
        modsource_ = ofr->modsource_;

        ofr_material = Engine::Instance()->GetAssetManager()->LoadSync<Material>(ofr->material_path);

        if(ofr_material.valid()) {
            PROFILER_ENTER(g_profiler_ctx, "Loading detail/weight texture refs");
            if(!ofr->weight_map.empty()){
                weight_map_ref_ = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(ofr->weight_map);
                if( weight_map_ref_.valid() == false ) {
                    LOGE << "Failed at loading weight_map " <<  ofr->weight_map << " for envobject " << type_file << std::endl;
                    ret = false;
                }
            }

            TextureRef detail_color_map_ref_ = textures->getDetailColorArray();
            detail_texture_color_indices_.resize(ofr->m_detail_color_maps.size());
            for(unsigned i=0; i<ofr->m_detail_color_maps.size(); ++i){
                detail_texture_color_indices_[i] = textures->loadArraySlice(detail_color_map_ref_, ofr->m_detail_color_maps[i]);
            }

            detail_texture_normal_indices_.resize(ofr->m_detail_normal_maps.size());
            TextureRef detail_normal_map_ref_ = textures->getDetailNormalArray();
            for(unsigned i=0; i<ofr->m_detail_normal_maps.size(); ++i){
                detail_texture_normal_indices_[i] = textures->loadArraySlice(detail_normal_map_ref_, ofr->m_detail_normal_maps[i]);
            }
            PROFILER_LEAVE(g_profiler_ctx);

            PROFILER_ENTER(g_profiler_ctx, "Getting average colors");
            detail_texture_color_.resize(ofr->m_detail_color_maps.size());
            detail_texture_color_srgb_.resize(ofr->m_detail_color_maps.size());
            for(unsigned i=0; i<ofr->m_detail_color_maps.size(); i++){
                vec4 &dtc = detail_texture_color_[i];
                vec4 &dtc_srgb = detail_texture_color_srgb_[i];
                //dtc = AverageColors::Instance()->ReturnRef(ofr->m_detail_color_maps[i])->color();

                AverageColorRef color_ref = Engine::Instance()->GetAssetManager()->LoadSync<AverageColor>(ofr->m_detail_color_maps[i]);
                if( color_ref.valid() ) {
                    average_color_refs.insert(color_ref);
                    dtc = color_ref->color();

                    dtc_srgb[0] = pow(dtc[0], 2.2f);
                    dtc_srgb[1] = pow(dtc[1], 2.2f);
                    dtc_srgb[2] = pow(dtc[2], 2.2f);
                    dtc_srgb[3] = dtc[3];
                } else {
                    LOGE << "Failed at loading AverageColor " << ofr->m_detail_color_maps[i] << " for EnvObject " << type_file << std::endl;
                    ret = false;
                }
            }
            PROFILER_LEAVE(g_profiler_ctx);

            PROFILER_ENTER(g_profiler_ctx, "Getting main textures");
            obj_file = type_file;

            if(ofr->clamp_texture){
                Textures::Instance()->setWrap(GL_CLAMP_TO_EDGE);
            } else {
                Textures::Instance()->setWrap(GL_REPEAT);
            }
            texture_ref_.clear();

            TextureAssetRef tex = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(ofr->color_map.c_str(),PX_SRGB,0x0);

            if( tex.valid() ) {
                texture_ref_.push_back(tex);
            } else {
                LOGE << "Failed at loading color map TextureAsset " << ofr->color_map.c_str() << " for EnvObject " << type_file << std::endl;
                ret = false;
            }

            normal_texture_ref_.clear();

            tex = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(ofr->normal_map.c_str());

            if( tex.valid() ) {
                normal_texture_ref_.push_back(tex);
            } else {
                LOGE << "Failed at loading normal map TextureAsset " << ofr->normal_map.c_str() << " for EnvObject " << type_file << std::endl;
                ret = false;
            }


            translucency_texture_ref_.clear();
            if(!ofr->translucency_map.empty()) {
                tex = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(ofr->translucency_map.c_str(),PX_SRGB,0x0);
                if( tex.valid() ) {
                    translucency_texture_ref_.push_back(tex);
                } else {
                    LOGE << "Failed at loading translucency map TextureAsset " << ofr->translucency_map.c_str() << " for EnvObject " << type_file << std::endl;
                    ret = false;
                }
            }
            PROFILER_LEAVE(g_profiler_ctx);


            if(ofr->shader_name == "cubemap"){
                ofr->shader_name = "envobject #TANGENT";
            } else if(ofr->shader_name == "cubemapobj" ||
                      ofr->shader_name == "cubemapobjchar" ||
                      ofr->shader_name == "cubemapobjitem")
            {
                ofr->shader_name = "envobject";
            } else if(ofr->shader_name == "cubemapalpha"){
                ofr->shader_name = "envobject #TANGENT #ALPHA";
            } else if(ofr->shader_name == "plant" || ofr->shader_name == "plant_less_movement"){
                ofr->shader_name = "envobject #TANGENT #ALPHA #PLANT";
            } else if(ofr->shader_name == "plant_foliage") {
                ofr->shader_name = "envobject #TANGENT #ALPHA #PLANT #NO_DECALS";
            } else if(ofr->shader_name == "detailmap4"){
                ofr->shader_name = "envobject #DETAILMAP4 #TANGENT";
            } else if(ofr->shader_name == "detailmap4tangent"){
                ofr->shader_name = "envobject #DETAILMAP4 #TANGENT #BASE_TANGENT";
            } else if(ofr->shader_name == "MagmaFloor"){
                ofr->shader_name = "envobject #MAGMA_FLOOR";
            } else if(ofr->shader_name == "MagmaFlow"){
                ofr->shader_name = "envobject #MAGMA_FLOW";
            }

            if(ofr->shader_name.find("#ALPHA") != std::string::npos){
                transparent = true;
            } else {
                transparent = ofr->transparent;
            }

            LoadModel();

            if(!ofr->wind_map.empty()) {
                if( Models::Instance()->GetModel(model_id_).LoadAuxFromImage(ofr->wind_map) == false ) {
                    LOGE << "Failed to acceptably load AuxFromImage " << ofr->wind_map << " for envobject " << type_file << std::endl;
                    ret = false;
                }
            }

            base_color_tint = ofr->color_tint;

            if(ofr->bush_collision){
                plant_component_.reset(new PlantComponent());
            }

            UpdateDetailObjectSurfaces(&detail_object_surfaces, ofr, texture_ref_[0],
                normal_texture_ref_[0], transform_, model_id_);
        } else {
            LOGE << "Failure loading Material " << ofr->material_path << " for envobject " << type_file << std::endl;
            ret = false;
        }
    } else {
        LOGE << "Failure loading envobject when loading its base ObjectFile " << type_file << std::endl;
        ret = false;
    }
    return ret;
}

static bool NeedsWindingFlip(const vec3 &scale) {
    int needs_flip = 0;
    for(int i=0; i<3; ++i){
        if (scale[i] < 0) {
            needs_flip++;
        }
    }
    return (needs_flip%2 == 1);
}

void EnvObject::LoadModel() {
    if(!ofr.valid()){
        return;
    }
    bool use_tangent = true;
    char flags = 0;
    if(use_tangent){
        flags = flags | _MDL_USE_TANGENT;
    }
    flags = flags | _MDL_CENTER;

    bool physics_recalc = false;
    if(!csg_modified_){
        int old_model_id = model_id_;
        if(NeedsWindingFlip(GetScale())){
            model_id_ = Models::Instance()->loadFlippedModel(ofr->model_name.c_str(), flags);
            winding_flip = true;
        } else {
            model_id_ = Models::Instance()->loadModel(ofr->model_name.c_str(), flags);
            winding_flip = false;
        }
        if(old_model_id != -1 && old_model_id != model_id_) {
            physics_recalc = true;
        }
    }
    if(scenegraph_ && physics_recalc){
        RemovePhysicsShape();

        if(!attached_) {
            CreatePhysicsShape();
        }
    }
}

void EnvObject::SetCSGModified() {
    csg_modified_ = true;
    RemovePhysicsShape();

    if(!attached_) {
        CreatePhysicsShape();
    }
}

void EnvObject::Reload() {
    if(ofr->color_tint !=  base_color_tint){
        base_color_tint = ofr->color_tint;
    }
}

std::string EnvObject::GetLabel() {
    return ofr->label;
}

const vec3 & EnvObject::GetColorTint() {
    return color_tint_component_.tint_;
}

const float& EnvObject::GetOverbright() {
    return color_tint_component_.overbright_;
}

void EnvObject::GetDesc(EntityDescription &desc) const {
    if(!attached_ || (attached_ && parent && parent->IsGroupDerived())){
        // Save everything if part of a group
        Object::GetDesc(desc);
    } else {
        // Skip transform info if object is attached to character
        desc.AddString(      EDF_NAME,          GetName());
        desc.AddVec3(        EDF_SCALE,         GetScale());
        desc.AddEntityType(  EDF_ENTITY_TYPE,   GetType());
        desc.AddInt(         EDF_ID,            GetID());
        desc.AddScriptParams(EDF_SCRIPT_PARAMS, sp.GetParameterMap());
    }
    color_tint_component_.AddToDescription(desc);
    desc.AddString(EDF_FILE_PATH, obj_file);
    desc.AddBool(EDF_NO_NAVMESH, no_navmesh);
}

MaterialRef EnvObject::GetMaterial( const vec3 &pos, int* tri ) {
    //return (*Materials::Instance()->ReturnRef(ofr->material_path));
    //TODO: Make sure that the reference is sent up instead, so that the asset isn't dropped.
    return ofr_material;
}

vec3 EnvObject::GetBoundingBoxSize() {
    return box_.dims;
}

HullCacheMap hull_cache;

void EnvObject::CreateBushPhysicsShape() {
    BulletWorld* bw = GetBulletWorld();
    Model &model = Models::Instance()->GetModel(model_id_);
    std::string cache_path;
    {
        std::string texture_short, model_short;
        ShortName(ofr->color_map, texture_short);
        ShortName(ofr->model_name, model_short);
        cache_path = ofr->model_name.substr(0,ofr->model_name.rfind('/')+1)+
            model_short+"_" + texture_short + ".hull";
    }

    HullCache *hc = NULL;
    HullCacheMap::iterator iter = hull_cache.find(cache_path);
    if(iter != hull_cache.end()){
        hc = &(iter->second);
    }

    if(!hc){
        char abs_path[kPathSize];
        bool found_cache = (FindFilePath(cache_path.c_str(), abs_path, kPathSize, kDataPaths|kModPaths|kWriteDir|kModWriteDirs, false) != -1);
        if(found_cache){
            FILE *file = my_fopen(abs_path,"rb");
            char version;
            fread(&version, sizeof(char), 1, file);
            if(version != _hull_cache_version){
                found_cache = false;
            } else {
                //printf("Loading hull cache file: %s\n", cache_path.c_str());
                hc = &hull_cache[cache_path];
                std::vector<vec3> &verts = hc->verts;
                std::vector<int> &faces = hc->faces;
                std::vector<vec3> &point_cloud = hc->point_cloud;
                {
                    char num_verts;
                    fread(&num_verts, sizeof(char), 1, file);
                    verts.resize(num_verts);
                    fread(&(verts[0]), sizeof(vec3), num_verts, file);
                }{
                    unsigned char num_face_indices;
                    fread(&num_face_indices, sizeof(unsigned char), 1, file);
                    faces.resize(num_face_indices);
                    fread(&(faces[0]), sizeof(int), num_face_indices, file);
                }{
                    int num_points;
                    fread(&num_points, sizeof(int), 1, file);
                    point_cloud.resize(num_points);
                    fread(&(point_cloud[0]), sizeof(vec3), num_points, file);
                }
                bullet_object_ = bw->CreateConvexObject(verts, faces, true);
            }
            fclose(file);
        }
        if(!found_cache){
            //ImageSamplerRef image = ImageSamplers::Instance()->ReturnRef(ofr->color_map);
            ImageSamplerRef image = Engine::Instance()->GetAssetManager()->LoadSync<ImageSampler>(ofr->color_map);

            hc = &hull_cache[cache_path];
            std::vector<vec3> &points = hc->point_cloud;
            std::vector<vec3> &verts = hc->verts;
            std::vector<int> &faces = hc->faces;
            CreatePointCloud(points, model, image);
            GetSimplifiedHull(points, verts, faces);

            bullet_object_ = bw->CreateConvexObject(verts, faces, true);

            char write_path[kPathSize];
            FormatString(write_path, kPathSize, "%s%s", GetWritePath(modsource_).c_str(), cache_path.c_str());
            FILE *file = my_fopen(write_path, "wb");
            fwrite(&_hull_cache_version, sizeof(char), 1, file);
            {
                char num_verts = verts.size();
                fwrite(&num_verts, sizeof(char), 1, file);
                fwrite(&(verts[0]), sizeof(vec3), num_verts, file);
            }{
                unsigned char num_face_indices = faces.size();
                fwrite(&num_face_indices, sizeof(unsigned char), 1, file);
                fwrite(&(faces[0]), sizeof(int), num_face_indices, file);
            }{
                int num_points = points.size();
                fwrite(&num_points, sizeof(int), 1, file);
                fwrite(&(points[0]), sizeof(vec3), num_points, file);
            }
            fclose(file);
        }
    } else {
        //printf("Instancing loaded hull cache: %s\n", cache_path.c_str());
        bullet_object_ = bw->CreateConvexObject(hc->verts, hc->faces, true);
    }
    if(plant_component_.get()){
        plant_component_->SetHullCache(hc);
    }
}

void EnvObject::ReceiveASVec3Message( int type, const vec3 &vec_a, const vec3 &vec_b ) {
    if(type == _plant_movement_msg && plant_component_.get()){
        const vec3 &pos = vec_a;
        const vec3 &vel = vec_b;
        plant_component_->HandleCollision(pos - GetTranslation(), vel);
        if(update_list_entry == -1){
            update_list_entry = scenegraph_->LinkUpdateObject(this);
        }
    }
}

void PlantComponent::Update(float timestep) {
    angle += ang_vel * timestep;
    if(length(angle) > 0.5f){
        angle = normalize(angle)*0.5f;
    }
    ang_vel -= angle * timestep * 20.0f;
    ang_vel *= 0.99f;
    plant_shake *= 0.99f;
}

PlantComponent::PlantComponent():
    plant_shake(0.0f),
    was_active(false),
    needs_set_inactive(false)
{}

void PlantComponent::HandleCollision( const vec3 &position, const vec3 &velocity ) {
    vec3 impulse = cross(normalize(position), velocity) * 0.01f;
    ang_vel = impulse + ang_vel;
    plant_shake = min(1.0f, plant_shake + length(impulse)*1.0f);
}

mat4 PlantComponent::GetTransform( float scale ) const {
    mat4 rotation;
    float angle_len2 = length_squared(angle);
    if(angle_len2 > 0.00001f){
        float angle_len = sqrtf(angle_len2);
        rotation.SetRotationAxisRad(angle_len / max(1.0f,scale*0.6f), angle/angle_len);
    }
    return rotation;
}

quaternion PlantComponent::GetQuaternion( float scale ) const {
    quaternion result;
    float angle_len2 = length_squared(angle);
    if(angle_len2 > 0.00001f){
        float angle_len = sqrtf(angle_len2);
        result = QuaternionFromAxisAngle(angle / angle_len, angle_len / std::max(1.0f, scale * 0.6f));
    }
    return result;
}

float PlantComponent::GetPlantShake( float scale ) const {
    return plant_shake * scale * 0.03f;
}

vec3 PlantComponent::GetPivot() const {
    return pivot;
}

void PlantComponent::SetPivot( BulletWorld &bw, const vec3 &pos, float radius ) {
    pivot = pos;
    const float _interval = 1.25f;
    float temp_rad = radius / pow(_interval,20.0f);
    vec3 offset = vec3(0.0f);
    while(offset == vec3(0.0f) && temp_rad <= radius){
        temp_rad *= _interval;
        offset = bw.CheckSphereCollisionSlide(pos, temp_rad) - pos;
    }
    if(offset != vec3(0.0f)){
        pivot = pos - normalize(offset) * temp_rad + offset;
    }
}

bool PlantComponent::IsPivotCalculated() const {
    return pivot != vec3(0.0f);
}

void PlantComponent::ClearPivot() {
    pivot = vec3(0.0f);
}

bool PlantComponent::IsActive() {
    static const float _angle_threshold = 0.0001f;
    static const float _plant_shake_threshold = 0.0001f;
    bool active = plant_shake > _plant_shake_threshold ||
                  length_squared(angle) > _angle_threshold;
    if(was_active && !active){
        needs_set_inactive = true;
        plant_shake = 0.0f;
    }
    was_active = active;
    return active;
}

bool PlantComponent::NeedsSetInactive() {
    bool val = needs_set_inactive;
    needs_set_inactive = false;
    return val;
}

HullCache * PlantComponent::GetHullCache() {
    return hull_cache;
}

void PlantComponent::SetHullCache( HullCache * hc ) {
    hull_cache = hc;
}

void EnvObject::CreateLeaf(vec3 pos, vec3 vel, int iterations) {
    if(!plant_component_.get()){
        return;
    }
    const mat4 &transform = transform_;
    HullCache* hc = plant_component_->GetHullCache();
    float closest_dist;
    vec3 closest_point;
    for(int i=0; i<iterations; ++i){
        vec3 point = transform * hc->point_cloud[rand()%(hc->point_cloud.size())];
        float dist = distance_squared(point, pos);
        if(i==0 || dist < closest_dist){
            closest_point = point;
            closest_dist = dist;
        }
    }
    scenegraph_->particle_system->MakeParticle(
        scenegraph_, "Data/Particles/leaf.xml", closest_point, vel, vec3(1.0f));
}

bool EnvObject::SetFromDesc( const EntityDescription& desc ) {
    bool ret = Object::SetFromDesc(desc);
    if( ret ) {
        for(unsigned i=0; i<desc.fields.size(); ++i){
            const EntityDescriptionField& field = desc.fields[i];
            switch(field.type){
                case EDF_FILE_PATH: {
                    std::string type_file;
                    field.ReadString(&type_file);
                    if(!ofr.valid() || ofr->path_ != type_file){
                        if( Load(type_file) == false ) {
                            LOGE << "Failed to load data for EnvObject" << std::endl;
                            ret = false;
                        }
                    }
                    break;}
                case EDF_NO_NAVMESH: {
                    field.ReadBool(&no_navmesh);
                    break;}
            }
        }

        if( ret ) {
            color_tint_component_.SetFromDescription(desc);
            ReceiveObjectMessage(OBJECT_MSG::SET_COLOR, &color_tint_component_.tint_);
            ReceiveObjectMessage(OBJECT_MSG::SET_OVERBRIGHT, &color_tint_component_.overbright_);

            const Model& model = Models::Instance()->GetModel(model_id_);
            vec3 min = model.min_coords;
            vec3 max = model.max_coords;
            box_.center = model.center_coords;
            box_.dims = max-min;
        }
    }
    return ret;
}

void EnvObject::ReceiveObjectMessageVAList( OBJECT_MSG::Type type, va_list args ) {
    switch(type){
    case OBJECT_MSG::SET_COLOR: {
        //vec3 old_color = color_tint_component_.temp_tint();
        color_tint_component_.ReceiveObjectMessageVAList(type, args);
        for(int i=0, len=detail_object_surfaces.size(); i<len; ++i){
            detail_object_surfaces[i]->SetColorTint(GetDisplayTint());
        }
        CalculateDisplayTint_();
        break;}
    case OBJECT_MSG::SET_OVERBRIGHT: {
        color_tint_component_.ReceiveObjectMessageVAList(type, args);
        for(int i=0, len=detail_object_surfaces.size(); i<len; ++i){
            detail_object_surfaces[i]->SetOverbright(GetOverbright());
        }
        CalculateDisplayTint_();
        break;}
    default:
        Object::ReceiveObjectMessageVAList(type, args);
        break;
    }
}

void EnvObject::CalculateDisplayTint_()
{
    cached_combined_tint_ = color_tint_component_.tint_ * (1.0f + color_tint_component_.overbright_ * 0.3f) * base_color_tint;
}

vec3 EnvObject::GetDisplayTint() {
    return cached_combined_tint_;
}

static int ASGetCollisionFace(EnvObject* eo, int index) {
    LOG_ASSERT( index >= 0 );
    if(eo->GetCollisionModelID() == -1 || eo->ofr->no_collision){
        return -1;
    } else {
        Model* model = &Models::Instance()->GetModel(eo->GetCollisionModelID());
        if(!model || (int)model->faces.size() <= index){
            return -1;
        } else {
            return model->faces[index];
        }
    }
}

static vec3 ASGetCollisionVertex(EnvObject* eo, int index) {
    LOG_ASSERT( index >= 0 );
    if(eo->GetCollisionModelID() == -1 || eo->ofr->no_collision){
        return vec3(0.0);
    } else {
        Model* model = &Models::Instance()->GetModel(eo->GetCollisionModelID());
        if(!model || (int)model->faces.size() <= index){
            return vec3(0.0);
        } else {
            vec3 vec;
            memcpy (&vec, &model->vertices[index*3], sizeof(vec3));
            return vec;
        }
    }
}

static int ASGetLedgeLine(EnvObject* eo, int index) {
    return eo->ledge_lines[index];
}

static int ASGetNumLedgeLines(EnvObject* eo) {
    return eo->ledge_lines.size();
}

void DefineEnvObjectTypePublic(ASContext* as_context) {
    as_context->RegisterObjectType("EnvObject", 0, asOBJ_REF | asOBJ_NOCOUNT);
    as_context->RegisterObjectMethod("EnvObject",
        "int GetID()",
        asMETHOD(EnvObject, GetID), asCALL_THISCALL);
    as_context->RegisterObjectMethod("EnvObject",
        "void CreateLeaf(vec3 target_position, vec3 velocity, int iterations)",
        asMETHOD(EnvObject, CreateLeaf), asCALL_THISCALL, "Spawn leaf particles from random point on object's surface; more iterations = closer to target_position");
    as_context->RegisterObjectMethod("EnvObject",
        "int GetCollisionFace(int index)",
        asFUNCTION(ASGetCollisionFace), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("EnvObject",
        "vec3 GetCollisionVertex(int index)",
        asFUNCTION(ASGetCollisionVertex), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("EnvObject",
        "int GetNumLedgeLines()",
        asFUNCTION(ASGetNumLedgeLines), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("EnvObject",
        "int GetLedgeLine(int index)",
        asFUNCTION(ASGetLedgeLine), asCALL_CDECL_OBJFIRST);
    as_context->DocsCloseBrace();
}
