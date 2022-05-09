//-----------------------------------------------------------------------------
//           Name: terrain.cpp
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
#include "terrain.h"

#include <Graphics/textures.h>
#include <Graphics/camera.h>
#include <Graphics/graphics.h>
#include <Graphics/geometry.h>
#include <Graphics/bytecolor.h>
#include <Graphics/shaders.h>
#include <Graphics/sky.h>
#include <Graphics/models.h>
#include <Graphics/ColorWheel.h>
#include <Graphics/pxdebugdraw.h>

#include <Internal/common.h>
#include <Internal/checksum.h>
#include <Internal/collisiondetection.h>
#include <Internal/filesystem.h>

#include <Images/texture_data.h>
#include <Images/image_export.hpp>

#include <Math/triangle.h>
#include <Math/vec3math.h>

#include <Timing/timingevent.h>
#include <Timing/intel_gl_perf.h>

#include <Main/scenegraph.h>
#include <Main/engine.h>

#include <Physics/physics.h>
#include <Compat/fileio.h>
#include <Logging/logdata.h>
#include <Memory/stack_allocator.h>
#include <Asset/Asset/averagecolorasset.h>
#include <GUI/widgetframework.h>
#include <Memory/allocation.h>

#include <cfloat>
#include <set>
#include <algorithm>
#include <cmath>

extern SceneLight *primary_light;

static const uint16_t _terrain_cache_file_version_number = 27;
static const float kHeightMapHeight = 140.0f;
static const float kUniformScale = 2.0f;
static const float kVerticalScale = 0.1f;

Terrain::Terrain() : model_id(-1),
                     framebuffer(INVALID_FRAMEBUFFER),
                     shader("secondterrain") {}

// Dispose of terrain
Terrain::~Terrain() {
    Dispose();
}

void Terrain::Dispose() {
    LOGI << "Disposing of terrain" << std::endl;
    terrain_patches.clear();
    LOGI << "Clearing color path" << std::endl;
    color_path.clear();
    LOGI << "Clearing detail texture ref" << std::endl;
    detail_texture_ref.clear();
    LOGI << "Clearing detail normal texture ref" << std::endl;
    detail_normal_texture_ref.clear();
    LOGI << "Clearing detail texture color" << std::endl;
    detail_texture_color.clear();
    LOGI << "Clearing texture color srgb" << std::endl;
    detail_texture_color_srgb.clear();
    LOGI << "Clearing detail maps" << std::endl;
    detail_maps_info.clear();
    LOGI << "Clearing detail object surfaces" << std::endl;
    for (auto &detail_object_surface : detail_object_surfaces) {
        delete detail_object_surface;
    }
    detail_object_surfaces.clear();
    detail_maps_info.clear();
    LOGI << "Done with Dispose()" << std::endl;
}

void Terrain::GLInit(Sky *sky) {
    if (minimal)
        return;

    // Create framebuffer object
    Graphics *graphics = Graphics::Instance();
    if (framebuffer == INVALID_FRAMEBUFFER) {
        graphics->PushFramebuffer();
        graphics->genFramebuffers(&framebuffer, "sky");
        graphics->bindFramebuffer(framebuffer);
        graphics->PopFramebuffer();
    }
    if (!normal_map_ref.valid()) {
        CHECK_GL_ERROR();

        // Create lighting textures
        Textures *textures = Textures::Instance();
        baked_texture_ref = textures->makeTexture(terrain_texture_size, terrain_texture_size, GL_RGBA, GL_RGBA, true);
        textures->SetTextureName(baked_texture_ref, "Terrain Baked Texture");

        // Create skybox without terrain
        sky->BakeFirstPass();

        {  // Draw to texture

            CHECK_GL_ERROR();
            // Get normal map write path
            std::string normal_map_path = heightmap_.path() + "_normal.png";
            std::string w_nmp = GetWritePath(heightmap_.modsource_) + normal_map_path;
            if (GetDateModifiedInt64(w_nmp.c_str()) >= GetDateModifiedInt64(normal_map_path.c_str())) {
                normal_map_path = w_nmp;
            }

            // Bake normal map
            FILE *test_file = my_fopen(normal_map_path.c_str(), "rb");
            if (test_file) {
                fclose(test_file);
            } else {
                float res_scale = 512.0f / heightmap_.width();
                int size = heightmap_.width();
                std::vector<unsigned char> color(size * size * 4);
                vec3 points[5];
                vec3 normals[4];
                vec3 normal;
                for (int j = 0; j < size; ++j) {
                    for (int i = 0; i < size; ++i) {
                        for (int k = 0; k < 5; ++k) {
                            points[k][0] = (float)(i);
                            points[k][2] = (float)(j);
                            switch (k) {
                                case 0:
                                    break;
                                case 1:
                                    points[k][2] -= 1.0f;
                                    break;
                                case 2:
                                    points[k][0] += 1.0f;
                                    break;
                                case 3:
                                    points[k][2] += 1.0f;
                                    break;
                                case 4:
                                    points[k][0] -= 1.0f;
                                    break;
                            }
                            points[k][1] = heightmap_.GetHeight((int)points[k][0], (int)points[k][2]) * kVerticalScale / res_scale;
                        }

                        for (int k = 0; k < 4; ++k) {
                            normals[k] = cross(normalize(points[(k + 1) % 4 + 1] - points[0]), normalize(points[k + 1] - points[0]));
                        }

                        normal = normalize(normals[0] + normals[1] + normals[2] + normals[3]);

                        color[j * size * 4 + i * 4 + 0] = (unsigned char)((normal[2] + 1.0f) * 0.5f * 255.0f);
                        color[j * size * 4 + i * 4 + 1] = (unsigned char)((normal[1] + 1.0f) * 0.5f * 255.0f);
                        color[j * size * 4 + i * 4 + 2] = (unsigned char)((normal[0] + 1.0f) * 0.5f * 255.0f);
                        color[j * size * 4 + i * 4 + 3] = 255;
                    }
                }
                ImageExport::SavePNG(normal_map_path.c_str(), &color[0], size, size);
            }
            normal_map_ref = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(heightmap_.path() + "_normal.png");

            // Get lighting texture write path
            std::string baked_map_path = heightmap_.path() + "_" + level_name + "_baked.png";
            std::string w_bmp = GetWritePath(heightmap_.modsource_) + baked_map_path;
            if (GetDateModifiedInt64(w_bmp.c_str()) >= GetDateModifiedInt64(baked_map_path.c_str())) {
                baked_map_path = w_bmp;
            }

            test_file = my_fopen(baked_map_path.c_str(), "rb");
            if (test_file) {  // Load texture if already baked
                fclose(test_file);
                baked_texture_asset = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(heightmap_.path() + "_" + level_name + "_baked.png", PX_SRGB, 0x0);
                baked_texture_ref = baked_texture_asset->GetTextureRef();
            } else {  // Otherwise calculate it
                BakeTerrainTexture(framebuffer, sky->GetSpecularCubeMapTexture());
            }
            CHECK_GL_ERROR();
        }

        sky->BakeSecondPass(&baked_texture_ref);
        heightmap_.LoadData(heightmap_.path(), HeightmapImage::DOWNSAMPLED);
        for (auto &detail_object_surface : detail_object_surfaces) {
            detail_object_surface->SetBaseTextures(color_texture_ref, normal_map_ref);
        }
        CHECK_GL_ERROR();
    }
}

