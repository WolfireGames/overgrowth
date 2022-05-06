//-----------------------------------------------------------------------------
//           Name: model.cpp
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
#include "model.h"

#include <Graphics/graphics.h>
#include <Graphics/shaders.h>
#include <Graphics/drawbatch.h>

#include <Internal/collisiondetection.h>
#include <Internal/checksum.h>
#include <Internal/common.h>
#include <Internal/datemodified.h>
#include <Internal/stopwatch.h>
#include <Internal/error.h>
#include <Internal/filesystem.h>
#include <Internal/profiler.h>

#include <Math/vec2.h>
#include <Math/vec2math.h>
#include <Math/vec3math.h>
#include <Math/overgrowth_geometry.h>

#include <Compat/filepath.h>
#include <Asset/Asset/image_sampler.h>
#include <Compat/fileio.h>
#include <Logging/logdata.h>
#include <Utility/assert.h>
#include <Main/engine.h>

#include <Timing/timingevent.h>
#include <Timing/intel_gl_perf.h>

#include <cmath>
#include <set> 

//-----------------------------------------------------------------------------
//Functions
//-----------------------------------------------------------------------------

namespace {
    void RearrangeVertices(Model& model, const std::vector<int> &new_order){
        std::vector<float> old_vertices = model.vertices;
        std::vector<float> old_normals = model.normals;
        std::vector<float> old_tex_coords = model.tex_coords;
        std::vector<float> old_tex_coords2 = model.tex_coords2;
        std::vector<float> old_tangents = model.tangents;
        std::vector<float> old_bitangents = model.bitangents;
        std::vector<float> old_aux = model.aux;
        std::vector<vec4> old_bone_weights = model.bone_weights;
        std::vector<vec4> old_bone_ids = model.bone_ids;

        for(unsigned i=0; i<new_order.size(); ++i){
            model.vertices[i*3+0] = old_vertices[new_order[i]*3+0];
            model.vertices[i*3+1] = old_vertices[new_order[i]*3+1];
            model.vertices[i*3+2] = old_vertices[new_order[i]*3+2];
            if(!model.normals.empty()){
                model.normals[i*3+0] = old_normals[new_order[i]*3+0];
                model.normals[i*3+1] = old_normals[new_order[i]*3+1];
                model.normals[i*3+2] = old_normals[new_order[i]*3+2];
            }
            if(!model.tex_coords.empty()){
                model.tex_coords[i*2+0] = old_tex_coords[new_order[i]*2+0];
                model.tex_coords[i*2+1] = old_tex_coords[new_order[i]*2+1];
            }
            if(!model.tex_coords2.empty()){
                model.tex_coords2[i*2+0] = old_tex_coords2[new_order[i]*2+0];
                model.tex_coords2[i*2+1] = old_tex_coords2[new_order[i]*2+1];
            }
            if(!model.tangents.empty()){
                model.tangents[i*3+0] = old_tangents[new_order[i]*3+0];
                model.tangents[i*3+1] = old_tangents[new_order[i]*3+1];
                model.tangents[i*3+2] = old_tangents[new_order[i]*3+2];
            }
            if(!model.bitangents.empty()){
                model.bitangents[i*3+0] = old_bitangents[new_order[i]*3+0];
                model.bitangents[i*3+1] = old_bitangents[new_order[i]*3+1];
                model.bitangents[i*3+2] = old_bitangents[new_order[i]*3+2];
            }
            if(!model.aux.empty()){
                model.aux[i*3+0] = old_aux[new_order[i]*3+0];
                model.aux[i*3+1] = old_aux[new_order[i]*3+1];
                model.aux[i*3+2] = old_aux[new_order[i]*3+2];
            }
            if(!model.bone_weights.empty()){
                model.bone_weights[i] = old_bone_weights[new_order[i]];
            }
            if(!model.bone_ids.empty()){
                model.bone_ids[i] = old_bone_ids[new_order[i]];
            }
        }
        model.ResizeVertices(new_order.size());
    }
} //namespace ""

//Initialize model
Model::Model()
:vbo_loaded(false),
 vbo_enabled(false),
 texel_density(1.0f),
 average_triangle_edge_length(1.0f),
 path("DEFAULT")
{
}

Model::Model( const Model &copy )
{
    (*this)=copy;
}

Model& Model::operator=( const Model &copy )
{
    vbo_loaded = false;
    vbo_enabled = false;

    vertices = copy.vertices;
    normals = copy.normals;
    tangents = copy.tangents;
    bitangents = copy.bitangents;
    face_normals = copy.face_normals;
    tex_coords = copy.tex_coords;
    tex_coords2 = copy.tex_coords2;
    aux = copy.aux;
    faces = copy.faces;
    bone_weights = copy.bone_weights;
    bone_ids = copy.bone_ids;
    checksum = copy.checksum;

    min_coords = copy.min_coords;
    max_coords = copy.max_coords;
    center_coords = copy.center_coords;
    old_center = copy.old_center;

    bounding_sphere_origin = copy.bounding_sphere_origin;
    bounding_sphere_radius = copy.bounding_sphere_radius;

    texel_density = copy.texel_density;

    precollapse_vert_reorder = copy.precollapse_vert_reorder;
    optimize_vert_reorder = copy.optimize_vert_reorder;

    path = copy.path;

    return (*this);
}


void Model::WriteToFile(FILE *file) {
    int count;
    count = vertices.size()/3;
    fwrite(&count, sizeof(int), 1, file);
    fwrite(&vertices.front(), sizeof(GLfloat), count*3, file);
    fwrite(&normals.front(), sizeof(GLfloat), count*3, file);
    fwrite(&tangents.front(), sizeof(GLfloat), count*3, file);
    fwrite(&bitangents.front(), sizeof(GLfloat), count*3, file);
    fwrite(&tex_coords.front(), sizeof(GLfloat), count*2, file);

    count = faces.size()/3;
    fwrite(&count, sizeof(int), 1, file);
    fwrite(&faces.front(), sizeof(GLfloat), count*3, file);
    fwrite(&face_normals.front(), sizeof(vec3), count, file);

    fwrite(&precollapse_num_vertices, sizeof(int), 1, file);
    count = precollapse_vert_reorder.size();
    fwrite(&count, sizeof(int), 1, file);
    fwrite(&precollapse_vert_reorder.front(), sizeof(int), count, file);
    count = optimize_vert_reorder.size();
    fwrite(&count, sizeof(int), 1, file);
    fwrite(&optimize_vert_reorder.front(), sizeof(int), count, file);

    count = tex_coords2.size()/2;
    fwrite(&count, sizeof(int), 1, file);
    if (count>0) {
        fwrite(&tex_coords2.front(), sizeof(GLfloat), count*2, file);
    }
    
    fwrite(&min_coords, sizeof(vec3), 1, file);
    fwrite(&max_coords, sizeof(vec3), 1, file);
    fwrite(&center_coords, sizeof(vec3), 1, file);
    fwrite(&old_center, sizeof(vec3), 1, file);
    fwrite(&bounding_sphere_origin, sizeof(vec3), 1, file);
    fwrite(&bounding_sphere_radius, sizeof(float), 1, file);

    fwrite(&texel_density, sizeof(float), 1, file);
    fwrite(&average_triangle_edge_length, sizeof(float), 1, file);
}

void Model::ReadFromFile(FILE *file)
{
    int count;
    fread(&count, sizeof(int), 1, file);
    ResizeVertices(count);
    fread(&vertices.front(), sizeof(GLfloat), count*3, file);
    fread(&normals.front(), sizeof(GLfloat), count*3, file);
    fread(&tangents.front(), sizeof(GLfloat), count*3, file);
    fread(&bitangents.front(), sizeof(GLfloat), count*3, file);
    fread(&tex_coords.front(), sizeof(GLfloat), count*2, file);
    fread(&count, sizeof(int), 1, file);
    ResizeFaces(count);
    fread(&faces.front(), sizeof(GLfloat), count*3, file);
    fread(&face_normals.front(), sizeof(vec3), count, file);

    fread(&precollapse_num_vertices, sizeof(int), 1, file);
    fread(&count, sizeof(int), 1, file);
    precollapse_vert_reorder.resize(count);
    fread(&precollapse_vert_reorder.front(), sizeof(int), count, file);

    fread(&count, sizeof(int), 1, file);
    optimize_vert_reorder.resize(count);
    fread(&optimize_vert_reorder.front(), sizeof(int), count, file);

    fread(&count, sizeof(int), 1, file);
    if (count > 0) {
        tex_coords2.resize(count*2);
        fread(&tex_coords2.front(), sizeof(GLfloat), count*2, file);
    }
    
    fread(&min_coords, sizeof(vec3), 1, file);
    fread(&max_coords, sizeof(vec3), 1, file);
    fread(&center_coords, sizeof(vec3), 1, file);
    fread(&old_center, sizeof(vec3), 1, file);
    fread(&bounding_sphere_origin, sizeof(vec3), 1, file);
    fread(&bounding_sphere_radius, sizeof(float), 1, file);

    fread(&texel_density, sizeof(float), 1, file);
    fread(&average_triangle_edge_length, sizeof(float), 1, file);
}

void Model::Dispose() {
    vertices.clear();
    normals.clear();
    tangents.clear();
    bitangents.clear();
    face_normals.clear();
    tex_coords.clear();
    tex_coords2.clear();
    aux.clear();
    faces.clear();

    bone_weights.clear();
    bone_ids.clear();
    transform_vec.clear();
    tex_transform_vec.clear();
    morph_transform_vec.clear();

    image_samplers.clear();

    optimize_vert_reorder.clear();
    precollapse_vert_reorder.clear();

    if(vbo_enabled&&vbo_loaded)
    {
        VBO_vertices.Dispose();
        VBO_tex_coords.Dispose();
        VBO_normals.Dispose();
        VBO_faces.Dispose();
        VBO_tangents.Dispose();
        VBO_bitangents.Dispose();
        VBO_tex_coords2.Dispose();
        VBO_aux.Dispose();
    }
    vbo_loaded = false;
}

//Dispose of model
Model::~Model() {    
    Dispose();
}

float StaticCalcTexelDensity(const std::vector<float> &vertices, const std::vector<float> &tex_coords, const std::vector<unsigned> &faces){
    float total_tex = 0.0f;
    float total_vert = 0.0f;
    int next;
    for (int i=0, len=faces.size()/3; i<len; i++) {
        int face_vertex_index[3];
        const float *vert[3];
        const float *tc[3];
        for(int j=0; j<3; ++j) {
            face_vertex_index[j] = faces[i*3+j];
            vert[j] = &vertices[face_vertex_index[j]*3];
            tc[j] = &tex_coords[face_vertex_index[j]*2];
        }
        for(int j=0; j<3; j++) {
            next = (j+1)%3;
            float tex_distance = sqrtf( square(tc[j][0] - tc[next][0]) +
                                        square(tc[j][1] - tc[next][1]));
            float vert_distance = sqrtf( square(vert[j][0] - vert[next][0]) +
                                         square(vert[j][1] - vert[next][1]) +
                                         square(vert[j][2] - vert[next][2]));
            if(vert_distance != 0.0f){
                total_tex += tex_distance;
                total_vert += vert_distance;
            }
        }
    }
    return (total_tex / total_vert);
}

void Model::CalcTexelDensity() {
    texel_density = StaticCalcTexelDensity(vertices, tex_coords, faces);
}

struct ObjFileStats {
    int num_normal;
    int num_tex;
    int num_vert;
    int num_face;
    int objects; //Number of objects specified in the file, for this game this should only be 1
};

