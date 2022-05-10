//-----------------------------------------------------------------------------
//           Name: graphics.h
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
#pragma once

#include <Math/vec2.h>
#include <Math/mat4.h>
#include <Math/mat3.h>

#include <Internal/error.h>
#include <Internal/hardware_specs.h>

#include <Graphics/glstate.h>
#include <Asset/Asset/texture.h>

#include <opengl.h>

#include <queue>
#include <stack>
#include <climits>
#include <stdint.h>  // cstdint would be more c++ but os x doesn't understand it

#define UBO_CLUSTER_DATA 1
#define UBO_SHADOW_CASCADES 2

const GLuint INVALID_FRAMEBUFFER = UINT_MAX;

enum ShadowedClientStateFlags {
    F_VERTEX_ARRAY = (1 << 0),
    F_NORMAL_ARRAY = (1 << 1),
    F_COLOR_ARRAY = (1 << 2),
    F_TEXTURE_COORD_ARRAY0 = (1 << 3),
    F_TEXTURE_COORD_ARRAY1 = (1 << 4),
    F_TEXTURE_COORD_ARRAY2 = (1 << 5),
    F_TEXTURE_COORD_ARRAY3 = (1 << 6),
    F_TEXTURE_COORD_ARRAY4 = (1 << 7),
    F_TEXTURE_COORD_ARRAY5 = (1 << 8),
    F_TEXTURE_COORD_ARRAY6 = (1 << 9),
    F_TEXTURE_COORD_ARRAY7 = (1 << 10)
};

enum ShadowedClientStateID {
    CS_VERTEX_ARRAY,
    CS_NORMAL_ARRAY,
    CS_COLOR_ARRAY,
    CS_TEXTURE_COORD_ARRAY0,
    CS_TEXTURE_COORD_ARRAY1,
    CS_TEXTURE_COORD_ARRAY2,
    CS_TEXTURE_COORD_ARRAY3,
    CS_TEXTURE_COORD_ARRAY4,
    CS_TEXTURE_COORD_ARRAY5,
    CS_TEXTURE_COORD_ARRAY6,
    CS_TEXTURE_COORD_ARRAY7,

    CS_MAX_SHADOWED_CLIENT_STATES
};

class Config;

namespace BloodLevel {
enum Type {
    kNone = 0,
    kSimple = 1,
    kFull = 2
};
}

namespace FullscreenMode {
enum Mode {
    kWindowed = 0  // Resizable floating window
    ,
    kFullscreen = 1  // Exclusive fullscreen
    ,
    kWindowed_borderless = 2,
    kFullscreen_borderless = 3
};
}

class GraphicsConfig {
   public:
    bool dynamic_character_cubemap_;
    bool gpu_skinning() const { return gpu_skinning_; }
    float anisotropy() const { return anisotropy_; }
    int FSAA_samples() const { return FSAA_samples_; }
    FullscreenMode::Mode full_screen() const { return full_screen_; }
    int target_monitor() const { return target_monitor_; }
    int screen_width() const { return screen_width_; }
    int screen_height() const { return screen_height_; }
    bool split_screen() const { return split_screen_; }
    int texture_reduce() const { return texture_reduce_; }
    int texture_reduction_factor() const { return texture_reduction_factor_; }
    bool vSync() const { return vSync_; }
    bool limit_fps_in_game() const { return limit_fps_in_game_; }
    int max_frame_rate() const { return max_frame_rate_; }
    int blood() const { return blood_; }
    bool simple_fog() const { return simple_fog_; }
    bool depth_of_field() const { return depth_of_field_; }
    bool depth_of_field_reduced() const { return depth_of_field_reduced_; }
    bool detail_objects() const { return detail_objects_; }
    bool detail_object_decals() const { return detail_object_decals_; }
    bool detail_object_lowres() const { return detail_object_lowres_; }
    bool detail_object_shadows() const { return detail_object_shadows_; }
    const vec3& blood_color() const { return blood_color_; }
    static vec3 BloodColorFromString(const std::string& str);

