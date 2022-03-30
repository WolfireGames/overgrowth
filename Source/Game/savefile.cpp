//-----------------------------------------------------------------------------
//           Name: savefile.cpp
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
#include "savefile.h"

#include <Internal/integer.h>
#include <Internal/modloading.h>
#include <Internal/error.h>

#include <Memory/allocation.h>
#include <Game/savefile.h>
#include <Compat/fileio.h>
#include <Utility/assert.h>
#include <Logging/logdata.h>

#include <cstdlib>
#include <cstring>

const char* SaveFile::kOvergrowthSaveMarker = "Overgrowth Save";
const uint16_t SaveFile::kCurrentVersion = 3;

namespace {
    // Read a 128-bit MD5 value into uint128_holder
    // Returns true on success, false on error
    bool ReadMD5(FileDescriptor *fd, uint128_holder* val){
        return fd->ReadBytes(val, sizeof(*val));
    }

    // Write a 128-bit MD5 value from uint128_holder
    // Returns true on success, false on error
    bool WriteMD5(FileDescriptor *fd, uint128_holder* val){
        return fd->WriteBytes(val, sizeof(*val));
    }

    // Reads an std::string from a file, with spec: 
    // uint16 num_chars
    // char[num_chars] str
    // Returns true on success, false on error
    bool ReadString(FileDescriptor *fd, std::string* val){
        uint16_t num_chars;
        if(!fd->ReadBytes(&num_chars, sizeof(num_chars))){
            return false;
        }
        val->resize(num_chars);
        return fd->ReadBytes(&(*val)[0], num_chars);
    }
    
    // Writes an std::string to a file, with spec: 
    // Returns true on success, false on error
    bool WriteString(FileDescriptor *fd, const std::string* val){
        uint16_t num_chars = val->length();
        if(!fd->WriteBytes(&num_chars, sizeof(num_chars))){
            return false;
        }
        return fd->WriteBytes(&(*val)[0], num_chars);
    }

    // Load file 'path' into head at pointer 'mem' 
    // Allocates memory, mem must be freed by caller if successful
    // Returns true on success, false on error
    bool LoadFileIntoRAM(void* &mem, const std::string &abs_path){
        DiskFileDescriptor file;
        if(!file.Open(abs_path, "rb")){
            return false;
        }
        int file_size = file.GetSize();

        mem = OG_MALLOC(file_size);
        if(!mem){
            FatalError("Error", "Could not allocate memory for file: %s", abs_path.c_str());
        }
        if(!file.ReadBytes(mem, file_size)){
            OG_FREE(mem);
            return false;
        }
        file.Close();
        return true;
    }
    std::string GetTemporaryPath( const std::string &filepath_ )  {
        return filepath_+"_temp";
    }
}

SavedLevel& SaveFile::GetSavedLevelDeprecated( const std::string& name ) {
    for( size_t i = 0; i < levels_.size(); i++ ) {
        if( levels_[i].campaign_id_ == "" &&
            levels_[i].save_category_ == "" &&
            levels_[i].save_name_ == name ) {
                return levels_[i];
        }
    }

    SavedLevel new_level;
    new_level.save_name_ = name;

    levels_.push_back(new_level);

    return levels_[levels_.size()-1];
}

SavedLevel& SaveFile::GetSave(const std::string campaign_id, const std::string save_category, const std::string level_name) {
    for( size_t i = 0; i < levels_.size(); i++ ) {
        if( levels_[i].campaign_id_ == campaign_id &&
            levels_[i].save_category_ == save_category &&
            levels_[i].save_name_ == level_name ) {
                return levels_[i];
        }
    }

    SavedLevel new_level;
    new_level.campaign_id_ = campaign_id;
    new_level.save_category_ = save_category;
    new_level.save_name_ = level_name;

    levels_.push_back(new_level);

    return levels_[levels_.size()-1];
}

SavedLevel& SaveFile::GetSaveIndex(size_t index) {
    return levels_[index];
}

size_t SaveFile::GetSaveCount() {
    return levels_.size();
}

