//-----------------------------------------------------------------------------
//           Name: assetloadwarningparser.cpp
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
#include "assetloadwarningparser.h"

#include <Utility/strings.h>
#include <Utility/serialize.h>

AssetLoadWarningParser::AssetWarning::AssetWarning(std::string path, uint32_t load_flags, std::string asset_type, std::string level_name) : 
path(path),
load_flags(load_flags),
asset_type(asset_type),
level_name(level_name) {

}

AssetLoadWarningParser::AssetLoadWarningParser() {

}

uint32_t AssetLoadWarningParser::Load( const std::string& path ) {
    TiXmlDocument doc( path.c_str() );
    doc.LoadFile();
    if( !doc.Error() ) {
        TiXmlElement* pRoot = doc.RootElement();
        if( pRoot ) {
            TiXmlElement* e = pRoot->FirstChildElement("AssetWarning");
            while(e) {
                uint32_t flags;
                string_flags_to_uint32(&flags, nullAsEmpty(e->Attribute("load_flags")));
                asset_warnings.insert(AssetWarning(nullAsEmpty(e->Attribute("path")),flags, nullAsEmpty(e->Attribute("asset_type")),nullAsEmpty(e->Attribute("level_name"))));
                e = e->NextSiblingElement("AssetWarning");
            }
        }
    }
    return 0;
}

bool AssetLoadWarningParser::Save( const std::string& path ) {
    TiXmlDocument doc;
    TiXmlDeclaration * decl = new TiXmlDeclaration( "2.0", "", "" );
    TiXmlElement * root = new TiXmlElement("AssetWarnings");

    for( std::set<AssetWarning>::iterator sit = asset_warnings.begin(); sit != asset_warnings.end(); sit++ ) {
        TiXmlElement * e = new TiXmlElement("AssetWarning");
        e->SetAttribute("asset_type",sit->asset_type.c_str());
        e->SetAttribute("path",sit->path.c_str());
        e->SetAttribute("level_name",sit->level_name.c_str());
        char flags[9]; flags_to_string(flags,sit->load_flags); e->SetAttribute("load_flags",flags);
        root->LinkEndChild(e);
    }

    doc.LinkEndChild(decl);
    doc.LinkEndChild(root);

    doc.SaveFile(path.c_str());
    return true;
}

void AssetLoadWarningParser::Clear() {
    asset_warnings.clear();
}

void AssetLoadWarningParser::AddAssetWarning(AssetWarning asset_warning) {
    asset_warnings.insert(asset_warning);
}

bool operator<( const AssetLoadWarningParser::AssetWarning& lhs,const AssetLoadWarningParser::AssetWarning& rhs) {
    if(lhs.asset_type < rhs.asset_type ) { 
        return true;
    } else if( lhs.asset_type > rhs.asset_type ) {
        return false;
    } else {
        if(lhs.load_flags < rhs.load_flags ) { 
            return true;
        } else if( lhs.load_flags > rhs.load_flags ) {
            return false;
        } else {
            if( lhs.path < rhs.path ) {
                return true;
            } else if( lhs.path > rhs.path ) {
                return false;
            } else {
                if( lhs.level_name < rhs.level_name ) {
                    return true;
                } else if (lhs.level_name > rhs.level_name) {
                    return false;
                } else {
                    return false;
                }
            }
        }
    }
}