    void SetGPUSkinning(bool val) { gpu_skinning_ = val; }
    void SetAnisotropy(float val) { anisotropy_ = val; }
    void SetFSAASamples(int val) { FSAA_samples_ = val; }
    void SetFullscreen(FullscreenMode::Mode val) { full_screen_ = val; }
    void SetTargetMonitor(int val) { target_monitor_ = val; }
    void SetScreenWidth(int val) { screen_width_ = val; }
    void SetScreenHeight(int val) { screen_height_ = val; }
    void SetSplitScreen(bool val) { split_screen_ = val; }
    void SetBlood(int val) { blood_ = val; }
    void SetBloodColor(const vec3& val) { blood_color_ = val; }
    void SetTextureReduce(int val) {
        texture_reduce_ = val;
        texture_reduction_factor_ = 1 << texture_reduce_;
    }
    void SetVSync(bool val) { vSync_ = val; }
    void SetLimitFpsInGame(bool val) { limit_fps_in_game_ = val; }
    void SetMaxFrameRate(int val) { max_frame_rate_ = val; }
    void SetSimpleFog(bool val) { simple_fog_ = val; }
    void SetDepthOfField(bool val) { depth_of_field_ = val; }
    void SetDepthOfFieldReduced(bool val) { depth_of_field_reduced_ = val; }
    void SetDetailObjects(bool val) { detail_objects_ = val; }
    void SetDetailObjectDecals(bool val) { detail_object_decals_ = val; }
    void SetDetailObjectLowres(bool val) { detail_object_lowres_ = val; }
    void SetDetailObjectShadows(bool val) { detail_object_shadows_ = val; }

    bool seamless_cubemaps_;
    float motion_blur_amount_;

   private:
    bool simple_fog_;
    bool depth_of_field_;
    bool depth_of_field_reduced_;
    bool detail_objects_;
    bool detail_object_decals_;
    bool detail_object_lowres_;
    bool detail_object_shadows_;
    bool gpu_skinning_;
    float anisotropy_;
    int FSAA_samples_;
    FullscreenMode::Mode full_screen_;
    int target_monitor_;  // Used when retrieving monitor data from SDL, though not 100% supported. Currently hard-coded to 0
    int screen_width_;
    int screen_height_;
    bool split_screen_;
    int texture_reduce_;
    int texture_reduction_factor_;
    bool vSync_;
    bool limit_fps_in_game_;
    int max_frame_rate_;
    int blood_;
    vec3 blood_color_;
};

class GraphicsFeatures {
   public:
    bool HDR_enable() const { return HDR_enable_; }
    bool frame_buffer_fsaa_enabled() const { return frame_buffer_fsaa_enabled_; }

    void SetHDREnable(bool val) { HDR_enable_ = val; }
    void SetFrameBufferFSAAEnabled(bool val) { frame_buffer_fsaa_enabled_ = val; }

   private:
    bool HDR_enable_;
    bool frame_buffer_fsaa_enabled_;
};

struct GraphicsShadowState {
    int depth_func_type_;  // Shadow the glDepthFunc values
    bool depth_func_unset_;

    GLState gl_state_;

    bool client_states_[CS_MAX_SHADOWED_CLIENT_STATES];
    int client_active_textures_;
    bool polygon_offset_enable_;
};

struct SDL_Window;

struct ViewportDims {
    int entries[4];
};

struct PostEffects {
    GLuint post_framebuffer;
    TextureRef temp_screen_tex;
    TextureRef tone_mapped_tex;
};

class Graphics {
   public:
    void Dispose();

   private:
    Graphics();

    bool gl_lib_loaded_;  // To avoid loading the gl library multiple times

    GraphicsFeatures features_;
    GraphicsShadowState shadow_state_;
    bool media_mode_;
    void* gl_context;
    uint32_t active_vertex_attrib_arrays;
    uint32_t wanted_vertex_attrib_arrays;
    int max_vertex_attrib_arrays;

    const char* shader;

    void SetVertexAttribArrays();

    FullscreenMode::Mode currentFullscreenType;

