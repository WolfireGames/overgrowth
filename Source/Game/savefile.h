//-----------------------------------------------------------------------------
//           Name: savefile.h
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
#pragma once

#include <Internal/integer.h>
#include <Internal/file_descriptor.h>
#include <Internal/modid.h>

#include <string>
#include <map>

struct uint128_holder {
    uint8_t val[16];
};

typedef std::map<std::string,std::vector<std::string> > ArrayDataMap;
typedef std::map<std::string, std::string> DataMap;
typedef std::map<std::string, uint32_t> Int32Map;

class SavedLevel {
public:
    void SetValue(const std::string &key, const std::string &val);
    const std::string& GetValue(const std::string &key);

    void SetArrayValue(const std::string &key, const int32_t index, const std::string &val);
    void DeleteArrayValue(const std::string &key, const int32_t index);
    void AppendArrayValueIfUnique(const std::string &key, const std::string& val);
    void AppendArrayValue(const std::string &key, const std::string& val);
    bool ArrayContainsValue(const std::string &key, const std::string& val);
    uint32_t GetArraySize(const std::string &key);
    std::string GetArrayValue( const std::string &key, const int32_t index);
    std::vector<std::string> GetArray(const std::string &key);

    void SetInt32Value(const std::string &key, const int32_t value);
    int32_t GetInt32Value(const std::string &key);
    bool HasInt32Value(const std::string &key);

    void SetKey(const std::string& campaign_id, const std::string& save_category, const std::string& save_name);
    bool SerializeToRAM( MemWriteFileDescriptor *file_ptr );

    void ClearData();

    const ArrayDataMap& GetArrayDataMap();
    const DataMap& GetDataMap();
    const Int32Map& GetIntMap();

    std::string campaign_id_;
    std::string save_category_;
    std::string save_name_;

private:

    DataMap data_;
    ArrayDataMap array_data_;
    Int32Map data_int32_; 

    uint128_holder md5_;
};

class SaveFile {
public:
    SavedLevel& GetSave(const std::string campaign_id, const std::string save_category, const std::string save_name);
    SavedLevel& GetSavedLevelDeprecated(const std::string& save_name);

    SavedLevel& GetSaveIndex(size_t index);
    size_t GetSaveCount();

    bool SaveExist(const std::string campaign_id, const std::string save_category, const std::string save_name);

    bool ReadFromFile(const std::string& filepath);
    bool LoadFromRAM( void* mem );
    void Clear();
    void SetWriteFile(const std::string& filepath);
    bool WriteToFile( const std::string& filepath );
    bool SerializeToRAM( std::vector<uint8_t> *mem_ptr );
    bool WriteInPlace();
    void QueueWriteInPlace();
    void ExecuteQueuedWrite();
    uint32_t GetLoadedVersion();

    static const char* kOvergrowthSaveMarker;
    static const uint16_t kCurrentVersion;
private:
    std::vector<SavedLevel> levels_;
    std::string filepath_;
    uint16_t loaded_version;
    bool queue_write_in_place_;
};

