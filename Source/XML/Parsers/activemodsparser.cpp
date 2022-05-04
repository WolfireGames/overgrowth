//-----------------------------------------------------------------------------
//           Name: activemodsparser.cpp
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
#include "activemodsparser.h"

#include <Internal/checksum.h>
#include <Internal/filesystem.h>

#include <Utility/commonregex.h>
#include <Utility/strings.h>

#include <XML/xml_helper.h>
#include <Logging/logdata.h>

#include <tinyxml.h>

ActiveModsParser::ActiveModsParser() {

}

uint32_t ActiveModsParser::Load( const std::string& path ) {
    load_checksum = Checksum(path);

    Clear();
    TiXmlDocument doc( path.c_str() );
    doc.LoadFile();
    if( !doc.Error() ) {
        TiXmlElement* pRoot = doc.RootElement();
        if( pRoot ) {
            TiXmlElement* eModInstance = pRoot->FirstChildElement("ModInstance");
            bool parse_error;
            while( eModInstance ) {
                ModInstance mi; 
                int err;
                parse_error = false;

                err = strscpy(mi.id, eModInstance->Attribute("id"), MOD_ID_MAX_LENGTH);
                if( err == SOURCE_TOO_LONG ) {
                    parse_error = true;
                } else if( err == SOURCE_IS_NULL ) {
                    parse_error = true;
                }

                int saystrue = saysTrue(eModInstance->Attribute("activated"));
                if( saystrue == 1 ) {
                    mi.activated = true;
                } else if( saystrue == 0 ) {
                    mi.activated = false;
                } else if( saystrue == SAYS_TRUE_NULL_INPUT) {
                    parse_error = true;
                } else if( saystrue == SAYS_TRUE_NO_MATCH) {
                    parse_error = true;
                } else {
                    parse_error = true;
                }

                const char* modsource = eModInstance->Attribute("modsource");
                if( modsource ) {
                    if( strmtch(modsource,"steamworks")) {
                        mi.modsource = ModSourceSteamworks;
                    } else if( strmtch(modsource,"local")) {
                        mi.modsource = ModSourceLocalModFolder;
                    } else {
                        LOGE << "Unknown modsource " << modsource << " on " << mi.id << std::endl;
                        parse_error = true;
                    }
                } else {
                    LOGE << "Missing modsource attribute" << std::endl;
                    parse_error = true;
                }

                err = strscpy(mi.version, eModInstance->Attribute("version"), MOD_VERSION_MAX_LENGTH);
                if( err == SOURCE_TOO_LONG ) {
                    parse_error = true;
                } else if( err == SOURCE_IS_NULL ) {
                    parse_error = true;
                }

                if( parse_error == false ) {
                    mod_instances.push_back(mi); 
                }

                eModInstance = eModInstance->NextSiblingElement();
            }
        }
    }
    return 1;
}

bool ActiveModsParser::SerializeInto( TiXmlDocument* doc ) {
    TiXmlDeclaration * decl = new TiXmlDeclaration( "2.0", "", "" );
    TiXmlElement * root = new TiXmlElement("ActiveMods");
    for(auto & mod_instance : mod_instances) {
        TiXmlElement * mi = new TiXmlElement("ModInstance");
        mi->SetAttribute( "id", mod_instance.id );
        mi->SetAttribute( "activated", mod_instance.activated ? "true" : "false" );
        const char* modsource = "";
        switch(mod_instance.modsource) {
            case ModSourceLocalModFolder:
                modsource = "local";
                break;
            case ModSourceSteamworks:
                modsource = "steamworks";
                break;
            case ModSourceUnknown:
                LOGE << "Serializing mod with modsource ModSourceUnknown" << std::endl;
                modsource = "unknown";
                break;
        }
        mi->SetAttribute("modsource", modsource); 
        mi->SetAttribute("version", mod_instance.version);
        root->LinkEndChild(mi);
    }
    doc->LinkEndChild(decl);
    doc->LinkEndChild(root);
    return !doc->Error();
}

uint16_t ActiveModsParser::LocalChecksum() 
{
    std::string path = AssemblePath(GetWritePath(CoreGameModID).c_str(),"Data/Temp/activemodsparser.temp.xml");
    TiXmlDocument doc;
    
    SerializeInto(&doc);

    doc.SaveFile(path.c_str());

    return Checksum(path);
}

bool ActiveModsParser::Save( const std::string& path ) {
    TiXmlDocument doc;

    SerializeInto(&doc);

    doc.SaveFile(path.c_str());
    save_checksum = Checksum(path);

    return !doc.Error();
}

void ActiveModsParser::SetModInstanceActive(const char* id, ModSource modsource, bool activated, const char* version) {
    ModInstance mi = ModInstance(id,modsource,activated,version);
    int found = -1;
    for( unsigned i = 0; i < mod_instances.size(); i++ ) {
        if(strmtch(mod_instances[i].id,id) && mod_instances[i].modsource == modsource) {
            found = i;
        }
    }
    if( found == -1 ) {
        mod_instances.push_back(mi);
    } else {
        mod_instances[found] = mi;
    }
}

bool ActiveModsParser::HasModInstance(const char* id, ModSource modsource) {
    for(auto & mod_instance : mod_instances) {
        if(strmtch(mod_instance.id,id) && mod_instance.modsource == modsource) {
            return true;
        }
    }
    return false;
}

ActiveModsParser::ModInstance ActiveModsParser::GetModInstance(const char* id, ModSource modsource) {
    for(auto & mod_instance : mod_instances) {
        if(strmtch(mod_instance.id,id) && mod_instance.modsource == modsource) {
            return mod_instance;
        }
    }
    return ModInstance(id,modsource,false,"");
}

void ActiveModsParser::RemoveModInstance(const char* id, ModSource modsource) {
    for( unsigned i = 0; i < mod_instances.size(); i++ ) {
        if(strmtch(mod_instances[i].id,id) && mod_instances[i].modsource == modsource) {
            mod_instances.erase(mod_instances.begin()+i);
        }
    }
}

void ActiveModsParser::Clear() {
    mod_instances.clear(); 
}

ActiveModsParser::ModInstance::ModInstance() {
    this->id[0] = '\0';
    this->modsource = ModSourceUnknown;
    this->activated = false;
    this->version[0] = '\0';
}

ActiveModsParser::ModInstance::ModInstance(const char* id, ModSource modsource, bool activated, const char* version) {
    strscpy(this->id,id,MOD_ID_MAX_LENGTH);
    this->modsource = modsource;
    this->activated = activated;
    strscpy(this->version,version,MOD_VERSION_MAX_LENGTH);
}