bool GetObjFileStats(std::string rel_path, ObjFileStats &stats, bool file_necessary) {
    stats.num_normal = 0;
    stats.num_tex = 0;
    stats.num_vert = 0;
    stats.num_face = 0;
    stats.objects = 0;

    char abs_path[kPathSize];
	FindFilePath(rel_path.c_str(), abs_path, kPathSize, kAnyPath, file_necessary);
    FILE *fp = my_fopen(abs_path,"r");

    if(!fp){
        if(file_necessary) {
            FatalError("Error", "Could not open file fs \"%s\"", rel_path.c_str());
        } else {
            return false;
        }
    }

    char buffer[256];
    while(!feof(fp))
    {
        memset(buffer,0,200);
        fgets(buffer,256,fp);

        if( strncmp("vn ",buffer,3) == 0 )
            ++stats.num_normal;
        else if( strncmp("vt ",buffer,3) == 0 )
            ++stats.num_tex;
        else if( strncmp("v ",buffer,2) == 0 )
            ++stats.num_vert;
        else if( strncmp("f ",buffer,2) == 0 )
            ++stats.num_face;
        else if( strncmp("o ",buffer,2) == 0 )
            ++stats.objects;
    }

    //There is always at least one object in a file.
    if( stats.objects == 0 )
        stats.objects = 1;

    fclose(fp);

    return true;
}

struct TempModel {
    std::vector<GLfloat> vertices;
    std::vector<GLfloat> tangents;
    std::vector<GLfloat> bitangents;
    std::vector<GLfloat> normals;
    std::vector<GLfloat> tex_coords;
    std::vector<GLfloat> tex_coords2;
    std::vector<GLuint> vert_indices;
    std::vector<GLuint> tex_indices;
    std::vector<GLuint> tex_indices2;
    std::vector<GLuint> norm_indices;
    unsigned int vc,nc,tc,fc;
};

void LoadTempModel(std::string rel_path, TempModel &temp, const ObjFileStats &obj_stats) {
    temp.vertices.resize(obj_stats.num_vert*3);
    temp.normals.resize(obj_stats.num_normal*3);
    temp.tangents.resize(obj_stats.num_tex*3);
    temp.bitangents.resize(obj_stats.num_tex*3);
    temp.tex_coords.resize(obj_stats.num_tex*2);
    temp.tex_coords2.resize(0);
    temp.vert_indices.resize(obj_stats.num_face*6);
    temp.tex_indices.resize(obj_stats.num_face*6);
    temp.norm_indices.resize(obj_stats.num_face*6);

    char abs_path[kPathSize];
	FindFilePath(rel_path.c_str(), abs_path, kPathSize, kAnyPath);
    FILE *fp = my_fopen(abs_path,"r");

    int num_read;

    temp.fc = 0;
    temp.nc = 0;
    temp.tc = 0;
    temp.vc = 0;

    char buffer[256];
    while(!feof(fp))
    {
        memset(buffer,0,255);
        fgets(buffer,256,fp);

        if( strncmp("vn ",buffer,3) == 0 )
        {
            sscanf((buffer+2),"%f%f%f",
                &temp.normals[temp.nc*3+0],
                &temp.normals[temp.nc*3+1],
                &temp.normals[temp.nc*3+2]);
            ++temp.nc;
        }
        else if( strncmp("vt ",buffer,3) == 0 )
        {
            sscanf((buffer+2),"%f%f",
                &temp.tex_coords[temp.tc*2+0],
                &temp.tex_coords[temp.tc*2+1]);
            ++temp.tc;
        }
        else if( strncmp("v ",buffer,2) == 0 )
        {
            sscanf((buffer+1),"%f%f%f",
                &temp.vertices[temp.vc*3+0],
                &temp.vertices[temp.vc*3+1],
                &temp.vertices[temp.vc*3+2]);
            ++temp.vc;
        }
        else if( strncmp("f ",buffer,2) == 0 )
        {
            if(obj_stats.num_tex&&obj_stats.num_normal) {
                num_read = sscanf(buffer+1,"%u/%u/%u %u/%u/%u %u/%u/%u %u/%u/%u",
                    &temp.vert_indices[temp.fc*3+0],
                    &temp.tex_indices[temp.fc*3+0],
                    &temp.norm_indices[temp.fc*3+0],

                    &temp.vert_indices[temp.fc*3+1],
                    &temp.tex_indices[temp.fc*3+1],
                    &temp.norm_indices[temp.fc*3+1],

                    &temp.vert_indices[temp.fc*3+2],
                    &temp.tex_indices[temp.fc*3+2],
                    &temp.norm_indices[temp.fc*3+2],

                    &temp.vert_indices[temp.fc*3+3],
                    &temp.tex_indices[temp.fc*3+3],
                    &temp.norm_indices[temp.fc*3+3]);
                if(num_read == 12) {
                    temp.vert_indices[temp.fc*3+4] = temp.vert_indices[temp.fc*3+0];
                    temp.tex_indices[temp.fc*3+4] = temp.tex_indices[temp.fc*3+0];
                    temp.norm_indices[temp.fc*3+4] = temp.norm_indices[temp.fc*3+0];

                    temp.vert_indices[temp.fc*3+5] = temp.vert_indices[temp.fc*3+2];
                    temp.tex_indices[temp.fc*3+5] = temp.tex_indices[temp.fc*3+2];
                    temp.norm_indices[temp.fc*3+5] = temp.norm_indices[temp.fc*3+2];

                    temp.fc++;
                }
            } else if (obj_stats.num_normal) {
                num_read = sscanf(buffer+1,"%u//%u %u//%u %u//%u %u//%u",
                    &temp.vert_indices[temp.fc*3+0],
                    &temp.norm_indices[temp.fc*3+0],

                    &temp.vert_indices[temp.fc*3+1],
                    &temp.norm_indices[temp.fc*3+1],

                    &temp.vert_indices[temp.fc*3+2],
                    &temp.norm_indices[temp.fc*3+2],

                    &temp.vert_indices[temp.fc*3+3],
                    &temp.norm_indices[temp.fc*3+3]);

                if(num_read == 8) {
                    temp.vert_indices[temp.fc*3+4] = temp.vert_indices[temp.fc*3+0];
                    temp.norm_indices[temp.fc*3+4] = temp.norm_indices[temp.fc*3+0];

                    temp.vert_indices[temp.fc*3+5] = temp.vert_indices[temp.fc*3+2];
                    temp.norm_indices[temp.fc*3+5] = temp.norm_indices[temp.fc*3+2];

                    temp.fc++;
                }
            } else if (obj_stats.num_tex) {
                num_read = sscanf(buffer+1,"%u/%u %u/%u %u/%u %u/%u",
                    &temp.vert_indices[temp.fc*3+0],
                    &temp.tex_indices[temp.fc*3+0],

                    &temp.vert_indices[temp.fc*3+1],
                    &temp.tex_indices[temp.fc*3+1],

                    &temp.vert_indices[temp.fc*3+2],
                    &temp.tex_indices[temp.fc*3+2],

                    &temp.vert_indices[temp.fc*3+3],
                    &temp.tex_indices[temp.fc*3+3]);

                if(num_read == 8) {
                    temp.vert_indices[temp.fc*3+4] = temp.vert_indices[temp.fc*3+0];
                    temp.tex_indices[temp.fc*3+4] = temp.tex_indices[temp.fc*3+0];

                    temp.vert_indices[temp.fc*3+5] = temp.vert_indices[temp.fc*3+2];
                    temp.tex_indices[temp.fc*3+5] = temp.tex_indices[temp.fc*3+2];

                    temp.fc++;
                }
            } else {
                num_read = sscanf(buffer+1,"%u %u %u %u",
                    &temp.vert_indices[temp.fc*3+0],
                    &temp.vert_indices[temp.fc*3+1],
                    &temp.vert_indices[temp.fc*3+2],
                    &temp.vert_indices[temp.fc*3+3]);

                if(num_read == 4) {
                    temp.vert_indices[temp.fc*3+4] = temp.vert_indices[temp.fc*3+0];
                    temp.vert_indices[temp.fc*3+5] = temp.vert_indices[temp.fc*3+2];                        
                    temp.fc++;
                }
            }
            temp.fc++;
        }
    }
    fclose(fp);

    bool vert_index_error = false;
    bool tex_index_error = false;
    bool norm_index_error = false;  

    for (unsigned j = 0;j<temp.fc*3;j++){

        if( temp.vert_indices[j] > 0 )
            temp.vert_indices[j]--;

        if( temp.tex_indices[j] > 0 )
            temp.tex_indices[j]--;

        if( temp.norm_indices[j] > 0 )
            temp.norm_indices[j]--;
            

        if( temp.vert_indices[j] > temp.vertices.size()/3 && temp.vert_indices[j] != 0)
        {
            temp.vert_indices[j] = 0; //This is better than NaN data or a segfault.
            vert_index_error = true;
        }

        if( temp.tex_indices[j] > temp.tex_coords.size()/2 && temp.tex_indices[j] != 0)
        {
            temp.tex_indices[j] = 0;
            tex_index_error = true;
        }

        if( temp.norm_indices[j] > temp.normals.size()/3 && temp.norm_indices[j] != 0 )
        {
            temp.norm_indices[j] = 0;
            norm_index_error = true;
        }
    }

    if( vert_index_error )
    {
        LOGE << "Error when loading: " << rel_path << " face (v) indices are higher than parsed data, file is probably corrupt." << std::endl;
    }

    if( tex_index_error )
    {
        LOGE << "Error when loading: " << rel_path << " face (vt) indices are higher than parsed data, file is probably corrupt." << std::endl;
    }

    if( norm_index_error )
    {
        LOGE << "Error when loading: " << rel_path << " face (vn) indices are higher than parsed data, file is probably corrupt." << std::endl;
    }
}

void LoadSecondUVs(std::string name, TempModel &temp, const ObjFileStats &obj_stats) {
    temp.tex_coords2.resize(obj_stats.num_tex*2);
    temp.tex_indices2.resize(obj_stats.num_face*6);
    
    char abs_path[kPathSize];
    FindFilePath(name.c_str(), abs_path, kPathSize, kDataPaths|kModPaths);
    FILE *fp = my_fopen(abs_path,"r");

    int tc2 = 0;
    int fc2 = 0;
    unsigned int trash;
    int num_read;

    char buffer[256];
    while(!feof(fp))
    {
        memset(buffer,0,255);
        fgets(buffer,256,fp);

        if( strncmp("vt ",buffer,3) == 0 ) {
            sscanf((buffer+2),"%f%f",
                &temp.tex_coords2[tc2*2+0],
                &temp.tex_coords2[tc2*2+1]);
            ++tc2;
        } else if( strncmp("f ",buffer,2) == 0 ) {
            if(obj_stats.num_tex&&obj_stats.num_normal) {
                num_read = sscanf(buffer+1,"%u/%u/%u %u/%u/%u %u/%u/%u %u/%u/%u",
                    &trash,
                    &temp.tex_indices2[fc2*3+0],
                    &trash,

                    &trash,
                    &temp.tex_indices2[fc2*3+1],
                    &trash,

                    &trash,
                    &temp.tex_indices2[fc2*3+2],
                    &trash,

                    &trash,
                    &temp.tex_indices2[fc2*3+3],
                    &trash);
                if(num_read == 12) {
                    temp.tex_indices2[fc2*3+4] = temp.tex_indices2[fc2*3+0];
                    
                    temp.tex_indices2[fc2*3+5] = temp.tex_indices2[fc2*3+2];
                    
                    ++fc2;
                }
            } else if (obj_stats.num_tex) {
                num_read = sscanf(buffer+1,"%u/%u %u/%u %u/%u %u/%u",
                    &trash,
                    &temp.tex_indices2[fc2*3+0],

                    &trash,
                    &temp.tex_indices2[fc2*3+1],

                    &trash,
                    &temp.tex_indices2[fc2*3+2],

                    &trash,
                    &temp.tex_indices2[fc2*3+3]);

                if(num_read == 8) {
                    temp.tex_indices2[fc2*3+4] = temp.tex_indices2[fc2*3+0];

                    temp.tex_indices2[fc2*3+5] = temp.tex_indices2[fc2*3+2];

                    ++fc2;
                }
            }
            ++fc2;
        }
    }

    for (int j = 0;j<fc2*3;j++){
        temp.tex_indices2[j]--;
    }

    fclose(fp);
}