bool SaveFile::SaveExist(const std::string campaign_id, const std::string save_category, const std::string level_name) {
    for( size_t i = 0; i < levels_.size(); i++ ) {
        if( levels_[i].campaign_id_ == campaign_id &&
            levels_[i].save_category_ == save_category &&
            levels_[i].save_name_ == level_name ) {
                return true;
        }
    }
    return false;
}

void SaveFile::Clear() {
    levels_.clear();
    loaded_version = 0;
}

bool SaveFile::LoadFromRAM( void* mem ) {
    Clear();
    MemReadFileDescriptor file(mem);
    {
        std::string marker;
        marker.resize(strlen(kOvergrowthSaveMarker));
        if(!file.ReadBytes(&marker[0], strlen(kOvergrowthSaveMarker)) ||
           marker != kOvergrowthSaveMarker)
        {
            LOGE << "Invalid save file marker, unlikely this is an overgrowth save file." << std::endl;
            return false;
        }
    }

    if(!file.ReadBytes(&loaded_version, sizeof(loaded_version))){
        LOGE << "Failed at reading loaded_version" << std::endl;
        return false;
    }

    if( loaded_version == 1 ) {
        LOGE << "Version 1 is not supported" << std::endl;
        return false;
    }

    if( loaded_version == 2 ) {
        LOGE << "Version 2 is not supported" << std::endl;
        return false;
    }

    if( loaded_version > 3 ) {
        LOGE << "Failed at reading save file, too new fileformat" << std::endl;
        return false;
    } 

    uint16_t num_levels;
    if(!file.ReadBytes(&num_levels, sizeof(num_levels))){
        LOGE << "Failed at reading num of levels" << std::endl;
        return false;
    }
    std::string key, val;
    for(uint16_t i=0; i<num_levels; ++i){
        SavedLevel level;

        if(!ReadString(&file, &level.campaign_id_)){
            LOGE << "Failed at reading campaign id" << std::endl;
            return false;
        }

        if(!ReadString(&file, &level.save_category_)){
            LOGE << "Failed at reading save category" << std::endl;
            return false;
        }

        if(!ReadString(&file, &level.save_name_)){
            LOGE << "Failed at reading level name" << std::endl;
            return false;
        }

        uint128_holder md5_val;
        if(!ReadMD5(&file, &md5_val)){
            LOGE << "Failed at reading md5" << std::endl;
            return false;
        }

        uint16_t num_saved_vals;
        if(!file.ReadBytes(&num_saved_vals, sizeof(num_saved_vals))){
            LOGE << "Failed at reading num_saved_vals" << std::endl;
            return false;
        }

        for(uint16_t j=0; j<num_saved_vals; ++j){
            if(!ReadString(&file, &key)){
                LOGE << "Failed at reading value key" << std::endl;
                return false;
            }
            if(!ReadString(&file, &val)){
                LOGE << "Failed at reading value" << std::endl;
                return false;
            }
            level.SetValue(key, val);

        }

        uint16_t num_saved_arr_vals;
        if(!file.ReadBytes(&num_saved_arr_vals, sizeof(num_saved_arr_vals))){
            LOGE << "Failed at reading num array values" << std::endl;
            return false;
        }

        for(uint16_t j=0; j<num_saved_arr_vals; ++j) {
            if(!ReadString(&file, &key)){
                LOGE << "Failed at reading num array value key" << std::endl;
                return false;
            }
            
            uint16_t num_saved_sub_arr_vals;
            if(!file.ReadBytes(&num_saved_sub_arr_vals, sizeof(num_saved_sub_arr_vals))){
                LOGE << "Failed at reading num sub array value count" << std::endl;
                return false;
            }

            for(uint16_t j=0; j<num_saved_sub_arr_vals; ++j) {
                if(!ReadString(&file, &val)){
                    LOGE << "Failed at reading num sub array value" << std::endl;
                    return false;
                }

                level.AppendArrayValue(key, val);
            }
        }

        uint16_t num_saved_int_vals;
        if(!file.ReadBytes(&num_saved_int_vals, sizeof(num_saved_int_vals))){
            LOGE << "Failed at reading int32 value count" << std::endl;
            return false;
        }

        for(uint16_t j=0; j<num_saved_int_vals; ++j){
            if(!ReadString(&file, &key)){
                LOGE << "Failed at reading int32 value key" << std::endl;
                return false;
            }

            int32_t val;
            if(!file.ReadBytes(&val,sizeof(val))) {
                LOGE << "Failed at reading int32 value" << std::endl;
                return false;
            }

            level.SetInt32Value(key, val);
        }
        

        levels_.push_back(level);
    }

    return true;
}

