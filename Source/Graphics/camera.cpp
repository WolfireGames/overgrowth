//-----------------------------------------------------------------------------
//           Name: camera.cpp
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
#include "camera.h"

#include <Graphics/graphics.h>
#include <Graphics/vbocontainer.h>
#include <Graphics/shaders.h>

#include <Internal/timer.h>
#include <Internal/profiler.h>

#include <UserInput/input.h>
#include <Math/vec3math.h>
#include <Wrappers/glm.h>
#include <Main/engine.h>
#include <Online/online.h>

#include <cstring>
#include <cmath>

#define USE_SSE

//-----------------------------------------------------------------------------
//Functions
//-----------------------------------------------------------------------------

extern Timer game_timer;

const vec3& Camera::GetFacing() const {
    return facing;
}

const vec3& Camera::GetFlatFacing() const {
    return flat_facing;
}
    
void Camera::SetFlatFacing(const vec3 &v) {
    flat_facing = v;
}

void Camera::FixDiscontinuity()
{
	old_chase_distance = chase_distance;
	old_y_rotation = y_rotation;
	old_x_rotation = x_rotation;
	old_pos = pos;
	old_z_rotation = z_rotation;
}

Camera::Camera() :
    m_camera_object(NULL),
    interp_chase_distance(0.0f),
    near_blur_amount(0.0f),
    far_blur_amount(0.0f),
    near_sharp_dist(3.0f),
    far_sharp_dist((float)pow(5.0f, 0.5f)),
    near_blur_transition_size(1.0f),
    far_blur_transition_size(2.0f) ,
    biasMatrix(0.5f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.5f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.5f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f){
        for (unsigned int i = 0; i < 16; i++) {
            modelview_matrix[i] = 0.0;
            projection_matrix[i] = 0.0;
        }
        reset();
}

void Camera::SetDistance(const float new_val) {
    old_chase_distance = chase_distance;
    chase_distance=new_val;
}

void Camera::SetYRotation(const float new_val) {
    old_y_rotation = y_rotation;
    y_rotation = new_val;

    while(old_y_rotation>=y_rotation+180){
        old_y_rotation-=360;
    }
    while(old_y_rotation<=y_rotation-180){
        old_y_rotation+=360;
    }
}

void Camera::SetXRotation(const float new_val) {
    old_x_rotation = x_rotation;
    x_rotation = new_val;
}

void Camera::SetFacing(const vec3 &_facing) {
    facing = _facing;
}

void Camera::SetUp(const vec3 &_up) {
    up = _up;
}

void Camera::SetZRotation(const float new_val) {
    old_z_rotation = z_rotation;
    z_rotation = new_val;
}

void Camera::SetPos(const vec3 &new_val) {
    old_pos = pos;
    pos = new_val;
}

void Camera::LookAt(const vec3 &new_val) {
    vec3 vector = normalize(new_val-pos);
    float new_y_rotation = YAxisRotationFromVector(vector);
    float new_x_rotation = asinf(vector.y())/3.14f*180.0f;
    SetYRotation(new_y_rotation);
    SetXRotation(new_x_rotation);
}

float Camera::GetYRotation() const {
    return mix(old_y_rotation,y_rotation,GetInterpWeight());
}

float Camera::GetChaseDistance() const {
    return mix(old_chase_distance,chase_distance,GetInterpWeight());
}

float Camera::GetXRotation() const {
    return mix(old_x_rotation,x_rotation,GetInterpWeight());
}    

float Camera::GetZRotation() const {
    return mix(old_z_rotation,z_rotation,GetInterpWeight());
}    

vec3 Camera::GetPos() const {
    return interp_pos-facing*interp_chase_distance;
}

const vec3& Camera::GetMouseRay() const {
    return mouseray;
}

const vec3& Camera::GetUpVector() const {
    return up;
}

const float& Camera::GetNearPlane() const {
    return near_plane;
}

const float& Camera::GetFarPlane() const {
    return far_plane;
}

const float& Camera::GetFOV() const {
    return target_horz_fov;
}

const mat4& Camera::getInverseCameraMatrix() const {
    return inverseCameraViewMatrix;
}

const mat4& Camera::getCameraMatrix() const {
    return cameraViewMatrix;
}

vec3 Camera::getRelative(const vec3 &absolute) {
    vec3 relative = absolute - GetPos();

    relative = doRotation(relative, 0, -y_rotation, 0);
    relative = doRotation(relative, -x_rotation, 0, 0);
    relative = doRotation(relative, 0, 0, -z_rotation);

    return relative;
}

void Camera::applyShadowViewpoint(vec3 direction, vec3 center, float scale, mat4 *proj_matrix, mat4 *view_matrix, float far_back) {
    if(!proj_matrix){
        proj_matrix = &lightProjectionMatrix;
    }
    if(!view_matrix){
        view_matrix = &lightViewMatrix;
    }
    proj_matrix->SetOrtho(-scale*0.5f, 
        scale*0.5f, 
        -scale*0.5f, 
        scale*0.5f, 
        10.0f, 
        2000.0f);

    facing = normalize(direction) * -1.0f;
    
    view_matrix->SetLookAt(center+direction*far_back, center-direction, vec3(0.0f,1.0f,0.0f));
    
    calcFrustumPlanes(*proj_matrix, *view_matrix);

    interp_pos = mix(old_pos, pos, GetInterpWeight());
}