void Model::SaveObj(std::string name) {
    std::ofstream file;
    char abs_path[kPathSize];
    FormatString(abs_path, kPathSize, "%s%s", GetWritePath(modsource_).c_str(), name.c_str());
    my_ofstream_open(file, abs_path);

    unsigned index = 0;
    for(int i=0, len=vertices.size()/3; i<len; ++i){
        file << "v ";
        file << vertices[index++] << " ";
        file << vertices[index++] << " ";
        file << vertices[index++] << "\n";
    }

    if(!tex_coords.empty()){
        index = 0;
        for(int i=0, len=vertices.size()/3; i<len; ++i){
            file << "vt ";
            file << tex_coords[index++] << " ";
            file << tex_coords[index++] << "\n";
        }
    }

    if(!normals.empty()){
        index = 0;
        for(int i=0, len=vertices.size()/3; i<len; ++i){
            file << "vn ";
            file << normals[index++] << " ";
            file << normals[index++] << " ";
            file << normals[index++] << "\n";
        }
    }

    index = 0;

    if(tex_coords.empty() && normals.empty()){
        for(int i=0, len=faces.size()/3; i<len; ++i){
            file << "f ";
            file << faces[index++]+1 << " ";
            file << faces[index++]+1 << " ";
            file << faces[index++]+1 << "\n";
        }
    } else if(tex_coords.empty() || normals.empty()){
        for(int i=0, len=faces.size()/3; i<len; ++i){
            file << "f ";
            file << faces[index]+1 << "/" << faces[index]+1 << " ";
            index++;
            file << faces[index]+1 << "/" << faces[index]+1 << " ";
            index++;
            file << faces[index]+1 << "/" << faces[index]+1 << "\n";
            index++;
        }
    } else {
        for(int i=0, len=faces.size()/3; i<len; ++i){
            file << "f ";
            file << faces[index]+1 << "/" << faces[index]+1 << "/" << faces[index]+1 << " ";
            index++;
            file << faces[index]+1 << "/" << faces[index]+1 << "/" << faces[index]+1 << " ";
            index++;
            file << faces[index]+1 << "/" << faces[index]+1 << "/" << faces[index]+1 << "\n";
            index++;
        }
    }

    file.close();
}

const int Model::ERROR_MORE_THAN_ONE_OBJECT = 1;

int Model::SimpleLoadTriangleCutObj(const std::string &name_to_load) {
    ObjFileStats obj_stats;
    GetObjFileStats(name_to_load, obj_stats, true);

    TempModel temp;
    LoadTempModel(name_to_load, temp, obj_stats);

    //Check if the obj file has more than one model object packed into it.
    if( obj_stats.objects == 1 )
    {
        int num_faces = temp.fc;
        int num_vertices = num_faces*3;

        vertices.resize(num_vertices*3);
        if(obj_stats.num_normal){
            normals.resize(num_vertices*3);
        } else {
            normals.clear();
        }
        tangents.resize(num_vertices*3,0.0f);
        bitangents.resize(num_vertices*3,0.0f);
        tex_coords.resize(num_vertices*2);
        faces.resize(num_faces*3);
        face_normals.resize(num_faces);

        for (int i=0, len=faces.size(); i<len; i++){
            faces[i] = i;

            vertices[i*3+0] = temp.vertices[temp.vert_indices[i]*3+0];
            vertices[i*3+1] = temp.vertices[temp.vert_indices[i]*3+1];
            vertices[i*3+2] = temp.vertices[temp.vert_indices[i]*3+2];

            if(obj_stats.num_normal){
                normals[i*3+0] = temp.normals[temp.norm_indices[i]*3+0];
                normals[i*3+1] = temp.normals[temp.norm_indices[i]*3+1];
                normals[i*3+2] = temp.normals[temp.norm_indices[i]*3+2];
            }

            if(obj_stats.num_tex){
                tangents[i*3+0] = temp.tangents[temp.tex_indices[i]*3+0];
                tangents[i*3+1] = temp.tangents[temp.tex_indices[i]*3+1];
                tangents[i*3+2] = temp.tangents[temp.tex_indices[i]*3+2];

                bitangents[i*3+0] = temp.bitangents[temp.tex_indices[i]*3+0];
                bitangents[i*3+1] = temp.bitangents[temp.tex_indices[i]*3+1];
                bitangents[i*3+2] = temp.bitangents[temp.tex_indices[i]*3+2];

                tex_coords[i*2+0] = temp.tex_coords[temp.tex_indices[i]*2+0];
                tex_coords[i*2+1] = temp.tex_coords[temp.tex_indices[i]*2+1];
            }
        }

        tex_coords2.resize(num_vertices*2);
        for (int i = 0;i<num_vertices;i++){
            tex_coords2[i*2+0] = tex_coords[i*2+0];
            tex_coords2[i*2+1] = tex_coords[i*2+1];
        }
    }
    else
    {
        return ERROR_MORE_THAN_ONE_OBJECT;
    }

    return 0;
}
    
const char* Model::GetLoadErrorString(int err)
{
    switch( err )
    {
        case 0:
            return "No error";
        case ERROR_MORE_THAN_ONE_OBJECT:
            return "More than one object in .obj file, incompatible with loader.";
        default:
            return "Unknown error";
    }
}

void CopyTexCoords2( Model &a, const Model& b ) {
    a.tex_coords2.resize(a.vertices.size()/3*2);
    std::map<float,std::map<float,std::map<float,
        std::map<float,std::map<float,std::map<float,
        std::vector<int> > > > > > > dup_map;
    unsigned vert_index = 0;
    unsigned normal_index = 0;
    vec3 n_i;
    for(int i=0, len=a.vertices.size()/3; i<len; ++i){
        a.tex_coords2[i*2+0] = 0.0f;
        a.tex_coords2[i*2+1] = 0.0f;
        if(!a.normals.empty() && !b.normals.empty()){ 
            n_i = vec3(a.normals[normal_index+0],
                       a.normals[normal_index+1],
                       a.normals[normal_index+2]);
        }
        std::vector<int> &indices = dup_map[floorf(a.vertices[vert_index+0]*100.0f)]
        [floorf(a.vertices[vert_index+1]*100.0f)]
        [floorf(a.vertices[vert_index+2]*100.0f)]
        [floorf(n_i[0]*100.0f)]
        [floorf(n_i[1]*100.0f)]
        [floorf(n_i[2]*100.0f)];
        normal_index += 3;
        vert_index += 3;
        indices.push_back(i);
    }

    vert_index = 0;
    normal_index = 0;
    for(int i=0, len=b.vertices.size()/3; i<len; ++i){
        if(!a.normals.empty() && !b.normals.empty()){ 
            n_i = vec3(b.normals[normal_index+0],
                        b.normals[normal_index+1],
                        b.normals[normal_index+2]);
        }
        std::vector<int> &indices = dup_map[floorf(b.vertices[vert_index+0]*100.0f)]
        [floorf(b.vertices[vert_index+1]*100.0f)]
        [floorf(b.vertices[vert_index+2]*100.0f)]
        [floorf(n_i[0]*100.0f)]
        [floorf(n_i[1]*100.0f)]
        [floorf(n_i[2]*100.0f)];
        normal_index += 3;
        vert_index += 3;
        for(int index : indices){
            a.tex_coords2[index*2+0] = b.tex_coords[i*2+0];
            a.tex_coords2[index*2+1] = b.tex_coords[i*2+1];
        }
    }

    dup_map.clear();
    vert_index = 0;
    normal_index = 0;
    for(int i=0, len=a.vertices.size()/3; i<len; ++i){
        if(!a.normals.empty() && !b.normals.empty()){ 
            n_i = vec3(a.normals[normal_index+0],
                       a.normals[normal_index+1],
                       a.normals[normal_index+2]);
        }
        std::vector<int> &indices = dup_map[floorf(a.vertices[vert_index+0]*10.0f+0.5f)]
        [floorf(a.vertices[vert_index+1]*10.0f+0.5f)]
        [floorf(a.vertices[vert_index+2]*10.0f+0.5f)]
        [floorf(n_i[0]*10.0f+0.5f)]
        [floorf(n_i[1]*10.0f+0.5f)]
        [floorf(n_i[2]*10.0f+0.5f)];
        normal_index += 3;
        vert_index += 3;
        indices.push_back(i);
    }

    vert_index = 0;
    normal_index = 0;
    for(int i=0, len=b.vertices.size()/3; i<len; ++i){
        if(!a.normals.empty() && !b.normals.empty()){ 
            n_i = vec3(b.normals[normal_index+0],
                b.normals[normal_index+1],
                b.normals[normal_index+2]);
        }
        std::vector<int> &indices = dup_map[floorf(b.vertices[vert_index+0]*10.0f+0.5f)]
        [floorf(b.vertices[vert_index+1]*10.0f+0.5f)]
        [floorf(b.vertices[vert_index+2]*10.0f+0.5f)]
        [floorf(n_i[0]*10.0f+0.5f)]
        [floorf(n_i[1]*10.0f+0.5f)]
        [floorf(n_i[2]*10.0f+0.5f)];
        normal_index += 3;
        vert_index += 3;
        for(int index : indices){
            if(a.tex_coords2[index*2+0] == 0.0f &&
               a.tex_coords2[index*2+1] == 0.0f)
            {
                a.tex_coords2[index*2+0] = b.tex_coords[i*2+0];
                a.tex_coords2[index*2+1] = b.tex_coords[i*2+1];
            }
        }
    }

    dup_map.clear();
    vert_index = 0;
    normal_index = 0;
    for(int i=0, len=a.vertices.size()/3; i<len; ++i){
        if(!a.normals.empty() && !b.normals.empty()){ 
            n_i = vec3(a.normals[normal_index+0],
                a.normals[normal_index+1],
                a.normals[normal_index+2]);
        }
        std::vector<int> &indices = dup_map[floorf(a.vertices[vert_index+0]*1.0f+0.5f)]
        [floorf(a.vertices[vert_index+1]*1.0f+0.5f)]
        [floorf(a.vertices[vert_index+2]*1.0f+0.5f)]
        [floorf(n_i[0]*1.0f+0.5f)]
        [floorf(n_i[1]*1.0f+0.5f)]
        [floorf(n_i[2]*1.0f+0.5f)];
        normal_index += 3;
        vert_index += 3;
        indices.push_back(i);
    }

    vert_index = 0;
    normal_index = 0;
    for(int i=0, len=b.vertices.size()/3; i<len; ++i){
        if(!a.normals.empty() && !b.normals.empty()){ 
            n_i = vec3(b.normals[normal_index+0],
                b.normals[normal_index+1],
                b.normals[normal_index+2]);
        }
        std::vector<int> &indices = dup_map[floorf(b.vertices[vert_index+0]*1.0f+0.5f)]
        [floorf(b.vertices[vert_index+1]*1.0f+0.5f)]
        [floorf(b.vertices[vert_index+2]*1.0f+0.5f)]
        [floorf(n_i[0]*1.0f+0.5f)]
        [floorf(n_i[1]*1.0f+0.5f)]
        [floorf(n_i[2]*1.0f+0.5f)];
        normal_index += 3;
        vert_index += 3;
        for(int index : indices){
            if(a.tex_coords2[index*2+0] == 0.0f &&
                a.tex_coords2[index*2+1] == 0.0f)
            {
                a.tex_coords2[index*2+0] = b.tex_coords[i*2+0];
                a.tex_coords2[index*2+1] = b.tex_coords[i*2+1];
            }
        }
    }
}


