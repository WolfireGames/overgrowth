//-----------------------------------------------------------------------------
//           Name: models.cpp
//      Developer: Wolfire Games LLC
//         Author: David Rosen
//    Description: The models class keeps track of all loaded models
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
#include "models.h"

#include <Internal/common.h>
#include <Internal/filesystem.h>

#include <Graphics/simplify.hpp>
#include <Compat/compat.h>
#include <Logging/logdata.h>
#include <Utility/waveform_obj_serializer.h>
#include <Utility/fqms_simplify.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4244)
#pragma warning(disable:4291)
#endif

#ifdef __GNU_LIBRARY__
#define HAVE_RINT
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

//-----------------------------------------------------------------------------
//Functions
//-----------------------------------------------------------------------------

void SimplifyModel(std::string name, Model& model, int edge_target) {
    LOGI << "Simplification - starting..." << std::endl;

    char path[kPathSize];
    FormatString(path, kPathSize, "%s%slow.obj", GetWritePath(CoreGameModID).c_str(), name.c_str());
    CreateParentDirs(path);

#ifdef _WIN32
    createfile(path);
    string short_path = path;
    ShortenWindowsPath(short_path);
    string output_filename = short_path.c_str();
#else
    string output_filename = path;
#endif

    Simplify::vertices.clear(); 
    for(int i = 0; i < model.vertices.size(); i+=3) {
        Simplify::Vertex v;
        v.p.x = model.vertices[i+0];
        v.p.y = model.vertices[i+1];
        v.p.z = model.vertices[i+2];
        Simplify::vertices.push_back(v);
    }

    Simplify::triangles.clear();
    for(int i = 0; i < model.faces.size(); i+=3){ 
        Simplify::Triangle t;

        t.v[0] = model.faces[i+0];
        t.v[1] = model.faces[i+1];
        t.v[2] = model.faces[i+2];
        t.deleted = false;
        t.dirty = false;
        t.attr = 0UL;
        t.material = -1;
        Simplify::triangles.push_back(t);
    }

    LOGW << "Simplifying terrain mesh, face count in: " << model.faces.size() << std::endl;
    Simplify::simplify_mesh(edge_target);
    LOGW << "Simplified terrain mesh, face count out: " << Simplify::triangle_count() << std::endl;

    Simplify::write_obj(output_filename.c_str());

    LOGW << "Saving simplified terrain mesh to \"" << output_filename << "\"" << std::endl; 
}

int Models::CopyModel(int model_id) {
    int id = AddModel();
    GetModel(id) = GetModel(model_id);
    return id;
}


//Load a model and return its ID
int Models::loadModel(const std::string& rel_path, char flags, const std::string& store_name) {
    int which_model=-1;    
    const std::string& check_name = store_name.empty()?rel_path:store_name;
    //Check if the model is already loaded
    std::map<std::string, int>::iterator iter = name_map.find(check_name);
    if(iter != name_map.end()){
        which_model = iter->second;
    }    
    //If model is not loaded; load it
    unsigned int i=0;
    if(which_model==-1){
        which_model = AddModel();
        ModelData& md = GetModelData(which_model);
        //Check extension
        while(i<rel_path.size()&&rel_path[i]!='.'){
            i++;
        }
        if(rel_path[i+1]=='o'){
            md.model.LoadObj(rel_path,flags,check_name);
        }
        md.name = rel_path;
        name_map[check_name] = which_model;
    }    
    return which_model;
}            

//Select which model to draw
void Models::drawModel(int which_model)
{
    _ASSERT(which_model != -1);
    GetModel(which_model).Draw();
}

void Models::drawModelAltTexCoords(int which_model)
{
    _ASSERT(which_model != -1);
    GetModel(which_model).DrawAltTexCoords();
}

//Return the radius of a model
float Models::Radius(int which_model)
{
    return GetModel(which_model).bounding_sphere_radius;
}

int Models::LoadModelAsMorph( const std::string& rel_path, int base_model_id, const std::string &base_model_name )
{
    MorphIndex index;
    index.first = rel_path;
    index.second = base_model_id;
    MorphMap::iterator iter = morph_map_.find(index);
    if(iter != morph_map_.end()){
        return iter->second;
    }

    int morph_model_id = AddModel();
    Model &morph_model = GetModel(morph_model_id);
    const std::string &name = base_model_name;
    morph_model.LoadObjMorph(rel_path, name);
    GetModelData(morph_model_id).name = std::string(rel_path);
    
    GetModel(morph_model_id).CopyVertCollapse(GetModel(base_model_id));

    morph_map_[index] = morph_model_id;

    return morph_model_id;
}

void Models::Dispose()
{
    models.clear();
    name_map.clear();
    id_map.clear();
}

void Models::DeleteModel(int id) {
    IDMap::iterator id_iter = id_map.find(id);
    if(id_iter == id_map.end()){
        return;
    }
    ModelData *data = id_iter->second;
    id_map.erase(id_iter);
    ModelList::iterator model_iter;
    for(model_iter = models.begin(); model_iter != models.end(); ++model_iter){
        ModelData& model_data = *model_iter;
        if(&model_data == data){
            break;
        }
    }
    if(model_iter != models.end()){
        NameMap::iterator name_iter = name_map.find(data->name);
        if(name_iter != name_map.end()){
            name_map.erase(name_iter);
        }
        models.erase(model_iter);
    }
}

int Models::AddModel()
{
    models.push_back(ModelData());
    int the_id = next_id++;
    id_map[the_id] = &models.back();
    return the_id;
}

Model& Models::GetModel( int id )
{
    return id_map[id]->model;
}

ModelData& Models::GetModelData( int id )
{
    return *id_map[id];
}

int Models::loadFlippedModel( const std::string& name, char flags /*= 0*/ )
{
    flags = flags | _MDL_FLIP_FACES;
    int id = loadModel(name, flags, name+"_flipped");
    return id;
}

int Models::NumModels() {
    return id_map.size();
}