// Draw terrain
bool TriangleSquareIntersection2D(vec3 &min, vec3 &max, vec3 *tri_point[3]);
bool PointInTriangle(vec3 &point, vec3 *tri_point[3]);
void Terrain::drawLayer(int which) {
    if (which == 0) {
        Model &terrain_simplified_model = Models::Instance()->GetModel(model_id);
        terrain_simplified_model.Draw();
    }
}

int Terrain::lineCheck(const vec3 &start, const vec3 &end, vec3 *point, vec3 *normal) {
    Model &terrain_simplified_model = Models::Instance()->GetModel(model_id);
    return terrain_simplified_model.lineCheckNoBackface(start, end, point, normal);
}

const int _down_sample = 1;

void Terrain::CalculateHighResVertices(Model &terrain_high_detail_model) {
    float res_scale = 512.0f / heightmap_.width();
    int size = heightmap_.width();
    int down_sample_size = size / _down_sample;
    int num_verts = size * size / _down_sample / _down_sample;
    terrain_high_detail_model.vertices.resize(num_verts * 3);
#pragma omp parallel for
    for (int i = 0; i < down_sample_size; i++) {
        int index;
        int fixed_i;
        int fixed_j;
        for (int j = 0; j < down_sample_size; j++) {
            fixed_i = i * _down_sample;
            fixed_j = j * _down_sample;
            index = (i * down_sample_size + j) * 3;
            terrain_high_detail_model.vertices[index + 0] = (fixed_i - size / 2) * kUniformScale * res_scale;
            terrain_high_detail_model.vertices[index + 1] = heightmap_.GetHeight(fixed_i, fixed_j) * kVerticalScale * kUniformScale;
            terrain_high_detail_model.vertices[index + 2] = (fixed_j - size / 2) * kUniformScale * res_scale;
        }
    }
}

void Terrain::CalculateHighResFaces(Model &terrain_high_detail_model) {
    int size = heightmap_.width();
    int down_sample_size = size / _down_sample;
    // terrain_high_detail_model.ResizeFaces((down_sample_size-1)*(down_sample_size-1)*2);
    int num_faces = (down_sample_size - 1) * (down_sample_size - 1) * 2;
    terrain_high_detail_model.faces.resize(num_faces * 3);
#pragma omp parallel for
    for (int i = 0; i < down_sample_size - 1; i++) {
        int index;
        for (int j = 0; j < down_sample_size - 1; j++) {
            index = (i * (down_sample_size - 1) + j) * 6;
            terrain_high_detail_model.faces[index + 0] = i + j * down_sample_size;
            terrain_high_detail_model.faces[index + 1] = i + 1 + j * down_sample_size;
            terrain_high_detail_model.faces[index + 2] = i + 1 + (j + 1) * down_sample_size;
            terrain_high_detail_model.faces[index + 3] = i + j * down_sample_size;
            terrain_high_detail_model.faces[index + 4] = i + 1 + (j + 1) * down_sample_size;
            terrain_high_detail_model.faces[index + 5] = i + (j + 1) * down_sample_size;
        }
    }
}

