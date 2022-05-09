//-----------------------------------------------------------------------------
//           Name: drawbatch.h
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

#include <Graphics/glstate.h>
#include <Graphics/vbocontainer.h>

#include <Math/vec4.h>
#include <Math/mat4.h>
#include <Math/mat3.h>

#include <Asset/Asset/texture.h>

#include <opengl.h>

#include <vector>
#include <string>

enum UniformType { _mat3,
                   _mat4,
                   _vec3,
                   _vec4,
                   _float };

class Uniform {
   public:
    UniformType type;
    std::string name;
    std::vector<GLfloat> data;
    bool individual;
    void Apply() const;
    Uniform();
};

class DrawBatch;

class AddedBatch {
   public:
    const DrawBatch *batch;
    GLuint start_face;
    GLuint end_face;
    GLuint start_vertex;
    GLuint end_vertex;
};

const int _vbo_threshold = 5;

class DrawBatch {
   public:
    bool visible;
    bool no_vbo;
    vec4 shadow_offset;
    vec4 color;
    std::vector<GLfloat> normals;
    std::vector<GLfloat> vertices;
    std::vector<GLfloat> tex_coords[8];
    int tex_coord_elements[8];
    std::vector<GLuint> faces;

    bool use_vbo_texcoords[8];

    RC_VBOContainer vbo_normals;
    RC_VBOContainer vbo_vertices;
    RC_VBOContainer vbo_tex_coords[8];
    RC_VBOContainer vbo_faces;

    vec3 bounding_sphere_center;
    float bounding_sphere_radius;

    GLfloat *normal_ptr;
    GLfloat *vertex_ptr;
    GLfloat *tex_coord_ptr[8];
    const GLuint *face_ptr;
    GLuint num_face_ids;

    int draw_times;

    std::vector<AddedBatch> added_batches;

    bool use_cam_pos;
    bool use_time;
    bool use_light;

    bool transparent;

    GLState gl_state;

    mat4 shadow_proj_matrix;
    mat4 shadow_view_matrix;
    GLenum depth_func;
    bool polygon_offset;
    mat4 transform;
    mat3 inv_transpose_transform;

    int shader_id;

    static const int kMaxTextures = 32;
    TextureRef texture_ref[kMaxTextures];

    bool to_delete;

    std::vector<Uniform> uniforms;
    void SetTransform(const mat4 &_transform);
    DrawBatch();

    enum CreateVBOParam { NO_FACE_VBO,
                          FACE_VBO };

    void Draw();

    void SetupVertexArrays() const;
    void SetEndState() const;
    void SetStartState() const;
    void ApplyIndividualUniforms() const;
    void AddUniformMat3(std::string _name, const GLfloat *_data);
    void AddUniformMat4(std::string _name, const GLfloat *_data);
    void AddUniformVec3(std::string _name, const GLfloat *_data);
    void AddUniformVec3(std::string _name, const vec3 &_data);
    void AddUniformVec4(std::string _name, const GLfloat *_data);
    void AddUniformVec4(std::string _name, const vec4 &_data);
    void TexCoordPointer(int size, GLfloat *data, int which_tex);
    void VertexPointer(GLfloat *data);
    void NormalPointer(GLfloat *data);
    void DrawElements(GLuint size, const GLuint *data);
    void AddBatch(const DrawBatch &other);
    void Dispose();
    void CreateVBO(CreateVBOParam create_vbo_param);
    void DrawVertexArrays() const;
    void DrawVertexArrayRange(GLuint start, GLuint end, GLuint start_vertex, GLuint end_vertex) const;
    void EndVertexArrays() const;
    void AddUniformFloat(std::string _name, GLfloat data);
    void SetUniformFloat(std::string _name, GLfloat data);
    void SetUniformVec3(std::string _name, const vec3 &_data);
    void SetUniformVec4(std::string _name, const vec4 &_data);
    void MakeUniformIndividual(std::string _name);
};