//Apply the camera viewpoint to the view matrix
void Camera::applyViewpoint() {
    Online* online = Online::Instance();
    Graphics *gi = Graphics::Instance();
    mat4 projection = GetPerspectiveMatrix();
    
    CalcFacing();        //Find the forward vector
    calcUp();            //Find the up vector
    
    float interp_weight = GetInterpWeight();
    interp_pos = mix(old_pos, pos, interp_weight); 
    interp_chase_distance = mix(old_chase_distance, chase_distance, interp_weight);
    
    //Apply camera transformation
	vec3 final_cam_pos;

	
	final_cam_pos = interp_pos - facing * interp_chase_distance;
	

    cameraViewMatrix.SetLookAt(final_cam_pos, final_cam_pos + facing, up);
    calcFrustumPlanes(projection, cameraViewMatrix);
    inverseCameraViewMatrix = invert(cameraViewMatrix);

    Input *userInput = Input::Instance();
    
    //Find mouse direction in world space
    vec3 mouse;
    mouse.x()=(float)userInput->getMouse().pos_[0];
    mouse.y()=(float)(userInput->getMouse().pos_[1]*-1+gi->window_dims[1]);
    mouse.z()=0.5f;

    for(unsigned i=0; i<16; ++i){
        modelview_matrix[i] = cameraViewMatrix.entries[i];
        projection_matrix[i] = projection.entries[i];
    }

    viewport[0]=0;
    viewport[1]=0;
    viewport[2]=gi->window_dims[0];
    viewport[3]=gi->window_dims[1];

    glm::vec3 ray = glm::unProject(glm::vec3(mouse.x(), mouse.y(), mouse.z()),
                                   glm::make_mat4(modelview_matrix),
                                   glm::make_mat4(projection_matrix),
                                   glm::make_vec4(viewport));

    mouseray.x() = (float)ray.x;
    mouseray.y() = (float)ray.y;
    mouseray.z() = (float)ray.z;
    
    mouse.x()=(float)(gi->window_dims[0])/2.0f;
    mouse.y()=(float)(gi->window_dims[1])/2.0f;
    
    ray = glm::unProject(glm::vec3(mouse.x(), mouse.y(), mouse.z()),
                         glm::make_mat4(modelview_matrix),
                         glm::make_mat4(projection_matrix),
                         glm::make_vec4(viewport));
    
    vec3 center = vec3((float)ray.x,(float)ray.y,(float)ray.z);
    center-=interp_pos;
    float ray_length = length(center);
    
    mouseray-=(interp_pos - facing * interp_chase_distance);
    //mouseoffset=length(mouseray)/ray_length;
    
    mouseray/=ray_length;
}

vec3 Camera::GetRayThroughPixel(int x, int y) {
    vec3 ray = UnProjectPixel(x,y);
    ray -= vec3((interp_pos - facing * interp_chase_distance));
    ray = normalize(ray);
    return ray;
}

vec3 Camera::UnProjectPixel(int x, int y) {
    float z = 0.5f;
    glm::vec3 ray = glm::unProject(glm::vec3(x, y, z),
                                   glm::make_mat4(modelview_matrix),
                                   glm::make_mat4(projection_matrix),
                                   glm::make_vec4(viewport));

    vec3 world_coords(ray.x, ray.y, ray.z);
    return world_coords;
}

vec3 Camera::ProjectPoint(const vec3 &point) {
    glm::vec3 screen_pos = glm::project(glm::vec3(point.x(), point.y(), point.z()),
                                        glm::make_mat4(modelview_matrix),
                                        glm::make_mat4(projection_matrix),
                                        glm::make_vec4(viewport));
    
    vec3 screen_coords(screen_pos.x, screen_pos.y, screen_pos.z);
    return screen_coords;
}

vec3 Camera::ProjectPoint(float x, float y, float z) {
    return ProjectPoint(vec3(x,y,z));
}

//Transform from world coordinates to screen coordinates
vec3 Camera::worldToScreen(const vec3 point) const {
    glm::vec3 window_pos = glm::project(glm::vec3(point.x(), point.y(), point.z()),
                                       glm::make_mat4(modelview_matrix),
                                       glm::make_mat4(projection_matrix),
                                       glm::make_vec4(viewport));

    vec3 window;
    window.x() = window_pos.x;
    window.y() = window_pos.y;
    window.z() = window_pos.z;

    return window;
}

float GetAspectRatio() {
    Graphics *gi = Graphics::Instance();
    return gi->viewport_dim[2]/(float)gi->viewport_dim[3];
}

float TargetVertFovFromHorz(float target_horz_fov) {
    return target_horz_fov / (4.0f/3.0f);
}