void Model::LoadObj(const std::string &rel_path, char flags, const std::string &alt_name, const PathFlags searchPaths) {

    //LOGI << "Loading model: " << rel_path << std::endl;
    const char* fail_whale = "./Data/Models/Default/default_model_2.obj";

    bool center = (flags & _MDL_CENTER)!=0;
    bool simple = (flags & _MDL_SIMPLE)!=0;
    use_tangent = (flags & _MDL_USE_TANGENT)!=0;

    bool load_model = true;

    Dispose();

    path = rel_path;

    std::string name_to_load = rel_path;
    const std::string &cache_name_to_load = SanitizePath(alt_name.empty()?rel_path:alt_name);

    char abs_path[kPathSize], abs_cache_path[kPathSize];
    ModID modsource, cache_modsource;
    bool found_model = (FindFilePath(name_to_load.c_str(), abs_path, kPathSize, searchPaths, false, NULL, &modsource) != -1);
    bool found_cache = (FindFilePath((cache_name_to_load+".cache").c_str(), abs_cache_path, kPathSize, searchPaths, false, NULL, &cache_modsource) != -1);

    if (!found_model && !found_cache) {    // To do: fix bug when only cache exists
        std::string error_msg = "Whale fail :(\nModel file \"" + std::string(rel_path) + "\" not found. Loading the fail whale instead.";
        DisplayError("Error", error_msg.c_str(), _ok, false);
        name_to_load = fail_whale;
    }

    if(found_model){
        checksum = Checksum(abs_path);
    }
    
    if(found_cache){
        FILE *cache_file = my_fopen(abs_cache_path, "rb");
        if(cache_file){
            unsigned short file_checksum = 0;
            fread(&file_checksum, sizeof(unsigned short), 1, cache_file);
            unsigned short version;
            fread(&version, sizeof(unsigned short), 1, cache_file);
            if(checksum == file_checksum && version == _model_cache_version){
                ReadFromFile(cache_file);
                load_model = false;
                modsource_ = cache_modsource;
            }
            fclose(cache_file);
        }
    }

    if (load_model) {
        int modelLoadError = SimpleLoadTriangleCutObj(name_to_load);

        if( 0 == modelLoadError )
        {
            precollapse_num_vertices = vertices.size()/3;

            ObjFileStats obj_stats2;
            bool uv2_file = GetObjFileStats(name_to_load+"_UV2", obj_stats2, false);
            if (uv2_file) {
                Model temp2;
                int uv2_err = temp2.SimpleLoadTriangleCutObj(name_to_load+"_UV2");

                if( uv2_err == 0 )
                {
                    //CopyTexCoords2((*this), temp2);
                    if(temp2.vertices.size() != vertices.size()){
                        DisplayError("Error", ("Mismatched number of vertices in: "+
                                              (name_to_load+"_UV2") + ". Will not use data.").c_str() );
                    }
                    else
                    {
                        for(int i=0, len=vertices.size()/3*2; i<len; ++i){
                            tex_coords2[i] = temp2.tex_coords[i];
                        }
                    }
                }
                else
                {
                    DisplayError("Error", ("Malformed data in: "+
                                          (name_to_load+"_UV2" + ", reason: " + GetLoadErrorString(uv2_err) + " Will not use data.")).c_str() );
                }
            }


            if(flags & _MDL_FLIP_FACES){
                for(unsigned i=0; i<faces.size(); i+=3){
                    std::swap(faces[i+0], faces[i+2]);
                }
            }

            if(!simple){
                if(normals.empty()) {
                    calcNormals();
                } else {
                    calcFaceNormals();
                    for(int i=0, len=normals.size(); i<len; i+=3){
                        float length = sqrtf(square(normals[i]) + square(normals[i+1]) + square(normals[i+2]));
                        if(length != 0.0f){
                            normals[i+0] /= length;
                            normals[i+1] /= length;
                            normals[i+2] /= length;
                        }
                    }
                }
                calcTangents();
            } else {
                tex_coords.clear();
                tex_coords.resize(vertices.size()/3*2, 0.0f);
                normals.resize(vertices.size(), 0.0f);
            }
            calcBoundingBox();
            old_center = center_coords;
            if(center) {
                CenterModel();
            }
            calcBoundingSphere();
            
            CalcTexelDensity();
            CalcAverageTriangleEdge();

            RemoveDuplicatedVerts();
            RemoveDegenerateTriangles();
            OptimizeTriangleOrder();
            OptimizeVertexOrder();

            FILE *cache_file = my_fopen((GetWritePath(modsource) + cache_name_to_load + ".cache").c_str(), "wb");
            if(cache_file){
                fwrite(&checksum, sizeof(unsigned short), 1, cache_file);
                fwrite(&_model_cache_version, sizeof(unsigned short), 1, cache_file);
                WriteToFile(cache_file);
                fclose(cache_file);
            }
            modsource_ = modsource;
        }
        else
        {
            if( fail_whale == rel_path )
            {
                FatalError( "Error", "Unable to load fail whale, this is bad.");
            }
            else
            {
                std::string error_msg = "Whale fail :(\nModel file \"" + std::string(rel_path) + "\" is corrupt because: "+ GetLoadErrorString(modelLoadError) + ". Loading the fail whale instead.";
                DisplayError("Error", error_msg.c_str(), _ok, false);

                LoadObj(fail_whale, flags, fail_whale);
                return; //Quick out.
            }
        }
    }
    vbo_enabled = true;
    //PrintACMR();
}

void DrawModelVerts(Model& model, const char* vert_attrib_str, Shaders* shaders, Graphics* graphics, int shader) {
    if(!model.vbo_loaded){
        model.createVBO();
    }       
    model.VBO_vertices.Bind();
    model.VBO_faces.Bind();
    int vert_attrib_id = shaders->returnShaderAttrib(vert_attrib_str, shader);
    graphics->EnableVertexAttribArray(vert_attrib_id);
    glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, 3*sizeof(GLfloat), 0);
    graphics->DrawElements(GL_TRIANGLES, model.faces.size(), GL_UNSIGNED_INT, 0);
    graphics->ResetVertexAttribArrays();
    graphics->BindArrayVBO(0);
    graphics->BindElementVBO(0);
}

//Draw model
void Model::Draw() {
    PROFILER_GPU_ZONE(g_profiler_ctx, "Model::Draw");
    Graphics *graphics = Graphics::Instance();
    CHECK_GL_ERROR();
    if(vbo_enabled){
        if(!vbo_loaded){
            createVBO();
		}       
		CHECK_GL_ERROR(); 
        int flags = F_VERTEX_ARRAY | F_NORMAL_ARRAY | F_TEXTURE_COORD_ARRAY0;
        if(!tangents.empty()){
            flags |= F_TEXTURE_COORD_ARRAY1 | F_TEXTURE_COORD_ARRAY2;
        }
        if (!tex_coords2.empty()) {
            flags |= F_TEXTURE_COORD_ARRAY3;
        }
        if (!aux.empty()) {
            flags |= F_COLOR_ARRAY;
		}
		CHECK_GL_ERROR();
		graphics->SetClientStates(flags);
		CHECK_GL_ERROR();
		VBO_vertices.Bind();    
		CHECK_GL_ERROR();
		glVertexPointer(3, GL_FLOAT, 0, 0);
		CHECK_GL_ERROR();
		VBO_normals.Bind();    
		CHECK_GL_ERROR();
		glNormalPointer(GL_FLOAT, 0, 0);
		CHECK_GL_ERROR();
		graphics->SetClientActiveTexture(0);
		CHECK_GL_ERROR();
        VBO_tex_coords.Bind();        
		glTexCoordPointer(2, GL_FLOAT, 0, 0);
		CHECK_GL_ERROR();
        if(!tangents.empty()){
			graphics->SetClientActiveTexture(1);
			CHECK_GL_ERROR();
            VBO_tangents.Bind();    
            glTexCoordPointer(3, GL_FLOAT, 0, 0);
			graphics->SetClientActiveTexture(2);
			CHECK_GL_ERROR();
            VBO_bitangents.Bind();        
			glTexCoordPointer(3, GL_FLOAT, 0, 0);
			CHECK_GL_ERROR();
        }
        if (!tex_coords2.empty()) {
			graphics->SetClientActiveTexture(3);
			CHECK_GL_ERROR();
            VBO_tex_coords2.Bind();        
			glTexCoordPointer(2, GL_FLOAT, 0, 0);
			CHECK_GL_ERROR();
        }
        if (!aux.empty()) {
            VBO_aux.Bind();        
			glColorPointer(3, GL_FLOAT, 0, 0);
			CHECK_GL_ERROR();
        }
        VBO_faces.Bind();    
		graphics->DrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);
		CHECK_GL_ERROR();
        if (!tex_coords2.empty()) {
            Graphics::Instance()->SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY3,false);
        }        
        if(!tangents.empty()){
            Graphics::Instance()->SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY2,false);
            Graphics::Instance()->SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY1,false);
		}
		CHECK_GL_ERROR();
		Graphics::Instance()->SetClientActiveTexture(0);
		CHECK_GL_ERROR();
		graphics->BindArrayVBO(0);
		CHECK_GL_ERROR();
		graphics->BindElementVBO(0);
		CHECK_GL_ERROR();
    }
    else if (transform_vec.empty()) {
        int flags = F_VERTEX_ARRAY | F_NORMAL_ARRAY | F_TEXTURE_COORD_ARRAY0;
        if(!tangents.empty()){
            flags |= F_TEXTURE_COORD_ARRAY1 | F_TEXTURE_COORD_ARRAY2;
        }
        if (!tex_coords2.empty()) {
            flags |= F_TEXTURE_COORD_ARRAY3;
        }
        if (!aux.empty()) {
            flags |= F_COLOR_ARRAY;
        }
        if(!bone_weights.empty()){
            flags |= F_TEXTURE_COORD_ARRAY5;
        }
        if(!bone_ids.empty()){
            flags |= F_TEXTURE_COORD_ARRAY6;
        }
        graphics->SetClientStates(flags);        
        graphics->SetClientActiveTexture(0);
        glTexCoordPointer(2, GL_FLOAT, 0, &tex_coords[0]);
        glNormalPointer(GL_FLOAT, 0, &normals[0]);
        glVertexPointer(3, GL_FLOAT, 0, &vertices[0]);
        if(!tangents.empty()){
            graphics->SetClientActiveTexture(1);
            glTexCoordPointer(3, GL_FLOAT, 0, &tangents[0]);
            graphics->SetClientActiveTexture(2);
            glTexCoordPointer(3, GL_FLOAT, 0, &bitangents[0]);
        }
        if(!tex_coords2.empty()){
            graphics->SetClientActiveTexture(3);
            glTexCoordPointer(2, GL_FLOAT, 0, &tex_coords2[0]);
        }
        if(!aux.empty()){
            glColorPointer(3, GL_FLOAT, 0, &aux[0]);
        }
        if(!bone_weights.empty()){
            graphics->SetClientActiveTexture(5);
            glTexCoordPointer(4, GL_FLOAT, 0, &bone_weights[0]);
        }
        if(!bone_ids.empty()){
            graphics->SetClientActiveTexture(6);
            glTexCoordPointer(4, GL_FLOAT, 0, &bone_ids[0]);
        }
        graphics->DrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, &faces[0]);
        if(!tex_coords2.empty()){
            Graphics::Instance()->SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY3,false);
        }
        if(!bone_weights.empty()){
            Graphics::Instance()->SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY5,false);
        }
        if(!bone_ids.empty()){
            Graphics::Instance()->SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY6,false);
        }
        if(!tangents.empty()){
            Graphics::Instance()->SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY2,false);
            Graphics::Instance()->SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY1,false);
        }
        Graphics::Instance()->SetClientActiveTexture(0);
    } else {
        int flags = F_VERTEX_ARRAY | F_NORMAL_ARRAY | F_TEXTURE_COORD_ARRAY0 | F_TEXTURE_COORD_ARRAY1 | F_TEXTURE_COORD_ARRAY2 | F_TEXTURE_COORD_ARRAY4 | F_TEXTURE_COORD_ARRAY5;
        if(!tangents.empty()){
            flags |= F_TEXTURE_COORD_ARRAY3;
        }
        if (!tex_coords2.empty()) {
            flags |= F_TEXTURE_COORD_ARRAY3;
        }
        if (!aux.empty()) {
            flags |= F_COLOR_ARRAY;
        }
        if(!bone_weights.empty()){
            flags |= F_TEXTURE_COORD_ARRAY5;
        }
        if(!bone_ids.empty()){
            flags |= F_TEXTURE_COORD_ARRAY6;
        }
        graphics->SetClientStates(flags);        
        glNormalPointer(GL_FLOAT, 0, &normals[0]);
        glVertexPointer(3, GL_FLOAT, 0, &vertices[0]);
        graphics->SetClientActiveTexture(0);
        glTexCoordPointer(2, GL_FLOAT, 0, &tex_coords[0]);        
        graphics->SetClientActiveTexture(1);
        glTexCoordPointer(4, GL_FLOAT, 0, &transform_vec[0][0]);
        graphics->SetClientActiveTexture(2);
        glTexCoordPointer(4, GL_FLOAT, 0, &transform_vec[1][0]);
        if(!tangents.empty()){
            graphics->SetClientActiveTexture(3);
            glTexCoordPointer(3, GL_FLOAT, 0, &tangents[0]);
        }
        graphics->SetClientActiveTexture(4);
        glTexCoordPointer(4, GL_FLOAT, 0, &transform_vec[2][0]);
        graphics->SetClientActiveTexture(5);
        glTexCoordPointer(4, GL_FLOAT, 0, &transform_vec[3][0]);
        graphics->DrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, &faces[0]);
        if(!tangents.empty()){
            Graphics::Instance()->SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY3,false);
        }        
        Graphics::Instance()->SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY4,false);
        Graphics::Instance()->SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY5,false);
        Graphics::Instance()->SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY2,false);
        Graphics::Instance()->SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY1,false);
        Graphics::Instance()->SetClientActiveTexture(0);
    }

    CHECK_GL_ERROR();
}

