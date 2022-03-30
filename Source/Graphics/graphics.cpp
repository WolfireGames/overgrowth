//-----------------------------------------------------------------------------
//           Name: graphics.cpp
//      Developer: Wolfire Games LLC
//    Description: 
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
#include "graphics.h"

#include <Internal/filesystem.h>
#include <Internal/integer.h>
#include <Internal/config.h>
#include <Internal/datemodified.h>
#include <Internal/hardware_specs.h>
#include <Internal/profiler.h>
#include <Internal/common.h>
#include <Internal/detect_settings.h>

#include <Graphics/textures.h>
#include <Graphics/shaders.h>
#include <Graphics/camera.h>
#include <Graphics/models.h>
#include <Graphics/flares.h>

#include <Utility/sdl_util.h>
#include <Utility/assert.h>

#include <Timing/timingevent.h>
#include <Timing/intel_gl_perf.h>

#include <Images/image_export.hpp>
#include <Math/vec3math.h>
#include <Main/scenegraph.h>
#include <Logging/logdata.h>
#include <Objects/decalobject.h>
#include <Main/engine.h>

#include <SDL.h>

#include <memory.h>
#include <cstdio>

#ifdef _WIN32
// Request more powerful graphics card if on a laptop with both integrated and discrete graphics
extern "C" { _declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001; }
extern "C" { _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001; }
#endif

extern SceneLight* primary_light;

#if PLATFORM_MACOSX
    #include <OpenGL/OpenGL.h>
#endif

bool g_level_shadows = true;
bool g_simple_shadows = true;
bool g_detail_objects_reduced = true;
bool g_detail_objects_reduced_dirty = false;
bool g_albedo_only = false;
bool g_disable_fog = false;
bool g_no_reflection_capture = false;
bool g_no_decals = false;
bool g_no_decal_elements = false;
bool g_character_decals_enabled = false;  // Note: This is enabled via 'Custom Shaders' level param. No config value for it
bool g_no_detailmaps = false;
bool g_single_pass_shadow_cascade = false;
bool g_simple_water = false;
bool g_particle_field_simple = false;
bool g_draw_vr = false;
bool g_s3tc_dxt5_support = false;
bool g_s3tc_dxt5_textures = true;
bool g_opengl_callback_error_dialog = true;
bool g_perform_occlusion_query = false;
bool g_gamma_correct_final_output = true;
bool g_attrib_envobj_intancing_support = false;
bool g_attrib_envobj_intancing_enabled = true;
bool g_ubo_batch_multiplier_force_1x = false;

// Variables for turning off features at runtime for testing performance. Not backed by config, so as not to pollute it
bool g_debug_runtime_disable_blood_surface_pre_draw = false;
bool g_debug_runtime_disable_debug_draw = false;
bool g_debug_runtime_disable_debug_ribbon_draw = false;
bool g_debug_runtime_disable_decal_object_draw = false;
bool g_debug_runtime_disable_decal_object_pre_draw_frame = false;
bool g_debug_runtime_disable_detail_object_surface_draw = false;
bool g_debug_runtime_disable_detail_object_surface_pre_draw = false;
bool g_debug_runtime_disable_dynamic_light_object_draw = false;
bool g_debug_runtime_disable_env_object_draw = false;
bool g_debug_runtime_disable_env_object_draw_depth_map = false;
bool g_debug_runtime_disable_env_object_draw_detail_object_instances = false;
bool g_debug_runtime_disable_env_object_draw_instances = false;
bool g_debug_runtime_disable_env_object_draw_instances_transparent = false;
bool g_debug_runtime_disable_env_object_pre_draw_camera = false;
bool g_debug_runtime_disable_flares_draw = false;
bool g_debug_runtime_disable_gpu_particle_field_draw = false;
bool g_debug_runtime_disable_group_pre_draw_camera = false;
bool g_debug_runtime_disable_group_pre_draw_frame = false;
bool g_debug_runtime_disable_hotspot_draw = false;
bool g_debug_runtime_disable_hotspot_pre_draw_frame = false;
bool g_debug_runtime_disable_item_object_draw = false;
bool g_debug_runtime_disable_item_object_draw_depth_map = false;
bool g_debug_runtime_disable_item_object_pre_draw_frame = false;
bool g_debug_runtime_disable_morph_target_pre_draw_camera = false;
bool g_debug_runtime_disable_movement_object_draw = false;
bool g_debug_runtime_disable_movement_object_draw_depth_map = false;
bool g_debug_runtime_disable_movement_object_pre_draw_camera = false;
bool g_debug_runtime_disable_movement_object_pre_draw_frame = false;
bool g_debug_runtime_disable_navmesh_connection_object_draw = false;
bool g_debug_runtime_disable_navmesh_hint_object_draw = false;
bool g_debug_runtime_disable_particle_draw = false;
bool g_debug_runtime_disable_particle_system_draw = false;
bool g_debug_runtime_disable_pathpoint_object_draw = false;
bool g_debug_runtime_disable_placeholder_object_draw = false;
bool g_debug_runtime_disable_reflection_capture_object_draw = false;
bool g_debug_runtime_disable_rigged_object_draw = false;
bool g_debug_runtime_disable_rigged_object_pre_draw_camera = false;
bool g_debug_runtime_disable_rigged_object_pre_draw_frame = false;
bool g_debug_runtime_disable_scene_graph_draw = false;
bool g_debug_runtime_disable_scene_graph_draw_depth_map = false;
bool g_debug_runtime_disable_scene_graph_prepare_lights_and_decals = false;
bool g_debug_runtime_disable_sky_draw = false;
bool g_debug_runtime_disable_terrain_object_draw_depth_map = false;
bool g_debug_runtime_disable_terrain_object_draw_terrain = false;
bool g_debug_runtime_disable_terrain_object_pre_draw_camera = false;


unsigned int Graphics::GetShadowSize(unsigned int target_shadow_size) {
    unsigned int shadow_size = 8;
    while(shadow_size<target_shadow_size) {
        shadow_size *= 2;
    }
    shadow_size /= 2;
    
    shadow_size /= config_.texture_reduction_factor();

    const unsigned int kMaxShadowSize = 1024;
    if(shadow_size>kMaxShadowSize) {
        shadow_size = kMaxShadowSize;
    }
    if(shadow_size<1) { 
        shadow_size = 1;
    }
    return shadow_size;
}

//Clear the framebuffer
void Graphics::Clear(const bool clear_color_b) {
    setDepthWrite(true);

    //Only clear the color buffer if will not be overdrawn anyways
    if(clear_color_b)glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    else glClear( GL_DEPTH_BUFFER_BIT );
}

void Graphics::setPolygonOffset( bool new_polygon_offset ) {
    if(new_polygon_offset == shadow_state_.polygon_offset_enable_) return;
    shadow_state_.polygon_offset_enable_ = new_polygon_offset;
    shadow_state_.polygon_offset_enable_?glEnable(GL_POLYGON_OFFSET_FILL):glDisable(GL_POLYGON_OFFSET_FILL);
}

void Graphics::setSimpleShadows(const bool val) {
    g_simple_shadows = val;
    if(initialized) {
        InitScreen();
    }
}

void Graphics::setSimpleWater(const bool val) {
    g_simple_water = val || g_draw_vr;
}

void Graphics::SetParticleFieldSimple(bool val) {
    g_particle_field_simple = val;
}

void Graphics::setAttribEnvObjInstancing(bool val) {
    g_attrib_envobj_intancing_enabled = val;
}

void Graphics::PushViewport() {
    ViewportDims dims;
    for(int i=0; i<4; ++i){
        dims.entries[i] = viewport_dim[i];
    }
    viewport_dims_stack.push(dims);
}

void Graphics::PopViewport() {
    LOG_ASSERT(!viewport_dims_stack.empty());
    ViewportDims dims = viewport_dims_stack.top();
    viewport_dims_stack.pop();
    for(int i=0; i<4; ++i){
        viewport_dim[i] = dims.entries[i];
    }
    glScissor(viewport_dim[0], viewport_dim[1], viewport_dim[2], viewport_dim[3]);
    glViewport(viewport_dim[0], viewport_dim[1], viewport_dim[2], viewport_dim[3]);
}

void Graphics::setBlend(const bool new_blend)
{
    GLState &gl_state_ = shadow_state_.gl_state_;
    if(new_blend==gl_state_.blend)return;
    gl_state_.blend=!gl_state_.blend;
    gl_state_.blend?glEnable(GL_BLEND):glDisable(GL_BLEND);
}

void Graphics::setDepthTest(const bool new_depth_test)
{
    GLState &gl_state_ = shadow_state_.gl_state_;
    if(new_depth_test==gl_state_.depth_test)return;
    gl_state_.depth_test=!gl_state_.depth_test;
    gl_state_.depth_test?glEnable(GL_DEPTH_TEST):glDisable(GL_DEPTH_TEST);
}

void Graphics::setCullFace(const bool new_cull_face)
{
    GLState &gl_state_ = shadow_state_.gl_state_;
    if(new_cull_face==gl_state_.cull_face)return;
    gl_state_.cull_face=!gl_state_.cull_face;
    gl_state_.cull_face?glEnable(GL_CULL_FACE):glDisable(GL_CULL_FACE);
}

void Graphics::setDepthWrite(const bool new_depth_write)
{
    GLState &gl_state_ = shadow_state_.gl_state_;
    if(new_depth_write==gl_state_.depth_write)return;
    gl_state_.depth_write=!gl_state_.depth_write;
    gl_state_.depth_write?glDepthMask(1):glDepthMask(0);
}

void Graphics::setGLState(const GLState &state) {
    setDepthTest(state.depth_test);
    setCullFace(state.cull_face);
    setBlend(state.blend);
    setDepthWrite(state.depth_write);
    SetBlendFunc(state.blend_src, state.blend_dst, state.blend_alpha_src, state.blend_alpha_dst);
}

