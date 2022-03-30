//-----------------------------------------------------------------------------
//           Name: models.h
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
#pragma once

#include <Graphics/model.h>
#include <Graphics/drawbatch.h>

#include <vector>
#include <string>
#include <list>
#include <map>

struct ModelData {
    Model model;
    std::string name;
};

class Models {
    private:
        int next_id;
        typedef std::list<ModelData> ModelList;
        ModelList models;
        typedef std::map<std::string, int> NameMap;
        NameMap name_map;
        typedef std::map<int, ModelData*> IDMap;
        IDMap id_map;
        
        typedef std::pair<std::string, int> MorphIndex;
        typedef std::map<MorphIndex, int> MorphMap;
        MorphMap morph_map_;

    public:
            int AddModel();
            int getModelGroups(int which);
            void drawModelGroup(int which, int group);
            void drawModel(int which);
            void drawModelAltTexCoords(int which);
            float Radius(int which);
            int loadModel(const std::string& name, char flags = 0, const std::string& store_name = "");
            int loadFlippedModel(const std::string& name, char flags = 0);
            int CopyModel(int model_id);
            void Dispose();
            Model& GetModel(int id);
            ModelData& GetModelData( int id );

            Models()
            {}

            static Models* Instance()
            {
                static Models instance;
                return &instance;
            }
            int LoadModelAsMorph( const std::string& path, int model_id, const std::string &base_model_name );
            void DeleteModel(int id);
            int NumModels();
};

void SimplifyModel(std::string name, Model& model, int edge_target);