float VertFOVFromHorz(float target_horz_fov, float aspect_ratio) {
    // Enforce minimum fov on each axis
    float target_horz_radians = target_horz_fov * deg2radf;
    float horz_unit = tan(target_horz_fov * deg2radf * 0.5f);
    float vert_unit = horz_unit / aspect_ratio;
    float vert_angle = atan(vert_unit) * 2.0f * rad2degf;
    vert_angle = max(vert_angle, TargetVertFovFromHorz(target_horz_fov));
    return vert_angle;
}

mat4 Camera::GetPerspectiveMatrix() const {
    float aspect_ratio = GetAspectRatio();
    mat4 matrix;
    if(flexible_fov){
        matrix.SetPerspectiveInfinite(VertFOVFromHorz(target_horz_fov, aspect_ratio), aspect_ratio, near_plane, far_plane);
    } else {
        matrix.SetPerspectiveInfinite(target_horz_fov, aspect_ratio, near_plane, far_plane);
    }
    return matrix;
}

//Set the camera field of view
void Camera::SetFOV(const float degrees) {
    target_horz_fov = degrees;
}

void Camera::DrawSafeZone() {
    PROFILER_GPU_ZONE(g_profiler_ctx, "DrawSafeZone");
    CHECK_GL_ERROR();
    GLState gl_state;
    gl_state.blend = true;
    gl_state.cull_face = false;
    gl_state.depth_test = false;
    gl_state.depth_write = false;
    
    Graphics::Instance()->setGLState(gl_state); 
    int shader_id = Shaders::Instance()->returnProgram("simple_2d");
    Shaders::Instance()->setProgram(shader_id);

    float aspect_ratio = GetAspectRatio();
    mat4 perspective_matrix;
    perspective_matrix.SetPerspectiveInfinite(VertFOVFromHorz(target_horz_fov, aspect_ratio), aspect_ratio, near_plane, far_plane);
    float vert_fov = tanf(TargetVertFovFromHorz(target_horz_fov)*3.14159266f/180.0f*0.45f);
    float horz_fov = tanf(target_horz_fov*3.14159266f/180.0f*0.45f);
    vec3 safe_pos(horz_fov,vert_fov,1);
    safe_pos = perspective_matrix * safe_pos;
    const float safe_pos_extend = 0.8f;

    glm::mat4 proj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);
    Shaders::Instance()->SetUniformMat4("mvp_mat", (const GLfloat*)&proj);
    Shaders::Instance()->SetUniformVec4("color", vec4(1.0f));

    Graphics::Instance()->SetLineWidth(1);

    float data[] = {
        safe_pos[0], safe_pos[1], safe_pos[2],
        safe_pos[0]*safe_pos_extend, safe_pos[1], safe_pos[2],
        safe_pos[0], safe_pos[1], safe_pos[2],
        safe_pos[0], safe_pos[1]*safe_pos_extend, safe_pos[2],

        -safe_pos[0], safe_pos[1], safe_pos[2],
        -safe_pos[0]*safe_pos_extend, safe_pos[1], safe_pos[2],
        -safe_pos[0], safe_pos[1], safe_pos[2],
        -safe_pos[0], safe_pos[1]*safe_pos_extend, safe_pos[2],

        safe_pos[0], -safe_pos[1], safe_pos[2],
        safe_pos[0]*safe_pos_extend, -safe_pos[1], safe_pos[2],
        safe_pos[0], -safe_pos[1], safe_pos[2],
        safe_pos[0], -safe_pos[1]*safe_pos_extend, safe_pos[2],

        -safe_pos[0], -safe_pos[1], safe_pos[2],
        -safe_pos[0]*safe_pos_extend, -safe_pos[1], safe_pos[2],
        -safe_pos[0], -safe_pos[1], safe_pos[2],
        -safe_pos[0], -safe_pos[1]*safe_pos_extend, safe_pos[2],
        
        -safe_pos[0] * 2.0f, -safe_pos[1] * 0.5f, safe_pos[2],
        safe_pos[0] * 2.0f, -safe_pos[1] * 0.5f, safe_pos[2]
    };

    static VBOContainer safe_zone_vbo;
    safe_zone_vbo.Fill(kVBODynamic | kVBOFloat, sizeof(data), data);
    safe_zone_vbo.Bind();

    int vert_attrib_id = Shaders::Instance()->returnShaderAttrib("vert_coord", shader_id);
    Graphics::Instance()->EnableVertexAttribArray(vert_attrib_id);
    CHECK_GL_ERROR();  
    glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, 3*sizeof(float), 0);
    Graphics::Instance()->DrawArrays(GL_LINES, 0, 16);
    CHECK_GL_ERROR();  
    Graphics::Instance()->DrawArrays(GL_LINES, 16, 2);
    Graphics::Instance()->ResetVertexAttribArrays();
    CHECK_GL_ERROR();  
}

//Calculate camera forward vector
void Camera::CalcFacing() {
    flat_facing = vec3(0.0f,0.0f,-1.0f);
    facing = flat_facing;

    float interp_weight = GetInterpWeight();
    float interp_y_rotation = mix(old_y_rotation,y_rotation,interp_weight);
    float interp_x_rotation = mix(old_x_rotation,x_rotation,interp_weight);

    facing=doRotation(facing,interp_x_rotation,0,0);
    facing=doRotation(facing,0,interp_y_rotation,0);

    flat_facing=doRotation(flat_facing,0,interp_y_rotation,0);
}