void Graphics::RenderFramebufferToTexture(GLuint framebuffer, TextureRef texture_ref) {
    PROFILER_ENTER(g_profiler_ctx, "bindFramebuffer");
    bindFramebuffer(framebuffer);
    PROFILER_LEAVE(g_profiler_ctx);
    PROFILER_ENTER(g_profiler_ctx, "framebufferColorTexture2D");
    framebufferColorTexture2D(texture_ref);
    PROFILER_LEAVE(g_profiler_ctx);
}

void Graphics::StartTextureSpaceRenderingCustom(int x, int y, int width, int height) {
    PushViewport();
    setViewport(x,y,x+width,y+height);
    CHECK_GL_ERROR();
}

void Graphics::EndTextureSpaceRendering() {
    PopViewport();
}

// Framebuffer wrappers
void Graphics::bindRenderbuffer(GLuint rb) {
        CHECK_GL_ERROR();
        glBindRenderbuffer(GL_RENDERBUFFER, rb);
        CHECK_GL_ERROR();
}

void Graphics::PushFramebuffer() {
    fbo_stack.push(curr_framebuffer);
}

void Graphics::PopFramebuffer() {
    curr_framebuffer = fbo_stack.top();
    fbo_stack.pop();
    bindFramebuffer(curr_framebuffer);
}

void Graphics::bindFramebuffer(GLuint fb) {
    curr_framebuffer = fb;
        CHECK_GL_ERROR();
        glBindFramebuffer(GL_FRAMEBUFFER, fb);
        CHECK_GL_ERROR(); 
}

void Graphics::framebufferDepthTexture2D(TextureRef t, int mipmap_level) {
        CHECK_GL_ERROR();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, Textures::Instance()->returnTexture(t), mipmap_level);
        CHECK_GL_ERROR();
}

//#pragma optimize("",off)
void Graphics::framebufferColorTexture2D(TextureRef t, int mipmap_level) {
    /*if(Textures::Instance()->IsCompressed(t)){
        Textures::Instance()->Uncompress(t);
    }*/
    PROFILER_ENTER(g_profiler_ctx, "EnsureInVRAM");
    Textures::Instance()->EnsureInVRAM(t);
    PROFILER_LEAVE(g_profiler_ctx);
        CHECK_GL_ERROR();
        GLuint tex = Textures::Instance()->returnTexture(t);
        LOGS << "Binding gl texture " << tex << " to framebuffer" << std::endl;
        if(!t.valid()) {
            LOGE << "Not valid..." << std::endl;
        }
        //if(!glIsTexture(tex)){
        //    LOGE << "Not a texture..." << std::endl;
        //}
        PROFILER_ENTER(g_profiler_ctx, "glFramebufferTexture2D");
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, mipmap_level);
        PROFILER_LEAVE(g_profiler_ctx);
        CHECK_GL_ERROR();
}
//#pragma optimize("",on)


void Graphics::genRenderbuffers(GLuint *rb) {
        CHECK_GL_ERROR();
        glGenRenderbuffers(1, rb);
        CHECK_GL_ERROR();
}

void Graphics::genFramebuffers (GLuint *fb, const char* human_name) {
        CHECK_GL_ERROR();
        glGenFramebuffers(1, fb);
        framebuffernames[*fb] = std::string(human_name);

        LOGI << "Created framebuffer: " << *fb << " " << framebuffernames[*fb] << std::endl;
        CHECK_GL_ERROR();
}

void Graphics::deleteFramebuffer(GLuint* fb) {
        if(*fb != (GLuint)INVALID_FRAMEBUFFER) {
            CHECK_GL_ERROR();
            glDeleteFramebuffers(1, fb);

            LOGI << "Deleted framebuffer: " << *fb << " " << framebuffernames[*fb] << std::endl;
            CHECK_GL_ERROR();
            *fb = INVALID_FRAMEBUFFER;
        }
}

void Graphics::renderbufferStorage(GLuint res) {
        CHECK_GL_ERROR();
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, res, res);
        CHECK_GL_ERROR();
}

void Graphics::framebufferRenderbuffer(GLuint rb) {
        CHECK_GL_ERROR();
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb);
        CHECK_GL_ERROR();
}

GLenum Graphics::checkFramebufferStatus() {
        return glCheckFramebufferStatus(GL_FRAMEBUFFER);
}

void Graphics::BindVBO(int target, int val) {
    switch(target){
        case GL_ARRAY_BUFFER:
            LOGS << "Binding array VBO " << val << std::endl;;
            BindArrayVBO(val);
            break;
        case GL_ELEMENT_ARRAY_BUFFER:
            LOGS << "Binding element VBO " << val << std::endl;;
            BindElementVBO(val);
            break;
        default:
            // No valid target
            SDL_assert(false);
            break;
    }
}

//Helping function to explicitally unbind a buffer (if it's bound)
//This will prevent the rebind-saving routine from messing up the state.
void Graphics::UnbindVBO( int target, int val )
{
    if( target == GL_ARRAY_BUFFER && vbo_array_bound == val )
    {
        BindVBO( GL_ARRAY_BUFFER, 0 );
    }
    
    if( target == GL_ELEMENT_ARRAY_BUFFER && vbo_element_bound == val )
    {
        BindVBO( GL_ELEMENT_ARRAY_BUFFER, 0 );
    }
}

void Graphics::BindArrayVBO(int val) {
    LOGS << "Current vbo_array_bound " << vbo_array_bound << std::endl; 
    LOGS << "Value " << val << std::endl; 
    if(vbo_array_bound != val){
        vbo_array_bound = val;
        glBindBuffer( GL_ARRAY_BUFFER, val );
    }
    else
    {
        LOGS << "Tried to rebind " << val << std::endl; 
    }
}

void Graphics::BindElementVBO(int val) {
    if(vbo_element_bound != val){
        vbo_element_bound = val;
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, val );
    }
}

//Start the scene drawing by clearing and setting the matrixmode
void Graphics::startDraw(vec2 start, vec2 end, ScreenType screen_type) {
    PROFILER_GPU_ZONE(g_profiler_ctx, "Graphics::startDraw()");
    if(screen_type == kRender){
        setViewport((GLint)((float)render_dims[0] * start[0]), 
                    (GLint)((float)render_dims[1] * start[1]),
                    (GLint)((float)render_dims[0] * end[0]), 
                    (GLint)((float)render_dims[1] * end[1]));
    } else if(screen_type == kWindow){
        setViewport((GLint)((float)window_dims[0] * start[0]), 
                    (GLint)((float)window_dims[1] * start[1]),
                    (GLint)((float)window_dims[0] * end[0]), 
                    (GLint)((float)window_dims[1] * end[1]));
    } else if(screen_type == kTexture){
        setViewport((GLint)((float)start[0]), 
                    (GLint)((float)start[1]),
                    (GLint)((float)end[0]), 
                    (GLint)((float)end[1]));
    }
}

void Graphics::TakeScreenshot() {
    unsigned long width = window_dims[0];
    unsigned long height = window_dims[1];
    
    unsigned long size = width * height;
    unsigned char *pixels = new unsigned char[size * 4];
    ::memset(pixels, 0, size * 4);
    
    //glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, width, height,
                 GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, (GLvoid *)pixels);
    
    if(screenshot_mode == kGameplay){
        int index3=0;
        int index4=0;
        for(unsigned i=0; i<width*height; i++){
            pixels[index3+0] = pixels[index4+0];
            pixels[index3+1] = pixels[index4+1];
            pixels[index3+2] = pixels[index4+2];
            index3 += 3;
            index4 += 4;
        }
        std::string path = ImageExport::FindEmptySequentialFile("Screenshots/OvergrowthScreenshot",".jpg");
        ImageExport::SaveJPEG(path.c_str(), pixels, width, height);
    } else if(screenshot_mode == kTransparentGameplay){
        std::string path = ImageExport::FindEmptySequentialFile("Screenshots/OvergrowthScreenshot",".png");
        ImageExport::SavePNGTransparent(path.c_str(), pixels, width, height);
    }

    delete [] pixels;
}

//End the scene drawing by drawing the backbuffer to the screen
void Graphics::SwapToScreen() {
    LOG_ASSERT(viewport_dims_stack.empty());
    PROFILER_GPU_ZONE(g_profiler_ctx, "Graphics::SwapToScreen()");    //glFlush(); Unnecessary on Mac OS X, as CGLFlushDrawable auto-flushes the command buffer
    {
        PROFILER_ZONE_STALL(g_profiler_ctx,"SDL_GL_SwapWindow");
        SDL_GL_SwapWindow(sdl_window_);
    }
	GL_SWAP();

    if(queued_screenshot == 2) {
        TakeScreenshot();
        queued_screenshot = 0;
        SetMediaMode(pre_screenshot_media_mode_state);
    }
    
    if(queued_screenshot){
        ++queued_screenshot;
    }

    settings_changed = false;
}

//Set area of screen to draw to
void Graphics::setViewport(const GLint startx, const GLint starty, const GLint endx, const GLint endy) {
    viewport_dim[0] = startx;
    viewport_dim[1] = starty;
    viewport_dim[2] = endx-startx;
    viewport_dim[3] = endy-starty;
	glScissor(viewport_dim[0], viewport_dim[1], viewport_dim[2], viewport_dim[3]);
    glViewport(viewport_dim[0], viewport_dim[1], viewport_dim[2], viewport_dim[3]);
}

void Graphics::setAdditiveBlend(const bool additive)
{
    if(additive)SetBlendFunc(GL_SRC_ALPHA,GL_ONE);
    else SetBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}