bool SaveFile::SerializeToRAM( std::vector<uint8_t> *mem_ptr ) {
    std::vector<uint8_t> &mem = *mem_ptr;
    MemWriteFileDescriptor file(mem);
    if(!file.WriteBytes(kOvergrowthSaveMarker, strlen(kOvergrowthSaveMarker))){
        return false;
    }
    if(!file.WriteBytes(&kCurrentVersion, sizeof(kCurrentVersion))){
        return false;
    }
    uint16_t num_levels = levels_.size();
    if(!file.WriteBytes(&num_levels, sizeof(num_levels))){
        return false;
    }
    for(std::vector<SavedLevel>::iterator it = levels_.begin(); it != levels_.end(); ++it){
        if(!WriteString(&file, &it->campaign_id_)){
            return false;
        }
        if(!WriteString(&file, &it->save_category_)){
            return false;
        }
        if(!WriteString(&file, &it->save_name_)){
            return false;
        }
        if(!it->SerializeToRAM(&file)){
            return false;
        }
    }
    return true;
}

bool SaveFile::ReadFromFile( const std::string& abs_path ) {
    filepath_ = abs_path;
    void *file_mem;
    if(!LoadFileIntoRAM(file_mem, abs_path)){
        if(!LoadFileIntoRAM(file_mem, GetTemporaryPath(abs_path))){
            return false;
        }
    }
    if(!LoadFromRAM(file_mem)){
        OG_FREE(file_mem);
        return false;
    }
    OG_FREE(file_mem);
    return true;
}

void SaveFile::SetWriteFile(const std::string& filepath) {
   filepath_ = filepath; 
}

bool SaveFile::WriteToFile( const std::string& filepath ) {
    std::vector<uint8_t> mem;
    if(!SerializeToRAM(&mem)){
        return false;
    }
    DiskFileDescriptor file;
    file.Open(filepath, "wb");
    file.WriteBytes(&mem[0], mem.size());
    file.Close();
    return true;
}

bool SaveFile::WriteInPlace() {
    LOGI << "Saving data file to: " << filepath_ << std::endl;
    std::string temp_path = GetTemporaryPath(filepath_);
    if(!WriteToFile(temp_path)){
        return false;
    }
    if(movefile(temp_path.c_str(), filepath_.c_str())){
        deletefile(filepath_.c_str());
        if(movefile(temp_path.c_str(), filepath_.c_str())){
            return false;
        }
    }
    return true;
}

void SaveFile::QueueWriteInPlace() {
    queue_write_in_place_ = true;
}

void SaveFile::ExecuteQueuedWrite() {
    if( queue_write_in_place_ ) {
        WriteInPlace();
        queue_write_in_place_ = false;
    }
}

uint32_t SaveFile::GetLoadedVersion() {
    return loaded_version;
}

const std::string& SavedLevel::GetValue( const std::string &key ) {
    return data_[key];
}

void SavedLevel::SetValue( const std::string &key, const std::string &val ) {
    data_[key] = val;
}

void SavedLevel::SetArrayValue(const std::string &key, const int32_t index, const std::string &val) {
    std::vector<std::string> &v = array_data_[key];

    LOG_ASSERT(index >= 0 && index <= 32000);

    if( (size_t)index < v.size() ) {
        v[index] = val;
    } else {
        while( v.size() < (size_t)index )  {
            LOGW << "Told to set value \"" << val << "\" on index " << index << " in " << key << " but currently only have " << v.size() << " elements, padding with an empty string" << std::endl; 
            v.push_back("");
        }
        v.push_back(val);
    }
}

void SavedLevel::DeleteArrayValue(const std::string &key, const int32_t index) {
    bool shift = false;
    std::vector<std::string> &v = array_data_[key];

    for( size_t i = 0; i < v.size(); i++ ) {
        if( shift ) {
            v[i-1] = v[i];
        } else if( i == (size_t)index ) {
            shift = true; 
        } 
    }
}

