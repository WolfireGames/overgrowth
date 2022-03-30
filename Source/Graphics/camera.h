//-----------------------------------------------------------------------------
//           Name: camera.cpp
//      Developer: Wolfire Games LLC
//         Author: David Rosen
//    Description: The camera class holds the position, and rotation of the
//                   camera, and the view settings
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

#include <Graphics/graphics.h>
#include <Math/vec4.h>

#include <opengl.h>

#include <vector>
#include <list>
#include <emmintrin.h>

class CameraObject;

/// The Camera class handles the view orientation in 3D space.
class Camera
{
private:
    vec3 facing;  ///< Vector for which direction the camera is pointing
    vec3 flat_facing; ///< Facing on the XZ plane
    float target_horz_fov; ///< The field of view, in degrees.
    float near_plane; ///< Distance to the near plane
    float far_plane; ///< Distance to the far plane
    float chase_distance; ///< How far the chase camera is from the player.
    float old_chase_distance;
    float interp_chase_distance;

    vec3 velocity;
    vec3 up; ///< Up vector (combined with facing, gives complete orientation)
    
    vec3 pos; ///< Current position
    vec3 old_pos; ///< Previous timestep position
	public: // Made this public temporarily to fix VR problem TODO: do this more cleanly
    vec3 interp_pos; ///< Interpolated position (for smooth rendering)
    private:
    float y_rotation; ///< Rotation on the y axis (left and right)
    float x_rotation; ///< Rotation on the x axis (up and down)
    float z_rotation; ///< Rotation on the z axis (roll clockwise and ccw)
    float old_y_rotation; ///< Previous timestep y_rotation
    float old_x_rotation; ///< Previous timestep x_rotation
    float old_z_rotation; ///< Previous timestep z_rotation

    mat4 cameraViewMatrix; ///< Matrix to transform world to view
    mat4 inverseCameraViewMatrix; ///< Matrix to transform view to world
    mat4 lightProjectionMatrix; ///< Ortographic projection matrix for shadows
    mat4 lightViewMatrix; ///< View matrix from light pov for shadows
    
    vec3 mouseray; ///< Vector in the direction that the mouse is pointing
    
    GLdouble modelview_matrix[16]; ///< Camera modelview matrix
    GLdouble projection_matrix[16]; ///< Camera projection matrix
    GLint viewport[4]; ///< Viewport dimensions (in pixels)

    int interp_steps;
    int interp_progress;
    float GetInterpWeight() const;

    bool auto_camera;
    int flags_;        

    /// Initializes the camera member variables to default
    void reset();

public:
    vec3 tint;
    vec3 vignette_tint;
    mat4 prev_view_mat;
    // Depth of field filter settings
    float near_blur_amount;
    float far_blur_amount;
    float near_sharp_dist;
    float far_sharp_dist;
    float near_blur_transition_size;
    float far_blur_transition_size;

    float frustumPlanes[6][4]; ///< Frustum plane parameters (ax+by+cz+d)

    struct SIMDPlane {  // TODO: Comment out if SIMD not supported?
        __m128 normal_x;  // same value pre-replicated 4 times for each of these
        __m128 normal_y;
        __m128 normal_z;
        __m128 d;
    };
    SIMDPlane simdFrustumPlanes[6];

    mat4 biasMatrix; ///< Bias matrix for shadow projection
    bool flexible_fov;
    enum CameraFlags {
        kEditorCamera = 1 << 0,
        kPreviewCamera = 1 << 1
    };

    vec3 GetCenterVel() const;
    const vec3& GetFacing() const; ///< Accessor for facing
    const vec3& GetFlatFacing() const; ///< Accessor for flat_facing
    void SetFlatFacing(const vec3 &v); ///< Mutator for flat_facing
	void FixDiscontinuity();

    /// The default Camera constructer initializes the biasMatrix and calls reset()
    Camera();

    CameraObject* m_camera_object; ///<CameraObject exposed for global access (fix)
    
    /**
    * Sets the CameraObject so that it can be accessed globally.
    * @param camera_object A pointer to the CameraObject.
    */
    inline void SetCameraObject(CameraObject* camera_object) { 
        reset(); 
        m_camera_object = camera_object; 
    };
    
    /**
    * Get the CameraObject that is controlling the camera.
    */
    CameraObject* getCameraObject() {
        return m_camera_object; 
    }

    /**
    * Set how far the camera is behind the player.
    * @param chase_distance The new chase distance.
    */
    void SetDistance(const float chase_distance);
    
    /**
    * Set the Y-axis (left and right) rotation of the camera.
    * @param y_rotation The new Y-axis rotation in degrees.
    */
    void SetYRotation(const float y_rotation);    
    