bool Terrain::LoadCachedSimplifiedTerrain() {
    bool success = false;
    bool rewrite_cache = false;

    char cache_rel_path[kPathSize];
    FormatString(cache_rel_path, kPathSize, "%s.cache", heightmap_.path().c_str());

    char uv2_rel_path[kPathSize];
    FormatString(uv2_rel_path, kPathSize, "%s.obj_UV2", heightmap_.path().c_str());

    char abs_uv2_path[kPathSize];
    // bool found_uv2 = false;
    unsigned short uv2_checksum = 0;
    if (FindFilePath(uv2_rel_path, abs_uv2_path, kPathSize, kDataPaths | kModPaths, false, NULL) == 0) {
        // found_uv2 = true;
        uv2_checksum = Checksum(abs_uv2_path);
    }

    const int kMaxPaths = 5;
    char abs_cache_paths[kPathSize * kMaxPaths];
    int num_paths_found = FindFilePaths(cache_rel_path, abs_cache_paths, kPathSize, kMaxPaths, kAnyPath, true, NULL, NULL);

    if (num_paths_found > 0) {
        for (int path_index = 0; path_index < num_paths_found; ++path_index) {
            char *curr_path = &abs_cache_paths[kPathSize * path_index];
            FILE *cache_file = my_fopen(curr_path, "rb");
            if (cache_file) {  // bug: sometimes cache_file is not null when fopen fails
                uint16_t version;
                fread(&version, sizeof(version), 1, cache_file);
                if (version == _terrain_cache_file_version_number) {
                    uint16_t checksum = 0;
                    fread(&checksum, sizeof(checksum), 1, cache_file);
                    if (checksum == heightmap_.checksum()) {
                        uint16_t uv2_checksum_read = 0;
                        fread(&uv2_checksum_read, sizeof(uv2_checksum_read), 1, cache_file);
                        if (uv2_checksum_read == uv2_checksum) {
                            AddLoadingText("Loading cached terrain...");
                            if (model_id == -1) {
                                model_id = Models::Instance()->AddModel();
                            }
                            Model &terrain_simplified_model = Models::Instance()->GetModel(model_id);
                            terrain_simplified_model.Dispose();
                            terrain_simplified_model.ReadFromFile(cache_file);
                            terrain_simplified_model.calcBoundingBox();
                            terrain_simplified_model.calcBoundingSphere();  // TO DO: This should be cached
                            terrain_simplified_model.vbo_enabled = true;
                            success = true;
                            LOGI << "Loaded cached terrain: \"" << cache_rel_path << "\"" << std::endl;
                        }
                    }
                }
                fclose(cache_file);
            }
            if (success) {
                break;
            }
        }
    }
    return success;
}

void TextureTerrainModel(Model &model, float size) {
    model.tex_coords.resize(model.vertices.size());
    model.tex_coords2.resize(model.vertices.size());
    for (int i = 0, len = model.vertices.size() / 3; i < len; i++) {
        model.tex_coords[i * 2 + 0] = model.vertices[i * 3 + 0] / size + 0.5f;
        model.tex_coords[i * 2 + 1] = model.vertices[i * 3 + 2] / size + 0.5f;
        model.tex_coords2[i * 2 + 0] = model.vertices[i * 3 + 0];
        model.tex_coords2[i * 2 + 1] = model.vertices[i * 3 + 2];
    }
}

void RemoveDoubledTriangles(Model *model) {
    // Used to find faces that share all three vertices
    std::map<int, std::map<int, std::map<int, std::set<int> > > > face_vertex_map;

    // Stores which faces each vertex is part of
    std::map<int, std::set<int> > vertex_connections;

    // Mark duplicate triangles as possibly bad
    std::vector<int> possible_bad_faces;
    std::vector<int> face_vertices(3);
    for (int i = 0, len = model->faces.size() / 3; i < len; i++) {
        face_vertices[0] = model->faces[i * 3 + 0];
        face_vertices[1] = model->faces[i * 3 + 1];
        face_vertices[2] = model->faces[i * 3 + 2];
        vertex_connections[face_vertices[0]].insert(i);
        vertex_connections[face_vertices[1]].insert(i);
        vertex_connections[face_vertices[2]].insert(i);
        std::sort(face_vertices.begin(), face_vertices.end());
        if (face_vertex_map[face_vertices[0]][face_vertices[1]][face_vertices[2]].size() == 1) {
            possible_bad_faces.push_back(*(face_vertex_map[face_vertices[0]][face_vertices[1]][face_vertices[2]].begin()));
        }
        if (!face_vertex_map[face_vertices[0]][face_vertices[1]][face_vertices[2]].empty()) {
            possible_bad_faces.push_back(i);
        }
        face_vertex_map[face_vertices[0]][face_vertices[1]][face_vertices[2]].insert(i);
    }

    // If possibly-bad triangles have one vertex that is only shared by two triangles,
    // mark as bad
    std::vector<int> bad_faces;
    for (unsigned int i = 0; i < possible_bad_faces.size(); i++) {
        if (vertex_connections[model->faces[i * 3 + 0]].size() <= 2 || vertex_connections[model->faces[i * 3 + 1]].size() <= 2 || vertex_connections[model->faces[i * 3 + 2]].size() <= 2) {
            bad_faces.push_back(possible_bad_faces[i]);
        }
    }

    // Mark triangles with normals facing down as bad
    for (int &possible_bad_face : possible_bad_faces) {
        if (model->face_normals[possible_bad_face][1] < 0) {
            bad_faces.push_back(possible_bad_face);
        }
    }

    for (int bad_face : bad_faces) {
        model->faces[bad_face * 3 + 0] = 0;
        model->faces[bad_face * 3 + 1] = 0;
        model->faces[bad_face * 3 + 2] = 0;
    }
}