void Graphics::SetBlendFunc(const int src, const int dst, const int alpha_src, const int alpha_dst)
{
    GLState &gl_state_ = shadow_state_.gl_state_;
    if(src != gl_state_.blend_src || dst != gl_state_.blend_dst || alpha_src != gl_state_.blend_alpha_src || alpha_dst != gl_state_.blend_alpha_dst) {
        //glBlendFunc(src,dst);
        glBlendFuncSeparate(src, dst, alpha_src, alpha_dst);
        gl_state_.blend_src = src;
        gl_state_.blend_dst = dst;
        gl_state_.blend_alpha_src = alpha_src;
        gl_state_.blend_alpha_dst = alpha_dst;
    }
}

void Graphics::setDepthFunc(const int type) {

    if (shadow_state_.depth_func_unset_||type!=shadow_state_.depth_func_type_) {
        glDepthFunc(type);
        shadow_state_.depth_func_type_ = type;
        shadow_state_.depth_func_unset_ = false;
    }
}

Graphics::Graphics():
    gl_lib_loaded_(false),
    depth_prepass(true),
    shadow_size(20),
    cascade_shadow_res(4096),
    shadow_res(1024*2),
    vbo_array_bound(-1),
    vbo_element_bound(-1),
    multisample_framebuffer_exists(false),
    framebuffer_exists(false),
    old_FSAA_samples(0),
    old_post_effects_enable(false),
    first_load(true),
    initialized(false),
    settings_changed(false),
    sdl_window_(NULL),
    gl_context(NULL),
    active_vertex_attrib_arrays(0),
    wanted_vertex_attrib_arrays(0),
    max_vertex_attrib_arrays(0),
    cascade_shadow_fb((GLuint)INVALID_FRAMEBUFFER),
    static_shadow_fb((GLuint)INVALID_FRAMEBUFFER),
    cascade_shadow_color_fb((GLuint)INVALID_FRAMEBUFFER),
    shader("post")
{
    old_render_dims[0] = 0;
    old_render_dims[1] = 0;
	config_.SetDetailObjectDecals(true);
	config_.SetDetailObjectShadows(true);
	config_.SetDetailObjectLowres(false);
    config_.SetSimpleFog(false);
    config_.SetSplitScreen(false);
    config_.SetFSAASamples(0);
    config_.SetAnisotropy(0);
    config_.SetTargetMonitor(0);
    config_.SetFullscreen(FullscreenMode::kWindowed);
    config_.SetVSync(false);
    config_.SetLimitFpsInGame(false);
    config_.SetMaxFrameRate(60);
    config_.dynamic_character_cubemap_ = false;
    shadow_state_.depth_func_unset_ = true;
    media_mode_ = false;
    memset(shadow_state_.client_states_,0,sizeof(shadow_state_.client_states_));
    shadow_state_.client_active_textures_ = 0;
}

void Graphics::Initialize()
{
    initialized = true;
}

void Graphics::GetShaderNames(std::map<std::string, int>& preload_shaders) {
    preload_shaders[shader] = 0;
}

void Graphics::WindowResized( ivec2 value ) {
    if(config_.full_screen() == FullscreenMode::kWindowed) {
        CheckForWindowResize();
    }
}

void Graphics::Dispose() {
    GL_PERF_FINALIZE();

    static_shadow_depth_ref.clear();
    cascade_shadow_depth_ref.clear();
    cascade_shadow_color_ref.clear();

    screen_color_tex.clear();
    screen_vel_tex.clear();
    screen_depth_tex.clear();

    post_effects.temp_screen_tex.clear();
    post_effects.tone_mapped_tex.clear();

    if (gl_context) {
        SDL_GL_DeleteContext(gl_context);
        gl_context = NULL;
    }

    if( sdl_window_ )
    {
        SDL_DestroyWindow(sdl_window_);
        sdl_window_ = NULL;
    }
}

bool CheckGLError(int line, const char* file, const char* exterrmsg) {
    GLenum errCode;
    const char *errString;
    errCode = glGetError();
    if (errCode != GL_NO_ERROR) {
        errString = (const char*)gluErrorString(errCode);
        if(errCode == 0x0506) {
            errString = "Invalid framebuffer operation";    
        }
        char error_msg[1024];
        int i = 0;
        int last_slash = 0;
        while(file[i] != '\0') {
            if(file[i] == '\\' || file[i] == '/') last_slash = i+1;
            i++;
        }
        if( exterrmsg ) {
            FormatString(error_msg, 1024, "%s\n%s", exterrmsg, errString);
        } else {
            FormatString(error_msg, 1024, "On line %d of %s: \n%s", line, &file[last_slash], errString);
        }
        DisplayError("OpenGL error", error_msg);
        return true;
    } else {
        return false;
    }
}

bool CheckGLErrorStr(char* output, unsigned length) {
    GLenum errCode;
    const char *errString;
    errCode = glGetError();
    if (errCode != GL_NO_ERROR) {
        errString = (const char*)gluErrorString(errCode);
        if(errCode == 0x0506) {
            errString = "Invalid framebuffer operation";    
        }
        FormatString(output,length,"%s",errString);
        return true;
    }
    return false;
}

void CheckFBOError(int line, const char* file) {
    if(!glCheckFramebufferStatus){
        return;
    }
    GLenum errCode;
    const int ERR_STRING_BUF_SIZE = 256;
    char errString[ERR_STRING_BUF_SIZE];
    errCode = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (errCode != GL_FRAMEBUFFER_COMPLETE) {
        switch (errCode) { 
            case GL_FRAMEBUFFER_UNSUPPORTED:
                FormatString(errString, ERR_STRING_BUF_SIZE, "Framebuffer - unsupported");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                FormatString(errString, ERR_STRING_BUF_SIZE, "Framebuffer - incomplete attachment");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                FormatString(errString, ERR_STRING_BUF_SIZE, "Framebuffer - missing attachment");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
                FormatString(errString, ERR_STRING_BUF_SIZE, "Framebuffer - incomplete dimensions");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
                FormatString(errString, ERR_STRING_BUF_SIZE, "Framebuffer - incomplete formats");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                FormatString(errString, ERR_STRING_BUF_SIZE, "Framebuffer - incomplete draw buffer");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                FormatString(errString, ERR_STRING_BUF_SIZE, "Framebuffer - incomplete read buffer");
                break;
            default:
                FormatString(errString, ERR_STRING_BUF_SIZE, "Framebuffer error");
                break;
        }
        char error_msg[1024];
        int i = 0;
        int last_slash = 0;
        while(file[i] != '\0') {
            if(file[i] == '\\' || file[i] == '/') last_slash = i+1;
            i++;
        }
        FormatString(error_msg, 1024, "On line %d of %s: \n%s", line, &file[last_slash], errString);
        DisplayError("OpenGL error", error_msg);
    }
}

void Graphics::BlitDepthBuffer() {
    Shaders::Instance()->setProgram(Shaders::Instance()->returnProgram(shader));
    GLState state;
    state.depth_test = true;
    state.depth_write = true;
    state.blend = false;
    state.cull_face = false;
    setGLState(state);
    // The above code is here to fix a mysterious problem
    GLint depth_attachment_object_type[2];
    GLint depth_attachment_object_name[2];
    bindFramebuffer(multisample_framebuffer);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &depth_attachment_object_type[0]);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depth_attachment_object_name[0]);
    bindFramebuffer(framebuffer);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &depth_attachment_object_type[1]);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depth_attachment_object_name[1]);
    CHECK_FBO_ERROR();
    CHECK_GL_ERROR();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, multisample_framebuffer);
    CHECK_FBO_ERROR();
    CHECK_GL_ERROR();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
    CHECK_FBO_ERROR();
    CHECK_GL_ERROR();
    glBlitFramebuffer(0, 0, render_dims[0], render_dims[1], 0, 0, render_dims[0], render_dims[1], GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    CHECK_FBO_ERROR();
    CHECK_GL_ERROR();
    glBindFramebuffer(GL_FRAMEBUFFER, curr_framebuffer);
    CHECK_FBO_ERROR();
    CHECK_GL_ERROR();
}

static void ApplyVsync(bool val) {
    // Set VSync
    if (val) {
        /* try late-swap-tearing first. If not supported, try normal vsync. */
        if (SDL_GL_SetSwapInterval(-1) == -1) {
            SDL_GL_SetSwapInterval(1);
        }
    } else {
        SDL_GL_SetSwapInterval(0);  /* disable vsync. */
    }
}

void Graphics::SetAnisotropy(float val){
    if(GLEW_EXT_texture_filter_anisotropic){
        static float max_anisotropy = -1.0f;
        if(max_anisotropy == -1.0f){
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy);
        }
        config_.SetAnisotropy(min(val,max_anisotropy));
		if(config_.anisotropy() != 0.0f){
	        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, config_.anisotropy());
		}
	} else {
        config_.SetAnisotropy(0.0f);
    }
}

static void SDL_GL_SetAttributeErrCheck(SDL_GLattr attr, int value) {
    if(SDL_GL_SetAttribute(attr, value) != 0){
        FatalError("Error", "Could not set SDL_GL_attribute %d", (int)attr);
    }
}

void GLAPIENTRY amd_debug_callback(GLuint id, GLenum category, GLenum severity, GLsizei length, const GLchar* message, void* userParam) {
	LOGW << message << std::endl;
	if(g_opengl_callback_error_dialog) {
		DisplayError("GL debug error", message);
	}
}

const char* arb_debug_enum_string(GLenum val){
    switch(val){
        case GL_DEBUG_SOURCE_API_ARB: return "Source: API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB: return "Source: Window System";
        case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: return "Source: Shader Compiler";
        case GL_DEBUG_SOURCE_THIRD_PARTY_ARB: return "Source: Third Party";
        case GL_DEBUG_SOURCE_APPLICATION_ARB: return "Source: Application";
        case GL_DEBUG_SOURCE_OTHER_ARB: return "Source: Other";
        case GL_DEBUG_TYPE_ERROR_ARB: return "Type: Error";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: return "Type: Deprecated Behavior";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: return "Type: Undefined Behavior";
        case GL_DEBUG_TYPE_PORTABILITY_ARB: return "Type: Portability";
        case GL_DEBUG_TYPE_PERFORMANCE_ARB: return "Type: Performance";
        case GL_DEBUG_TYPE_OTHER_ARB: return "Type: Other";
        case GL_DEBUG_SEVERITY_HIGH_ARB: return "Severity: High";
        case GL_DEBUG_SEVERITY_MEDIUM_ARB: return "Severity: Medium";
        case GL_DEBUG_SEVERITY_LOW_ARB: return "Severity: Low";
        default: return "Unknown ARB Debug Enum";
    }
}

