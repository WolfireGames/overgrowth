//-----------------------------------------------------------------------------
//           Name: skeletonasset.cpp
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
#include "skeletonasset.h"

#include <Compat/fileio.h>
#include <Compat/filepath.h>

#include <Graphics/models.h>
#include <Graphics/skeleton.h>

#include <Internal/checksum.h>
#include <Internal/filesystem.h>
#include <Internal/error.h>

#include <Asset/AssetLoader/fallbackassetloader.h>
#include <Physics/bulletworld.h>
#include <Logging/logdata.h>
#include <Math/vec3math.h>
#include <Main/engine.h>

#include <tinyxml.h>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

static const int _skeleton_version = 9;
static const int _min_skeleton_version = 6;

float CapsuleDistance (const vec3 &a, const vec3 &b, const vec3 &p) {
    float length = distance(b,a);
    vec3 dir = (b-a)/length;
    float t = dot(p-a,dir);
    if(t<0){
        return distance(a,p);
    }
    if(t>length){
        return distance(b,p);
    }
    return distance(a,p-t*dir);
}

int GetClosestBone( const SkeletonFileData &data, const vec3& point ) {
    int closest = -1;
    float closest_dist = 0.0f;
    float dist;
    for(unsigned i=0; i<data.bone_ends.size(); i+=2){
        dist = CapsuleDistance(data.points[data.bone_ends[i]], 
                               data.points[data.bone_ends[i+1]],
                               point);
        if(closest == -1 || dist < closest_dist){
            closest_dist = dist;
            closest = i/2;
        }
    }
    return closest;
}