void Terrain::CalculateSimplifiedTerrain() {
    // Model terrain_high_detail_model;
    if (model_id == -1) {
        model_id = Models::Instance()->AddModel();
    }
    Model &terrain_simplified_model = Models::Instance()->GetModel(model_id);
    CalculateHighResVertices(terrain_simplified_model);
    CalculateHighResFaces(terrain_simplified_model);
    /*{
        terrain_simplified_model.tex_coords.resize(terrain_simplified_model.vertices.size()/3*2);
        float size = (float)m_heightmap.width();
        for(int i=0, len=terrain_simplified_model.vertices.size()/3; i<len; i++){
            terrain_simplified_model.tex_coords[i*2+0] = terrain_simplified_model.vertices[i*3+0]/size+0.5f;
            terrain_simplified_model.tex_coords[i*2+1] = terrain_simplified_model.vertices[i*3+2]/size+0.5f;
        }
        std::fstream file;
        file.open(GetWritePath("terrain_high.obj").c_str(), std::fstream::out);
        for(unsigned i=0; i<terrain_simplified_model.vertices.size(); i+=3){
            file << "v " << terrain_simplified_model.vertices[i+0] << " "
                         << terrain_simplified_model.vertices[i+1] << " "
                         << terrain_simplified_model.vertices[i+2] << "\n";
        }
        for(unsigned i=0; i<terrain_simplified_model.tex_coords.size(); i+=2){
            file << "vt " << terrain_simplified_model.tex_coords[i+0] << " "
                          << terrain_simplified_model.tex_coords[i+1] << "\n";
        }
        for(unsigned i=0; i<terrain_simplified_model.faces.size(); i+=3){
            file << "f " << terrain_simplified_model.faces[i+0]+1 << "/"
                         << terrain_simplified_model.faces[i+0]+1 << " "
                         << terrain_simplified_model.faces[i+1]+1 << "/"
                         << terrain_simplified_model.faces[i+1]+1 << " "
                         << terrain_simplified_model.faces[i+2]+1 << "/"
                         << terrain_simplified_model.faces[i+2]+1 << "\n";
        }
        file.close();
    }*/
#ifndef _DEPLOY
    AddLoadingText("Simplifying terrain. If it gets stuck, start without debugger (ctrl-F5 in MSVC).");
#else
    AddLoadingText("Simplifying terrain. This may take a while, please be patient!");
#endif
    SimplifyModel("Data/Temp/terrain", terrain_simplified_model, 70000);
    char abs_path[kPathSize];
    if (FindFilePath("Data/Temp/terrainlow.obj", abs_path, kPathSize, kWriteDir) == -1) {
        FatalError("Error", "Could not find: Data/Temp/terrainlow.obj");
    }
    LOGI << "Loading terrain hard coded model: " << abs_path << std::endl;
    terrain_simplified_model.LoadObj(abs_path, 0, "", kAbsPath);
    RemoveDoubledTriangles(&terrain_simplified_model);
    TextureTerrainModel(terrain_simplified_model, (float)heightmap_.width());

    int index = 1;
    for (int i = 0, len = terrain_simplified_model.vertices.size() / 3; i < len; i++) {
        terrain_simplified_model.vertices[index] -= kHeightMapHeight;
        index += 3;
    }

    AddLoadingText("Calculating terrain normals...");
    terrain_simplified_model.calcNormals();
    AddLoadingText("Calculating terrain tangents...");
    terrain_simplified_model.calcTangents();

    terrain_simplified_model.calcBoundingBox();
    terrain_simplified_model.calcBoundingSphere();

    terrain_simplified_model.CalcTexelDensity();
    terrain_simplified_model.CalcAverageTriangleEdge();

    char uv2_rel_path[kPathSize];
    FormatString(uv2_rel_path, kPathSize, "%s.obj_UV2", heightmap_.path().c_str());

    char abs_uv2_path[kPathSize];
    // bool found_uv2 = false;
    uint16_t uv2_checksum = 0;

    if (FindFilePath(uv2_rel_path, abs_uv2_path, kPathSize, kDataPaths | kModPaths, false, NULL) == 0) {
        // found_uv2 = true;
        uv2_checksum = Checksum(abs_uv2_path);

        Model temp;
        temp.SimpleLoadTriangleCutObj(abs_uv2_path);
        CopyTexCoords2(terrain_simplified_model, temp);
        for (float &i : terrain_simplified_model.tex_coords2) {
            i *= 2048.0f;
        }
    }

    std::string path = GetWritePath(heightmap_.modsource_) + heightmap_.path() + ".cache";
    FILE *cache_file = my_fopen(path.c_str(), "wb");
    if (cache_file) {
        AddLoadingText("Writing terrain cache file...");
        fwrite(&_terrain_cache_file_version_number, sizeof(_terrain_cache_file_version_number), 1, cache_file);
        uint16_t checksum = heightmap_.checksum();
        fwrite(&checksum, sizeof(checksum), 1, cache_file);
        fwrite(&uv2_checksum, sizeof(uv2_checksum), 1, cache_file);
        Model &terrain_simplified_model = Models::Instance()->GetModel(model_id);
        terrain_simplified_model.WriteToFile(cache_file);
        fclose(cache_file);
    }

    terrain_simplified_model.SaveObj(heightmap_.path() + ".obj");

    terrain_simplified_model.vbo_enabled = true;
}

void Terrain::CalculateMinimalTerrain() {
    if (model_id == -1) {
        model_id = Models::Instance()->AddModel();
    }
    Model &terrain_minimal_model = Models::Instance()->GetModel(model_id);
    CalculateHighResVertices(terrain_minimal_model);
    CalculateHighResFaces(terrain_minimal_model);
    TextureTerrainModel(terrain_minimal_model, (float)heightmap_.width());

    int index = 1;
    for (int i = 0, len = terrain_minimal_model.vertices.size() / 3; i < len; i++) {
        terrain_minimal_model.vertices[index] -= kHeightMapHeight;
        index += 3;
    }

    terrain_minimal_model.calcNormals();
    terrain_minimal_model.calcTangents();
    terrain_minimal_model.calcBoundingBox();
    terrain_minimal_model.calcBoundingSphere();
    terrain_minimal_model.texel_density = 0.0f;
    terrain_minimal_model.average_triangle_edge_length = 0.0f;
    terrain_minimal_model.vbo_enabled = true;
}

Model &Terrain::GetModel() const {
    Model &terrain_simplified_model = Models::Instance()->GetModel(model_id);
    return terrain_simplified_model;
}
const float _light_offset = 0.002f;