void GLAPIENTRY arb_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    const int kBufSize = 1024;
    char buf[kBufSize];
    FormatString(buf, kBufSize, "%s\n%s\nID: %d\n%s\n%s", 
        arb_debug_enum_string(source), arb_debug_enum_string(type), 
        id, arb_debug_enum_string(severity), message);

    if(source == GL_DEBUG_SOURCE_API_ARB && type == GL_DEBUG_TYPE_OTHER_ARB && id == 131185){
        //LOGD << buf << std::endl;
        return; // Ignore NVIDIA notifications that VBO will use Video RAM as source
    }
    if(source == GL_DEBUG_SOURCE_API_ARB && id == 131186){
        //LOGD << buf << std::endl;
        return; // Ignore NVIDIA notifications that VBO will use Video RAM as source
    }
    if(source == GL_DEBUG_SOURCE_API_ARB && type == GL_DEBUG_TYPE_PERFORMANCE_ARB && id == 131218){
        LOGD << buf << std::endl;
        return; // Ignore NVIDIA notifications that shader is going to be recompiled 
        // because the shader key based on GL state mismatches, because I don't
        // know what to do about that right now
        // TODO: fix this and turn the warning back on
    }
    if(source == GL_DEBUG_SOURCE_API_ARB && type == GL_DEBUG_TYPE_PERFORMANCE_ARB && id == 2){
        LOGD << buf << std::endl;
        return; // Same as above but for Intel
    }
    if(source == GL_DEBUG_SOURCE_API_ARB && type == GL_DEBUG_TYPE_PERFORMANCE_ARB && id == 131154){
        LOGD << buf << std::endl;
        return; // Ignore NVIDIA warning that pixel transfer is synchronized with 3d rendering
    }

    LOGW << buf << std::endl;
	if(g_opengl_callback_error_dialog) {
		DisplayError("GL debug error", buf);
	}
}

extern float last_shadow_update_time;