void SavedLevel::AppendArrayValueIfUnique(const std::string &key, const std::string& val) {
    if( ArrayContainsValue(key,val) == false ) {
        AppendArrayValue(key,val);
    }
}

void SavedLevel::AppendArrayValue(const std::string &key, const std::string& val) {
    std::vector<std::string> &v = array_data_[key];
    v.push_back(val);
}

bool SavedLevel::ArrayContainsValue(const std::string &key, const std::string& val) {
    std::vector<std::string> &v = array_data_[key];
    for( size_t i = 0; i < v.size(); i++ ) {
        if(v[i] == val){
            return true; 
        }
    }
    return false;
}

uint32_t SavedLevel::GetArraySize(const std::string &key) {
    std::vector<std::string> &v = array_data_[key];
    return v.size();
}

std::string SavedLevel::GetArrayValue( const std::string &key, const int32_t index) {
    std::vector<std::string> &v = array_data_[key];
    if( (size_t)index < v.size() ) {
        return v[index];
    } else {
        return std::string();
    }
}

std::vector<std::string> SavedLevel::GetArray( const std::string &key) {
    return array_data_[key];
}

void SavedLevel::SetInt32Value(const std::string &key, const int32_t value) {
    data_int32_[key] = value;
}

int32_t SavedLevel::GetInt32Value(const std::string &key) {
    return data_int32_[key];
}

bool SavedLevel::HasInt32Value(const std::string &key) {
    return (data_int32_.find(key) != data_int32_.end());
}

void SavedLevel::SetKey(const std::string& campaign_id, const std::string& save_category, const std::string& level_name) {
    this->campaign_id_ = campaign_id;
    this->save_category_ = save_category;
    this->save_name_ = level_name;
}

bool SavedLevel::SerializeToRAM( MemWriteFileDescriptor *file_ptr ) {
    MemWriteFileDescriptor &file = *file_ptr;
    if(!WriteMD5(&file, &md5_)){
        return false;
    }

    {
        uint16_t num_saved_vals = data_.size();
        if(!file.WriteBytes(&num_saved_vals, sizeof(num_saved_vals))){
            return false;
        }
        for(DataMap::iterator it = data_.begin(); it != data_.end(); ++it){
            if(!WriteString(&file, &it->first)){
                return false;
            }
            if(!WriteString(&file, &it->second)){
                return false;
            }
        }
    }

    {
        uint16_t num_saved_array_vals = array_data_.size();
        if(!file.WriteBytes(&num_saved_array_vals, sizeof(num_saved_array_vals))){
            return false;
        }
        for(ArrayDataMap::iterator it = array_data_.begin(); it != array_data_.end(); it++) {
            if(!WriteString(&file, &it->first)){
                return false;
            }

            uint16_t num_saved_array_sub_vals = it->second.size();
            if(!file.WriteBytes(&num_saved_array_sub_vals, sizeof(num_saved_array_sub_vals))){
                return false;
            }

            for(std::vector<std::string>::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++) {
                if(!WriteString(&file, &(*it2))){
                    return false; 
                } 
            }
        }
    }

    {
        uint16_t num_saved_int32_vals = data_int32_.size();
        if(!file.WriteBytes(&num_saved_int32_vals, sizeof(num_saved_int32_vals))){
            return false;
        }
        for(Int32Map::iterator it = data_int32_.begin(); it != data_int32_.end(); ++it){
            if(!WriteString(&file, &it->first)){
                return false;
            }
            int32_t val = it->second;
            if(!file.WriteBytes(&val, sizeof(val))){
                return false;
            }
        }
    }
    return true;
}

void SavedLevel::ClearData() {
    data_.clear();
    array_data_.clear();
    data_int32_.clear();
}

const ArrayDataMap& SavedLevel::GetArrayDataMap() {
    return array_data_;
}

const DataMap& SavedLevel::GetDataMap() {
    return data_;
}

const Int32Map& SavedLevel::GetIntMap() { 
    return data_int32_;
}