//Calculate camera up vector
void Camera::calcUp() {
    up = vec3(0.0f,1.0f,0.0f);

    float interp_weight = GetInterpWeight();
    float interp_y_rotation = mix(old_y_rotation,y_rotation,interp_weight);
    float interp_x_rotation = mix(old_x_rotation,x_rotation,interp_weight);
    float interp_z_rotation = mix(old_z_rotation,z_rotation,interp_weight);

    up=doRotation(up,0,0,interp_z_rotation);
    up=doRotation(up,interp_x_rotation,0,0);
    up=doRotation(up,0,interp_y_rotation,0);
}

//Calculate the view frustum planes
void Camera::calcFrustumPlanes( const mat4 &_p, const mat4 &_mv ) {
    const GLfloat *p = &_p.entries[0];   // projection matrix
    const GLfloat *mv = &_mv.entries[0];  // model-view matrix
    float mvp[16]; // model-view-projection matrix
    float t;

    //
    // Concatenate the projection matrix and the model-view matrix to produce 
    // a combined model-view-projection matrix.
    //
    
    mvp[ 0] = mv[ 0] * p[ 0] + mv[ 1] * p[ 4] + mv[ 2] * p[ 8] + mv[ 3] * p[12];
    mvp[ 1] = mv[ 0] * p[ 1] + mv[ 1] * p[ 5] + mv[ 2] * p[ 9] + mv[ 3] * p[13];
    mvp[ 2] = mv[ 0] * p[ 2] + mv[ 1] * p[ 6] + mv[ 2] * p[10] + mv[ 3] * p[14];
    mvp[ 3] = mv[ 0] * p[ 3] + mv[ 1] * p[ 7] + mv[ 2] * p[11] + mv[ 3] * p[15];

    mvp[ 4] = mv[ 4] * p[ 0] + mv[ 5] * p[ 4] + mv[ 6] * p[ 8] + mv[ 7] * p[12];
    mvp[ 5] = mv[ 4] * p[ 1] + mv[ 5] * p[ 5] + mv[ 6] * p[ 9] + mv[ 7] * p[13];
    mvp[ 6] = mv[ 4] * p[ 2] + mv[ 5] * p[ 6] + mv[ 6] * p[10] + mv[ 7] * p[14];
    mvp[ 7] = mv[ 4] * p[ 3] + mv[ 5] * p[ 7] + mv[ 6] * p[11] + mv[ 7] * p[15];

    mvp[ 8] = mv[ 8] * p[ 0] + mv[ 9] * p[ 4] + mv[10] * p[ 8] + mv[11] * p[12];
    mvp[ 9] = mv[ 8] * p[ 1] + mv[ 9] * p[ 5] + mv[10] * p[ 9] + mv[11] * p[13];
    mvp[10] = mv[ 8] * p[ 2] + mv[ 9] * p[ 6] + mv[10] * p[10] + mv[11] * p[14];
    mvp[11] = mv[ 8] * p[ 3] + mv[ 9] * p[ 7] + mv[10] * p[11] + mv[11] * p[15];

    mvp[12] = mv[12] * p[ 0] + mv[13] * p[ 4] + mv[14] * p[ 8] + mv[15] * p[12];
    mvp[13] = mv[12] * p[ 1] + mv[13] * p[ 5] + mv[14] * p[ 9] + mv[15] * p[13];
    mvp[14] = mv[12] * p[ 2] + mv[13] * p[ 6] + mv[14] * p[10] + mv[15] * p[14];
    mvp[15] = mv[12] * p[ 3] + mv[13] * p[ 7] + mv[14] * p[11] + mv[15] * p[15];

    //
    // Extract the frustum's right clipping plane and normalize it.
    //

    frustumPlanes[0][0] = mvp[ 3] - mvp[ 0];
    frustumPlanes[0][1] = mvp[ 7] - mvp[ 4];
    frustumPlanes[0][2] = mvp[11] - mvp[ 8];
    frustumPlanes[0][3] = mvp[15] - mvp[12];

    t = (float) sqrtf( frustumPlanes[0][0] * frustumPlanes[0][0] + 
                      frustumPlanes[0][1] * frustumPlanes[0][1] + 
                      frustumPlanes[0][2] * frustumPlanes[0][2] );

    frustumPlanes[0][0] /= t;
    frustumPlanes[0][1] /= t;
    frustumPlanes[0][2] /= t;
    frustumPlanes[0][3] /= t;

    //
    // Extract the frustum's left clipping plane and normalize it.
    //

    frustumPlanes[1][0] = mvp[ 3] + mvp[ 0];
    frustumPlanes[1][1] = mvp[ 7] + mvp[ 4];
    frustumPlanes[1][2] = mvp[11] + mvp[ 8];
    frustumPlanes[1][3] = mvp[15] + mvp[12];

    t = (float) sqrtf( frustumPlanes[1][0] * frustumPlanes[1][0] + 
                      frustumPlanes[1][1] * frustumPlanes[1][1] + 
                      frustumPlanes[1][2] * frustumPlanes[1][2] );

    frustumPlanes[1][0] /= t;
    frustumPlanes[1][1] /= t;
    frustumPlanes[1][2] /= t;
    frustumPlanes[1][3] /= t;

    //
    // Extract the frustum's bottom clipping plane and normalize it.
    //

    frustumPlanes[2][0] = mvp[ 3] + mvp[ 1];
    frustumPlanes[2][1] = mvp[ 7] + mvp[ 5];
    frustumPlanes[2][2] = mvp[11] + mvp[ 9];
    frustumPlanes[2][3] = mvp[15] + mvp[13];

    t = (float) sqrtf( frustumPlanes[2][0] * frustumPlanes[2][0] + 
                      frustumPlanes[2][1] * frustumPlanes[2][1] + 
                      frustumPlanes[2][2] * frustumPlanes[2][2] );

    frustumPlanes[2][0] /= t;
    frustumPlanes[2][1] /= t;
    frustumPlanes[2][2] /= t;
    frustumPlanes[2][3] /= t;

    //
    // Extract the frustum's top clipping plane and normalize it.
    //

    frustumPlanes[3][0] = mvp[ 3] - mvp[ 1];
    frustumPlanes[3][1] = mvp[ 7] - mvp[ 5];
    frustumPlanes[3][2] = mvp[11] - mvp[ 9];
    frustumPlanes[3][3] = mvp[15] - mvp[13];

    t = (float) sqrtf( frustumPlanes[3][0] * frustumPlanes[3][0] + 
                      frustumPlanes[3][1] * frustumPlanes[3][1] + 
                      frustumPlanes[3][2] * frustumPlanes[3][2] );

    frustumPlanes[3][0] /= t;
    frustumPlanes[3][1] /= t;
    frustumPlanes[3][2] /= t;
    frustumPlanes[3][3] /= t;

    //
    // Extract the frustum's far clipping plane and normalize it.
    //

    frustumPlanes[4][0] = mvp[ 3] - mvp[ 2];
    frustumPlanes[4][1] = mvp[ 7] - mvp[ 6];
    frustumPlanes[4][2] = mvp[11] - mvp[10];
    frustumPlanes[4][3] = mvp[15] - mvp[14];

    t = (float) sqrtf( frustumPlanes[4][0] * frustumPlanes[4][0] +  
                      frustumPlanes[4][1] * frustumPlanes[4][1] + 
                      frustumPlanes[4][2] * frustumPlanes[4][2] );

    frustumPlanes[4][0] /= t;
    frustumPlanes[4][1] /= t;
    frustumPlanes[4][2] /= t;
    frustumPlanes[4][3] /= t;

    //
    // Extract the frustum's near clipping plane and normalize it.
    //

    frustumPlanes[5][0] = mvp[ 3] + mvp[ 2];
    frustumPlanes[5][1] = mvp[ 7] + mvp[ 6];
    frustumPlanes[5][2] = mvp[11] + mvp[10];
    frustumPlanes[5][3] = mvp[15] + mvp[14];

    t = (float) sqrtf( frustumPlanes[5][0] * frustumPlanes[5][0] + 
                      frustumPlanes[5][1] * frustumPlanes[5][1] + 
                      frustumPlanes[5][2] * frustumPlanes[5][2] );

    frustumPlanes[5][0] /= t;
    frustumPlanes[5][1] /= t;
    frustumPlanes[5][2] /= t;
    frustumPlanes[5][3] /= t;

    // Copy to SIMD values (duplicated out 4 times each) for SIMD optimized implementations
    // TODO: Remove non-SIMD values above if all frustum culling can be made to use SIMD,
    //       and if functions that call frustum culling can all be made to be 4x aligned?

    simdFrustumPlanes[0].normal_x = _mm_set_ps1(frustumPlanes[0][0]);
    simdFrustumPlanes[0].normal_y = _mm_set_ps1(frustumPlanes[0][1]);
    simdFrustumPlanes[0].normal_z = _mm_set_ps1(frustumPlanes[0][2]);
    simdFrustumPlanes[0].d = _mm_set_ps1(frustumPlanes[0][3]);

    simdFrustumPlanes[1].normal_x = _mm_set_ps1(frustumPlanes[1][0]);
    simdFrustumPlanes[1].normal_y = _mm_set_ps1(frustumPlanes[1][1]);
    simdFrustumPlanes[1].normal_z = _mm_set_ps1(frustumPlanes[1][2]);
    simdFrustumPlanes[1].d = _mm_set_ps1(frustumPlanes[1][3]);

    simdFrustumPlanes[2].normal_x = _mm_set_ps1(frustumPlanes[2][0]);
    simdFrustumPlanes[2].normal_y = _mm_set_ps1(frustumPlanes[2][1]);
    simdFrustumPlanes[2].normal_z = _mm_set_ps1(frustumPlanes[2][2]);
    simdFrustumPlanes[2].d = _mm_set_ps1(frustumPlanes[2][3]);

    simdFrustumPlanes[3].normal_x = _mm_set_ps1(frustumPlanes[3][0]);
    simdFrustumPlanes[3].normal_y = _mm_set_ps1(frustumPlanes[3][1]);
    simdFrustumPlanes[3].normal_z = _mm_set_ps1(frustumPlanes[3][2]);
    simdFrustumPlanes[3].d = _mm_set_ps1(frustumPlanes[3][3]);

    simdFrustumPlanes[4].normal_x = _mm_set_ps1(frustumPlanes[4][0]);
    simdFrustumPlanes[4].normal_y = _mm_set_ps1(frustumPlanes[4][1]);
    simdFrustumPlanes[4].normal_z = _mm_set_ps1(frustumPlanes[4][2]);
    simdFrustumPlanes[4].d = _mm_set_ps1(frustumPlanes[4][3]);

    simdFrustumPlanes[5].normal_x = _mm_set_ps1(frustumPlanes[5][0]);
    simdFrustumPlanes[5].normal_y = _mm_set_ps1(frustumPlanes[5][1]);
    simdFrustumPlanes[5].normal_z = _mm_set_ps1(frustumPlanes[5][2]);
    simdFrustumPlanes[5].d = _mm_set_ps1(frustumPlanes[5][3]);
}