void Terrain::BakeTerrainTexture(GLuint framebuffer, const TextureRef &light_cube) {
    if (minimal)
        return;

    Shaders *shaders = Shaders::Instance();
    Textures *textures = Textures::Instance();
    Graphics *graphics = Graphics::Instance();

    Model &terrain_simplified_model = Models::Instance()->GetModel(model_id);
    if (!textures->IsRenderable(baked_texture_ref)) {
        baked_texture_ref = textures->makeTexture(terrain_texture_size, terrain_texture_size, GL_RGBA, GL_RGBA, true);
        textures->SetTextureName(baked_texture_ref, "Terrain Baked Texture");
    }
    graphics->PushFramebuffer();
    graphics->RenderFramebufferToTexture(framebuffer, baked_texture_ref);
    graphics->PushViewport();
    graphics->setViewport(0, 0, terrain_texture_size, terrain_texture_size);

    GLState gl_state;
    gl_state.blend = false;
    gl_state.cull_face = false;
    gl_state.depth_test = false;
    gl_state.depth_write = false;
    graphics->setGLState(gl_state);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    int prepare_shader_id = shaders->returnProgram(shader);
    shaders->setProgram(prepare_shader_id);

    vec3 light_pos = primary_light->pos;
    shaders->SetUniformVec3("light_pos", light_pos);
    shaders->SetUniformVec4("primary_light_color", vec4(primary_light->color, primary_light->intensity));

    textures->bindTexture(color_texture_ref, 0);
    textures->bindTexture(light_cube, 3);
    textures->bindTexture(normal_map_ref, 4);

    {
        CHECK_GL_ERROR();
        if (!terrain_simplified_model.vbo_loaded) {
            terrain_simplified_model.createVBO();
        }
        terrain_simplified_model.VBO_tex_coords.Bind();
        int vert_attrib_id = shaders->returnShaderAttrib("uv", prepare_shader_id);

        CHECK_GL_ERROR();
        graphics->EnableVertexAttribArray(vert_attrib_id);
        CHECK_GL_ERROR();
        glVertexAttribPointer(vert_attrib_id, 2, GL_FLOAT, false, 2 * sizeof(GLfloat), 0);
        CHECK_GL_ERROR();
        terrain_simplified_model.VBO_faces.Bind();
        CHECK_GL_ERROR();
        graphics->DrawElements(GL_TRIANGLES, terrain_simplified_model.faces.size(), GL_UNSIGNED_INT, 0);
        CHECK_GL_ERROR();
        graphics->ResetVertexAttribArrays();
        CHECK_GL_ERROR();
        graphics->BindArrayVBO(0);
        CHECK_GL_ERROR();
        graphics->BindElementVBO(0);
        CHECK_GL_ERROR();
    }

    graphics->PopViewport();
    graphics->PopFramebuffer();

    Textures::Instance()->GenerateMipmap(baked_texture_ref);
}

// Load terrain
void Terrain::Load(const char *name, const std::string &model_override) {
    minimal = false;
    heightmap_.LoadData(name, HeightmapImage::DOWNSAMPLED);

    LOGI << "Loading terrain \"" << name << "\"" << std::endl;
    AddLoadingText("Checking for terrain cache file...");

    if (!LoadCachedSimplifiedTerrain()) {
        LOGI << "Failed to load cached terrain, calculating a simplified terrain" << std::endl;
        CalculateSimplifiedTerrain();
    }

    if (!model_override.empty()) {
        Model &terrain_simplified_model = Models::Instance()->GetModel(model_id);
        terrain_simplified_model.LoadObj(model_override, 0);
        for (float &i : terrain_simplified_model.tex_coords2) {
            i *= 2048.0f;
        }
    }

    LOGI << "*****************" << std::endl;

    CalculatePatches();

    heightmap_.LoadData(heightmap_.path(), HeightmapImage::ORIGINAL_RES);

    terrain_texture_size = heightmap_.width() / Graphics::Instance()->config_.texture_reduction_factor();
}

void Terrain::LoadMinimal(const char *name, const std::string &model_override) {
    minimal = true;
    heightmap_.LoadData(name, HeightmapImage::DOWNSAMPLED);
    CalculateMinimalTerrain();
    CalculatePatches();
    terrain_texture_size = heightmap_.width() / Graphics::Instance()->config_.texture_reduction_factor();
}

void Terrain::GetShaderNames(std::map<std::string, int> &preload_shaders) {
    preload_shaders[shader] = 0;
}

