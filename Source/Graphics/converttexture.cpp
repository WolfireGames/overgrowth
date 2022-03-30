//-----------------------------------------------------------------------------
//           Name: converttexture.cpp
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
#include "converttexture.h"

#include <Internal/common.h>
#include <Internal/checksum.h>
#include <Internal/filesystem.h>
#include <Internal/datemodified.h>
#include <Internal/error.h>

#include <Compat/fileio.h>
#include <Compat/compat.h>

#include <Images/freeimage_wrapper.h>
#include <Logging/logdata.h>

#include <cstring>
#include <sstream>
#include <string>

using std::string;
using std::endl;
using std::ostringstream;

extern const int overgrowth_dds_cache_version;
extern const char* overgrowth_dds_cache_intro;

bool ConvertImage( string src, string dst, string temp_conversion_path, TextureData::ConversionQuality quality ) {
    LOGI << "Converting " << src << " to " << dst << " by way of " << temp_conversion_path << endl;

    bool dds_suffix = false;
    int end = dst.length() - 1;
    if (end >= 3 &&
        dst[end - 3] == '.' &&
        dst[end - 2] == 'd' &&
        dst[end - 1] == 'd' &&
        dst[end - 0] == 's')
    {
        dds_suffix = true;
    }

    TextureData textureData;
    if (!textureData.Load(src.c_str())) {
        LOGE << "Failed loading texture " << src << endl;
        return false;
    }

    if (!textureData.GenerateMipmaps()) {
        LOGE << "Failed generating mipmaps for texture " << src << endl;
        return false;
    }

    //const char* hint;
	//hint = strrchr( src.c_str(), '_');
	#ifdef _WIN32
	ShortenWindowsPath(src);
	ShortenWindowsPath(dst);
	ShortenWindowsPath(temp_conversion_path);
	#endif

    CreateParentDirs(temp_conversion_path);

    bool successful_conversion = false;

    if (dds_suffix) {
        if (!textureData.ConvertDXT(crnlib::PIXEL_FMT_DXT5, quality)) {
            LOGE << "Failed converting texture " << src << endl;
            return false;
        }
        successful_conversion = textureData.SaveDDS(temp_conversion_path.c_str());
    } else {
        successful_conversion = textureData.SaveCRN(temp_conversion_path.c_str(), cCRNFmtDXT5, quality);
    }

    if( successful_conversion ) {
        unsigned short checksum = Checksum(src);
        FILE *file = my_fopen(temp_conversion_path.c_str(), "a");
        if(file){
            fwrite(overgrowth_dds_cache_intro, strlen(overgrowth_dds_cache_intro), 1, file);
            fwrite(&overgrowth_dds_cache_version, sizeof(overgrowth_dds_cache_version), 1, file);
            fwrite(&checksum, sizeof(checksum), 1, file);
            fclose(file);
        }

        if(movefile(temp_conversion_path.c_str(), dst.c_str())){
            LOGI << "Removing existing dst file" << endl;
            deletefile(dst.c_str());
            CreateParentDirs(dst);
            if(movefile(temp_conversion_path.c_str(), dst.c_str())){
                #ifndef NO_ERR
                    DisplayError("Error",
                        (string("Problem moving file \"") + strerror(errno) + "\". From " + 
                        temp_conversion_path +
                        " to " + dst).c_str());
                #endif
                return false;
            }
        }
        LOGI << "Conversion completed" << endl;
        //Textures::Instance()->NotifyConvertedTexture(src);
        return true;
        //return textureData;
    } else {
        LOGE << "Conversion failed, unable to write texture to " << temp_conversion_path << endl;
        return false;    
    }
}

string GetTempDDSPath(string dst, bool write_path, int concurrent_id){
    string temp_conversion_path;
    if(write_path) {
        ostringstream oss;
        oss << "Data/Temp/conversion" << concurrent_id << ".dds";
        temp_conversion_path = oss.str();
    } else {
        temp_conversion_path = dst+"_tmp";
    }
    if(write_path){
        char path[kPathSize];
        FormatString(path, kPathSize, "%s%s", GetWritePath(CoreGameModID).c_str(), temp_conversion_path.c_str());
        temp_conversion_path = path;
    }
    return temp_conversion_path;
}