//Set up the screen
void Graphics::InitScreen(){
    PROFILER_ZONE(g_profiler_ctx, "Graphics::InitScreen");
    Textures* textures = Textures::Instance();
    Shaders* shaders = Shaders::Instance();
    if(first_load){
        PROFILER_ENTER(g_profiler_ctx, "Setting up driver and window");
        int n_video_driver = SDL_GetNumVideoDrivers();
        LOGI << "There are " << n_video_driver << " available video drivers." << std::endl;
        LOGI << "Video drivers are following:" << std::endl;
        
        for( int i = 0; i < n_video_driver; i++ )
        {
            LOGI << "Video driver " << i << ": " << SDL_GetVideoDriver(i) << std::endl;
        }

        // Initialize default video driver
        if (SDL_VideoInit(NULL) < 0) {
            FatalError("Error", "Could not initialize default video driver");
        } else {
            LOGI << "Initialized video driver: " << SDL_GetCurrentVideoDriver() << std::endl;
        }   
        //Set GL attributes
        SDL_GL_SetAttributeErrCheck( SDL_GL_RED_SIZE, 8 );
        SDL_GL_SetAttributeErrCheck( SDL_GL_GREEN_SIZE, 8 );
        SDL_GL_SetAttributeErrCheck( SDL_GL_BLUE_SIZE, 8 );
        SDL_GL_SetAttributeErrCheck( SDL_GL_ALPHA_SIZE, 8 );
        SDL_GL_SetAttributeErrCheck( SDL_GL_DEPTH_SIZE, 24 );
        SDL_GL_SetAttributeErrCheck( SDL_GL_DOUBLEBUFFER, 1 );
        SDL_GL_SetAttributeErrCheck( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttributeErrCheck( SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttributeErrCheck( SDL_GL_CONTEXT_MINOR_VERSION, 2);
		SDL_GL_SetAttributeErrCheck( SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
#ifndef NO_GL_ERROR_CHECKING
        SDL_GL_SetAttributeErrCheck( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
        // MSAA is handled later with renderbuffers and fbo
        SDL_GL_SetAttributeErrCheck( SDL_GL_MULTISAMPLEBUFFERS, false);
        SDL_GL_SetAttributeErrCheck( SDL_GL_MULTISAMPLESAMPLES, 0);

        LOGI << "Attribute group 1 set" << std::endl;

        // Create window
        const char* window_title = "Overgrowth";
        uint32_t window_flags = SDL_WINDOW_OPENGL;

        int target_monitor = config_.target_monitor();
        if(target_monitor >= config.GetMonitorCount()) {
            target_monitor = config.GetMonitorCount() - 1;
            config.GetRef("target_monitor") = target_monitor;
        }

        currentFullscreenType = config_.full_screen();
        switch (config_.full_screen()) {
            case FullscreenMode::kWindowed: {
                window_flags |= SDL_WINDOW_RESIZABLE;

                window_dims[0] = config_.screen_width();
                window_dims[1] = config_.screen_height();
                break;
            }
            case FullscreenMode::kFullscreen: {
                window_flags |= SDL_WINDOW_FULLSCREEN;

                window_dims[0] = config_.screen_width();
                window_dims[1] = config_.screen_height();
                break;
            }
            case FullscreenMode::kWindowed_borderless: {
                window_flags |= SDL_WINDOW_BORDERLESS;

                window_dims[0] = config_.screen_width();
                window_dims[1] = config_.screen_height();
                break;
            }
            case FullscreenMode::kFullscreen_borderless: {
                window_flags |= SDL_WINDOW_BORDERLESS;

                SDL_DisplayMode displayMode;
                SDL_GetCurrentDisplayMode(target_monitor, &displayMode);

                window_dims[0] = displayMode.w;
                window_dims[1] = displayMode.h;
            }
        }

        SDL_DisplayMode desktop_mode;
        SDL_GetCurrentDisplayMode(target_monitor, &desktop_mode);

        if (window_dims[0] > desktop_mode.w || window_dims[1] > desktop_mode.h) {
            bool fitsW = false;
            bool fitsH = false;
         
            int displayModeCount = SDL_GetNumDisplayModes(target_monitor);
            for (int i = 0; i < displayModeCount; ++i) {
                SDL_DisplayMode mode;
                SDL_GetDisplayMode(target_monitor, i, &mode);

                if (mode.w >= window_dims[0])
                    fitsW = true;
                if (mode.h >= window_dims[1])
                    fitsH = true;

                if (fitsW && fitsH)
                    break;
            }

            // When DPI scaling is active in windows the window might not fit (says windows)
            // but if it's fullscreen the display settings will be overridden anyway
            if (!fitsW || !fitsH || config_.full_screen() != FullscreenMode::kFullscreen)
            {
                window_dims[0] = std::min(window_dims[0], desktop_mode.w);
                window_dims[1] = std::min(window_dims[1], desktop_mode.h);

                config.GetRef("screenwidth") = window_dims[0];
                config.GetRef("screenheight") = window_dims[1];

                config_.SetScreenWidth(window_dims[0]);
                config_.SetScreenHeight(window_dims[1]);
            }
        }

        sdl_window_ = SDL_CreateWindow(
            window_title, 
            SDL_WINDOWPOS_UNDEFINED_DISPLAY(target_monitor), SDL_WINDOWPOS_UNDEFINED_DISPLAY(target_monitor),
            window_dims[0], window_dims[1],
            window_flags);

        if(!sdl_window_){
            const char* sdl_error = SDL_GetError();
            LOGE << "Unable to create window, sdl reason: " << sdl_error << std::endl;
            if(strcmp("No matching GL pixel format available", sdl_error) == 0) {
                FatalError("Error", "Could not create window. Make sure your computer is set to 32 bit color depth");
            } else {
                FatalError("Error", "Could not create window");
            }

        }    
        LOGI << "Created window" << std::endl;
        SDL_SetWindowMinimumSize(sdl_window_, 640, 480);
        LOGI << "Set minimum size" << std::endl;
        // Set window to be fullscreen (or not)
        SDL_DisplayMode fullscreen_mode;
        SDL_zero(fullscreen_mode);
        fullscreen_mode.format = SDL_PIXELFORMAT_RGB888;
        fullscreen_mode.w = window_dims[0];
        fullscreen_mode.h = window_dims[1];
        SDL_DisplayMode closest_fullscreen_mode;
        SDL_GetClosestDisplayMode(target_monitor, &fullscreen_mode, &closest_fullscreen_mode);
        if(SDL_SetWindowDisplayMode(sdl_window_, &closest_fullscreen_mode) != 0){
            FatalError("Error", "Could not set window display mode");
        }
        LOGI << "Set display mode" << std::endl;

        gl_context = SDL_GL_CreateContext(sdl_window_);
        LOGI << "Created OpenGL Context" << std::endl;
        if (!gl_context) {
            FatalError("Error", "SDL_GL_CreateContext(): %s\n%s", SDL_GetError(), "It's likely your computer can't create an OpenGL 3.2 context. Do you have the latest drivers installed?");
        }
        SDL_GL_GetDrawableSize(sdl_window_, &window_dims[0], &window_dims[1]);
        // Check what GL context we actually received
        int data[5];
        SDL_GL_GetAttribute( SDL_GL_RED_SIZE, &data[0] );
        SDL_GL_GetAttribute( SDL_GL_GREEN_SIZE, &data[1] );
        SDL_GL_GetAttribute( SDL_GL_BLUE_SIZE, &data[2] );
        SDL_GL_GetAttribute( SDL_GL_ALPHA_SIZE, &data[3] );
        SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE, &data[4] );
        LOGI << "RGBA bits, depth: " << data[0] << " " << data[1] << " " << data[2] << " " << data[3] << " " << data[4] << std::endl;
        SDL_GL_GetAttribute( SDL_GL_DOUBLEBUFFER, &data[0]);
        SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &data[1]);
        SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &data[2]);        
        if(!data[0]){
            FatalError("Error","Could not create double-buffered OpenGL context");
        }
        LOGI << "Anti-aliasing samples: " << data[2] << std::endl;
        int sdl_gl_major, sdl_gl_minor, sdl_gl_profile_mask;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &sdl_gl_profile_mask);
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &sdl_gl_major);
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &sdl_gl_minor);

        LOGI << "SDL OpenGL Context Result: " << sdl_gl_major << "." << sdl_gl_minor << " With a " << SDL_GLprofile_string((SDL_GLprofile)sdl_gl_profile_mask) << " profile context" << std::endl; 

        // Initialize GLEW
        GLenum err = glewInit();
		GL_PERF_INIT( );
        if (err != GLEW_OK) {
            FatalError("Error", "GLEW error: %s", glewGetErrorString(err));
        }        
        LOGI << "GLEW Version loaded: " << glewGetString(GLEW_VERSION) << std::endl;

        GLint gl_major = 0, gl_minor = 0;
        if( sdl_gl_major >= 3 ) {
            glGetIntegerv(GL_MAJOR_VERSION, &gl_major);
            glGetIntegerv(GL_MINOR_VERSION, &gl_minor);

            LOGI << "OpenGL Self Reported version string: " << glGetString(GL_VERSION) << std::endl;
            LOGI << "OpenGL Self Reported version values: " << gl_major << "." << gl_minor << std::endl;
        } else if( sdl_gl_major >= 2 ) {
            LOGI << "OpenGL Self Reported version: " << glGetString(GL_VERSION) << std::endl;
        } else {
            LOGI << "OpenGL has no self reporting routine for 1.x contexts." << std::endl;
        }

        if (!GLEW_VERSION_3_2) {
            LOGE << "GLEW claims it doesn't have an OpenGL 3.2 context" << std::endl;
        }

        if( GLEW_VERSION_3_2 || (sdl_gl_major == 3 && sdl_gl_minor >= 2) || sdl_gl_major > 3 ) {
            LOGI << "Context seems acceptable for running the application, continuing." << std::endl;
        } else {
            FatalError("Error", "OpenGL-3.2-compatible graphics drivers not found");
        }

        DetectAndSetOpenGLFeatureRestrictions();

        g_s3tc_dxt5_textures = config["gl_load_s3tc"].toBool() && g_s3tc_dxt5_support;

        if(g_s3tc_dxt5_textures == false) {
            FatalError("Error", "No support for S3TC DXT5 textures detected. This means either your GPU is too old to run the game, or your drivers are out-of-date");
        }

		bool g_opengl_callback_error_dialog = config["opengl_callback_error_dialoge"].toNumber<bool>();

		if(config["opengl_callback_errors"].toNumber<bool>()) {
			LOGI << "Activating OpenGL callback errors, [opengl_callback_errors]" << std::endl;
			if(GLEW_ARB_debug_output){
				glDebugMessageCallbackARB(&arb_debug_callback, NULL);
				glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			} else if(GLEW_AMD_debug_output){
				glDebugMessageEnableAMD(0, 0, 0, NULL, true);
				glDebugMessageCallbackAMD(&amd_debug_callback, NULL);
                // GL_DEBUG_OUTPUT_SYNCHRONOUS is part of the KHR and ARB
                // extension, but not the AMD one
				//glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			}
		} else {
			LOGI << "Deactivating OpenGL callback errors, [opengl_callback_errors]" << std::endl;
		}

        while(glGetError() != GL_NO_ERROR) {
            // Throw away GL errors from SDL/GLEW init
        }
        CHECK_GL_ERROR();
        // Determine what framebuffer features are supported
        if (GLEW_VERSION_3_0 || GLEW_EXT_framebuffer_object || GLEW_ARB_framebuffer_object) {
        } else {
            FatalError("Error", "No support for Framebuffer Objects detected");
        }
        features_.SetFrameBufferFSAAEnabled(GLEW_EXT_framebuffer_multisample!=0);
        CHECK_GL_ERROR();
        // Determine what instanced array features are supported
        if (GLEW_VERSION_3_3 || GLEW_ARB_instanced_arrays) {
            g_attrib_envobj_intancing_support = true;
        } else {
            g_attrib_envobj_intancing_support = false;
        }
        ApplyVsync(config_.vSync());
        // Clear screen
        glClearColor( 0.5f, 0.5f, 0.5f, 0 );
        setViewport( 0, 0, window_dims[0], window_dims[1] );
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        SDL_GL_SwapWindow(sdl_window_);
        CHECK_GL_ERROR();
        PROFILER_LEAVE(g_profiler_ctx);
        
        textures->ResetVRAM();
        shaders->ResetVRAM(); 
        CHECK_GL_ERROR();

        vendor = GetGLVendor();
        SetAnisotropy(config_.anisotropy());
  
        features_.SetHDREnable( GLEW_VERSION_3_0 || GLEW_ARB_texture_float );

        // Set initial GL state so it is in sync with shadow state
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        shadow_state_.gl_state_.depth_test = false;
        shadow_state_.gl_state_.cull_face = false;
        shadow_state_.gl_state_.blend = false;
        shadow_state_.gl_state_.blend_src = GL_SRC_ALPHA;
        shadow_state_.gl_state_.blend_dst = GL_ONE_MINUS_SRC_ALPHA;
        shadow_state_.gl_state_.blend_alpha_src = GL_ONE;
        shadow_state_.gl_state_.blend_alpha_dst = GL_ONE_MINUS_SRC_ALPHA;
        shadow_state_.polygon_offset_enable_ = false;

        curr_framebuffer = 0;

        drawing_shadow = false;

        line_width = -1;

        // Warn if not enough shader varyings are supported
        #ifndef __APPLE__ // Why does this not work on Mac OS 10.7.5 core profile?
        int assumed_floats = 32;
        int max_varying_floats;
        glGetIntegerv( GL_MAX_VARYING_FLOATS, &max_varying_floats );
        if(max_varying_floats < assumed_floats) {
            std::ostringstream oss;
            oss << "Only " << max_varying_floats << " varying floats supported, "
                << "up to " << assumed_floats << " are used in shaders.";
            DisplayError("Warning",oss.str().c_str());
        }
        #endif

        textures->setWrap(GL_CLAMP_TO_EDGE); 
        CHECK_GL_ERROR();

        int w = config_.screen_width();
        int h = config_.screen_height();
        SetUpWindowDims(w, h);

        PROFILER_ENTER(g_profiler_ctx, "Initializing decaltextures");
        DecalTextures::Instance()->Init();
        PROFILER_LEAVE(g_profiler_ctx);

        if(GLEW_ARB_tessellation_shader){
            glPatchParameteri( GL_PATCH_VERTICES, 3 );
        }
    }
    
	last_shadow_update_time = 0.0f;

    use_sample_alpha_to_coverage = true;
    if(config_.FSAA_samples()<2){
        use_sample_alpha_to_coverage = false;
    }
    
    if (render_dims[0] != old_render_dims[0] || render_dims[1] != old_render_dims[1] || config_.FSAA_samples() != old_FSAA_samples) {
        PROFILER_GPU_ZONE(g_profiler_ctx, "Setting up framebuffers for post effects");
        PushFramebuffer();
        
        if(multisample_framebuffer_exists){
            deleteFramebuffer(&multisample_framebuffer);
            glDeleteRenderbuffers(1, &multisample_color);
            glDeleteRenderbuffers(1, &multisample_depth);
            glDeleteRenderbuffers(1, &multisample_vel);
            deleteFramebuffer(&multisample_framebuffer);
            multisample_framebuffer_exists = false;
        }
        if (features_.frame_buffer_fsaa_enabled() && config_.FSAA_samples() > 1) {
            CHECK_GL_ERROR();
            genFramebuffers(&multisample_framebuffer, "multisample");
            bindFramebuffer(multisample_framebuffer);

            CHECK_GL_ERROR();
            int samples = config_.FSAA_samples();
            glGenRenderbuffers(1, &multisample_color);
            CHECK_GL_ERROR();
            glBindRenderbuffer(GL_RENDERBUFFER, multisample_color);
            CHECK_GL_ERROR();
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, features_.HDR_enable()?GL_RGBA16F:GL_RGBA, render_dims[0], render_dims[1]);
            CHECK_GL_ERROR();
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, multisample_color);
            CHECK_GL_ERROR();

            glGenRenderbuffers(1, &multisample_vel);
            CHECK_GL_ERROR();
            glBindRenderbuffer(GL_RENDERBUFFER, multisample_vel);
            CHECK_GL_ERROR();
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, features_.HDR_enable()?GL_RGBA16F:GL_RGBA, render_dims[0], render_dims[1]);
            CHECK_GL_ERROR();
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_RENDERBUFFER, multisample_vel);
            CHECK_GL_ERROR();

            glGenRenderbuffers( 1, &multisample_depth );
            glBindRenderbuffer( GL_RENDERBUFFER, multisample_depth );
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, render_dims[0], render_dims[1]);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, multisample_depth);
            CHECK_GL_ERROR();
            multisample_framebuffer_exists = true;
        }
        if(framebuffer_exists){
            deleteFramebuffer(&framebuffer);
            screen_color_tex.clear();
            screen_depth_tex.clear();
            deleteFramebuffer(&post_effects.post_framebuffer);
            post_effects.temp_screen_tex.clear();
            post_effects.tone_mapped_tex.clear();
            textures->DeleteUnusedTextures();
        }

        genFramebuffers(&framebuffer, "generic");
        bindFramebuffer(framebuffer);

        screen_color_tex = textures->makeRectangularTexture(render_dims[0],render_dims[1],features_.HDR_enable()?GL_RGBA16F:GL_RGBA,GL_RGBA);
        textures->SetTextureName(screen_color_tex, "Post::Screen Color");
        screen_vel_tex = textures->makeRectangularTexture(render_dims[0],render_dims[1],features_.HDR_enable()?GL_RGBA16F:GL_RGBA,GL_RGBA);
        textures->SetTextureName(screen_vel_tex, "Post::Screen Velocity");
        screen_depth_tex = textures->makeRectangularTexture(render_dims[0],render_dims[1],GL_DEPTH_COMPONENT24,GL_DEPTH_COMPONENT);
        textures->SetTextureName(screen_depth_tex, "Post::Screen Depth");

        genFramebuffers(&post_effects.post_framebuffer, "post_effects");
        post_effects.temp_screen_tex = textures->makeRectangularTexture(
            render_dims[0],render_dims[1],
            features().HDR_enable()?GL_RGBA16F:GL_RGBA,GL_RGBA);
        textures->SetTextureName(post_effects.temp_screen_tex, "Post::Screen Color - Temp");

        post_effects.tone_mapped_tex = textures->makeRectangularTexture(
            render_dims[0],render_dims[1],
            features().HDR_enable()?GL_RGBA16F:GL_RGBA,GL_RGBA);
        textures->SetTextureName(post_effects.tone_mapped_tex, "Post::Tone-Mapped");

        framebufferColorTexture2D(screen_color_tex);
        {
            TextureRef t = screen_vel_tex;
            Textures::Instance()->EnsureInVRAM(t);
                CHECK_GL_ERROR();
                GLuint tex = Textures::Instance()->returnTexture(t);
                LOGS << "Binding gl texture " << tex << " to framebuffer" << std::endl;
                if(!t.valid()) {
                    LOGE << "Not valid..." << std::endl;
                }
                if(!glIsTexture(tex)){
                    LOGE << "Not a texture..." << std::endl;
                }
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, tex, 0);
        }
        CHECK_FBO_ERROR();
        framebufferDepthTexture2D(screen_depth_tex);
        CHECK_FBO_ERROR();
        framebuffer_exists = true;

        PopFramebuffer();
    }

    SetUpShadowTextures();

    if(first_load){
        textures->setWrap(GL_REPEAT);
        noise_ref = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/noise.tga", PX_NOREDUCE | PX_NOCONVERT, 0x0);
        pure_noise_ref = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/purenoise_nocompress.tga", PX_NOREDUCE | PX_NOCONVERT, 0x0);

        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,&max_texture_units);
        LOGI << max_texture_units << " texture units supported." << std::endl;
        if(max_texture_units<16){
            DisplayError("Warning","Fewer than 16 texture units supported!");
        }
                
    #ifdef __APPLE__
        // Mac multithreaded OpenGL?
        /*CGLContextObj cctx = CGLGetCurrentContext();
        CGLError err = CGLEnable( cctx, kCGLCEMPEngine);
        if(err != kCGLNoError){
            DisplayError("Error","Problem initializing multi-threaded OpenGL");
        }*/
    #endif

        GLint param;
        glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &param);
        LOGI << "GL_MAX_VERTEX_UNIFORM_COMPONENTS: " << param << std::endl;
    }

    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attrib_arrays);
    old_render_dims[0] = render_dims[0];
    old_render_dims[1] = render_dims[1];
    old_FSAA_samples = config_.FSAA_samples();
    first_load = false;
    gl_lib_loaded_ = true;
    settings_changed = true;
    SetSeamlessCubemaps(config_.seamless_cubemaps_);

    if(!glGenVertexArrays){
        FatalError("Error", "No support detected for vertex array objects");
    }
    // Needed for GL 3.2 core
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    CHECK_GL_ERROR();

    if(GLEW_ARB_texture_cube_map_array){
        LOGI << "GLEW_ARB_texture_cube_map_array is available" << std::endl;
    } else {
        LOGI << "GLEW_ARB_texture_cube_map_array is NOT available" << std::endl;
    }

	if( !GLEW_EXT_texture_compression_s3tc ) {
		LOGF << "Missing necessary GL extension: EXT_texture_compression_s3tc" << std::endl;
	}

	if( !GLEW_EXT_texture_sRGB ) {
		LOGF << "Missing necessary GL extension: GLEW_EXT_texture_sRGB" << std::endl;
	}

    //Assume that the bindings in the state aren't valid anymore.
    //There have been indications in the NVIDIA driver on linux that this is
    //the case.
    LOGI << "Unassuming that any EBO or VBO is bound" << std::endl;
    vbo_element_bound = -1;
    vbo_array_bound = -1;
    
}