void Terrain::CalcDetailTextures() {
    LOGI << "Calculating detail textures." << std::endl;
    unsigned num_colors = detail_maps_info.size();

    Textures::Instance()->setWrap(GL_REPEAT, GL_REPEAT);
    detail_texture_ref = Textures::Instance()->makeArrayTexture(num_colors, PX_SRGB);
    Textures::Instance()->SetTextureName(detail_texture_ref, "Terrain Detail Texture Array - Color");
    detail_texture_color.resize(num_colors);
    detail_texture_color_srgb.resize(num_colors);
    detail_normal_texture_ref = Textures::Instance()->makeArrayTexture(num_colors);
    Textures::Instance()->SetTextureName(detail_normal_texture_ref, "Terrain Detail Texture Array - Normals");

    Textures::Instance()->setWrap(GL_REPEAT, GL_REPEAT);

    // Get average color of each detail texture
    std::vector<ByteColor> average_color(num_colors);
    for (unsigned i = 0; i < num_colors; i++) {
        Textures::Instance()->loadArraySlice(detail_texture_ref, i, detail_maps_info[i].colorpath);
        Textures::Instance()->loadArraySlice(detail_normal_texture_ref, i, detail_maps_info[i].normalpath);

        // detail_texture_color[i] = AverageColors::Instance()->ReturnRef(detail_maps_info[i].colorpath)->color();
        AverageColorRef color_ref = Engine::Instance()->GetAssetManager()->LoadSync<AverageColor>(detail_maps_info[i].colorpath);
        average_colors.insert(color_ref);
        detail_texture_color[i] = color_ref->color();
        for (int channel = 0; channel < 3; ++channel) {
            average_color[i].color[channel] = (int)(detail_texture_color[i][channel] * 255.0f);
            detail_texture_color_srgb[i][channel] = pow(detail_texture_color[i][channel], 2.2f);
        }
        detail_texture_color_srgb[i][3] = detail_texture_color[i][3];
    }

    char abs_weight_map_path[kPathSize];
    bool found_weight_path = false;
    if (!weight_map_path.empty()) {
        found_weight_path =
            (FindFilePath(weight_map_path.c_str(), abs_weight_map_path, kPathSize, kDataPaths | kModPaths, false) != -1);
    }

    if (found_weight_path) {
        detail_texture_weights = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(weight_map_path);
        // weight_bitmap = ImageSamplers::Instance()->ReturnRef(weight_map_path);
        weight_bitmap = Engine::Instance()->GetAssetManager()->LoadSync<ImageSampler>(weight_map_path);
        return;
    }

    std::string path = heightmap_.path() + "_" + level_name + "_weights.png";
    found_weight_path =
        (FindImagePath(path.c_str(), abs_weight_map_path, kPathSize, kDataPaths | kModPaths | kWriteDir | kModWriteDirs, false) != -1);

    if (found_weight_path) {
        detail_texture_weights = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(path);
        // weight_bitmap = ImageSamplers::Instance()->ReturnRef(path);
        weight_bitmap = Engine::Instance()->GetAssetManager()->LoadSync<ImageSampler>(path);
    } else {
        LOGI << "Calculating detail texture weights" << std::endl;
        // Load terrain color map
        TextureData texture_data;
        char abs_path[kPathSize];
        if (FindFilePath(color_path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths) == -1) {
            // Fall back on finding the .dds if the raw is missing.
            if (FindImagePath(color_path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths) == -1) {
                FatalError("Error", "Could not find color path: %s", color_path.c_str());
            }
        }
        texture_data.Load(abs_path);
        // TODO: check this
        unsigned total_bytes = texture_data.GetWidth() *
                               texture_data.GetHeight() *
                               32 / 8;

        // Compare each pixel to each average color
        // Create weight map and tint texture
        //#pragma omp parallel for
        std::vector<unsigned char> image_data;
        image_data.resize(total_bytes);
        texture_data.GetUncompressedData(&image_data[0]);
        for (int i = 0; i < (int)total_bytes; i += 4) {
            ByteColor color;
            color.Set(image_data[i + 0],
                      image_data[i + 1],
                      image_data[i + 2]);

            std::vector<float> distances(num_colors);
            for (unsigned j = 0; j < num_colors; j++) {
                distances[j] = hue_saturation_distance_squared(color, average_color[j]);
            }

            float lowest_distance = FLT_MIN;
            unsigned int which_lowest = (unsigned int)-1;
            for (unsigned int j = 0; j < num_colors; j++) {
                if (which_lowest == (unsigned int)-1 || distances[j] < lowest_distance) {
                    which_lowest = j;
                    lowest_distance = distances[j];
                }
            }

            // Create weight map
            image_data[i + 0] = which_lowest == 0 ? 255 : 0;
            image_data[i + 1] = which_lowest == 1 ? 255 : 0;
            image_data[i + 2] = which_lowest == 2 ? 255 : 0;
            image_data[i + 3] = 255;
            // texture_data.m_nImageData[i+3] = which_lowest==3?255:0;
        }

        for (unsigned i = 0; i < total_bytes; i += 4) {
            std::swap(image_data[i + 0],
                      image_data[i + 2]);
        }

        LOGI << "Saving detail texture weights " << path << std::endl;
        // Save weight and tint maps as textures
        path = heightmap_.path() + "_" + level_name + "_weights.png";
        std::string write_path = GetWritePath(heightmap_.modsource_) + heightmap_.path() + "_" + level_name + "_weights.png";
        ImageExport::SavePNG(write_path.c_str(), &image_data[0], texture_data.GetWidth(), texture_data.GetHeight());
        // SavePNG("detail_tint_no_compress.png", tint_texture_data.m_nImageData, tint_texture_data.m_nImageWidth, tint_texture_data.m_nImageHeight);

        // Load weight and tint map textures
        detail_texture_weights = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(path);
        // weight_bitmap = ImageSamplers::Instance()->ReturnRef(path);
        weight_bitmap = Engine::Instance()->GetAssetManager()->LoadSync<ImageSampler>(path);

        // detail_texture_tint = Textures::Instance()->returnTextureAssetRef("detail_tint_no_compress.png");
    }
}

const float terrain_size = 500.0;
const float fade_distance = 50.0;
const float fade_mult = 1.0f / fade_distance;
float GetAlpha(const vec3 &v) {
    float alpha = min(1.0f, (terrain_size - v[0]) * fade_mult) *
                  min(1.0f, (v[0] + 500.0f) * fade_mult) *
                  min(1.0f, (terrain_size - v[2]) * fade_mult) *
                  min(1.0f, (v[2] + 500.0f) * fade_mult);

    alpha = max(0.0f, alpha);

    return alpha;
}