   public:
    GraphicsConfig config_;
    SDL_Window* sdl_window_;
    bool depth_prepass;
    PostEffects post_effects;
    enum ScreenType { kWindow,
                      kRender,
                      kTexture };

    void Initialize();
    void GetShaderNames(std::map<std::string, int>& preload_shaders);
    void WindowResized(ivec2 value);
    GraphicsFeatures& features() { return features_; }
    void SetMediaMode(bool val) { media_mode_ = val; }
    void SetSeamlessCubemaps(bool val);
    bool media_mode() const { return media_mode_; }
    GLVendor vendor;
    TextureAssetRef noise_ref;
    TextureAssetRef pure_noise_ref;

    int queued_screenshot;
    bool pre_screenshot_media_mode_state;
    enum ScreenshotMode { kGameplay,
                          kTransparentGameplay };
    ScreenshotMode screenshot_mode;

    float shadow_size;

    TextureRef static_shadow_depth_ref;
    TextureRef cascade_shadow_depth_ref;
    TextureRef cascade_shadow_color_ref;
    GLuint cascade_shadow_res;
    GLuint cascade_shadow_fb;
    GLuint static_shadow_fb;
    GLuint cascade_shadow_color_fb;
    mat4 cascade_shadow_mat[4];
    mat4 simple_shadow_mat;
    float cascade_shadow_radius[4];

    float hdr_white_point;
    float hdr_black_point;
    float hdr_bloom_mult;

    GLuint shadow_res;
    bool drawing_shadow;
    std::stack<ViewportDims> viewport_dims_stack;
    int viewport_dim[4];

    int vbo_array_bound;
    int vbo_element_bound;

    bool multisample_framebuffer_exists;
    GLuint multisample_framebuffer;
    GLuint multisample_depth;
    GLuint multisample_color;
    GLuint multisample_vel;

    bool framebuffer_exists;
    GLuint framebuffer;
    TextureRef screen_color_tex;
    TextureRef screen_vel_tex;
    TextureRef screen_depth_tex;
    std::string post_shader_name;  // Just used for saving

    bool use_sample_alpha_to_coverage;

    int max_texture_units;

    int old_FSAA_samples;
    int old_render_dims[2];
    bool old_post_effects_enable;
    bool first_load;
    bool initialized;
    bool settings_changed;  // for detail objects

    std::stack<GLuint> fbo_stack;
    GLuint curr_framebuffer;
    bool nav_mesh_out_of_date;
    int nav_mesh_out_of_date_chunk;
    int line_width;

    // The following allow us to render the game, UI, and window at different resolutions
    // E.g.
    int window_dims[2];         // dimensions of OS window
    int render_dims[2];         // dimensions of game render target. Same as window_dims if windowed, otherwise internal resolution
    int render_output_dims[2];  // dimensions of render target output in window. Same as window_dims, unless the aspect ratio is off?

    void Clear(const bool clear_color);
    void setViewport(const GLint startx, const GLint starty, const GLint endx, const GLint endy);
    void setAdditiveBlend(const bool what);
    void SetBlendFunc(const int src, const int dest, const int alpha_src = GL_ONE, const int alpha_dst = GL_ONE_MINUS_SRC_ALPHA);
    void setDepthFunc(const int type);
    void setBlend(const bool what);
    void setDepthTest(const bool what);
    void setCullFace(const bool what);
    void setDepthWrite(const bool what);
    void setPolygonOffset(const bool polygon_offset);
    void setSimpleShadows(const bool val);
    void setSimpleWater(const bool val);
    void SetParticleFieldSimple(bool val);
    void setDepthOfField(const bool val);
    void setAttribEnvObjInstancing(bool val);

    void PushViewport();
    void PopViewport();

    void TakeScreenshot();

    void setGLState(const GLState& state);

    void RenderFramebufferToTexture(GLuint framebuffer, TextureRef texture_ref);
    void EndTextureSpaceRendering();

    void PushFramebuffer();
    void PopFramebuffer();
    void bindRenderbuffer(GLuint rb);
    void bindFramebuffer(GLuint fb);
    void framebufferDepthTexture2D(TextureRef t, int mipmap_level = 0);
    void framebufferColorTexture2D(TextureRef t, int mipmap_level = 0);
    void genRenderbuffers(GLuint* rb);

