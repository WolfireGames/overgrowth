//-----------------------------------------------------------------------------
//           Name: lipsyncfile.cpp
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

#include <Asset/Asset/lipsyncfile.h>
#include <Asset/AssetLoader/fallbackassetloader.h>

#include <Internal/filesystem.h>
#include <Internal/error.h>
#include <Internal/timer.h>

#include <Math/enginemath.h>
#include <Compat/fileio.h>
#include <Logging/logdata.h>
#include <Main/engine.h>

LipSyncFile::LipSyncFile(AssetManager* owner, uint32_t asset_id ) : Asset(owner,asset_id), sub_error(0) {

}

int LipSyncFile::Load( const std::string &path, uint32_t load_flags ) {
    sub_error = 0;
    char abs_path[kPathSize];
    if(FindFilePath(path.c_str(), abs_path, kPathSize, kDataPaths|kModPaths) != 0) {
        return kLoadErrorMissingFile;
    } 

    std::ifstream file;
    my_ifstream_open(file, abs_path);
    if(file.fail()){
        return kLoadErrorCouldNotOpen;
    }

    const std::map<std::string, int> &phn2vis = 
        Engine::Instance()->GetLipSyncSystem()->phn2vis[0];
    std::map<std::string, int>::const_iterator iter;
    LipSyncKey new_key;
    std::string line;
    while(!file.eof()){
        // Extract lines with format:
        // phn_vis start_time end_time num_keys phoneme weight...  
        file >> line;
        if(line == "phn_vis"){
            file >> new_key.time;
            new_key.time -= 50.0f; //"Disney trick" to make shape before sound
            //new_key.time *= 0.001f;
            new_key.time /= 970.0f;
            file.ignore(256,' ');
            file.ignore(256,' ');
            file >> line;
            int num_keys = atoi(line.c_str());
            new_key.keys.resize(num_keys);
            for(int i = 0; i < num_keys; ++i){
                file >> line;
                iter = phn2vis.find(line);
                if(iter == phn2vis.end()){
                    FatalError("Error",
                        "Could not find \"%d\" in phn2id", line.c_str());
                }
                new_key.keys[i].id = iter->second;
                file >> new_key.keys[i].weight;
            }
            keys.push_back(new_key);
        } else {
            file.ignore(256, '\n'); // Skip this line
        }
    }
    file.close();

    time_bound[0] = keys[0].time;
    time_bound[1] = keys[0].time;
    for(unsigned i=1; i<keys.size(); ++i){
        time_bound[0] = min(time_bound[0], keys[i].time);
        time_bound[1] = max(time_bound[1], keys[i].time);
    }
    path_ = path;
    return kLoadOk;
}

const char* LipSyncFile::GetLoadErrorString() {
    return "";
}

void LipSyncFile::Unload() {

}


void LipSyncFile::Reload() {
    Load(path_,0x0);
}

void LipSyncFile::GetWeights(float time, 
                             int &marker, 
                             std::vector<KeyWeight> &weights ) {
    while(keys[marker].time < time){
        ++marker;
    }
    weights = keys[marker].keys;
}

AssetLoaderBase* LipSyncFile::NewLoader() {
    return new FallbackAssetLoader<LipSyncFile>();
}

void LipSyncFileReader::Update(float timestep) {
    if(!valid()){
        return;
    }
    time += timestep;
    if(time > ls_ref->time_bound[1]){
        ls_ref.clear();
    }
}

void LipSyncFileReader::UpdateWeights() {
    if(!valid()){
        return;
    }
    ls_ref->GetWeights(time, marker, vis_weights);

    std::map<std::string, float>::iterator iter;
    for(iter = morph_weights.begin(); iter != morph_weights.end(); ++iter){
        iter->second = 0.0f;
    }

    for(auto & vis_weight : vis_weights){
        morph_weights[id2morph[vis_weight.id]] += vis_weight.weight;    
    }
}

bool LipSyncFileReader::valid() {
    return ls_ref.valid();
}

void LipSyncFileReader::AttachTo( LipSyncFileRef& _ls_ref ) {
    ls_ref = _ls_ref;
    time = ls_ref->time_bound[0];
    marker = 0;
}

void LipSyncFileReader::SetVisemeMorphs(const std::map<std::string, std::string> &phn2morph )
{
    std::string phn;
    std::string morph;
    std::map<std::string, int> phn2vis = Engine::Instance()->GetLipSyncSystem()->phn2vis[0];
    std::map<std::string, std::string>::const_iterator iter;
    for(iter = phn2morph.begin(); iter != phn2morph.end(); ++iter){
        phn = iter->first;
        morph = iter->second;
        int id = phn2vis[phn];
        id2morph[id] = morph;
        morph_weights[morph] = 0.0f;
    }
}

std::map<std::string, float>& LipSyncFileReader::GetMorphWeights() {
    return morph_weights;
}