    /**
    * Set the X-axis (up and down) rotation of the camera.
    * @param x_rotation The new X-axis rotation in degrees.
    */
    void SetXRotation(const float x_rotation);

    /**
    * Set the Z-axis (roll) rotation of the camera.
    * @param z_rotation The new Z-axis rotation in degrees.
    */
    void SetZRotation(const float z_rotation);

    void SetInterpSteps(int num_steps);
    void IncrementProgress();

    /**
    * Set the position of the camera.
    * @param pos The new position.
    */
    void SetPos(const vec3 &pos);
    void ASSetPos(vec3 pos);

    /**
    * Set the field of view of the camera.
    * @param fov The new FOV in degrees.
    */
    void SetFOV(const float fov);
    
    /**
    * Set the x and y rotation of the camera to look at a point.
    * @param point The point to look at.
    */
    void LookAt(const vec3 &point);
    

    // Lots of accessors
    float GetYRotation() const;
    float GetXRotation() const;
    float GetZRotation() const;
    vec3 GetPos() const;
    vec3 ASGetPos() const;
    const vec3& GetMouseRay() const;
    vec3 ASGetMouseRay() const;
    const vec3& GetUpVector() const;
    vec3 ASGetUpVector() const;
    const float& GetNearPlane() const;
    const float& GetFarPlane() const;
    const float& GetFOV() const;
    const vec3& GetVelocity() const;
    void SetVelocity(const vec3 &vel);

    void SetAutoCamera( bool val );
    bool GetAutoCamera( );

    /**
    * Get the Camera->World transformation matrix.
    * @return The 4x4 transform matrix.
    */
    const mat4& getInverseCameraMatrix() const;
    
    /**
    * Get the World->Camera transformation matrix.
    * @return The 4x4 transform matrix.
    */
    const mat4& getCameraMatrix() const;

    /**
    * Convert a point from world coordinates to camera coordinates.
    * @param absolute The point in world space to convert.
    * @return The point in camera space.
    */ 
    vec3 getRelative(const vec3 &absolute);
    
    /**
    * Convert a point from world coordinates to screen coordinates.
    * @param point The point in world space to convert.
    * @return The point in screen space.
    */ 
    vec3 worldToScreen(const vec3 point) const;

    /**
    * Set up an orthographic projection for use with shadow rendering.
    * @param direction The direction that the light is coming from.
    *                   The projection will face the opposite direction.
    * @param center       Where the projection will start.
    * @param scale       The dimensions of the projection square.
    */
    void applyShadowViewpoint(vec3 direction, vec3 center, float scale, mat4 *proj_matrix = NULL, mat4 *view_matrix = NULL, float far_back = 1000.0f);

    /**
    * Apply the camera view to the OpenGL matrices.
    */
    void applyViewpoint();

    /**
    * Gets a vector that points through a specific pixel on the screen.
    * @param x X-coordinate in pixels
    * @param y Y-coordinate in pixels
    * @return The 3D vector that would go through that pixel.
    */
    vec3 GetRayThroughPixel(int x, int y);

    /**
    * Get the coordinates of a pixel (un)projected into world space.
    * @param x X-coordinate in pixels
    * @param y Y-coordinate in pixels
    * @return The 3D coordinates of the (un)projected pixel.
    */
    vec3 UnProjectPixel(int x, int y);

    /**
    * Get the coordinates of a world space point projected on the screen.
    * @param point The world space point to project
    * @return The coordinates of the pixel.
    */
    vec3 ProjectPoint(const vec3 &point);
    vec3 ProjectPoint(float x, float y, float z);

    /**
    * Calculate facing vectors based on rotation values.
    */
    void CalcFacing();
    
    /**
    * Calculate up vector based on facing vector and global up (0,1,0).
    */
    void calcUp();
    
    /**
    * Calculate frustum plane parameters based on modelview and 
    * projection matrices.
    */
    void calcFrustumPlanes( const mat4 &p, const mat4 &mv );
        
    /**
    * Check if a sphere is within the view frustum.
    * @param where The center of the sphere.
    * @param radius The radius of the sphere.
    * @return Returns 2 if the sphere is entirely in the frustum, 1 if partially, 0 if not at all
    */
    int checkSphereInFrustum(vec3 where, float radius) const;

    /**
    * Check if an array of spheres are within the view frustum.
    * @param count The number of spheres to check.
    * @param where_x The center of the sphere (x).
    * @param where_y The center of the sphere (y).
    * @param where_z The center of the sphere (z).
    * @param radius The radius of the spheres (same across all the spheres).
    * @param radius The view distance to cull against before checking against frustum planes.
    * @param is_visible_result The result of the visibility checks will be written here, not bit-packed.
    */
    void checkSpheresInFrustum(int count, float* where_x, float* where_y, float* where_z, float radius, float cull_distance_squared, uint32_t* is_visible_result) const;