void Model::StartPreset() {
    Graphics *graphics = Graphics::Instance();
    if(vbo_enabled){
        if(!vbo_loaded){
            createVBO();
        }        
        int flags = F_VERTEX_ARRAY | F_NORMAL_ARRAY | F_TEXTURE_COORD_ARRAY0;
        if(!tangents.empty()){
            flags |= F_TEXTURE_COORD_ARRAY1 | F_TEXTURE_COORD_ARRAY2;
        }
        if (!tex_coords2.empty()) {
            flags |= F_TEXTURE_COORD_ARRAY3;
        }
        if (!aux.empty()) {
            flags |= F_COLOR_ARRAY;
        }
        graphics->SetClientStates(flags);
        VBO_vertices.Bind();    
        glVertexPointer(3, GL_FLOAT, 0, 0);
        VBO_normals.Bind();    
        glNormalPointer(GL_FLOAT, 0, 0);
        graphics->SetClientActiveTexture(0);
        VBO_tex_coords.Bind();        
        glTexCoordPointer(2, GL_FLOAT, 0, 0);
        if(!tangents.empty()){
            graphics->SetClientActiveTexture(1);
            VBO_tangents.Bind();    
            glTexCoordPointer(3, GL_FLOAT, 0, 0);
            graphics->SetClientActiveTexture(2);
            VBO_bitangents.Bind();        
            glTexCoordPointer(3, GL_FLOAT, 0, 0);
        }
        if (!tex_coords2.empty()) {
            graphics->SetClientActiveTexture(3);
            VBO_tex_coords2.Bind();        
            glTexCoordPointer(2, GL_FLOAT, 0, 0);
        }
        if (!aux.empty()) {
            VBO_aux.Bind();        
            glColorPointer(3, GL_FLOAT, 0, 0);
        }        
        VBO_faces.Bind();    
    }
}

void Model::EndPreset() {
    if(vbo_enabled){
        if (!aux.empty()) {
            //glClientActiveTexture(GL_TEXTURE4);
            //glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        }
        
        if (!tex_coords2.empty()) {
            Graphics::Instance()->SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY3,false);
        }
        
        if(!tangents.empty()){
            Graphics::Instance()->SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY2,false);
            Graphics::Instance()->SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY1,false);
        }
        Graphics::Instance()->SetClientActiveTexture(0);
        Graphics::Instance()->BindArrayVBO(0);
        Graphics::Instance()->BindElementVBO(0);
    }
}

void Model::DrawPreset()
{
    if(vbo_enabled){
        Graphics::Instance()->DrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);
    }
}

struct AuxVertData {
    std::vector<int> verts;
    vec3 total_aux;
};

bool Model::LoadAuxFromImage(const std::string &image_path) {    
    bool ret = true;
    if(aux.empty()) {
        ImageSamplerRef image = Engine::Instance()->GetAssetManager()->LoadSync<ImageSampler>(image_path);
        
        if(image.valid()) {
            image_samplers.insert(image);

            aux.resize(vertices.size());
            for(int i=0, len=vertices.size()/3; i<len; i++) {
                vec4 color = image->GetInterpolatedColorUV(tex_coords[i*2+0], tex_coords[i*2+1]);
                aux[i*3+0] = color.r();
                aux[i*3+1] = color.g();
                aux[i*3+2] = color.b();
            }

            // Duplicate map keeps a list of all vertices that share the same position
            std::map<vec3, AuxVertData> dup_map;
            unsigned vert_index = 0;
            for(int i=0, len=vertices.size()/3; i<len; i++) {
                vec3 index(vertices[vert_index+0],
                           vertices[vert_index+1],
                           vertices[vert_index+2]);
                AuxVertData &avd = dup_map[index];
                avd.verts.push_back(i);
                avd.total_aux[0] += aux[vert_index + 0];
                avd.total_aux[1] += aux[vert_index + 1];
                avd.total_aux[2] += aux[vert_index + 2];
                vert_index += 3;
            }
            std::map<vec3, AuxVertData>::iterator iter;
            for(iter = dup_map.begin(); iter != dup_map.end(); ++iter){
                AuxVertData &avd = iter->second;
                if(avd.verts.size() > 1){
                    avd.total_aux /= (float)avd.verts.size();
                    for(unsigned i=0; i<avd.verts.size(); ++i){
                        int index = avd.verts[i]*3;
                        aux[index + 0] = avd.total_aux[0];
                        aux[index + 1] = avd.total_aux[2];
                        aux[index + 2] = avd.total_aux[1];
                    }
                }
            }
        } else {
            LOGE << "Failed to load wind map from image: " << image_path << std::endl;
            ret = false;
        }
    } else {
        //Nop instruction, this state is ok.
    }
    return ret;
}

void Model::DrawAltTexCoords()
{
    PROFILER_GPU_ZONE(g_profiler_ctx, "Model::DrawAltTexCoords");
    CHECK_GL_ERROR();
    
    Graphics *graphics = Graphics::Instance();
    int flags = F_VERTEX_ARRAY | F_NORMAL_ARRAY | F_TEXTURE_COORD_ARRAY0;
    if(!tangents.empty()){
        flags |= F_TEXTURE_COORD_ARRAY1 | F_TEXTURE_COORD_ARRAY2;
    }
    graphics->SetClientStates(flags);  

    if(vbo_enabled){
        if(!vbo_loaded){
            createVBO();
        }
        VBO_vertices.Bind();    
        glVertexPointer(3, GL_FLOAT, 0, 0);
        VBO_normals.Bind();        
        glNormalPointer(GL_FLOAT, 0, 0);
        graphics->SetClientActiveTexture(0);
        VBO_tex_coords.Bind();    
        glTexCoordPointer(2, GL_FLOAT, 0, 0);
        if (!tex_coords2.empty()) {
            VBO_tex_coords2.Bind();    
            glTexCoordPointer(2, GL_FLOAT, 0, 0);
        }
        if(!tangents.empty()){
            graphics->SetClientActiveTexture(1);
            VBO_tangents.Bind();        
            glTexCoordPointer(3, GL_FLOAT, 0, 0);
            graphics->SetClientActiveTexture(2);
            VBO_bitangents.Bind();            
            glTexCoordPointer(3, GL_FLOAT, 0, 0);
        }
        
        VBO_faces.Bind();
        graphics->DrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);

        if(!tangents.empty()){
            Graphics::Instance()->SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY2,false);
            Graphics::Instance()->SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY1,false);
        }
        Graphics::Instance()->SetClientActiveTexture(0);
        Graphics::Instance()->BindArrayVBO(0);
        Graphics::Instance()->BindElementVBO(0);
    }
    else
    {
        graphics->SetClientActiveTexture(0);
        glTexCoordPointer(2, GL_FLOAT, 0, &tex_coords[0]);
        glNormalPointer(GL_FLOAT, 0, &normals[0]);
        glVertexPointer(3, GL_FLOAT, 0, &vertices[0]);
        
        if(!tex_coords2.empty()){
            glTexCoordPointer(2, GL_FLOAT, 0, &tex_coords2[0]);
        }
        if(!tangents.empty()){
            graphics->SetClientActiveTexture(1);
            glTexCoordPointer(3, GL_FLOAT, 0, &tangents[0]);
            graphics->SetClientActiveTexture(2);
            glTexCoordPointer(3, GL_FLOAT, 0, &bitangents[0]);
        }
        
        graphics->DrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, &faces[0]);
    
        if(!tangents.empty()){
            Graphics::Instance()->SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY2,false);
            Graphics::Instance()->SetClientStateEnabled(CS_TEXTURE_COORD_ARRAY1,false);
        }
        Graphics::Instance()->SetClientActiveTexture(0);
    }

    CHECK_GL_ERROR();
}

void Model::DrawToTextureCoords()
{
    PROFILER_GPU_ZONE(g_profiler_ctx, "Model::DrawToTextureCoords");
    CHECK_GL_ERROR();
    
    Graphics *graphics = Graphics::Instance();
    int flags = F_VERTEX_ARRAY | F_NORMAL_ARRAY | F_TEXTURE_COORD_ARRAY0;
    if(!tangents.empty()){
        flags |= F_TEXTURE_COORD_ARRAY1 | F_TEXTURE_COORD_ARRAY2;
    }
    if (!tex_coords2.empty()) {
        flags |= F_TEXTURE_COORD_ARRAY3;
    }
    CHECK_GL_ERROR();
    graphics->SetClientStates(flags);  
    CHECK_GL_ERROR();

    if(vbo_enabled){
        if(!vbo_loaded){
            createVBO();
        }
        VBO_tex_coords.Bind();    
        glVertexPointer(2, GL_FLOAT, 0, 0);
        VBO_normals.Bind();            
        glNormalPointer(GL_FLOAT, 0, 0);
        graphics->SetClientActiveTexture(0);  
        VBO_tex_coords.Bind();        
        glTexCoordPointer(2, GL_FLOAT, 0, 0);
        if(!tangents.empty()){
            graphics->SetClientActiveTexture(1);  
            VBO_tangents.Bind();        
            glTexCoordPointer(3, GL_FLOAT, 0, 0);
            graphics->SetClientActiveTexture(2);  
            VBO_bitangents.Bind();        
            glTexCoordPointer(3, GL_FLOAT, 0, 0);
        }
        if (!tex_coords2.empty()) {
            graphics->SetClientActiveTexture(3);  
            VBO_tex_coords2.Bind();        
            glTexCoordPointer(2, GL_FLOAT, 0, 0);
        }        
        VBO_faces.Bind();    
        graphics->DrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);
        Graphics::Instance()->SetClientActiveTexture(0);
        Graphics::Instance()->BindArrayVBO(0);
        Graphics::Instance()->BindElementVBO(0); 
    } else {
        graphics->SetClientActiveTexture(0);  
        glTexCoordPointer(2, GL_FLOAT, 0, &tex_coords[0]);
        glNormalPointer(GL_FLOAT, 0, &normals[0]);
        glVertexPointer(2, GL_FLOAT, 0, &tex_coords[0]);
        
        if(!tangents.empty()){
            graphics->SetClientActiveTexture(1);  
            glTexCoordPointer(3, GL_FLOAT, 0, &tangents[0]);
            graphics->SetClientActiveTexture(2);  
            glTexCoordPointer(3, GL_FLOAT, 0, &bitangents[0]);
        }
        if(!tex_coords2.empty()){
            graphics->SetClientActiveTexture(3);  
            glTexCoordPointer(2, GL_FLOAT, 0, &tex_coords2[0]);
        }
        graphics->DrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, &faces[0]);
        Graphics::Instance()->SetClientActiveTexture(0);
    }
    graphics->SetClientStates(F_VERTEX_ARRAY | F_NORMAL_ARRAY | F_TEXTURE_COORD_ARRAY0);

    CHECK_GL_ERROR();
}