void Terrain::CalculatePatches() {
    const int patch_resolution = 6;

    // Create 3-dimensional array to store triangles for each patch, e.g.:
    // patch_vertex_ids[0][2][61] would give 62nd triangle id for patch (0,2)
    std::vector<std::vector<std::vector<int> > > patch_vertex_ids;
    patch_vertex_ids.resize(patch_resolution);
    for (int i = 0; i < patch_resolution; i++) {
        patch_vertex_ids[i].resize(patch_resolution);
    }

    std::vector<std::vector<std::vector<int> > > edge_patch_vertex_ids;
    edge_patch_vertex_ids.resize(patch_resolution);
    for (int i = 0; i < patch_resolution; i++) {
        edge_patch_vertex_ids[i].resize(patch_resolution);
    }

    // Loop through terrain faces to find out which patch it belongs to
    Model &terrain_simplified_model = Models::Instance()->GetModel(model_id);
    const Model &model = terrain_simplified_model;
    const vec3 dimensions = (model.max_coords - model.min_coords);

    // added an epsilon so the edge is not exceeded.
    const float patch_resolution_f = ((float)patch_resolution) - 0.01f;
    int face_index, vert_index[3];
    static const float one_third = 1.0f / 3.0f;
    for (int i = 0, len = model.faces.size() / 3; i < len; i++) {
        face_index = i * 3;
        for (unsigned j = 0; j < 3; ++j) {
            vert_index[j] = model.faces[face_index + j] * 3;
        }
        vec3 midpoint;
        for (unsigned j = 0; j < 3; ++j) {
            midpoint[j] = (model.vertices[vert_index[0] + j] +
                           model.vertices[vert_index[1] + j] +
                           model.vertices[vert_index[2] + j]) *
                          one_third;
        }

        const vec3 unit_midpoint = (midpoint - model.min_coords) /
                                   dimensions;

        const unsigned x_coord = (unsigned)(unit_midpoint[0] *
                                            patch_resolution_f);

        const unsigned z_coord = (unsigned)(unit_midpoint[2] *
                                            patch_resolution_f);

        float alpha = min(GetAlpha(vec3(model.vertices[vert_index[0] + 0],
                                        model.vertices[vert_index[0] + 1],
                                        model.vertices[vert_index[0] + 2])),
                          min(GetAlpha(vec3(model.vertices[vert_index[1] + 0],
                                            model.vertices[vert_index[1] + 1],
                                            model.vertices[vert_index[1] + 2])),
                              GetAlpha(vec3(model.vertices[vert_index[2] + 0],
                                            model.vertices[vert_index[2] + 1],
                                            model.vertices[vert_index[2] + 2]))));
        if (alpha == 1.0f) {
            patch_vertex_ids[x_coord][z_coord].push_back(i);
        } else {
            edge_patch_vertex_ids[x_coord][z_coord].push_back(i);
        }
    }

    terrain_patches.clear();
    edge_terrain_patches.clear();
    for (int i = 0; i < patch_resolution; i++) {
        for (int j = 0; j < patch_resolution; j++) {
            terrain_patches.resize(terrain_patches.size() + 1);
            Model &patch_model = terrain_patches.back();
            patch_model.CopyFacesFromModel(terrain_simplified_model,
                                           patch_vertex_ids[i][j]);
            patch_model.calcBoundingBox();
            if (patch_model.vertices.empty()) {
                terrain_patches.resize(terrain_patches.size() - 1);
            }

            if (!edge_patch_vertex_ids[i][j].empty()) {
                edge_terrain_patches.resize(edge_terrain_patches.size() + 1);
                Model &edge_patch_model = edge_terrain_patches.back();
                edge_patch_model.CopyFacesFromModel(terrain_simplified_model,
                                                    edge_patch_vertex_ids[i][j]);
                edge_patch_model.calcBoundingBox();
                if (edge_patch_model.vertices.empty()) {
                    edge_terrain_patches.resize(edge_terrain_patches.size() - 1);
                }
            }
        }
    }
}

vec2 Terrain::GetUVAtPoint(const vec3 &point, int *tri) const {
    int face = -1;
    vec3 intersection_point;
    Model &terrain_simplified_model = Models::Instance()->GetModel(model_id);
    const Model &model = terrain_simplified_model;
    // vec3 normal;
    if (!tri) {
        vec3 point_high(point[0], point[1] + 1000.0f, point[2]);
        vec3 point_low(point[0], point[1] - 1000.0f, point[2]);
        face = model.lineCheckNoBackface(point_high,
                                         point_low,
                                         &intersection_point);
    } else {
        face = *tri;
        intersection_point = point;
    }

    if (face == -1) {
        return vec2(0.0f);
    }

    int index[3];
    index[0] = model.faces[face * 3 + 0];
    index[1] = model.faces[face * 3 + 1];
    index[2] = model.faces[face * 3 + 2];

    vec3 points[3];
    for (unsigned j = 0; j < 3; ++j) {
        points[j][0] = model.vertices[index[j] * 3 + 0];
        points[j][1] = model.vertices[index[j] * 3 + 1];
        points[j][2] = model.vertices[index[j] * 3 + 2];
    }

    // DebugDraw::Instance()->AddLine(point_high, point_low, vec4(1.0f), _delete_on_update);
    // DebugDraw::Instance()->AddWireSphere(intersection_point, 0.1f, vec4(1.0f), _delete_on_update);

    // DebugDraw::Instance()->AddLine(points[0], points[1], vec4(1.0f), _fade, _DD_XRAY);
    // DebugDraw::Instance()->AddLine(points[1], points[2], vec4(1.0f), _fade, _DD_XRAY);
    // DebugDraw::Instance()->AddLine(points[2], points[0], vec4(1.0f), _fade, _DD_XRAY);

    vec3 normal = normalize(cross(points[1] - points[0], points[2] - points[0]));

    vec3 barycentric_coords = barycentric(intersection_point,
                                          normal, points[0], points[1], points[2]);

    float total = 0.0f;
    for (unsigned i = 0; i < 3; ++i) {
        barycentric_coords[i] = min(1.0f, max(0.0f, barycentric_coords[i]));
        total += barycentric_coords[i];
    }

    if (total == 0.0f) {
        barycentric_coords = vec3(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f);
    } else {
        barycentric_coords *= (1.0f / total);
    }

    vec2 uv;
    uv[0] = model.tex_coords[index[0] * 2 + 0] * barycentric_coords[0] +
            model.tex_coords[index[1] * 2 + 0] * barycentric_coords[1] +
            model.tex_coords[index[2] * 2 + 0] * barycentric_coords[2];

    uv[1] = model.tex_coords[index[0] * 2 + 1] * barycentric_coords[0] +
            model.tex_coords[index[1] * 2 + 1] * barycentric_coords[1] +
            model.tex_coords[index[2] * 2 + 1] * barycentric_coords[2];
    return uv;
}