    /**
    * Check if an axis-aligned box is within the view frustum.
    * @param start The minimum corner of the box.
    * @param end The maximum corner of the box.
    * @return Returns 2 if fully in, 1 if partly in, 0 if out of frustum.
    */
    int checkBoxInFrustum(vec3 start, vec3 end) const;

    /**
    * Camera uses the Singleton pattern, this returns a pointer to the static Camera.
    * @return Returns a pointer to the static Camera.
    */
    void PushState();
    void PopState();
    mat4 GetPerspectiveMatrix() const;
    void ASSetVelocity(vec3 vel);
    void UpdateShake();
    float GetChaseDistance() const;
    void SetFacing(const vec3 &_facing);
    void SetUp(const vec3 &_up);
    void ASSetUp( vec3 _up );
    void ASSetFacing( vec3 _facing );
    void ASLookAt(vec3 new_val);
    mat4 GetViewMatrix();
    mat4 GetProjMatrix();
    void DrawSafeZone();
    int GetFlags();
    void SetFlags(int val);

    //void PrintDifferences( const Camera& cam );
    void SetMatrices(const mat4& view, const mat4& proj);
};

class ActiveCameras {
private:
    //Cameras used logically to represent a remote virtual player, but isn't used for rendering.
    std::vector<Camera> virtual_cameras;
    std::vector<int> free_virtual_cameras;

    std::list<Camera> cameras;
    std::vector<Camera*> camera_ptrs;
    Camera* active;
    int active_id;

    void mSet(int which) {
        if(which >= (int)cameras.size()){
            cameras.resize(which+1);
            camera_ptrs.resize(cameras.size());
            int counter = 0;
            for(std::list<Camera>::iterator iter = cameras.begin(); 
                iter != cameras.end(); ++iter)
            {
                camera_ptrs[counter++] = &(*iter);
            }
        }
        active = camera_ptrs[which];
        active_id = which;
    }
public:
	std::vector<Camera>& GetVirtualCameras() {
		return virtual_cameras;
	}
 

    int CreateVirtualCameraInstance() {
        int virtual_camera_index = 0;
        if(free_virtual_cameras.size() > 0) {
            virtual_camera_index = *free_virtual_cameras.rbegin();
            free_virtual_cameras.resize(free_virtual_cameras.size() - 1);
        } else {
            virtual_cameras.resize(virtual_cameras.size() + 1);
            virtual_camera_index = (int) virtual_cameras.size() - 1;
        }

        return virtual_camera_index + 255;
    }

    void FreeVirtualCameraInstance(int index) {
        int virtual_camera_index = index - 255;

        if(virtual_camera_index >= 0) {
            free_virtual_cameras.push_back(virtual_camera_index);
        }
    }

    static ActiveCameras* Instance() {
        static ActiveCameras instance;
        return &instance;
    }

    static Camera* Get() {
        return Instance()->active;
    }

    static Camera* GetCamera(int camera_id) {
        if (camera_id >= 255) {
            int virtual_camera_index = camera_id - 255;

            if (virtual_camera_index >= 0 && virtual_camera_index < Instance()->virtual_cameras.size()) {
                return &Instance()->virtual_cameras[virtual_camera_index];
            } else {
                return nullptr;
            }
        } else {
#ifdef DEBUG
            if(NumCameras() <= camera_id) {
                LOGW << "Trying to get camera with id " << camera_id << ", but only " << NumCameras() << " are available" << std::endl;
                return Instance()->camera_ptrs[camera_id];
            }
#endif
            if (camera_id >= 0 && camera_id < Instance()->camera_ptrs.size()) {
                return Instance()->camera_ptrs[camera_id];
            } else {
                return nullptr;
            }
        }
    }

    static int NumCameras() {
        return (int) Instance()->cameras.size();
    }

    void UpdatePrevViewMats() {
        for(std::list<Camera>::iterator iter = cameras.begin(); 
            iter != cameras.end(); ++iter)
        {
            Camera* camera = &(*iter);
            camera->prev_view_mat = camera->GetViewMatrix();
        }

        for(std::vector<Camera>::iterator iter = virtual_cameras.begin(); 
            iter != virtual_cameras.end(); ++iter)
        {
            Camera* camera = &(*iter);
            camera->prev_view_mat = camera->GetViewMatrix();
        }
    }

    static int GetID() {
        return Instance()->active_id;
    }

    static void Set(int which) {
        Instance()->mSet(which);
    }

    void Dispose() {
        virtual_cameras.clear();
        cameras.clear();
        camera_ptrs.clear();
        active = NULL;
    }

};
