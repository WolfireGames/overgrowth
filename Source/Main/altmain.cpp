//-----------------------------------------------------------------------------
//           Name: altmain.cpp
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
#include "altmain.h"

#include <Internal/levelxml.h>
#include <Internal/zip_util.h>
#include <Internal/casecorrectpath.h>
#include <Internal/textfile.h>
#include <Internal/snprintf.h>

#include <Compat/processpool.h>
#include <Compat/fileio.h>
#include <Compat/filepath.h>
#include <Compat/platformsetup.h>

#include <Graphics/shaders.h>
#include <Graphics/textures.h>
#include <Graphics/graphics.h>
#include <Graphics/text.h>
#include <Graphics/font_renderer.h>

#include <Asset/Asset/soundgroup.h>
#include <Asset/Asset/fzx_file.h>
#include <Asset/Asset/animation.h>

#include <Sound/sound.h>
#include <Editors/actors_editor.h>
#include <UserInput/mouse.h>
#include <Images/texture_data.h>
#include <Logging/logdata.h>
#include <Main/engine.h>
#include <Game/EntityDescription.h>

#include <SDL.h>
#include <utf8/utf8.h>
#include <tinyxml.h>

void CompressPathSet(PathSet path_set, const std::string &base_folder, const std::string &dst){
    bool first_path = true;
    for(const auto & full_path : path_set){
        const char* truncated_path = &full_path[base_folder.length()];
        Zip(full_path, dst, truncated_path, first_path?_YES_OVERWRITE:_APPEND_OVERWRITE);
        first_path = false;
    }
}

const char* GetSuffix(const char* path){
    const char* current = &path[0];
    while(*current != '\0'){
        if(*current == '.'){ 
            return current+1;
        }
        ++current;
    }
    FatalError("Error", "Path has no suffix!");
    return current;
}

void LoadSoundGroup(const char* path, const char* data) {
    //Disabled function as there is currently no support for loading data from within zip files, if the feature 
    //should exists in the future, it should be part of the asset manager. /Max Danielsson 2016 September 15
    assert(false); 
    /*
    LOGI << "Loading SoundGroupInfo \"" <<  path << "\"" << std::endl;
    SoundGroupInfo sgi;
    sgi.ParseXML(data); 
    */
}

void LoadSound(const char* path, const char* data){
    LOGI << "Loading Sound file \"" << path << "\"" << std::endl;
    SoundPlayInfo spi;
    spi.path = std::string("Data/Sounds/weapon_foley/cut/")+path;
    Sound sound("");
    unsigned long handle = sound.CreateHandle();
    sound.Play(handle,spi);
    SDL_Delay(1000);
}

void LoadSoundGroupZipFile(const std::string& path){
    ExpandedZipFile ezf;
    UnZip(path, ezf);
    
    unsigned num_entries = ezf.GetNumEntries();
    for(unsigned i=0; i<num_entries; ++i){
        const char* filename;
        const char* data; 
        unsigned size;
        ezf.GetEntry(i, filename, data, size);

        const char* suffix = GetSuffix(filename);
        switch(suffix[0]){
            case 'x': //xml
                LoadSoundGroup(filename, data);
                break;
            case 'w': //wav
                LoadSound(filename, data);
                break;
        }
    } 
}

void CheckCase(const char* path){
    std::string corrected;
    if(IsPathCaseCorrect(path, &corrected)){
        LOGI << "Case is correct: " << corrected << std::endl;
    } else {
        LOGI << "Case is not correct: " <<  path << std::endl;
        LOGI << "The correct case is: " << corrected << std::endl;
    }
}

static const int FZX_VERSION_ = 1;

int TestMain( int argc, char* argv[], const char* overloaded_write_dir, const char* overloaded_working_dir) {
    SetUpEnvironment(argv[0],overloaded_write_dir, overloaded_working_dir);

    //FZXAssetRef ref = FZXAssets::Instance()->ReturnRef("Data/Models/Characters/IGF_Guard/guard_physics.fzx");
    FZXAssetRef ref = Engine::Instance()->GetAssetManager()->LoadSync<FZXAsset>("Data/Models/Characters/IGF_Guard/guard_physics.fzx");
    LOGI << "Objects: " << ref->objects.size() << std::endl;
	const char* key_words[] = {"left", "capsule", "box", "sphere"};
	const int num_key_words = sizeof(key_words)/sizeof(key_words[0]);
	for(auto & object : ref->objects){
        // Parse label string
        const std::string &label = object.label;      
        size_t last_space_index = 0;
        for(size_t j=0, len=label.size()+1; j<len; ++j){      
			if(label[j] == ' ' || label[j] == '\0'){
                std::string sub_str_buf;
                sub_str_buf.resize(j-last_space_index);
                int count = 0;
				for(size_t k=last_space_index; k<j; ++k){
					char c = label[k];
					if(c >= 'A' && c <= 'Z'){
						c -= ('A' - 'a');
					}
                    sub_str_buf[count] = c;
                    ++count;
				}
                bool match = false;
                for(auto & key_word : key_words){
                    bool word_match = (strcmp(key_word, sub_str_buf.c_str())==0);
                    if(word_match){
                        LOGI << "Token found: " << key_word << std::endl;
                        match = true;
                    }
                }
                if(!match){
                    LOGE << "Unrecognized token: " << sub_str_buf << std::endl;
                }
                last_space_index = j+1;
			}
        }
        LOGI << "Object name: " << label << std::endl;
        const quaternion &quat = object.rotation;
        LOGI << "Quaternion: " << quat << std::endl;
        const vec3 &location = object.location;
        LOGI << "Location: " << location << std::endl;
        const vec3 &scale = object.scale;
        LOGI << "Scale: " << scale << std::endl;
	}
	
    printf("Press enter to exit\n");
    getchar();

    DisposeEnvironment();

    return 0;
}


#include "Graphics/converttexture.h"

int DDSConvertMain( int argc, char* argv[], const char* overloaded_write_dir, const char* overloaded_working_dir ) {
    SetUpEnvironment(argv[0],overloaded_write_dir,overloaded_working_dir);

    printf("Running DDS converter...\n");

    for(int i=2; i<argc; ++i){
        printf("Loading path: %s\n", argv[i]);
        std::string src = argv[i];
        std::string dst = src + "_converted.dds";
        std::string temp = GetTempDDSPath(dst, true, 0);
        ConvertImage(src, dst, temp, TextureData::Nice);
        printf("Converted.\n");
    } 
    if(argc <= 2) {
        printf("No path found.\n");
    }
    printf("Press any key to exit...\n");
    getchar();

    DisposeEnvironment();

    return 0;
}