//Check if a sphere is visible
int Camera::checkSphereInFrustum( vec3 where, float radius ) const {
    int sphere_in_frustum = 2;
    for (int i = 0; i < 6; ++i ) {
        float d = 
            frustumPlanes[i][0] * where.x() +
            frustumPlanes[i][1] * where.y() +
            frustumPlanes[i][2] * where.z() +
            frustumPlanes[i][3];
        if( d <= -radius ) {
            return 0; // Sphere is entirely outside one of the frustum planes
        } else if(d < radius){
            sphere_in_frustum = 1; // Sphere is intersecting a frustum plane
        }
    }
    return sphere_in_frustum;
}

#if defined(USE_SSE)
static __m128 simd_dot_product(__m128 x1, __m128 y1, __m128 z1, __m128 x2, __m128 y2, __m128 z2)
{
    const auto mul_x = _mm_mul_ps(x1, x2);
    const auto mul_y = _mm_mul_ps(y1, y2);
    const auto mul_z = _mm_mul_ps(z1, z2);
    return _mm_add_ps(_mm_add_ps(mul_x, mul_y), mul_z);
}

static __m128 simd_distance_squared(__m128 x1, __m128 y1, __m128 z1, __m128 x2, __m128 y2, __m128 z2)
{
    const auto diff_x = _mm_sub_ps(x1, x2);
    const auto mul_x = _mm_mul_ps(diff_x, diff_x);
    const auto diff_y = _mm_sub_ps(y1, y2);
    const auto mul_y = _mm_mul_ps(diff_y, diff_y);
    const auto diff_z = _mm_sub_ps(z1, z2);
    const auto mul_z = _mm_mul_ps(diff_z, diff_z);
    return _mm_add_ps(_mm_add_ps(mul_x, mul_y), mul_z);
}
#endif  // defined(USE_SSE)