void Graphics::SetModelMatrix( const mat4& matrix, const mat3& normal_matrix ) {
    Shaders::Instance()->GetCurrentProgram()->SetUniform(MODEL_MATRIX,matrix);
    Shaders::Instance()->GetCurrentProgram()->SetUniform(NORMAL_MATRIX,normal_matrix);
}

struct SimpleLineDrawState {
    GLState state;
    SimpleLineDrawState() {
        state.blend = true;
        state.cull_face = false;
        state.depth_test = true;
        state.depth_write = false;
    }
};

static SimpleLineDrawState simple_line_draw_state;

void Graphics::SetSimpleLineDrawState() {
    setGLState(simple_line_draw_state.state);
    Shaders::Instance()->noProgram();
}

void Graphics::BlitColorBuffer()
{
    CHECK_FBO_ERROR();
    CHECK_GL_ERROR();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, multisample_framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
    glReadBuffer(GL_COLOR_ATTACHMENT0); 
    glDrawBuffer(GL_COLOR_ATTACHMENT0); 
    glBlitFramebuffer(0, 0, render_dims[0], render_dims[1], 0, 0, render_dims[0], render_dims[1], GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glReadBuffer(GL_COLOR_ATTACHMENT1); 
    glDrawBuffer(GL_COLOR_ATTACHMENT1); 
    glBlitFramebuffer(0, 0, render_dims[0], render_dims[1], 0, 0, render_dims[0], render_dims[1], GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, curr_framebuffer);
    CHECK_FBO_ERROR();
    CHECK_GL_ERROR();
}

void Graphics::SetClientActiveTexture(int index) {
    if (shadow_state_.client_active_textures_ != index) {
        glClientActiveTexture(GL_TEXTURE0 + index);
        shadow_state_.client_active_textures_ = index;
    }
}

GLenum GLEnumTransformer(uint32_t e)
{
    switch(e)
    {
        case CS_VERTEX_ARRAY: return GL_VERTEX_ARRAY;
        case CS_NORMAL_ARRAY: return GL_NORMAL_ARRAY;
        case CS_COLOR_ARRAY: return GL_COLOR_ARRAY;
        case CS_TEXTURE_COORD_ARRAY0: return GL_TEXTURE_COORD_ARRAY;
        case CS_TEXTURE_COORD_ARRAY1: return GL_TEXTURE_COORD_ARRAY;
        case CS_TEXTURE_COORD_ARRAY2: return GL_TEXTURE_COORD_ARRAY;
        case CS_TEXTURE_COORD_ARRAY3: return GL_TEXTURE_COORD_ARRAY;
        case CS_TEXTURE_COORD_ARRAY4: return GL_TEXTURE_COORD_ARRAY;
        case CS_TEXTURE_COORD_ARRAY5: return GL_TEXTURE_COORD_ARRAY;
        case CS_TEXTURE_COORD_ARRAY6: return GL_TEXTURE_COORD_ARRAY;
        case CS_TEXTURE_COORD_ARRAY7: return GL_TEXTURE_COORD_ARRAY;
        default: return 0;
    }
}

void Graphics::SetClientStateEnabled(int type, bool enabled) {
    LOG_ASSERT(type < CS_MAX_SHADOWED_CLIENT_STATES);
    if (enabled != shadow_state_.client_states_[type])     {
        if (type >= CS_TEXTURE_COORD_ARRAY0 && type <= CS_TEXTURE_COORD_ARRAY7) {
            SetClientActiveTexture(type - CS_TEXTURE_COORD_ARRAY0);
        }    
        if (enabled) {
            glEnableClientState(GLEnumTransformer(type));
        } else {
            glDisableClientState(GLEnumTransformer(type));
        }
        shadow_state_.client_states_[type] = enabled;
    }
}

void Graphics::SetFromConfig( const Config &config, bool dynamic ) {
    if(dynamic && config_.full_screen() == FullscreenMode::kWindowed){
        config_.SetScreenWidth(config["screenwidth"].toNumber<int>());
        config_.SetScreenHeight(config["screenheight"].toNumber<int>());
    }
    if(!dynamic){
        config_.SetScreenWidth(config["screenwidth"].toNumber<int>());
        config_.SetScreenHeight(config["screenheight"].toNumber<int>());
        config_.SetFullscreen(static_cast<FullscreenMode::Mode>(config["fullscreen"].toNumber<int>()));
        config_.SetTextureReduce(config["texture_reduce"].toNumber<int>());
        config_.SetFSAASamples(config["multisample"].toNumber<int>());
        config_.SetAnisotropy(config["anisotropy"].toNumber<float>());
        config_.SetVSync(config["vsync"].toNumber<bool>());
        config_.SetLimitFpsInGame(config["limit_fps_in_game"].toNumber<bool>());
        config_.SetMaxFrameRate(config["max_frame_rate"].toNumber<int>());
        config_.SetGPUSkinning(config["gpu_skinning"].toNumber<bool>());
        config_.motion_blur_amount_ = config["motion_blur_amount"].toNumber<float>();
        config_.SetSimpleFog(config["simple_fog"].toNumber<bool>());
        config_.SetDetailObjects(config["detail_objects"].toNumber<bool>());
        config_.SetDetailObjectDecals(config["detail_object_decals"].toNumber<bool>());
        config_.SetDetailObjectLowres(config["detail_object_lowres"].toNumber<bool>());
        config_.SetDetailObjectShadows(!config["detail_object_disable_shadows"].toNumber<bool>());
        config_.SetDepthOfField(config["depth_of_field"].toNumber<bool>());
        config_.SetDepthOfFieldReduced(config["depth_of_field_reduced"].toNumber<bool>());
    }
    config_.SetBlood(config["blood"].toNumber<int>());
    config_.SetBloodColor(GraphicsConfig::BloodColorFromString(config["blood_color"].str()));
    config_.SetSplitScreen(config["split_screen"].toNumber<bool>());
    config_.seamless_cubemaps_ = config["seamless_cubemaps"].toNumber<bool>();
    setSimpleShadows(config["simple_shadows"].toNumber<bool>());
    setSimpleWater(config["simple_water"].toNumber<bool>());
    SetParticleFieldSimple(config["particle_field_simple"].toNumber<bool>());
    SetDetailObjectsReduced(config["detail_objects_reduced"].toNumber<bool>());
    g_attrib_envobj_intancing_enabled = config["attrib_envobj_instancing"].toNumber<bool>();
    g_perform_occlusion_query = config["occlusion_query"].toNumber<bool>();
	g_gamma_correct_final_output = config["gamma_correct_final_output"].toNumber<bool>();

    config_.SetTargetMonitor(config["target_monitor"].toNumber<int>());
}

void Graphics::SetWindowGrab( bool val ) {
    if(!val || SDL_GetKeyboardFocus() == sdl_window_){
        SDL_SetWindowGrab(sdl_window_, val?SDL_TRUE:SDL_FALSE);
    }
}

void Graphics::SetClientStates( int flags ) {
    SetClientStateEnabled(CS_VERTEX_ARRAY, (flags & F_VERTEX_ARRAY)!=0);
    SetClientStateEnabled(CS_NORMAL_ARRAY, (flags & F_NORMAL_ARRAY)!=0);
    SetClientStateEnabled(CS_COLOR_ARRAY, (flags & F_COLOR_ARRAY)!=0);
    SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY0, (flags & F_TEXTURE_COORD_ARRAY0)!=0);
    SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY1, (flags & F_TEXTURE_COORD_ARRAY1)!=0);
    SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY2, (flags & F_TEXTURE_COORD_ARRAY2)!=0);
    SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY3, (flags & F_TEXTURE_COORD_ARRAY3)!=0);
    SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY4, (flags & F_TEXTURE_COORD_ARRAY4)!=0);
    SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY5, (flags & F_TEXTURE_COORD_ARRAY5)!=0);
    SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY6, (flags & F_TEXTURE_COORD_ARRAY6)!=0);
    SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY7, (flags & F_TEXTURE_COORD_ARRAY7)!=0);
}