vec4 Terrain::SampleWeightMapAtPoint(vec3 point, int *tri) {
    vec2 uv = GetUVAtPoint(point, tri);

    vec3 color = weight_bitmap->GetInterpolatedColorUV(uv[0], uv[1]).xyz();

    // DebugDraw::Instance()->AddWireSphere(point, 0.2f, vec4(color, 1.0f), _fade);

    float missing_component = 1.0f - (color[0] + color[1] + color[2]);
    vec4 final_color = vec4(color, missing_component);
    return final_color;
}

vec3 Terrain::SampleColorMapAtPoint(vec3 point, int *tri) {
    vec2 uv = GetUVAtPoint(point, tri);
    vec3 color = color_bitmap->GetInterpolatedColorUV(uv[0], uv[1]).xyz();
    for (int i = 0; i < 3; ++i) {
        color[i] = pow(color[i], 2.2f);
    }
    return color;
}

const MaterialRef Terrain::GetMaterialAtPoint(vec3 point, int *tri) {
    vec4 weights = SampleWeightMapAtPoint(point, tri);

    int strongest_weight = 0;
    float strongest_weight_amount = weights[0];
    for (int i = 1; i < 4; i++) {
        if (weights[i] > strongest_weight_amount) {
            strongest_weight = i;
            strongest_weight_amount = weights[i];
        }
    }

    const std::string &path = detail_maps_info[strongest_weight].materialpath;
    LOGS << path << std::endl;
    // Materials* materials = Materials::Instance();
    // MaterialRef material = materials->ReturnRef(path);
    MaterialRef material = Engine::Instance()->GetAssetManager()->LoadSync<Material>(path);
    return material;
}

void Terrain::HandleMaterialEvent(const std::string &the_event, const vec3 &event_pos, int *tri) {
    MaterialRef material_ref = GetMaterialAtPoint(event_pos, tri);
    Material &material = (*material_ref);
    material.HandleEvent(the_event, event_pos);
}

void Terrain::SetDetailObjectLayers(const std::vector<DetailObjectLayer> &_detail_object_layers) {
    if (detail_object_layers == _detail_object_layers || minimal) {
        return;
    }
    detail_object_layers = _detail_object_layers;
    detail_object_surfaces.clear();
    detail_object_surfaces.resize(detail_object_layers.size());
    int counter = 0;
    Model &terrain_simplified_model = Models::Instance()->GetModel(model_id);
    for (auto &detail_object_surface : detail_object_surfaces) {
        static const mat4 identity;
        detail_object_surface = new DetailObjectSurface();
        DetailObjectSurface &dos = *detail_object_surface;
        dos.AttachTo(terrain_simplified_model, identity);
        dos.GetTrisInPatches(identity);
        dos.LoadDetailModel(detail_object_layers[counter].obj_path);
        dos.LoadWeightMap(detail_object_layers[counter].weight_path);
        dos.SetDensity(detail_object_layers[counter].density);
        dos.tint_weight = (detail_object_layers[counter].tint_weight);
        dos.SetNormalConform(detail_object_layers[counter].normal_conform);
        dos.SetMinEmbed(detail_object_layers[counter].min_embed);
        dos.SetMaxEmbed(detail_object_layers[counter].max_embed);
        dos.SetMinScale(detail_object_layers[counter].min_scale);
        dos.SetMaxScale(detail_object_layers[counter].max_scale);
        dos.SetViewDist(detail_object_layers[counter].view_dist);
        dos.SetJitterDegrees(detail_object_layers[counter].jitter_degrees);
        dos.SetOverbright(detail_object_layers[counter].overbright);
        dos.SetCollisionType(detail_object_layers[counter].collision_type);
        ++counter;
    }
    if (normal_map_ref.valid()) {
        for (auto &detail_object_surface : detail_object_surfaces) {
            detail_object_surface->SetBaseTextures(color_texture_ref, normal_map_ref);
        }
    }
}

void Terrain::SetDetailTextures(const std::vector<DetailMapInfo> &_detail_maps_info) {
    if (normal_map_ref.valid() && (_detail_maps_info != detail_maps_info)) {
        detail_maps_info = _detail_maps_info;
        CalcDetailTextures();
    } else {
        detail_maps_info = _detail_maps_info;
    }
}

void Terrain::SetColorTexture(const char *path) {
    color_path = path;
    color_bitmap = Engine::Instance()->GetAssetManager()->LoadSync<ImageSampler>(path);
    color_texture_ref = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(path, PX_SRGB, 0x0);
}

void Terrain::SetWeightTexture(const char *path) {
    weight_map_path = path;
    CalcDetailTextures();
}
