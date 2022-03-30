//-----------------------------------------------------------------------------
//           Name: manifestparser.cpp
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

#include "manifestparser.h"

#include <XML/xml_helper.h>
#include <XML/Parsers/jobxmlparser.h>

#include <Utility/commonregex.h>
#include <Logging/logdata.h>
#include <Internal/filesystem.h>

#include <tinyxml.h>

ManifestXMLParser::ManifestXMLParser()
{

}

ManifestXMLParser::Item::Item( std::string path, std::string type, std::string hash ) :
path(path),
type(type),
hash(hash)
{

}

ManifestXMLParser::BuilderResult::BuilderResult( std::string dest, std::string dest_hash, std::string builder, std::string builder_version, std::string type, bool success, bool fresh_built, std::vector<Item> items ):
dest(dest),
dest_hash(dest_hash),
builder(builder),
builder_version(builder_version),
type(type),
success(success),
fresh_built(fresh_built),
items(items)
{

}

ManifestXMLParser::GeneratorResult::GeneratorResult( std::string dest, std::string dest_hash, std::string generator, std::string generator_version, std::string type, bool success, bool fresh_built ) :
dest(dest),
dest_hash(dest_hash),
generator(generator),
generator_version(generator_version),
type(type),
success(success),
fresh_built(fresh_built)
{

}


ManifestXMLParser::Manifest::Manifest( std::vector<BuilderResult> builder_results, std::vector<GeneratorResult> generator_results ) :
builder_results(builder_results),
generator_results(generator_results)
{

}

ManifestXMLParser::Manifest::Manifest()
{

}

bool ManifestXMLParser::Load( const std::string& manifest_path )
{
    Clear();

    CommonRegex cr;
    TiXmlDocument doc( manifest_path.c_str() );
    doc.LoadFile();

    if( !doc.Error() )
    {
        TiXmlElement* pRoot = doc.RootElement();

        if( pRoot )  
        {
            TiXmlHandle hRoot(pRoot);

            TiXmlNode* nResult = hRoot.FirstChild().ToNode();

            std::vector<BuilderResult> builder_results;
            std::vector<GeneratorResult> generator_results;

            while( nResult )
            {
                TiXmlElement* eResult = nResult->ToElement();
                
                if( eResult )
                {
                    if( strcmp(eResult->Value(), "BuilderResult") == 0 )
                    {
                        const char* dest = eResult->Attribute("dest");
                        const char* dest_hash = eResult->Attribute("dest_hash");
                        const char* builder = eResult->Attribute("builder");
                        const char* builder_version = eResult->Attribute("builder_version");
                        const char* type =              eResult->Attribute("type");
                        bool     success = cr.saysTrue(eResult->Attribute("success"));
                        bool     fresh_built = cr.saysTrue(eResult->Attribute("fresh_built"));

                        std::string dest_s;
                        if( dest )
                            dest_s = std::string( dest );
                        std::string dest_hash_s;
                        if( dest_hash )
                            dest_hash_s = std::string( dest_hash );
                        std::string builder_s;
                        if( builder )
                            builder_s = std::string( builder ); 
                        std::string builder_version_s;
                        if( builder_version )
                            builder_version_s = std::string(builder_version);
                        std::string type_s;
                        if( type )
                            type_s = std::string(type);

                        std::vector<Item> items;

                        TiXmlElement* eItem = nResult->FirstChild("Item")->ToElement();

                        while( eItem )
                        {
                            const char* item_path = eItem->Attribute("path");
                            const char* item_type = eItem->Attribute("type");
                            const char* item_hash = eItem->Attribute("hash");

                            std::string item_path_s;
                            if( item_path )
                                item_path_s = std::string( item_path );
                            std::string item_type_s;
                            if( item_type )
                                item_type_s = std::string( item_type );
                            std::string item_hash_s;
                            if( item_hash )
                                item_hash_s = std::string( item_hash );

                            items.push_back(Item(item_path_s, item_type_s, item_hash_s));

                            eItem = eItem->NextSiblingElement("Item");
                        }

                        builder_results.push_back( BuilderResult( dest_s, dest_hash_s, builder_s, builder_version_s, type_s, success, fresh_built,  items) );
                    }
                    else if(strcmp(eResult->Value(), "GeneratorResult") == 0 )
                    {
                        const char* dest =              eResult->Attribute("dest");
                        const char* dest_hash =         eResult->Attribute("dest_hash");
                        const char* generator =         eResult->Attribute("generator");
                        const char* generator_version = eResult->Attribute("generator_version");
                        const char* type =              eResult->Attribute("type");
                        bool     success =              cr.saysTrue(eResult->Attribute("success"));
                        bool     fresh_built =          cr.saysTrue(eResult->Attribute("fresh_built"));
               
                        std::string dest_s;
                        if( dest )
                            dest_s = std::string( dest );
                        std::string dest_hash_s;
                        if( dest_hash )
                            dest_hash_s = std::string( dest_hash );
                        std::string generator_s;
                        if( generator )
                            generator_s = std::string( generator ); 
                        std::string generator_version_s;
                        if( generator_version )
                            generator_version_s = std::string(generator_version);
                        std::string type_s;
                        if( type )
                            type_s = std::string(type);

                        generator_results.push_back( GeneratorResult(dest_s, dest_hash_s, generator_s, generator_version_s, type_s, success, fresh_built ) );
                    }
                    else
                    {
                        LOGE << "Unknown element name: " << eResult->Value() << std::endl;
                    }
                }
                else
                {
                    LOGE << "Malformed element in manifest" << std::endl; 
                }

                nResult = nResult->NextSibling(); 
            }

            manifest = Manifest( builder_results, generator_results );
        }
        else
        {
            LOGE << "Problem loading manifest file" << manifest_path << std::endl;
            return false;
        }
    }
    else
    {
        LOGE << "Error parsing manifest file: \"" << doc.ErrorDesc() << "\"" << std::endl;
        return false;
    }
    return true;
}

bool ManifestXMLParser::Save( const std::string& manifest )
{
    LOGE << "No manifest save function implemented" << std::endl;
    return false;
}

void ManifestXMLParser::Clear()
{
    manifest = Manifest();   
}