void Camera::checkSpheresInFrustum(int count, float* where_x, float* where_y, float* where_z, float radius, float cull_distance_squared, uint32_t* is_visible_result) const {
    const vec3 cam_pos = GetPos();
    int i = 0;

#if defined(USE_SSE)
    // This loop exhausts all up to the last 0-3
    const __m128 all_true = _mm_set1_ps((float) 0xFFFFFFFF);
    const __m128 neg_radius4 = _mm_set1_ps(-radius);

    const __m128 cam_pos_x4 = _mm_set1_ps(cam_pos.x());
    const __m128 cam_pos_y4 = _mm_set1_ps(cam_pos.y());
    const __m128 cam_pos_z4 = _mm_set1_ps(cam_pos.z());
    const __m128 cull_distance_squared4 = _mm_set1_ps(cull_distance_squared);

    for (; i <= count - 4; i += 4) {
        const __m128 where_x4 = _mm_load_ps(&where_x[i]);
        const __m128 where_y4 = _mm_load_ps(&where_y[i]);
        const __m128 where_z4 = _mm_load_ps(&where_z[i]);

        __m128 inside = all_true;

        __m128 distance_squared = simd_distance_squared(where_x4, where_y4, where_z4, cam_pos_x4, cam_pos_y4, cam_pos_z4);
        __m128 is_in_view_distance = _mm_cmplt_ps(distance_squared, cull_distance_squared4);
        inside = _mm_and_ps(inside, is_in_view_distance);

        for (unsigned p = 0; p < 6; ++p) {
            const __m128& plane_n_x4 = simdFrustumPlanes[p].normal_x;
            const __m128& plane_n_y4 = simdFrustumPlanes[p].normal_y;
            const __m128& plane_n_z4 = simdFrustumPlanes[p].normal_z;
            __m128 n_dot_pos = simd_dot_product(where_x4, where_y4, where_z4, plane_n_x4, plane_n_y4, plane_n_z4);

            __m128 plane_test = _mm_cmpgt_ps(_mm_add_ps(n_dot_pos, simdFrustumPlanes[p].d), neg_radius4);
            inside = _mm_and_ps(inside, plane_test);
        }

        _mm_store_ps((float*)&is_visible_result[i], inside);
    }
#endif  // defined(USE_SSE)

    for (; i < count; ++i) {
        bool inside = true;

        float center_to_cam[3] = {
            where_x[i] - cam_pos.x(),
            where_y[i] - cam_pos.y(),
            where_z[i] - cam_pos.z(),
        };
        float distance_squared = center_to_cam[0] * center_to_cam[0] +
            center_to_cam[1] * center_to_cam[1] +
            center_to_cam[2] * center_to_cam[2];

        if (distance_squared < cull_distance_squared) {
            for (int p = 0; p < 6; ++p) {
                float n_dot_pos =
                    frustumPlanes[p][0] * where_x[i] +
                    frustumPlanes[p][1] * where_y[i] +
                    frustumPlanes[p][2] * where_z[i];
                bool plane_test = n_dot_pos + frustumPlanes[p][3] > -radius;
                inside = inside && plane_test;
            }
        } else {
            inside = false;
        }

        is_visible_result[i] = inside ? 0xFFFFFFFF : 0;
    }
}