    std::map<GLuint, std::string> framebuffernames;
    void genFramebuffers(GLuint* fb, const char* human_name);
    void deleteFramebuffer(GLuint* fb);
    void renderbufferStorage(GLuint res);
    void framebufferRenderbuffer(GLuint rb);
    GLenum checkFramebufferStatus();

    unsigned int GetShadowSize(unsigned int target_shadow_size);
    void startDraw(vec2 start = vec2(0.0f), vec2 end = vec2(1.0f), ScreenType screen_type = kRender);
    void SwapToScreen();
    void InitScreen();
    void ResizeWindow(int& w, int& h);

    void SetUpWindowDims(int& w, int& h);
    static Graphics* Instance() {
        static Graphics instance;
        return &instance;
    }
    void SetModelMatrix(const mat4& matrix, const mat3& normal_matrix);
    void SetSimpleLineDrawState();
    void BlitDepthBuffer();
    void BlitColorBuffer();

    void SetClientStates(int flags);
    void SetClientStateEnabled(int shadowStateID, bool enabled);
    void SetClientActiveTexture(int index);
    void SetFromConfig(const Config& config, bool dynamic = false);
    void SetWindowGrab(bool val);
    void BindVBO(int target, int val);
    void UnbindVBO(int target, int val);
    void BindArrayVBO(int val);
    void BindElementVBO(int val);
    void SetLineWidth(int val);
    // void SetFullscreen(bool val);
    void SetFullscreen(FullscreenMode::Mode val);

    void SetFSAA(int val);
    void SetVsync(bool val);
    void SetLimitFpsInGame(bool val);
    void SetMaxFrameRate(int val);
    void SetSimpleFog(bool val);
    void SetDepthOfField(bool val);
    void SetDepthOfFieldReduced(bool val);
    void SetDetailObjects(bool val);
    void SetDetailObjectDecals(bool val);
    void SetDetailObjectLowres(bool val);
    void SetDetailObjectShadows(bool val);
    void SetDetailObjectsReduced(bool val);
    void SetAnisotropy(float val);
    // void SetWindowDimensions( int w, int h );
    void SetResolution(int w, int h, bool force);
    void SetTargetMonitor(int targetMonitor);
    void CheckForWindowResize();
    void StartTextureSpaceRenderingCustom(int x, int y, int width, int height);

    void DrawArrays(GLenum mode, int first, unsigned int count);
    void DrawElements(GLenum mode, unsigned int count, GLenum type, const void* indices);
    void DrawElementsInstanced(GLenum mode, unsigned int count, GLenum type, const void* indices, unsigned int primcount);
    void DrawRangeElements(GLenum mode, unsigned int start, unsigned int end, unsigned int count, GLenum type, const void* indices);

    void EnableVertexAttribArray(unsigned int index);
    void DisableVertexAttribArray(unsigned int index);
    void ResetVertexAttribArrays();
    void ClearGLState();

    void SetUpShadowTextures();

    // output a debug message to OpenGL trace
    void DebugTracePrint(const char* message);
};

bool CheckGLError(int line, const char* file, const char* errstring);
void CheckFBOError(int line, const char* file);

bool CheckGLErrorStr(char* output, unsigned length);

#if GPU_MARKERS
#undef NO_GL_ERROR_CHECKING
#define NO_GL_ERROR_CHECKING 1
#endif  // GPU_MARKERS

#define FORCE_CHECK_GL_ERROR() CheckGLError(__LINE__, __FILE__, NULL)
#define FORCE_CHECK_FBO_ERROR() CheckFBOError(__LINE__, __FILE__)
#ifndef NO_GL_ERROR_CHECKING
#define CHECK_GL_ERROR() CheckGLError(__LINE__, __FILE__, NULL)
#define CHECK_FBO_ERROR() CheckFBOError(__LINE__, __FILE__)
#else
#define CHECK_GL_ERROR()
#define CHECK_FBO_ERROR()
#endif