void Graphics::SetLineWidth( int val ) {
    if(line_width != val){
        line_width = val;
        #ifndef PLATFORM_MACOSX
            glLineWidth((float)val);
        #endif
    }
}

void Graphics::ResizeWindow(int& w, int& h) {
    if (config_.full_screen() == FullscreenMode::kFullscreen) {
        SDL_DisplayMode fullscreen_mode;
        SDL_zero(fullscreen_mode);
        fullscreen_mode.format = SDL_PIXELFORMAT_RGB888;
        fullscreen_mode.w = w;
        fullscreen_mode.h = h;
        SDL_DisplayMode closest_fullscreen_mode;
        if (SDL_GetClosestDisplayMode(config_.target_monitor(), &fullscreen_mode, &closest_fullscreen_mode) == NULL) {
            LOGF << SDL_GetError() << std::endl;
        }
        if (SDL_SetWindowDisplayMode(sdl_window_, &closest_fullscreen_mode) != 0)
            LOGF << SDL_GetError() << std::endl;

        SDL_SetWindowSize(sdl_window_, closest_fullscreen_mode.w, closest_fullscreen_mode.h);

        w = closest_fullscreen_mode.w;
        h = closest_fullscreen_mode.h;
    }
    else
        SDL_SetWindowSize(sdl_window_, w, h);

    if (config_.full_screen() == FullscreenMode::kFullscreen_borderless
        || config_.full_screen() == FullscreenMode::kWindowed_borderless) {
        int targetMonitor = config_.target_monitor();
        SDL_SetWindowPosition(sdl_window_, SDL_WINDOWPOS_CENTERED_DISPLAY(targetMonitor), SDL_WINDOWPOS_CENTERED_DISPLAY(targetMonitor));
    }

    SDL_GL_GetDrawableSize(sdl_window_, &window_dims[0], &window_dims[1]);
}

void Graphics::SetUpWindowDims(int& w, int& h) {
    switch (config_.full_screen()) {
        case FullscreenMode::kWindowed:
        case FullscreenMode::kWindowed_borderless:
            ResizeWindow(w, h);
            window_dims[0] = w;
            window_dims[1] = h;
            render_dims[0] = window_dims[0];
            render_dims[1] = window_dims[1];
            render_output_dims[0] = window_dims[0];
            render_output_dims[1] = window_dims[1];
            break;
        case FullscreenMode::kFullscreen:
            ResizeWindow(w, h);
            SDL_GetWindowSize(sdl_window_, &render_dims[0], &render_dims[1]);
            SDL_GetWindowSize(sdl_window_, &render_output_dims[0], &render_output_dims[1]);
            break;
        case FullscreenMode::kFullscreen_borderless:
            render_dims[0] = w;
            render_dims[1] = h;
            render_output_dims[0] = window_dims[0];
            render_output_dims[1] = window_dims[1];
            if(render_output_dims[0] > render_output_dims[1] * render_dims[0] / render_dims[1]){
                render_output_dims[0] = render_output_dims[1] * render_dims[0] / render_dims[1];
            } else if(render_output_dims[0] < render_output_dims[1] * render_dims[0] / render_dims[1]){
                render_output_dims[1] = render_output_dims[0] * render_dims[1] / render_dims[0];
            }
            break;
    }

    config_.SetScreenWidth(w);
    config_.SetScreenHeight(h);
}

void Graphics::SetResolution(int w, int h, bool force) {
    LOGI << "SetResolution has been called" << std::endl;
    if(render_dims[0] == w && render_dims[1] == h && !force){
        return;
    }
    SetUpWindowDims(w, h);
    if( initialized ) {
        InitScreen();
    }
    LOGI << "Updating the screen resolution in the config file." << std::endl;
    config.GetRef("screenwidth") = w;
    config.GetRef("screenheight") = h;
    if( initialized ) {
        Engine::Instance()->InjectWindowResizeEvent(ivec2(w,h));
    }
}

void Graphics::SetTargetMonitor(int targetMonitor) {
    if (SDL_GetWindowDisplayIndex(sdl_window_) != targetMonitor) {
        if (currentFullscreenType == FullscreenMode::kFullscreen) {
            SDL_SetWindowFullscreen(sdl_window_, SDL_FALSE);
            SDL_SetWindowPosition(sdl_window_, SDL_WINDOWPOS_CENTERED_DISPLAY(targetMonitor), SDL_WINDOWPOS_CENTERED_DISPLAY(targetMonitor));
            SDL_SetWindowFullscreen(sdl_window_, SDL_TRUE);
        }
        else
            SDL_SetWindowPosition(sdl_window_, SDL_WINDOWPOS_CENTERED_DISPLAY(targetMonitor), SDL_WINDOWPOS_CENTERED_DISPLAY(targetMonitor));

        SDL_DisplayMode displayMode;
        SDL_GetDesktopDisplayMode(config_.target_monitor(), &displayMode);

        if (window_dims[0] > displayMode.w || window_dims[1] > displayMode.h)
            SetResolution(displayMode.w, displayMode.h, true);
    }
}

void Graphics::CheckForWindowResize() {
    int test[2];
    SDL_GL_GetDrawableSize(sdl_window_, &test[0], &test[1]);
    if(test[0] != window_dims[0] || test[1] != window_dims[1]){
        SetResolution(test[0], test[1], true);
    }
}

