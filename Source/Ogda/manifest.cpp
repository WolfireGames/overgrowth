//-----------------------------------------------------------------------------
//           Name: manifest.cpp
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
#include "manifest.h"

#include <tinyxml.h>
#include <XML/xml_helper.h>

#include <Utility/commonregex.h>
#include <Logging/logdata.h>
#include <Internal/filesystem.h>
#include <XML/Parsers/jobxmlparser.h>
#include <Version/version.h>

#include "jobhandler.h"
#include "manifestthreadpool.h"

Manifest::Manifest(int thread): thread(thread)
{
}

bool Manifest::Load(const std::string& manifest)
{
    results.clear();
    CommonRegex cr;
    TiXmlDocument doc( manifest.c_str() );
    doc.LoadFile();

    if( !doc.Error() )
    {
        TiXmlElement* pRoot = doc.RootElement();

        if( pRoot )  
        {
            TiXmlHandle hRoot(pRoot);

            TiXmlNode* nResult = hRoot.FirstChild().ToNode();

            while( nResult )
            {
                TiXmlElement* eResult = nResult->ToElement();
                
                if( eResult )
                {
                    if( strcmp(eResult->Value(), "Result") == 0 //Result is a backwards compatible name, never generated.
                        || strcmp(eResult->Value(), "BuilderResult") == 0 )
                    {

                        const char* dest =              eResult->Attribute("dest");
                        const char* dest_hash =         eResult->Attribute("dest_hash");
                        const char* builder =           eResult->Attribute("builder");
                        const char* builder_version =   eResult->Attribute("builder_version");
                        const char* type =              eResult->Attribute("type");
                        bool     success = cr.saysTrue(eResult->Attribute("success"));

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

                            items.push_back(Item("", item_path_s, item_type_s, item_hash_s, JobXMLParser::Item()));

                            eItem = eItem->NextSiblingElement("Item");
                        }

                        results.push_back( ManifestResult( dest_hash_s, items, dest_s, success, builder_s, builder_version_s, ManifestResult::BUILT, type_s ) );
                    }
                    else if( strcmp(eResult->Value(), "DatabaseResult") == 0 )
                    {

                        const char* dest =              eResult->Attribute("dest");
                        const char* dest_hash =         eResult->Attribute("dest_hash");
                        const char* builder =           eResult->Attribute("builder");
                        const char* builder_version =   eResult->Attribute("builder_version");
                        const char* type =              eResult->Attribute("type");

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

                            items.push_back(Item("", item_path_s, item_type_s, item_hash_s, JobXMLParser::Item()));

                            eItem = eItem->NextSiblingElement("Item");
                        }

                        results.push_back( ManifestResult( dest_hash_s, items, dest_s, true, builder_s, builder_version_s, ManifestResult::DATABASE, type_s ) );
                    }
                    else if(strcmp(eResult->Value(), "GeneratorResult") == 0 )
                    {
                        const char* dest =              eResult->Attribute("dest");
                        const char* dest_hash =         eResult->Attribute("dest_hash");
                        const char* generator =         eResult->Attribute("generator");
                        const char* generator_version = eResult->Attribute("generator_version");
                        const char* type =              eResult->Attribute("type");
                        bool     success = cr.saysTrue(eResult->Attribute("success"));
               
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

                        results.push_back( ManifestResult( dest_hash_s, dest_s, success, generator_s, generator_version_s, ManifestResult::GENERATED, type_s  ) );

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
        }
        else
        {
            LOGE << "Problem loading manifest file" << manifest << std::endl;
            return false;
        }
    }
    else
    {
        LOGE << "Error parsing manifest file: \"" << doc.ErrorDesc() << "\"" << std::endl;
        return false;
    }

    std::vector<ManifestResult>::const_iterator mrit;
    for( mrit = results.begin(); mrit != results.end(); mrit++ )
    {
        std::vector<Item>::const_iterator itemit;
        for( itemit = mrit->items.begin(); itemit != mrit->items.end(); itemit++ )
        {  
            hash_set.insert(itemit->GetSubHash());
        }
    }

    return true;
}

bool Manifest::Save(const std::string& manifest)
{
    TiXmlDocument doc;
    TiXmlDeclaration * decl = new TiXmlDeclaration( "2.0", "", "" );
    TiXmlElement * root = new TiXmlElement( "Manifest" );

    TiXmlElement* builder_info = new TiXmlElement( "ProgramInfo" );
    builder_info->SetAttribute("name", "Ogda");
    builder_info->SetAttribute("build_version", GetBuildVersion());
    builder_info->SetAttribute("build_date", GetBuildTimestamp());
    builder_info->SetAttribute("platform", GetPlatform());
    builder_info->SetAttribute("arch", GetArch());
    root->LinkEndChild(builder_info);

    TiXmlElement* execution_info = new TiXmlElement( "ExecutionInfo" );
    //Add timestamp when it was run in here.
    //Add how long it took here.
    root->LinkEndChild(execution_info);

    std::vector<ManifestResult>::const_iterator mrit;

    for( mrit = results.begin(); mrit != results.end(); mrit++ )
    {
        if( mrit->mr_type == ManifestResult::BUILT )
        {
            TiXmlElement* eResult = new TiXmlElement( "BuilderResult" );

            std::vector<Item>::const_iterator itemit;

            for( itemit = mrit->items.begin(); itemit != mrit->items.end(); itemit++ )
            {
                TiXmlElement* eItem = new TiXmlElement( "Item" );

                eItem->SetAttribute( "path",  itemit->GetPath().c_str() );
                eItem->SetAttribute( "type",  itemit->type.c_str() );
                eItem->SetAttribute( "hash",  itemit->hash.c_str() );

                eResult->LinkEndChild( eItem );
            }
                
            eResult->SetAttribute( "dest", mrit->dest.c_str() );
            eResult->SetAttribute( "dest_hash", mrit->dest_hash.c_str() );
            eResult->SetAttribute( "builder", mrit->name.c_str() );
            eResult->SetAttribute( "builder_version", mrit->version.c_str() );
            eResult->SetAttribute( "type", mrit->type.c_str() );
            eResult->SetAttribute( "success", mrit->success ? "true" : "false" );
            eResult->SetAttribute( "fresh_built", mrit->fresh_built ? "true" : "false" );

            root->LinkEndChild( eResult );
        }
        else if( mrit->mr_type == ManifestResult::DATABASE )
        {
            TiXmlElement* eResult = new TiXmlElement( "DatabaseResult" );

            std::vector<Item>::const_iterator itemit;

            for( itemit = mrit->items.begin(); itemit != mrit->items.end(); itemit++ )
            {
                TiXmlElement* eItem = new TiXmlElement( "Item" );

                eItem->SetAttribute( "path",  itemit->GetPath().c_str() );
                eItem->SetAttribute( "type",  itemit->type.c_str() );
                eItem->SetAttribute( "hash",  itemit->hash.c_str() );

                eResult->LinkEndChild( eItem );
            }
                
            eResult->SetAttribute( "dest", mrit->dest.c_str() );
            eResult->SetAttribute( "dest_hash", mrit->dest_hash.c_str() );
            eResult->SetAttribute( "builder", mrit->name.c_str() );
            eResult->SetAttribute( "builder_version", mrit->version.c_str() );
            eResult->SetAttribute( "type", mrit->type.c_str() );

            root->LinkEndChild( eResult );
        }
        else if( mrit->mr_type == ManifestResult::GENERATED )
        {
            TiXmlElement* eResult = new TiXmlElement( "GeneratorResult" );

            eResult->SetAttribute( "dest", mrit->dest.c_str() );
            eResult->SetAttribute( "dest_hash", mrit->dest_hash.c_str() );
            eResult->SetAttribute( "generator", mrit->name.c_str() );
            eResult->SetAttribute( "generator_version", mrit->version.c_str() );
            eResult->SetAttribute( "typ", mrit->type.c_str() );
            eResult->SetAttribute( "success", mrit->success ? "true" : "false" );
            eResult->SetAttribute( "fresh_built", mrit->fresh_built ? "true" : "false" );

            root->LinkEndChild( eResult );
        }
        else
        {
            LOGE << "Unknown ManifestResult type" << std::endl;
        }
    }

    doc.LinkEndChild( decl );
    doc.LinkEndChild( root );
    doc.SaveFile( manifest.c_str() );

    return !doc.Error();
}

void Manifest::AddResult( const ManifestResult& a ) {
    results.push_back(a);
    std::vector<Item>::const_iterator itemit;
    for( itemit = a.items.begin(); itemit != a.items.end(); itemit++ ) {  
        hash_set.insert(itemit->GetSubHash());
    }
}

bool Manifest::HasError()
{
    std::vector<ManifestResult>::const_iterator mrit;
    for( mrit = results.begin(); mrit != results.end(); mrit++ )
    {
        if( mrit->success == false )
            return true;
    }
    return false;
}

std::vector<std::string> Manifest::GetDestinationFiles()
{
    std::vector<std::string> values;

    std::vector<ManifestResult>::const_iterator mrit;
    for( mrit = results.begin(); mrit != results.end(); mrit++ )
    {
        values.push_back( mrit->dest ); 
    }

    return values; 
}

bool Manifest::IsUpToDate( JobHandler& jh, const Item& item, const Builder& builder )
{
    std::vector<ManifestResult>::iterator mrit;
    if( hash_set.find(item.GetSubHash()) != hash_set.end() ) 
    {
        for( mrit = results.begin(); mrit != results.end(); mrit++ )
        {
            if( mrit->items.size() == 1 )
            {
                //First, check that it's the same item.
                //This "magically" includes the hash as item contains the hash value
                if( mrit->items[0] == item )
                {
                    //Check that it's the same builder, one builder, one type of possible output file
                    if( mrit->name == builder.GetBuilderName() )
                    {
                        //Check the builder version, to verify that the expected output wouldn't change.
                        if( mrit->version == builder.GetBuilderVersion() )
                        {
                            //Check if the claimed destination file from the manifest differs from the file on disk according to hash 
                            //and that the hash isn't invalid
                            if( mrit->GetCurrentDestHash(jh.output_folder) == mrit->dest_hash && mrit->dest_hash != std::string("") )
                            {
                                return true;
                            }
                        }
                    }  
                }    
            }
            else if( mrit->items.size() > 1 )
            {
                LOGW << "System is not currently built to handle matching ManifestResults with more than one item" << std::endl;
            }
        }
    }
    return false;
}

ManifestResult Manifest::GetPreviouslyBuiltResult(const Item& item, const Builder& builder)
{
    std::vector<ManifestResult>::const_iterator mrit;
    for( mrit = results.begin(); mrit != results.end(); mrit++ )
    {
        if( mrit->items.size() == 1 )
        {
            //First, check that it's the same item.
            //This "magically" includes the hash as item contains the hash value
            if( mrit->items[0] == item )
            {
                //Check that it's the same builder, one builder, one type of possible output file
                if( mrit->name == builder.GetBuilderName() )
                {
                    //This is mostly for sanity, but should never really evaluate to false as we already found the right builder and there is only one per name.
                    //If this was false, that would mean we have changed the version of the builder and the asset we're looking for is actually outdated.
                    //Meaning that previous checks and evalutation should have lead us to the conclusion to rebuild this item.
                    if( mrit->version == builder.GetBuilderVersion() )
                    {
                        return *mrit;
                    }
                }  
            }    
        }
        else if( mrit->items.size() > 1 )
        {
            LOGW << "System is not currently built to handle matching ManifestResults with more than one item" << std::endl;
        }
    }
    throw "Big error";
}


void Manifest::PrecalculateCurrentDestinationHashes( const std::string& dest_path )
{
    ManifestThreadPool mtp(thread);

    mtp.RunHashCalculation(dest_path, results);
}

std::vector<ManifestResult>::const_iterator Manifest::ResultsBegin() const
{
    return results.begin();
}

std::vector<ManifestResult>::const_iterator Manifest::ResultsEnd() const
{
    return results.end();
}
