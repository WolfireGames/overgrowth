//-----------------------------------------------------------------------------
//           Name: fzx_file.cpp
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
#include "fzx_file.h"

#include <Memory/allocation.h>

#include <Asset/AssetLoader/fallbackassetloader.h>
#include <Asset/assetloaderrors.h>

#include <Internal/error.h>
#include <Internal/filesystem.h>
#include <Internal/error.h>

#include <Compat/filepath.h>
#include <Compat/fileio.h>

#include <Math/quaternions.h>
#include <Math/vec3.h>

#include <cstdio>
#include <cstdlib>

const int FZX_VERSION = 1;

FZXAsset::FZXAsset( AssetManager* owner, uint32_t asset_id ) : Asset( owner, asset_id ), sub_error(0) {
}

int FZXAsset::Load(const std::string &path, uint32_t load_flags) {
    sub_error = 0;
    char abs_path[kPathSize];
    if(FindFilePath(path.c_str(), abs_path, kPathSize, kDataPaths|kModPaths) == -1) {
        return kLoadErrorMissingFile;
    }
	FILE *file = my_fopen(abs_path, "rb");
	if(file == NULL){
        return kLoadErrorCouldNotOpen;
	}
	
	// obtain file size:
	fseek (file , 0 , SEEK_END);
	int bytes_in_file = ftell(file);
	rewind(file);

	// allocate memory to contain the whole file
	StackMem buffer_mem(alloc.stack.Alloc(bytes_in_file));
	unsigned char *buffer = (unsigned char*)buffer_mem.ptr();
	if (buffer == NULL) {
        FatalError("Error", "Failed to allocate RAM for FZX file");
    }

	// copy the file into the buffer:
	size_t bytes_read = fread(buffer,1,bytes_in_file, file);
	if (bytes_read != (size_t)bytes_in_file) {
        sub_error = 1;
        return kLoadErrorCouldNotRead;
    }

	// close file
	fclose (file);

    // Check file header
    const unsigned char header[] = {211,'F','Z','X','\r','\n',32,'\n'};
    const unsigned int header_len = sizeof(header);
    if(bytes_read < header_len){
        sub_error = 2;
        return kLoadErrorCorruptFile;
    }
    for(unsigned i=0; i<header_len; ++i){
        if(header[i] != buffer[i]){
            sub_error = 3;
            return kLoadErrorCorruptFile;
        }
    }
    // Check version
	int read_pos = header_len;
	const int version = *((int*)(&buffer[read_pos]));
	read_pos += sizeof(int);
    if(version != FZX_VERSION){
        sub_error = 4;
        return kLoadErrorIncompatibleFileVersion;
    }
    // Check file footer
    const unsigned char footer[] = {'F','Z','X'};
    const int footer_len = sizeof(footer);
    for(int i=0; i<footer_len; ++i){
        if(footer[i] != buffer[bytes_read-footer_len+i]){
            sub_error = 5;
            return kLoadErrorCorruptFile;
        }
    }
    // Check file size
    const int expected_file_size = *((int*)&buffer[bytes_read-footer_len-sizeof(int)]);
    if(expected_file_size != (int)(bytes_read-footer_len-sizeof(int))) {
        sub_error = 6;
        return kLoadErrorCorruptFile;
    }
    // Everything's good! Read the actual data
	int num_objects = *((int*)&buffer[read_pos]);
    objects.resize(num_objects);
	read_pos += sizeof(int);
	for(int i=0; i<num_objects; ++i){
        FZXObject &object = objects[i];
        // Read label string
		int str_len = *((int*)&buffer[read_pos]);
		read_pos += sizeof(int);
		const int BUF_SIZE = 256;
		char str_buf[BUF_SIZE];
		if(str_len > BUF_SIZE-1){
            sub_error = 7;
            return kLoadErrorCorruptFile;
        }
		for(int j=0; j<str_len; ++j){
			str_buf[j] = *((char*)(&buffer[read_pos+j]));
		}
		str_buf[str_len] = '\0';
        object.label = str_buf;
		read_pos += str_len;
		for(int j=0; j<4; ++j){
			object.rotation[j] = *((float*)(&buffer[read_pos]));
			read_pos += sizeof(float);
		}
		vec3 location;
		for(int j=0; j<3; ++j){
			object.location[j] = *((float*)(&buffer[read_pos]));
			read_pos += sizeof(float);
		}
		for(int j=0; j<3; ++j){
			object.scale[j] = *((float*)(&buffer[read_pos]));
			read_pos += sizeof(float);
		}
	}
    return kLoadOk;
}

const char* FZXAsset::GetLoadErrorString() {
    switch(sub_error) {
        case 0: return "";
        case 1: return "Failed to read FZX file into buffer";
        case 2: return "FZX file too small for header";
        case 3: return "FZX header is incorrect";
        case 4: return "FZX version incorrect";
        case 5: return "FZX footer incorrect";
        case 6: return "FZX file has unexpected file size";
        case 7: return "String is too long in FZX file";
        default: return "Undefined error";
    }
}

void FZXAsset::Unload() {

}

void FZXAsset::Reload() {

}

AssetLoaderBase* FZXAsset::NewLoader() {
    return new FallbackAssetLoader<FZXAsset>();
}
