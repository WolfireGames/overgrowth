//-----------------------------------------------------------------------------
//           Name: path_set.cpp
//      Developer: Wolfire Games LLC
//    Description: 
//        License: Read below
//-----------------------------------------------------------------------------
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
#include "path_set.h"

#include <Asset/Asset/image_sampler.h>
#include <Graphics/textures.h>
#include <Main/engine.h>

void PathSetUtil::GetCachedFiles( PathSet& path_set ) {
    PathSet new_path_set;
    for(PathSet::iterator iter = path_set.begin(); iter != path_set.end(); ++iter){
        const std::string &labeled_path = (*iter);
        size_t space_pos = labeled_path.find(' ');
        if(space_pos == std::string::npos){
            DisplayError("Error", ("No space found in labeled string: "+labeled_path).c_str());
            continue;
        }
        const std::string& type = labeled_path.substr(0, space_pos);
        const std::string& path = labeled_path.substr(space_pos+1, labeled_path.size() - (space_pos+1));
        switch(type[0]){
            case 'i': 
                if(type == "image_sample"){
                    ImageSamplerRef ref = Engine::Instance()->GetAssetManager()->LoadSync<ImageSampler>(path);   
                    if( ref.valid() ) {
                        std::string fixed;
                        if(ref->GetCachePath(&fixed)) {
                            new_path_set.insert("image_sample_cache "+fixed);
                        }
                    } else {
                        LOGE << "Failed loading " << path << std::endl;
                    }
                } 
                break;
            default:
                new_path_set.insert(labeled_path);
        }
    }
    path_set = new_path_set;
}