void Graphics::SetFullscreen(FullscreenMode::Mode val) {
    config_.SetFullscreen(val);

    // If entering windowed mode, resize the window to half the display resolution and center it,
    // Otherwise the title bar (on windows at least) might appear outside of the screen bounds
    bool resize = false;
    bool recenter = false;

    switch (val) {
        case FullscreenMode::kWindowed: {
            if (currentFullscreenType != FullscreenMode::kWindowed) {
                resize = true;
                recenter = true;
            }

            SDL_SetWindowFullscreen(sdl_window_, SDL_FALSE);
            SDL_SetWindowBordered(sdl_window_, SDL_TRUE);
#if PLATFORM_WINDOWS //Following function added in SDL 2.0.5
            SDL_SetWindowResizable(sdl_window_, SDL_TRUE);
#endif
            break;
        }
        case FullscreenMode::kFullscreen: {
            int w = config_.screen_width();
            int h = config_.screen_height();
            ResizeWindow(w, h);

            SDL_SetWindowFullscreen(sdl_window_, SDL_WINDOW_FULLSCREEN);
            break;
        }
        case FullscreenMode::kWindowed_borderless: {
            SDL_SetWindowFullscreen(sdl_window_, SDL_WINDOW_BORDERLESS);
            SDL_SetWindowBordered(sdl_window_, SDL_FALSE);
#if PLATFORM_WINDOWS //Following function added in SDL 2.0.5
            SDL_SetWindowResizable(sdl_window_, SDL_FALSE);
#endif
            recenter = true;
            break;
        }
        case FullscreenMode::kFullscreen_borderless: {
            SDL_SetWindowFullscreen(sdl_window_, SDL_WINDOW_BORDERLESS);
            SDL_SetWindowBordered(sdl_window_, SDL_FALSE);
#if PLATFORM_WINDOWS //Following function added in SDL 2.0.5
            SDL_SetWindowResizable(sdl_window_, SDL_FALSE);
#endif
            
            SDL_DisplayMode displayMode;
            SDL_GetDesktopDisplayMode(config_.target_monitor(), &displayMode);

            ResizeWindow(displayMode.w, displayMode.h); // Recenters window
            break;
        }
    }


    if(resize) {
        SDL_DisplayMode displayMode;
        SDL_GetDesktopDisplayMode(config_.target_monitor(), &displayMode);
        SetResolution(displayMode.w / 2, displayMode.h / 2, true);
    } else {
        SetResolution(render_dims[0], render_dims[1], true);
    }

    if(recenter) {
        SDL_SetWindowPosition(sdl_window_, SDL_WINDOWPOS_CENTERED_DISPLAY(config_.target_monitor()), SDL_WINDOWPOS_CENTERED_DISPLAY(config_.target_monitor()));
    } 

    SDL_GL_GetDrawableSize(sdl_window_, &window_dims[0], &window_dims[1]);
    currentFullscreenType = val;
}

void Graphics::SetFSAA(int val) {
    if(config_.FSAA_samples() != val){
        config_.SetFSAASamples(val);
        if( initialized ) {
            InitScreen();
        }
    }
}

void Graphics::SetVsync(bool val) {
    config_.SetVSync(val);
    ApplyVsync(val);
}

void Graphics::SetLimitFpsInGame(bool val) {
    config_.SetLimitFpsInGame(val);
}

void Graphics::SetMaxFrameRate(int val) {
    config_.SetMaxFrameRate(val);
}

void Graphics::SetSimpleFog(bool val){
    config_.SetSimpleFog(val);
}

void Graphics::SetDepthOfField(bool val){
    config_.SetDepthOfField(val);
}

void Graphics::SetDepthOfFieldReduced(bool val){
    config_.SetDepthOfFieldReduced(val);
}

void Graphics::SetDetailObjects(bool val){
    config_.SetDetailObjects(val);    
}

void Graphics::SetDetailObjectDecals(bool val){
    config_.SetDetailObjectDecals(val);
}

void Graphics::SetDetailObjectLowres(bool val){
    config_.SetDetailObjectLowres(val);
}

void Graphics::SetDetailObjectsReduced(bool val) {
    g_detail_objects_reduced = val;
    g_detail_objects_reduced_dirty = true;
}

void Graphics::SetDetailObjectShadows(bool val) {
    config_.SetDetailObjectShadows(val);
}

void Graphics::SetSeamlessCubemaps(bool val) {
    if(GL_VERSION_3_2 || GLEW_ARB_seamless_cube_map || GLEW_ARB_seamless_cubemap_per_texture){
        if(val){
            glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        } else {
            glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        }
        config_.seamless_cubemaps_ = val;
    }
}

void Graphics::DrawArrays(GLenum mode, int first, unsigned int count) {
    SetVertexAttribArrays();
    GL_PERF_START();
    glDrawArrays(mode, first, count);
    GL_PERF_END();
}

void Graphics::DrawElements(GLenum mode, unsigned int count, GLenum type, const void * indices) {
    SetVertexAttribArrays();
    GL_PERF_START();
    glDrawElements(mode, count, type, indices);
    GL_PERF_END();
}

void Graphics::DrawElementsInstanced(GLenum mode, unsigned int count, GLenum type, const void * indices, unsigned int primcount) {
    SetVertexAttribArrays();
    GL_PERF_START();
    glDrawElementsInstanced(mode, count, type, indices, primcount);
    GL_PERF_END();
}

void Graphics::DrawRangeElements(GLenum mode, unsigned int start, unsigned int end, unsigned int count, GLenum type, const void * indices) {
    SetVertexAttribArrays();
    GL_PERF_START();
    glDrawRangeElements(mode, start, end, count, type, indices);
    GL_PERF_END();
}

void Graphics::EnableVertexAttribArray(unsigned int index) {
    assert(index < sizeof(wanted_vertex_attrib_arrays) * 8);
    wanted_vertex_attrib_arrays |= (1 << index);
}

void Graphics::DisableVertexAttribArray(unsigned int index) {
    assert(index < sizeof(wanted_vertex_attrib_arrays) * 8);
    wanted_vertex_attrib_arrays &= ~(1 << index);
}

void Graphics::ResetVertexAttribArrays() {
    wanted_vertex_attrib_arrays = 0;
}


#ifdef __GNUC__


unsigned char BitScanForward(unsigned long *index, unsigned long mask) {
    int temp = __builtin_ffs(mask);

    if (temp == 0) {
        *index = 0;
        return 0;
    } else {
        *index = temp - 1;
        return 1;
    }
}


#else  // __GNUC__

#ifndef _MSC_VER
#error Unknown compiler
#endif  // _MSC_VER

#define BitScanForward _BitScanForward

#endif  // __GNUC__


void Graphics::SetVertexAttribArrays() {
    uint32_t changed_vertex_attrib_arrays = active_vertex_attrib_arrays ^ wanted_vertex_attrib_arrays;

    while (changed_vertex_attrib_arrays != 0) {
        unsigned long bit = 0;
        unsigned char nonZero = BitScanForward(&bit, changed_vertex_attrib_arrays);
        // can't be zero
        LOG_ASSERT(nonZero != 0);
        unsigned int mask = 1u << bit;
        LOG_ASSERT(mask != 0);

        if ((wanted_vertex_attrib_arrays & mask) != 0) {
            glEnableVertexAttribArray(bit);
        } else {
            glDisableVertexAttribArray(bit);
        }

        changed_vertex_attrib_arrays ^= mask;

#ifdef DEBUG
        active_vertex_attrib_arrays ^= mask;
#endif  // DEBUG
    }

#ifdef DEBUG
    LOG_ASSERT_EQ(active_vertex_attrib_arrays, wanted_vertex_attrib_arrays);
#endif  // DEBUG

    active_vertex_attrib_arrays = wanted_vertex_attrib_arrays;
}

void Graphics::ClearGLState() {
    // reset everything in case someone else has changed these
    active_vertex_attrib_arrays = 0;
    wanted_vertex_attrib_arrays = 0;
    for (int i = 0; i < max_vertex_attrib_arrays; i++) {
        glDisableVertexAttribArray(i);
    }
}

void Graphics::SetUpShadowTextures() {
    PROFILER_GPU_ZONE(g_profiler_ctx, "Set up shadow textures");
    Textures* textures = Textures::Instance();

    PushFramebuffer();

    if( first_load ) {
        static_shadow_depth_ref = textures->makeTexture(cascade_shadow_res,cascade_shadow_res,GL_DEPTH_COMPONENT24,GL_DEPTH_COMPONENT);
        textures->SetTextureName(static_shadow_depth_ref, "Static Shadow Depth");

        textures->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
        genFramebuffers(&static_shadow_fb, "static_shadow_fb");
        bindFramebuffer(static_shadow_fb);
        framebufferDepthTexture2D(static_shadow_depth_ref);

        textures->bindTexture(static_shadow_depth_ref);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        CHECK_FBO_ERROR();
    }

    deleteFramebuffer(&cascade_shadow_fb);
    deleteFramebuffer(&cascade_shadow_color_fb);

    cascade_shadow_depth_ref = TextureRef(); 
    cascade_shadow_color_ref = TextureRef();
    
    if(!g_simple_shadows && g_level_shadows) {
        cascade_shadow_depth_ref = textures->makeTexture(cascade_shadow_res,cascade_shadow_res,GL_DEPTH_COMPONENT24,GL_DEPTH_COMPONENT);
        textures->SetTextureName(cascade_shadow_depth_ref, "Cascade Shadow Depth");

        textures->setFilters(GL_NEAREST, GL_NEAREST);
        cascade_shadow_color_ref = textures->makeTexture(2048,2048,GL_RGBA,GL_RGBA);
        textures->SetTextureName(cascade_shadow_color_ref, "Cascade Shadow Color");

        textures->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
        genFramebuffers(&cascade_shadow_fb, "cascade_shadow_fb");
        bindFramebuffer(cascade_shadow_fb);
        framebufferDepthTexture2D(cascade_shadow_depth_ref);

        genFramebuffers(&cascade_shadow_color_fb, "cascade_shadow_color_fb");
        bindFramebuffer(cascade_shadow_color_fb);
        framebufferColorTexture2D(cascade_shadow_color_ref);
        framebufferDepthTexture2D(cascade_shadow_depth_ref);

        textures->bindTexture(cascade_shadow_depth_ref);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    }

    PopFramebuffer();

    Textures::Instance()->InvalidateBindCache();
}


void Graphics::DebugTracePrint(const char *message) {
#if GPU_MARKERS
    if (GLEW_KHR_debug) {
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0, GL_DEBUG_SEVERITY_NOTIFICATION, -1, message);
    }
#endif  // GPU_MARKERS
}


vec3 GraphicsConfig::BloodColorFromString(const std::string& blood_color_str) {
    vec3 blood_color(0.5f,0.0f,0.0f);
    {
        int num_chars = blood_color_str.length();
        blood_color[0] = (float)atof(&blood_color_str[0]);
        int which_num = 1;
        for(int i=0; i<num_chars-1; ++i){
            if(blood_color_str[i] == ' '){
                blood_color[which_num] = (float)atof(&blood_color_str[i+1]);
                ++which_num;
                if(which_num > 2){
                    break;
                }
            }
        }
    }
    return blood_color;
}