int SkeletonAsset::Load( const std::string &rel_path, uint32_t load_flags ) {
    char abs_path[kPathSize];
    bool retry = true;
    if (FindFilePath(rel_path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths) == -1) {
        return kLoadErrorMissingFile;
    }

    TiXmlDocument doc(abs_path);
    doc.LoadFile();
    if( doc.Error() ) {
        return kLoadErrorCouldNotOpenXML;
    }

    checksum_ = Checksum(abs_path);
    TiXmlHandle h_doc(&doc);
    TiXmlElement *rig = h_doc.FirstChildElement().ToElement();

    if( rig ) {
        std::string bone_path = rig->Attribute("bone_path");
        FindFilePath(bone_path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths);
        checksum_ += Checksum(abs_path);
        std::string model_path = rig->Attribute("model_path");
        FindFilePath(model_path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths);
        checksum_ += Checksum(abs_path);
        const char* mass_path_cstr = rig->Attribute("mass_path");
        SkeletonAssetRef mass_path_ref;
        if(mass_path_cstr){
            //SkeletonAssets::Instance()->ReturnRef(mass_path_cstr);
            mass_path_ref = Engine::Instance()->GetAssetManager()->LoadSync<SkeletonAsset>(mass_path_cstr);
            checksum_ += mass_path_ref->checksum();
        }

        int model_id = Models::Instance()->loadModel(model_path, _MDL_CENTER);
        const Model &model = Models::Instance()->GetModel(model_id);

        FindFilePath(bone_path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths);
        FILE *file = my_fopen(abs_path,"rb");
        if(!file) {
            sub_error = 1;
            return kLoadErrorMissingSubFile;
        }

        int version = 5;

        int temp_read = 0;
        fread(&temp_read, sizeof(int), 1, file);

        if(temp_read>=_min_skeleton_version){
            version = temp_read;
            fread(&temp_read, sizeof(int), 1, file);
        }

        data.rigging_stage = (RiggingStage)temp_read;

        if(data.rigging_stage == _animate){
            data.rigging_stage = _control_joints;
        }

        int num_points = 0;
        fread(&num_points, sizeof(int), 1, file);
        float point_center_float[3];
        vec3 point_center;
        for(int i=0; i<num_points; i++){
            fread(point_center_float,sizeof(float),3,file);
            point_center = vec3(point_center_float[0],
                                point_center_float[1],
                                point_center_float[2]);
            data.points.push_back(point_center);
        }

        if(version >= 8){
            data.point_parents.resize(data.points.size());
            for(unsigned i=0; i<data.points.size(); i++){
                fread(&data.point_parents[i], sizeof(int), 1, file);
            }
        }

        int num_bones = 0;
        fread(&num_bones, sizeof(int), 1, file);
        int f_bone_ends[2] = {0,0};
        for(int i=0; i<num_bones; i++){
            fread(&f_bone_ends, sizeof(int), 2, file);
            data.bone_ends.push_back(f_bone_ends[0]);
            data.bone_ends.push_back(f_bone_ends[1]);
            data.bone_mats.push_back(GetBoneMat(data.points[f_bone_ends[0]],
                                                data.points[f_bone_ends[1]]));
        }

        if(version >= 8){
            data.bone_parents.resize(num_bones);
            for(int i=0; i<num_bones; i++){
                fread(&data.bone_parents[i], sizeof(int), 1, file);
            }
        }

        if(version>=6){
            data.bone_mass.resize(num_bones);
            for(int i=0; i<num_bones; i++){
                fread(&data.bone_mass[i], sizeof(float), 1, file);
            }
            if(mass_path_ref.valid()){
                int shared_bones = std::min(data.bone_mass.size(), mass_path_ref->GetData().bone_mass.size());
                for(int i=0; i<shared_bones; ++i){
                    data.bone_mass[i] = mass_path_ref->GetData().bone_mass[i];
                }
            }
        }

        float com_float[3];
        data.bone_com.resize(num_bones);
        if(version>=7){
            for(int i=0; i<num_bones; i++){
                fread(&com_float, sizeof(float), 3, file);
                data.bone_com[i] = vec3(com_float[0],
                                        com_float[1],
                                        com_float[2]);
            }
        }  else {
            for(int i=0; i<num_bones; i++){
                data.bone_com[i] = (data.points[data.bone_ends[i*2+0]] +
                                    data.points[data.bone_ends[i*2+1]]) *
                                    0.5f;
            }
        }

        if(version>=9) {
            mat4 mat;
            for(int i=0; i<num_bones; i++){
                fread(&mat.entries[0], sizeof(float), 16, file);
            //    data.bone_mats.push_back(mat);
            }
        }

        if(data.rigging_stage == _control_joints){
            int num_read_verts;
            int precollapse_num_vertices = model.precollapse_num_vertices;
            if(version>=11){
                fread(&num_read_verts, sizeof(int), 1, file);
                if(num_read_verts != precollapse_num_vertices) {
                    sub_error = 4; 
                    return kLoadErrorCorruptFile;
                }
            }
            int model_num_vertices = model.vertices.size()/3;
            data.model_bone_weights.resize(model_num_vertices);
            data.model_bone_ids.resize(model_num_vertices);

            std::vector<vec4> temp_bone_weights(precollapse_num_vertices);
            std::vector<vec4> temp_bone_ids(precollapse_num_vertices);
            fread(&temp_bone_weights[0], sizeof(vec4), temp_bone_weights.size(), file);
            fread(&temp_bone_ids[0], sizeof(vec4), temp_bone_ids.size(), file);
            std::vector<vec4> temp_bone_weights2(model.precollapse_vert_reorder.size());
            std::vector<vec4> temp_bone_ids2(model.precollapse_vert_reorder.size());
            for(int i=0; i<model_num_vertices; ++i){
                temp_bone_weights2[i] = temp_bone_weights[model.precollapse_vert_reorder[i]];
                temp_bone_ids2[i] = temp_bone_ids[model.precollapse_vert_reorder[i]];
            }
            for(int i=0; i<model_num_vertices; ++i){
                data.model_bone_weights[i] = temp_bone_weights2[model.optimize_vert_reorder[i]];
                data.model_bone_ids[i] = temp_bone_ids2[model.optimize_vert_reorder[i]];
            }
            for(int i=0; i<model_num_vertices; ++i){
                const vec4 &w = data.model_bone_weights[i];
                if(w[0] + w[1] + w[2] + w[3] < 0.99f){
                    vec3 vert = vec3(model.vertices[i*3+0], model.vertices[i*3+1], model.vertices[i*3+2]);
                    int closest_bone = GetClosestBone(data, vert);
                    data.model_bone_weights[i][0] = 1.0f;
                    data.model_bone_ids[i][0] = (float)closest_bone;
                }
            }
            data.old_model_center = model.old_center;

            data.hier_parents.resize(num_bones);

            int bone_id;
            for(int i=0; i<num_bones; i++){
                fread(&bone_id, sizeof(int), 1, file);
                data.hier_parents[i] = bone_id;
            }

            int num_joints;
            fread(&num_joints, sizeof(int), 1, file);
            for(int i=0; i<num_joints; i++){
                JointData joint;
                fread(&joint.type, sizeof(int), 1, file);
                if(joint.type == _amotor_joint){
                    fread(&joint.stop_angle, sizeof(float), 6, file);
                    joint.stop_angle[4] *= -1.0f;
                    joint.stop_angle[5] *= -1.0f;
                    std::swap(joint.stop_angle[4], joint.stop_angle[5]);
                } else if(joint.type == _hinge_joint){
                    fread(&joint.stop_angle, sizeof(float), 2, file);
                }
                fread(&joint.bone_id, sizeof(int), 2, file);

                if(joint.type == _hinge_joint) {
                    fread(&joint.axis,sizeof(vec3),1,file);
                } 
                data.joints.push_back(joint);
            }
        }

        if(version>=10) {
            int num_ik_bones;
            fread(&num_ik_bones, sizeof(int), 1, file);
            for(int i=0; i<num_ik_bones; ++i){
                SimpleIKBone bone;
                fread(&bone.bone_id, sizeof(int), 1, file);
                fread(&bone.chain_length, sizeof(int), 1, file);
                int num_chars;
                fread(&num_chars, sizeof(int), 1, file);
                std::string the_string;
                the_string.resize(num_chars);
                fread(&the_string[0], sizeof(char), num_chars, file);
                data.simple_ik_bones[the_string] = bone;
            }
        }

        data.symmetry.resize(num_bones, -1);
        bool missing_symmetry = false;
        std::stringstream sstream;
        sstream << " Asymmetrical bones: ";
        for(int i=0; i<num_bones; ++i){
            vec3 points[2];
            points[0] = data.points[data.bone_ends[i*2+0]];
            points[1] = data.points[data.bone_ends[i*2+1]];
            for(int j=i; j<num_bones; ++j){
                vec3 reverse[2];    
                reverse[0] = data.points[data.bone_ends[j*2+0]];
                reverse[1] = data.points[data.bone_ends[j*2+1]];
                reverse[0][0] *= -1.0f;
                reverse[1][0] *= -1.0f;
                if(distance_squared(points[0], reverse[0]) < 0.0001f &&
                   distance_squared(points[1], reverse[1]) < 0.0001f)
                {
                    data.symmetry[i] = j;
                    data.symmetry[j] = i;
                }
            }
            if(data.symmetry[i] == -1){
                missing_symmetry = true;
                sstream << "bone " << i  << " at " << points[0] << ", " << points[1] << ", ";
            }
        }
        if(missing_symmetry){
            error_string = sstream.str();
            error_string = error_string.substr(0, error_string.size() - 2);
            sub_error = 3;
            fclose(file);
            return kLoadErrorCorruptFile;
        }

        fclose(file);
    } else {
        sub_error = 2;
        return kLoadErrorIncompleteXML;
    }

    return kLoadOk;
}

const char* SkeletonAsset::GetLoadErrorString() {
    switch(sub_error) {
        case 0: return "";
        case 1: return "Could not find bone_path file from attribute.";
        case 2: return "Missing root Rig node in xml file.";
        case 3: return "PHXBN skeleton is not symmetrical.";
        case 4: return "Skeleton file and model file have different vertex counts.";
        default: return "Undefined error.";
    } 
}

const char* SkeletonAsset::GetLoadErrorStringExtended() {
    return error_string.c_str();
}

void SkeletonAsset::Unload() {

}

void SkeletonAsset::Reload() {

}

void SkeletonAsset::ReportLoad()
{

}

SkeletonAsset::SkeletonAsset( AssetManager* owner, uint32_t asset_id ) : Asset(owner, asset_id), sub_error(0)
{

}

SkeletonAsset::~SkeletonAsset()
{

}

const SkeletonFileData& SkeletonAsset::GetData()
{
    return data;
}

AssetLoaderBase* SkeletonAsset::NewLoader() {
    return new FallbackAssetLoader<SkeletonAsset>();
}