//Check if a box is visible
int Camera::checkBoxInFrustum(vec3 start, vec3 end) const
{
    int total_in = 0;
    vec3 point;

    for(int p = 0; p < 6; ++p) {
    
        int in_count = 8;
        bool box_in_frustum = 1;

        for (int i = 0; i < 8; ++i) {
            if(i<4)point.x()=start.x();
            else point.x()=end.x();
            if(i%4<2)point.y()=start.y();
            else point.y()=end.y();
            if(i%2==0)point.z()=start.z();
            else point.z()=end.z();

            // test this point against the planes
            if( frustumPlanes[p][0] * point.x() +
                frustumPlanes[p][1] * point.y() +
                frustumPlanes[p][2] * point.z() +
                frustumPlanes[p][3] < 0) {
                box_in_frustum = 0;
                in_count--;
            }
        }

        // were all the points outside of plane p?
        if(in_count == 0)
            return(0);

        // check if they were all on the right side of the plane
        total_in += box_in_frustum;
    }

    // so if iTotalIn is 6, then all are inside the view
    if(total_in == 6)
        return(2);

    // we must be partly in then otherwise
    return(1);
}

const vec3& Camera::GetVelocity() const
{
    return velocity;
}

void Camera::SetVelocity( const vec3 &vel )
{
    velocity = vel;
}

void Camera::ASSetPos( vec3 pos )
{
    SetPos(pos);
}

vec3 Camera::ASGetPos() const
{
    return GetPos();
}

void Camera::ASSetVelocity(vec3 vel)
{
    SetVelocity(vel);
}

vec3 Camera::ASGetUpVector() const
{
    return GetUpVector();
}

void Camera::ASSetFacing( vec3 _facing ) {
    SetFacing(_facing);
}

void Camera::ASSetUp( vec3 _up )
{
    SetUp(_up);
}

void Camera::ASLookAt(vec3 new_val)
{
    LookAt(new_val);
}

vec3 Camera::ASGetMouseRay() const
{
    return GetMouseRay();
}

void Camera::reset() {
    target_horz_fov=90.0f;
    near_plane=0.1f;
    far_plane=1000.0f;
    chase_distance=0.0f;
    old_chase_distance=0.0f;
    y_rotation=0.0f;
    x_rotation=0.0f;
    z_rotation=0.0f;
    old_y_rotation=0.0f;
    old_x_rotation=0.0f;
    old_z_rotation=0.0f;
    interp_steps = 1;
    interp_progress = 0;
    flags_ = 0;
    flexible_fov = true;
}

void Camera::SetInterpSteps( int num_steps )
{
    interp_steps = num_steps;
    interp_progress = 0;
}

void Camera::IncrementProgress()
{
    ++interp_progress;
    if(interp_progress >= interp_steps){
        interp_progress = 0;
    }
}

mat4 Camera::GetViewMatrix()
{
    mat4 view_mat;
    for(int i=0; i<16; ++i){
        view_mat.entries[i] = (float)modelview_matrix[i]; 
    }
    return view_mat;
}

mat4 Camera::GetProjMatrix()
{
    mat4 proj_mat;
    for(int i=0; i<16; ++i){
        proj_mat.entries[i] = (float)projection_matrix[i]; 
    }
    return proj_mat;
}

void Camera::SetAutoCamera( bool val )
{
    auto_camera = val;
}

bool Camera::GetAutoCamera()
{
    return auto_camera;
}

float Camera::GetInterpWeight() const {
    return game_timer.GetInterpWeightX(interp_steps,interp_progress);
}

vec3 Camera::GetCenterVel() const {
    return (pos - old_pos) * (float)game_timer.simulations_per_second / (float)interp_steps;
}

int Camera::GetFlags() {
    return flags_;
}

void Camera::SetFlags(int val) {
    flags_ = val;
}

void Camera::SetMatrices(const mat4& view, const mat4& proj) {
    for(int i=0; i<16; ++i){
        modelview_matrix[i] = (double)view[i]; 
    }
    for(int i=0; i<16; ++i){
        projection_matrix[i] = (double)proj[i]; 
    }
}

