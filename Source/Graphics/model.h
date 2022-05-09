//-----------------------------------------------------------------------------
//           Name: model.h
//      Developer: Wolfire Games LLC
//         Author: David Rosen
//    Description: This class loads and displays models
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

#include "Math/vec2.h"
#include "Math/mat3.h"

#include "Graphics/vbocontainer.h"
#include "Internal/filesystem.h"
#include "Asset/Asset/image_sampler.h"

#include <opengl.h>

#include <vector>
#include <stdio.h>
#include <string>

class Mesh;
class DrawBatch;

//-----------------------------------------------------------------------------
// Class Definition
//-----------------------------------------------------------------------------

enum ModelFlags {
    _MDL_CENTER = (1 << 0),
    _MDL_SIMPLE = (1 << 1),
    _MDL_FLIP_FACES = (1 << 2),
    _MDL_USE_TANGENT = (1 << 3)
};

const unsigned short _model_cache_version = 41;
const unsigned short _morph_cache_version = 3;

class Model;
class vec4;

class Model {
   public:
    std::vector<GLfloat> vertices;
    std::vector<GLfloat> normals;
    std::vector<GLfloat> tangents;
    std::vector<GLfloat> bitangents;
    std::vector<vec3> face_normals;
    std::vector<GLfloat> tex_coords;
    std::vector<GLfloat> tex_coords2;
    std::vector<GLfloat> aux;
    std::vector<GLuint> faces;

    std::vector<vec4> bone_weights;
    std::vector<vec4> bone_ids;
    std::vector<std::vector<vec4> > transform_vec;
    std::vector<vec2> tex_transform_vec;
    std::vector<vec3> morph_transform_vec;

    std::set<ImageSamplerRef> image_samplers;

    vec3 min_coords, max_coords, center_coords;

    bool vbo_loaded;
    bool vbo_enabled;
    unsigned short checksum;

    VBOContainer VBO_vertices;
    VBOContainer VBO_normals;
    VBOContainer VBO_tangents;
    VBOContainer VBO_bitangents;
    VBOContainer VBO_tex_coords;
    VBOContainer VBO_tex_coords2;
    VBOContainer VBO_aux;
    VBOContainer VBO_faces;

    std::vector<int> optimize_vert_reorder;
    std::vector<int> precollapse_vert_reorder;
    int precollapse_num_vertices;

    bool use_tangent;
    vec3 old_center;

    vec3 bounding_sphere_origin;
    float bounding_sphere_radius;

    float texel_density;
    float average_triangle_edge_length;

    std::string path;  // Source file. Primarily for debugging purposes.
    ModID modsource_;

    int lineCheckNoBackface(const vec3 &p1, const vec3 &p2, vec3 *p, vec3 *normal = 0) const;
    int lineCheck(const vec3 &p1, const vec3 &p2, vec3 *p, vec3 *normal = 0, bool backface = true) const;
    void LoadObj(const std::string &name, char flags = _MDL_CENTER, const std::string &alt_name = "", const PathFlags searchPaths = kAnyPath);
    void RemoveDoubledTriangles();
    void Draw();
    void DrawAltTexCoords();
    void DrawToTextureCoords();
    void SmoothNormals();
    void SmoothTangents();
    void calcFaceNormals();
    void calcBoundingBox();
    void calcBoundingSphere();
    void CenterModel();
    void calcNormals();
    void calcTangents();
    void calcNormals(bool smooth);
    void createVBO();
    void ResizeVertices(int size);
    void ResizeFaces(int size);
    void WriteToFile(FILE *file);
    void ReadFromFile(FILE *file);
    void CalcTexelDensity();
    void CalcAverageTriangleEdge();
    void Dispose();
    bool LoadAuxFromImage(const std::string &image_path);

    Model();
    Model(const Model &copy);
    Model &operator=(const Model &copy);

    virtual ~Model();

    void StartPreset();
    void EndPreset();
    void DrawPreset();
    void CopyFacesFromModel(const Model &source_model, const std::vector<int> &faces);
    void LoadObjMorph(const std::string &name, const std::string &base_name);
    void CopyVertCollapse(const Model &source_model);
    void RemoveDuplicatedVerts();
    void OptimizeTriangleOrder();
    void SortTrianglesBackToFront(const vec3 &camera);
    void PrintACMR();
    void OptimizeVertexOrder();
    void SaveObj(std::string name);
    void RemoveDegenerateTriangles();

    const static int ERROR_MORE_THAN_ONE_OBJECT;
    int SimpleLoadTriangleCutObj(const std::string &name_to_load);
    const char *GetLoadErrorString(int err);
};
void CopyTexCoords2(Model &a, const Model &b);

class Shaders;
class Graphics;
void DrawModelVerts(Model &model, const char *vert_attrib_str, Shaders *shaders, Graphics *graphics, int shader);
