//-----------------------------------------------------------------------------
//           Name: activemodsparser.h
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
#pragma once
#include <XML/Parsers/xmlparserbase.h>
#include <Internal/modid.h>

#include <map>
#include <vector>
#include <string>
#include <iostream>

class TiXmlDocument;

class ActiveModsParser : public XMLParserBase 
{
private:
    uint16_t load_checksum;
    uint16_t save_checksum;

    bool SerializeInto( TiXmlDocument* doc );  

    uint16_t LocalChecksum();
    
public:
    ActiveModsParser();

    uint32_t Load( const std::string& path ) override;
    bool Save( const std::string& path ) override;

    void Clear() override;

    bool FileChangedSinceLastLoad();
    bool LocalChangedSinceLastSave();

    class ModInstance
    {
    public:
        ModInstance();
        ModInstance(const char* id,ModSource modsource,bool activated, const char* version);
        
        char id[MOD_ID_MAX_LENGTH];
        ModSource modsource; 
        bool activated;        
        char version[MOD_VERSION_MAX_LENGTH];
    };

    std::vector<ModInstance> mod_instances;

    void SetModInstanceActive(const char* id, ModSource modsource, bool activated, const char* version);
    bool HasModInstance(const char* id, ModSource modsource);
    ModInstance GetModInstance(const char* id, ModSource modsource);
    void RemoveModInstance(const char* id, ModSource modsource);
};