/*
void Camera::PrintDifferences( const Camera& cam )
{
    LOGI << "Following things are different" << std::endl;

    if( facing != cam.facing )
    {
        LOGI << "facing" << std::endl;
    }

    if( flat_facing != cam.flat_facing )
    {
        LOGI << "flat_facing" << std::endl;
    }

    if( target_horz_fov != cam.target_horz_fov )
    {
        LOGI << "target_horz_fov" << std::endl;
    }

    if( near_plane != cam.near_plane )
    {
        LOGI << "near_plane" << std::endl;
    }

    if( far_plane != cam.far_plane )
    {
        LOGI << "far_plane" << std::endl;
    }

    if( chase_distance != cam.chase_distance )
    {
        LOGI << "chase_distance" << std::endl;    
    }

    if( old_chase_distance != cam.old_chase_distance )
    {
        LOGI << "old_chase_distance" << std::endl;
    }

    if( interp_chase_distance != cam.interp_chase_distance )
    {
        LOGI << "interp_chase_distance" << std::endl;
    }

    if( velocity != cam.velocity )
    {
        LOGI << "velocity" << std::endl;
    }

    if( up != cam.up )
    {
        LOGI << "up" << std::endl; 
    }

    if( shake_facing != cam.shake_facing )
    {
        LOGI << "shake_facing" << std::endl;
    }
    
    if( pos != cam.pos )
    {
        LOGI << "pos" << std::endl;
    }

    if( old_pos != cam.old_pos )
    {
        LOGI << "old_pos" << std::endl;
    }

    if( interp_pos != cam.interp_pos )
    {
        LOGI << "interp_pos" << std::endl;
    }

    if( x_rotation != cam.x_rotation )
    {
        LOGI << "x_rotation" << std::endl;
    }
    
    if( y_rotation != cam.y_rotation )
    {
        LOGI << "y_rotation" << std::endl;
    }

    if( z_rotation != cam.z_rotation )
    {
        LOGI << "z_rotation" << std::endl;
    }

    if( old_x_rotation != cam.old_x_rotation )
    {
        LOGI << "old_x_rotation" << std::endl;
    }

    if( old_y_rotation != cam.old_y_rotation )
    {
        LOGI << "old_y_rotation" << std::endl;
    }

    if( old_z_rotation != cam.old_z_rotation )
    {
        LOGI << "old_z_rotation" << std::endl;
    }

    if( collision_detection != cam.collision_detection )
    {
        LOGI << "collision_detection" << std::endl;
    }

    if( shake_amount != cam.shake_amount )
    {
        LOGI << "shake_amount" << std::endl;
    }

    if( cameraViewMatrix != cam.cameraViewMatrix )
    {
        LOGI << "cameraViewMatrix" << std::endl;
    }

    if( inverseCameraViewMatrix != cam.inverseCameraViewMatrix )
    {
        LOGI << "inverseCameraViewMatrix" << std::endl;
    }

    if( lightProjectionMatrix != cam.lightProjectionMatrix )
    {
        LOGI << "lightProjectionMatrix" << std::endl;
    }

    if( lightViewMatrix != cam.lightViewMatrix )
    {
        LOGI << "lightViewMatrix" << std::endl;
    }
    
    if( mouseray != cam.mouseray )
    {
        LOGI << "mouseray" << std::endl;
    }
    
    //modelview_matrix[16];
    //projection_matrix[16]; 
    //viewport[4];

    if( interp_steps != cam.interp_steps )
    {
        LOGI << "interp_steps" << std::endl;
    }

    if( interp_progress != cam.interp_progress )
    {
        LOGI << "interp_progress" << std::endl;
    }

    if( auto_camera != cam.auto_camera )
    {
        LOGI << "auto_camera" << std::endl;
    }

    if( flags_ != cam.flags_ )
    {
        LOGI << "flags_" << std::endl;
    }

    if( prev_view_mat != cam.prev_view_mat )
    {
        LOGI << "prev_view_mat" << std::endl;    
    }

    if( near_blur_amount != cam.near_blur_amount )
    {
        LOGI << "near_blur_amount" << std::endl;
    }

    if( far_blur_amount != cam.far_blur_amount )
    {
        LOGI << "far_blur_amount" << std::endl;
    }

    if( near_sharp_dist != cam.near_sharp_dist )
    {
        LOGI << "near_sharp_dist" << std::endl;
    }

    if( far_sharp_dist != cam.far_sharp_dist )
    {
        LOGI << "far_sharp_dist" << std::endl;
    }

    if( near_blur_transition_size != cam.near_blur_transition_size )
    {
        LOGI << "near_blur_transition_size" << std::endl;
    }

    if( far_blur_transition_size != cam.far_blur_transition_size )
    {
        LOGI << "far_blur_transition_size" << std::endl;
    }

    //frustumPlanes[6][4];
    
    if( biasMatrix != cam.biasMatrix )
    {
        LOGI << "biasMatrix" << std::endl;
    }

    if( flexible_fov != cam.flexible_fov )
    {
        LOGI << "flexible_fov" << std::endl;
    }

    LOGI << "end camera diff" << std::endl;
}
*/