void Model::calcBoundingBox() {
    min_coords=0;
    max_coords=0;
    center_coords = 0;
    
    for(int i=0, len=vertices.size()/3, vert_index=0; i<len; i++, vert_index+=3) {
        for(int j=0; j<3; ++j){ 
            float coord = vertices[vert_index+j]; 
            if(i==0 || coord > max_coords[j]){ 
                max_coords[j] = coord; 
            } 
            if(i==0 || coord < min_coords[j]){ 
                min_coords[j] = coord; 
            } 
        } 
    }

    center_coords = (min_coords + max_coords)/2;
}

//Calculate bounding sphere
void Model::calcBoundingSphere() {
    bounding_sphere_origin = center_coords;

    float longest_distance=0;
    vec3 vert;
    for (int i=0, len=vertices.size()/3; i<len; i++) {
        vert.x()=vertices[i*3];
        vert.y()=vertices[i*3+1];
        vert.z()=vertices[i*3+2];
        if(distance_squared(bounding_sphere_origin, vert)>longest_distance)
            longest_distance=distance_squared(bounding_sphere_origin, vert);
    }
    bounding_sphere_radius=sqrtf(longest_distance);
}

// must be called after center_coords has been set (i.e. after call to calcBoundingSphere
void Model::CenterModel() {
    for(int i=0, len=vertices.size()/3; i<len; i++) {
        vertices[3*i] -= center_coords.x();
        vertices[3*i+1] -= center_coords.y();
        vertices[3*i+2] -= center_coords.z();
    }
    center_coords = vec3(0,0,0);

    calcBoundingBox();
}

void Model::ResizeVertices(int size) {
    vertices.resize(size*3);
    normals.resize(size*3);
    tangents.resize(size*3);
    bitangents.resize(size*3);
    tex_coords.resize(size*2);
    if(!bone_weights.empty()){
        bone_weights.resize(size);
    }if(!bone_ids.empty()){
        bone_ids.resize(size);
    }
}

void Model::ResizeFaces(int size) {
    bool success = false;
    while(!success){
        try {
            faces.resize(size*3);
            face_normals.resize(size);
            success = true;
        } catch (std::bad_alloc const&) {
            success = false;
            SDL_Delay(100);
            LOGE << "Vector alloc failed, trying again..." << std::endl;
        }
    }
}


void Model::calcFaceNormals() {
    face_normals.resize(faces.size() / 3);
    #pragma omp parallel for
    for(int i=0, len=faces.size()/3; i<len; i++){
        face_normals[i][0] = (vertices[faces[i*3+1]*3+1] - vertices[faces[i*3+0]*3+1])*(vertices[faces[i*3+2]*3+2] - vertices[faces[i*3+0]*3+2]) - (vertices[faces[i*3+1]*3+2] - vertices[faces[i*3+0]*3+2])*(vertices[faces[i*3+2]*3+1] - vertices[faces[i*3+0]*3+1]);
        face_normals[i][1] = (vertices[faces[i*3+1]*3+2] - vertices[faces[i*3+0]*3+2])*(vertices[faces[i*3+2]*3+0] - vertices[faces[i*3+0]*3+0]) - (vertices[faces[i*3+1]*3+0] - vertices[faces[i*3+0]*3+0])*(vertices[faces[i*3+2]*3+2] - vertices[faces[i*3+0]*3+2]);
        face_normals[i][2] = (vertices[faces[i*3+1]*3+0] - vertices[faces[i*3+0]*3+0])*(vertices[faces[i*3+2]*3+1] - vertices[faces[i*3+0]*3+1]) - (vertices[faces[i*3+1]*3+1] - vertices[faces[i*3+0]*3+1])*(vertices[faces[i*3+2]*3+0] - vertices[faces[i*3+0]*3+0]);
    }
}

void Model::calcNormals() {
    normals.resize(vertices.size());

    #pragma omp parallel for
    for(int i=0, len=vertices.size()/3; i<len; i++){
        normals[i*3+0]=0;
        normals[i*3+1]=0;
        normals[i*3+2]=0;
    }

    //vec3 edge1,edge2;

    calcFaceNormals();
    for(int i=0, len=faces.size()/3; i<len; i++){
        normals[faces[i*3+0]*3+0]+=face_normals[i].x();
        normals[faces[i*3+0]*3+1]+=face_normals[i].y();
        normals[faces[i*3+0]*3+2]+=face_normals[i].z();

        normals[faces[i*3+1]*3+0]+=face_normals[i].x();
        normals[faces[i*3+1]*3+1]+=face_normals[i].y();
        normals[faces[i*3+1]*3+2]+=face_normals[i].z();

        normals[faces[i*3+2]*3+0]+=face_normals[i].x();
        normals[faces[i*3+2]*3+1]+=face_normals[i].y();
        normals[faces[i*3+2]*3+2]+=face_normals[i].z();
    }
    #pragma omp parallel for
    for(int i=0, len=vertices.size()/3; i<len; i++){
        float length=sqrtf(square(normals[i*3+0])+square(normals[i*3+1])+square(normals[i*3+2]));
        
        //For some reasons the normals are sometimes 0,0,0 We have to check for this to prevent NaN.
        if( length != 0.0f )
        {
            normals[i*3+0]/=length;
            normals[i*3+1]/=length;
            normals[i*3+2]/=length;
        }
    }
    #pragma omp parallel for
    for(int i=0, len=faces.size()/3; i<len; i++){
        face_normals[i] = normalize(face_normals[i]);
    }
}

void Model::calcTangents() {
    tangents.resize(vertices.size());
    bitangents.resize(vertices.size());
#pragma omp parallel for
    for(int i=0, len=vertices.size(); i<len; i+=3){
        tangents[i+0]=0;
        tangents[i+1]=0;
        tangents[i+2]=0;

        bitangents[i+0]=0;
        bitangents[i+1]=0;
        bitangents[i+2]=0;
    }
#pragma omp parallel for
    for (long a=0, len=faces.size()/3; a<len; a++)
    {
        float x1, x2, y1, y2, z1, z2;
        float s1, s2, t1, t2;
        float r;

        vec3 sdir;
        vec3 tdir;

        x1 = vertices[faces[a*3+1]*3+0] - vertices[faces[a*3+0]*3+0];
        x2 = vertices[faces[a*3+2]*3+0] - vertices[faces[a*3+0]*3+0];
        y1 = vertices[faces[a*3+1]*3+1] - vertices[faces[a*3+0]*3+1];
        y2 = vertices[faces[a*3+2]*3+1] - vertices[faces[a*3+0]*3+1];
        z1 = vertices[faces[a*3+1]*3+2] - vertices[faces[a*3+0]*3+2];
        z2 = vertices[faces[a*3+2]*3+2] - vertices[faces[a*3+0]*3+2];

        s1 = tex_coords[faces[a*3+1]*2+0] - tex_coords[faces[a*3+0]*2+0];
        s2 = tex_coords[faces[a*3+2]*2+0] - tex_coords[faces[a*3+0]*2+0];
        t1 = tex_coords[faces[a*3+1]*2+1] - tex_coords[faces[a*3+0]*2+1];
        t2 = tex_coords[faces[a*3+2]*2+1] - tex_coords[faces[a*3+0]*2+1];

        float denom = (s1 * t2 - s2 * t1);
        if(denom != 0.0f) {
            r = 1.0F / denom;
        } else {
            r = 99999.0f;
        }
        sdir = vec3((t2 * x1 - t1 * x2) * r,
            (t2 * y1 - t1 * y2) * r,
            (t2 * z1 - t1 * z2) * r);

        tdir = vec3((s1 * x2 - s2 * x1) * r,
            (s1 * y2 - s2 * y1) * r,
            (s1 * z2 - s2 * z1) * r);

        #pragma omp critical
        {
        tangents[faces[a*3+0]*3+0] += sdir.x();
        tangents[faces[a*3+0]*3+1] += sdir.y();
        tangents[faces[a*3+0]*3+2] += sdir.z();
        tangents[faces[a*3+1]*3+0] += sdir.x();
        tangents[faces[a*3+1]*3+1] += sdir.y();
        tangents[faces[a*3+1]*3+2] += sdir.z();
        tangents[faces[a*3+2]*3+0] += sdir.x();
        tangents[faces[a*3+2]*3+1] += sdir.y();
        tangents[faces[a*3+2]*3+2] += sdir.z();

        bitangents[faces[a*3+0]*3+0] += tdir.x();
        bitangents[faces[a*3+0]*3+1] += tdir.y();
        bitangents[faces[a*3+0]*3+2] += tdir.z();
        bitangents[faces[a*3+1]*3+0] += tdir.x();
        bitangents[faces[a*3+1]*3+1] += tdir.y();
        bitangents[faces[a*3+1]*3+2] += tdir.z();
        bitangents[faces[a*3+2]*3+0] += tdir.x();
        bitangents[faces[a*3+2]*3+1] += tdir.y();
        bitangents[faces[a*3+2]*3+2] += tdir.z();
        }
    }

#pragma omp parallel for
    for(int i=0, len=vertices.size(); i<len; i+=3) {
        vec3 n(normals[i+0],normals[i+1],normals[i+2]);
        vec3 t(tangents[i+0],tangents[i+1],tangents[i+2]);

        // Gram-Schmidt orthogonalize
        vec3 tangent = normalize(t - n * dot(n, t));
        
        tangents[i+0]=tangent.x();
        tangents[i+1]=tangent.y();
        tangents[i+2]=tangent.z();
    }

#pragma omp parallel for
    for(int i=0, len=vertices.size(); i<len; i+=3){
        vec3 n(normals[i+0],normals[i+1],normals[i+2]);
        vec3 t(bitangents[i+0],bitangents[i+1],bitangents[i+2]);

        // Gram-Schmidt orthogonalize
        vec3 tangent = normalize(t - n * dot(n, t));
        
        bitangents[i+0]=tangent.x();
        bitangents[i+1]=tangent.y();
        bitangents[i+2]=tangent.z();
    }
}

//Set up simp_vertex buffer object
void Model::createVBO() {
    CHECK_GL_ERROR();
    
    VBO_vertices.Fill(kVBOFloat | kVBOStatic, vertices.size()*sizeof(float), &vertices[0]);
    if(!normals.empty()){
        VBO_normals.Fill(kVBOFloat | kVBOStatic, vertices.size()*sizeof(float), &normals[0]);
    }
    CHECK_GL_ERROR();
    if(!tangents.empty()){
        VBO_tangents.Fill(kVBOFloat | kVBOStatic, vertices.size()*sizeof(float), &tangents[0]);
    }
    CHECK_GL_ERROR();
    if(!bitangents.empty()){
        VBO_bitangents.Fill(kVBOFloat | kVBOStatic, vertices.size()*sizeof(float), &bitangents[0]);
    }
    
    CHECK_GL_ERROR();
    VBO_tex_coords.Fill(kVBOFloat | kVBOStatic, vertices.size()/3*2*sizeof(float), &tex_coords[0]);
    
    CHECK_GL_ERROR();
    if (!tex_coords2.empty()) {
        VBO_tex_coords2.Fill(kVBOFloat | kVBOStatic, vertices.size()/3*2*sizeof(float), &tex_coords2[0]);
    }
    
    if (!aux.empty()) {
        VBO_aux.Fill(kVBOFloat | kVBOStatic, vertices.size()*sizeof(float), &aux[0]);
    }
    
    CHECK_GL_ERROR();
    VBO_faces.Fill(kVBOElement | kVBOStatic, faces.size()*sizeof(GLuint), &faces[0]);
    
    CHECK_GL_ERROR();
    Graphics *graphics = Graphics::Instance();
    graphics->BindArrayVBO(0);
    graphics->BindElementVBO(0);
    vbo_loaded = true;

    CHECK_GL_ERROR();
}

struct ChildHit {
    float time;
    bool hit;
    int child;
};

class ChildHitCompare {
   public:
       bool operator()(const ChildHit &a, const ChildHit &b) {
            return a.time < b.time;
        }
};

//Check a line against the model for collision
int Model::lineCheck(const vec3& p1,const vec3& p2, vec3* p, vec3 *normal, bool backface) const {    
    if(!sphere_line_intersection(p1,p2,bounding_sphere_origin,bounding_sphere_radius))return -1;
    
    float distance;
    float olddistance=0;
    int intersecting=0;
    int firstintersecting=-1;
    vec3 point;
    vec3 end = p2;
    int face_index, vert_index;
    vec3 points[3];
    for (int j=0, len=faces.size()/3;j<len;j++){
        face_index = j*3;
        for(unsigned k=0; k<3; ++k){
            vert_index = faces[face_index+k]*3;
            points[k][0] = vertices[vert_index+0];
            points[k][1] = vertices[vert_index+1];
            points[k][2] = vertices[vert_index+2];
        }
        intersecting=LineFacet(p1,end,points[0],points[1],points[2],&point,face_normals[j]);
        if(intersecting){
            distance=distance_squared(p1, point);
            if(distance<olddistance||firstintersecting==-1){
                olddistance=distance; firstintersecting=j; if(p)*p=point; end=point; if(normal)*normal = face_normals[j];}
        }
    }
    if(firstintersecting!=-1)return firstintersecting;

    return -1;
}

//Check a line against the model for collision against front face only
int Model::lineCheckNoBackface(const vec3& p1,const vec3& p2, vec3* p, vec3 *normal) const {    
    return lineCheck(p1, p2, p, normal, false);
}

float StaticCalcAverageTriangleEdge(const std::vector<unsigned> &faces, const std::vector<float> &vertices) {
    float total_vert = 0.0f;
    int num_sample = 0;
    for(int i=0, len=faces.size(); i<len; i+=3){
        const float *vert[3];
        for(int j=0; j<3; j++) {
            int vert_index = faces[i+j];
            vert[j] = &vertices[vert_index*3];
        }
        for(int j=0; j<3; j++) {
            int next = (j+1)%3;
            float vert_distance = sqrtf( square(vert[j][0]-vert[next][0]) +
                                         square(vert[j][1]-vert[next][1]) +
                                         square(vert[j][2]-vert[next][2]));
            if(vert_distance != 0.0f){
                ++num_sample;
                total_vert += vert_distance;
            }
        }
    }
    return total_vert / ((float)num_sample);
}

void Model::CalcAverageTriangleEdge() {
    average_triangle_edge_length = StaticCalcAverageTriangleEdge(faces, vertices);
}

void Model::CopyFacesFromModel( const Model &source_model, const std::vector<int> &copy_faces ) {
    vbo_loaded = false;
    vbo_enabled = false;

    int num_faces = copy_faces.size();        
    faces.resize(num_faces*3);
    face_normals.resize(num_faces);

    for(int i=0; i<num_faces; i++){
        const int index = i*3;
        const int copy_index = copy_faces[i]*3;
        faces[index+0] = source_model.faces[copy_index+0];
        faces[index+1] = source_model.faces[copy_index+1];
        faces[index+2] = source_model.faces[copy_index+2];

        face_normals[i] = source_model.face_normals[copy_faces[i]];
    }

    std::vector<bool> vertex_included(source_model.vertices.size()/3, false);
    for(int i=0; i<num_faces; i++){
        const int index = i*3;
        vertex_included[faces[index+0]] = true;
        vertex_included[faces[index+1]] = true;
        vertex_included[faces[index+2]] = true;
    }

    std::vector<int> vertex_index(source_model.vertices.size()/3);
    std::vector<int> reverse_vertex_index;
    {
        int counter = 0;
        for(unsigned i=0; i<vertex_index.size(); i++){
            if(vertex_included[i]){
                reverse_vertex_index.push_back(i);
                vertex_index[i] = counter++;
            }
        }
    }
    
    for(int i=0; i<num_faces; i++){
        const int index = i*3;
        faces[index+0] = vertex_index[faces[index+0]];
        faces[index+1] = vertex_index[faces[index+1]];
        faces[index+2] = vertex_index[faces[index+2]];
    }

    int num_vertices = reverse_vertex_index.size();

    vertices.resize(num_vertices*3);
    normals.resize(num_vertices*3);

    if(!source_model.tangents.empty()){
        tangents.resize(num_vertices*3);
        bitangents.resize(num_vertices*3);
    }

    tex_coords.resize(num_vertices*2);
    if(!source_model.tex_coords2.empty()){
        tex_coords2.resize(num_vertices*2);
    }

    for(int i=0; i<num_vertices; i++){
        const int index = i*3;
        const int copy_index = reverse_vertex_index[i]*3;
        vertices[index+0] = source_model.vertices[copy_index+0];
        vertices[index+1] = source_model.vertices[copy_index+1];
        vertices[index+2] = source_model.vertices[copy_index+2];

        normals[index+0] = source_model.normals[copy_index+0];
        normals[index+1] = source_model.normals[copy_index+1];
        normals[index+2] = source_model.normals[copy_index+2];
    
        if(!source_model.tangents.empty()){
            tangents[index+0] = source_model.tangents[copy_index+0];
            tangents[index+1] = source_model.tangents[copy_index+1];
            tangents[index+2] = source_model.tangents[copy_index+2];

            bitangents[index+0] = source_model.bitangents[copy_index+0];
            bitangents[index+1] = source_model.bitangents[copy_index+1];
            bitangents[index+2] = source_model.bitangents[copy_index+2];
        }

        const int tex_index = i*2;
        const int copy_tex_index = reverse_vertex_index[i]*2;
        
        tex_coords[tex_index+0] = source_model.tex_coords[copy_tex_index+0];
        tex_coords[tex_index+1] = source_model.tex_coords[copy_tex_index+1];
        
        if(!source_model.tex_coords2.empty()){
            tex_coords2[tex_index+0] = source_model.tex_coords2[copy_tex_index+0];
            tex_coords2[tex_index+1] = source_model.tex_coords2[copy_tex_index+1];
        }
    }

    texel_density = source_model.texel_density;
    vbo_enabled = true;
}

void Model::LoadObjMorph(const std::string &rel_path, const std::string &rel_base_path){
    Dispose();
    char abs_path[kPathSize];
    ModID modsource;
    if( FindFilePath(rel_path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths, true, NULL, &modsource) == -1 ){
        FatalError("Error", "Could not find morph file: \"%s\"", rel_path.c_str());
    }

    bool load_model = true;
   
    size_t read_count = 0;
    unsigned short checksum = Checksum(abs_path);
    FILE *cache_file = my_fopen((GetWritePath(modsource) + rel_path + ".mcache").c_str(), "rb");
    if(cache_file){
        unsigned short file_checksum = 0;
        read_count = fread(&file_checksum, sizeof(unsigned short), 1, cache_file);
        LOG_ASSERT(read_count == 1);
        unsigned short version;
        read_count = fread(&version, sizeof(unsigned short), 1, cache_file);
        LOG_ASSERT(read_count == 1);
        if(checksum == file_checksum && version == _morph_cache_version){
            size_t count = 0;
            read_count = fread(&count, sizeof(int), 1, cache_file);
            LOG_ASSERT(read_count == 1);
            ResizeVertices(count);
            read_count = fread(&vertices[0], sizeof(GLfloat), count*3, cache_file);
            LOG_ASSERT_EQ(read_count, count*3);
            read_count = fread(&tex_coords[0], sizeof(GLfloat), count*2, cache_file);
            LOG_ASSERT_EQ(read_count, count*2);
            load_model = false;
        }
        fclose(cache_file);
    }
    
    if (load_model) {
        // Load base model to get bounding box
        ObjFileStats obj_stats;
        GetObjFileStats(rel_base_path, obj_stats, true);
        TempModel base_model;
        LoadTempModel(rel_base_path, base_model, obj_stats);
        GetObjFileStats(rel_path, obj_stats, true);
        TempModel morph_model;
        LoadTempModel(rel_path, morph_model, obj_stats);

        for(unsigned i=0; i<base_model.fc*3; ++i){
            vertices.push_back(base_model.vertices[base_model.vert_indices[i]*3+0]);
            vertices.push_back(base_model.vertices[base_model.vert_indices[i]*3+1]);
            vertices.push_back(base_model.vertices[base_model.vert_indices[i]*3+2]);
        }

        std::vector<GLfloat> old_tex_coords;
        for(unsigned i=0; i<base_model.fc*3; ++i){
            old_tex_coords.push_back(base_model.tex_coords[base_model.tex_indices[i]*2+0]);
            old_tex_coords.push_back(base_model.tex_coords[base_model.tex_indices[i]*2+1]);
        }

        std::vector<GLfloat> old_vertices = vertices;

        // Load morph vertices and tex coords
        vertices.clear();
        tex_coords.clear();
        for(unsigned i=0; i<base_model.fc*3; ++i){
            vertices.push_back(morph_model.vertices[morph_model.vert_indices[i]*3+0]);
            vertices.push_back(morph_model.vertices[morph_model.vert_indices[i]*3+1]);
            vertices.push_back(morph_model.vertices[morph_model.vert_indices[i]*3+2]);
        }
        for(unsigned i=0; i<base_model.fc*3; ++i){
            tex_coords.push_back(morph_model.tex_coords[morph_model.tex_indices[i]*2+0]);
            tex_coords.push_back(morph_model.tex_coords[morph_model.tex_indices[i]*2+1]);
        }

        for(unsigned i=0, len=vertices.size(); i<len; ++i){
            vertices[i] -= old_vertices[i];
        }
        for(unsigned i=0, len=tex_coords.size(); i<len; ++i){
            tex_coords[i] -= old_tex_coords[i];
        }
        
        FILE *cache_file = my_fopen((GetWritePath(modsource) + rel_path + ".mcache").c_str(), "wb");
        if(cache_file){
            fwrite(&checksum, sizeof(unsigned short), 1, cache_file);
            fwrite(&_morph_cache_version, sizeof(unsigned short), 1, cache_file);
            size_t count = vertices.size()/3;
            fwrite(&count, sizeof(int), 1, cache_file);
            fwrite(&vertices[0], sizeof(GLfloat), count*3, cache_file);
            fwrite(&tex_coords[0], sizeof(GLfloat), count*2, cache_file);
            fclose(cache_file);
        }
    }
}

namespace {
    struct VertInfo {
        static const int NUM_VERT_INFO_ENTRIES = 8;
        float entries[NUM_VERT_INFO_ENTRIES];
        int old_id;
    };

    bool operator<(const VertInfo &a, const VertInfo &b){
        static const int LEN = VertInfo::NUM_VERT_INFO_ENTRIES - 1;
        for(int i=0; i<LEN; ++i){
            if(a.entries[i] != b.entries[i]){
                return (a.entries[i] < b.entries[i]);
            }
        }
        return (a.entries[LEN] < b.entries[LEN]);
    }

    bool operator==(const VertInfo &a, const VertInfo &b){
        for(int i=0; i<VertInfo::NUM_VERT_INFO_ENTRIES; ++i){
            if(a.entries[i] != b.entries[i]){
                return false;
            }
        }
        return true;
    }

    bool operator!=(const VertInfo &a, const VertInfo &b){
        return !(a==b);
    }
} // namespace ""

void Model::RemoveDuplicatedVerts() {
    if(vertices.empty()){
        DisplayError("Warning", "Calling RemoveDuplicatedVerts on empty mesh.");
        return;
    }
    // Sort vector of vertices so that identical vertices are neighbors
    std::vector<VertInfo> vert_info;
    for(int i=0, vert_index=0, tc_index=0, len=vertices.size(); vert_index<len; ++i, vert_index += 3, tc_index += 2){
        VertInfo vi;
        vi.old_id = i;
        for(int i=0; i<3; ++i){
            vi.entries[i] = vertices[vert_index+i];
        }
        if(use_tangent && !normals.empty()){ // Only check normal info if the model uses tangent
            for(int i=0; i<3; ++i){
                vi.entries[i+3] = normals[vert_index+i];
            }
        } else {
            for(int i=0; i<3; ++i){
                vi.entries[i+3] = 0.0f;
            }
        }
        if(tex_coords.empty() == false) {
            for(int i=0; i<2; ++i){
                vi.entries[i+6] = tex_coords[tc_index+i];
            }
        }
        vert_info.push_back(vi);
    }
    std::sort(vert_info.begin(), vert_info.end());
    // Collect a vector of each first unique vertex, and the mapping from each old vertex to new unique vertices 
    std::vector<int> unique;
    std::vector<int> new_vert(vert_info.size(),-1);
    for(int i=0, len=vert_info.size(); i<len; ++i){
        if(i==0 || vert_info[i] != vert_info[i-1]){
            unique.push_back(i);
        }
        new_vert[vert_info[i].old_id] = unique.size()-1;
    }
    // Invert to precollapse vert reorder
    precollapse_vert_reorder.resize(unique.size());
    for(int i=0, len=unique.size(); i<len; ++i){
        precollapse_vert_reorder[i] = vert_info[unique[i]].old_id;
    }
    for(unsigned int & face : faces){
        face = new_vert[face];
    }

    RearrangeVertices(*this, precollapse_vert_reorder);

    LOGI << vert_info.size() - unique.size() << " of " << vert_info.size() << " vertices are duplicates." << std::endl;
    LOGI << "New vertex size: " << vertices.size()/3 << std::endl;
}


void Model::RemoveDegenerateTriangles() {
    unsigned count = 0;
    unsigned index = 0;
    unsigned copy_index = 0;
    for(int i=0, len=faces.size()/3; i<len; i++){
        if(faces[index] == faces[index+1] || 
           faces[index+1] == faces[index+2] ||
           faces[index] == faces[index+2])
        {
            count++;
            index += 3;
            continue;
        }
        faces[copy_index] = faces[index];
        faces[copy_index+1] = faces[index+1];
        faces[copy_index+2] = faces[index+2];
        copy_index += 3;
        index += 3;
    }
    faces.resize(copy_index);
    LOGI << "Removed " << count << " degenerate triangles." << std::endl;
}

void Model::CopyVertCollapse( const Model &source_model ) {
    RearrangeVertices(*this, source_model.precollapse_vert_reorder);
    RearrangeVertices(*this, source_model.optimize_vert_reorder);
}

struct VertData {
    int cache_pos;
    float score;
    unsigned not_added_triangles;
};

struct TriData {
    bool added;
    float score;
    unsigned verts[3];
};

const unsigned _vertex_cache_size = 32;
const float _fvs_cache_decay_power = 1.5f;
const float _fvs_last_tri_score = 0.75f;
const float _fvs_valence_boost_scale = 2.0f;
const float _fvs_valence_boost_power = 0.5f;

// Based on http://home.comcast.net/~tom_forsyth/papers/fast_vert_cache_opt.html
float FindVertexScore ( VertData *vertex_data )
{
    if ( vertex_data->not_added_triangles == 0 )
    {
        // No tri needs this vertex!
        return -1.0f;
    }

    float score = 0.0f;
    int cache_pos = vertex_data->cache_pos;
    if ( cache_pos < 0 )
    {
        // Vertex is not in FIFO cache - no score.
    }
    else
    {
        if ( cache_pos < 3 )
        {
            // This vertex was used in the last triangle,
            // so it has a fixed score, whichever of the three
            // it's in. Otherwise, you can get very different
            // answers depending on whether you add
            // the triangle 1,2,3 or 3,1,2 - which is silly.
            score = _fvs_last_tri_score;
        }
        else
        {
            assert ( cache_pos < (int)_vertex_cache_size );
            // Points for being high in the cache.
            const float scaler = 1.0f / ( _vertex_cache_size - 3 );
            score = 1.0f - ( cache_pos - 3 ) * scaler;
            score = pow ( score, _fvs_cache_decay_power );
        }
    }

    // Bonus points for having a low number of tris still to
    // use the vert, so we get rid of lone verts quickly.
    float ValenceBoost = pow ( (float)(vertex_data->not_added_triangles),
        -_fvs_valence_boost_power );
    score += _fvs_valence_boost_scale * ValenceBoost;

    return score;
}

void Model::PrintACMR(){
    unsigned cache_hits = 0;
    unsigned total = 0;
    std::vector<int> fifo(32, -1);
    unsigned index = 0;
    for(unsigned int face : faces){
        for(int j : fifo){
            if(j == (int)face){
                cache_hits++;
                break;
            }
        }
        fifo[index] = face;
        index = (index+1)%fifo.size();
        total++;
    }
    LOGI << "Opt: " << cache_hits << " cache hits out of " << total << "." << std::endl;
    LOGI << "Opt: ACMR: " << ((float)(total-cache_hits))/(float)total*3.0f << std::endl;
}

void Model::OptimizeTriangleOrder() {
    std::vector<TriData> tris(faces.size()/3);
    std::vector<VertData> verts(vertices.size()/3);

    unsigned index = 0;
    for(int i=0, len=faces.size()/3; i<len; i++){
        tris[i].verts[0] = faces[index++];
        tris[i].verts[1] = faces[index++];
        tris[i].verts[2] = faces[index++];
        for(unsigned int vert : tris[i].verts){
            verts[vert].not_added_triangles++;
        }
    }

    for(int i=0, len=vertices.size()/3; i<len; ++i){
        verts[i].cache_pos = -1;
        verts[i].score = FindVertexScore(&verts[i]);
    }

    int best_triangle = 0;
    float best_score = 0.0f;
    for(int i=0, len=faces.size()/3; i<len; i++){
        tris[i].score = verts[tris[i].verts[0]].score + 
                        verts[tris[i].verts[1]].score + 
                        verts[tris[i].verts[2]].score;
        if(tris[i].score > best_score){
            best_score = tris[i].score;
            best_triangle = i;
        }
    }

    int num_faces = faces.size()/3;;
    std::vector<int> lru(_vertex_cache_size,-1);
    std::vector<unsigned> draw_list(num_faces);
    int draw_list_index = 0;
    while(draw_list_index < num_faces){
        draw_list[draw_list_index++] = best_triangle;
        if(draw_list_index == num_faces){
            break;
        }
        tris[best_triangle].added = true;
        // Update not_added_triangles
        for(unsigned int vert_id : tris[best_triangle].verts){
            --verts[vert_id].not_added_triangles;
        }
        // Update LRU cache
        for(unsigned int vert_id : tris[best_triangle].verts){
            int cp = verts[vert_id].cache_pos;
            // If vert is in cache, remove it and slide other verts down
            if(cp != -1){
                for(unsigned j=cp; j<_vertex_cache_size; ++j){
                    if(j == _vertex_cache_size-1){
                        lru[j] = -1;
                    } else {
                        lru[j] = lru[j+1];
                        if(lru[j] != -1){
                            verts[lru[j]].cache_pos--;
                        }
                    }
                }
            }
            // Slide other verts up and add vert to cache
            for(int j=_vertex_cache_size-1; j>=0; --j){
                if(lru[j] != -1){
                    verts[lru[j]].cache_pos++;
                    if(j >= (int)_vertex_cache_size-1){
                        verts[lru[j]].cache_pos = -1; //Out of cache entirely
                    }
                    verts[lru[j]].score = FindVertexScore(&verts[lru[j]]);
                }
                if(j != 0){
                    lru[j] = lru[j-1];
                }
            }
            lru[0] = vert_id;
            verts[vert_id].cache_pos = 0;
            verts[vert_id].score = FindVertexScore(&verts[vert_id]);
        }
        // This can be optimized a lot, Forsyth's method just checks triangles
        // thave have a vertex in the LRU cache
        best_score = 0.0f;
        best_triangle = -1;
        for(int i=0; i<num_faces; ++i){
            if(tris[i].added){
                continue;
            }
            tris[i].score = verts[tris[i].verts[0]].score + 
                            verts[tris[i].verts[1]].score + 
                            verts[tris[i].verts[2]].score;
            if(best_triangle == -1 || tris[i].score > best_score){
                best_score = tris[i].score;
                best_triangle = i;
            }
        }
    }

    {
        unsigned index = 0;
        for(unsigned int i : draw_list){
            faces[index++] = tris[i].verts[0];
            faces[index++] = tris[i].verts[1];
            faces[index++] = tris[i].verts[2];
        }
    }
}


void Model::OptimizeVertexOrder() {
    std::vector<int> order(vertices.size()/3, -1);
    unsigned index = 0;
    for(unsigned int & face : faces){
        if(order[face] == -1){
            order[face] = index++;    
        }
        face = order[face];
    }
    optimize_vert_reorder.clear();
    optimize_vert_reorder.resize(index, -1);
    for(int i=0, len=order.size(); i<len; i++)
    {
        if( order[i] >= 0 && order[i] < (int)optimize_vert_reorder.size() )
        {
            optimize_vert_reorder[order[i]] = i;
        }
        else
        {
            LOGW << "Oddity when running " << __FUNCTION__ << ", order[i] is out of bounds, value is " << order[i] << " on index " << i << std::endl;
        }
    }
    RearrangeVertices(*this, optimize_vert_reorder);
}

struct edge {
    int vert_id[2];
};

class TriScoreSorter {
public:
    bool operator()(const TriData &a, const TriData &b) {
        return a.score > b.score;
    }
};

void Model::SortTrianglesBackToFront( const vec3 &camera )
{
    std::vector<TriData> tris(faces.size()/3);

    unsigned index = 0;
    for(int i=0, len=faces.size()/3; i<len; ++i){
        tris[i].added = false;
        tris[i].score = 0.0f;
        tris[i].verts[0] = faces[index++];
        tris[i].verts[1] = faces[index++];
        tris[i].verts[2] = faces[index++];
    }

    vec3 center;
    for(int i=0, len=faces.size()/3; i<len; ++i){
        center[0] = (vertices[tris[i].verts[0]*3+0] + 
                     vertices[tris[i].verts[1]*3+0] +
                     vertices[tris[i].verts[2]*3+0])/3.0f;
        center[1] = (vertices[tris[i].verts[0]*3+1] + 
                     vertices[tris[i].verts[1]*3+1] +
                     vertices[tris[i].verts[2]*3+1])/3.0f;
        center[2] = (vertices[tris[i].verts[0]*3+2] + 
                     vertices[tris[i].verts[1]*3+2] +
                     vertices[tris[i].verts[2]*3+2])/3.0f;
        tris[i].score = distance_squared(center, camera);
    }

    std::sort(tris.begin(), tris.end(), TriScoreSorter());

    index = 0;
    for(auto & tri : tris){
        faces[index++] = tri.verts[0];
        faces[index++] = tri.verts[1];
        faces[index++] = tri.verts[2];
    }
}
