//-----------------------------------------------------------------------------
//           Name: database_manifest.cpp
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
#include "database_manifest.h"

#include <tinyxml.h>
#include <XML/xml_helper.h>

#include <Utility/commonregex.h>
#include <Logging/logdata.h>
#include <Internal/filesystem.h>
#include <XML/Parsers/jobxmlparser.h>
#include <Version/version.h>

#include "jobhandler.h"

DatabaseManifest::DatabaseManifest()
{
}

bool DatabaseManifest::Load(const std::string& manifest)
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
                    if( strcmp(eResult->Value(), "DatabaseResult") == 0 )
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

                        TiXmlElement* eItem = nResult->FirstChild("Item")->ToElement();
                        
                        if( eItem )
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


                            eItem = eItem->NextSiblingElement("Item");

                            
                            results.push_back( DatabaseManifestResult( Item("", item_path_s, item_type_s, item_hash_s, JobXMLParser::Item()), dest_hash_s, dest_s, builder_s, builder_version_s, type_s ) );
                        } else {
                            LOGE << "Missing item for DatabaseManifestResult" << std::endl;
                        }

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

    std::vector<DatabaseManifestResult>::const_iterator mrit;
    for( mrit = results.begin(); mrit != results.end(); mrit++ )
    {
        hash_set.insert(mrit->item.GetSubHash());
    }

    return true;
}

bool DatabaseManifest::Save(const std::string& manifest)
{
    TiXmlDocument doc;
    TiXmlDeclaration * decl = new TiXmlDeclaration( "2.0", "", "" );
    TiXmlElement * root = new TiXmlElement( "DatabaseManifest" );
    root->SetAttribute("version", "1");

    std::vector<DatabaseManifestResult>::const_iterator mrit;

    for( mrit = results.begin(); mrit != results.end(); mrit++ )
    {
        TiXmlElement* eResult = new TiXmlElement( "DatabaseResult" );

        TiXmlElement* eItem = new TiXmlElement( "Item" );

        eItem->SetAttribute( "path",  mrit->item.GetPath().c_str() );
        eItem->SetAttribute( "type",  mrit->item.type.c_str() );
        eItem->SetAttribute( "hash",  mrit->item.hash.c_str() );

        eResult->LinkEndChild( eItem );
            
        eResult->SetAttribute( "dest", mrit->dest.c_str() );
        eResult->SetAttribute( "dest_hash", mrit->dest_hash.c_str() );
        eResult->SetAttribute( "builder", mrit->name.c_str() );
        eResult->SetAttribute( "builder_version", mrit->version.c_str() );
        eResult->SetAttribute( "type", mrit->type.c_str() );

        root->LinkEndChild( eResult );
    }

    doc.LinkEndChild( decl );
    doc.LinkEndChild( root );
    doc.SaveFile( manifest.c_str() );

    return !doc.Error();
}

void DatabaseManifest::AddResult( const DatabaseManifestResult& a ) {
    results.push_back(a);
    hash_set.insert(a.item.GetSubHash());
}

bool DatabaseManifest::HasBuiltResultFor( JobHandler& jh, const Item& item, const Builder& builder )
{
    std::vector<DatabaseManifestResult>::iterator mrit;
    if( hash_set.find(item.GetSubHash()) != hash_set.end() ) 
    {
        for( mrit = results.begin(); mrit != results.end(); mrit++ )
        {
            if( mrit->item == item )
            {
                //Check that it's the same builder, one builder, one type of possible output file
                if( mrit->name == builder.GetBuilderName() )
                {
                    //Check the builder version, to verify that the expected output wouldn't change.
                    if( mrit->version == builder.GetBuilderVersion() )
                    {
                        //Check if the claimed destination file from the manifest differs from the file on disk according to hash 
                        //and that the hash isn't invalid
                        if( mrit->GetCurrentDestHash(jh.databasedir) == mrit->dest_hash && mrit->dest_hash != std::string("") )
                        {
                            return true;
                        }
                    }
                }  
            }    
        }
    }
    return false;
}

DatabaseManifestResult DatabaseManifest::GetPreviouslyBuiltResult(const Item& item, const Builder& builder)
{
    std::vector<DatabaseManifestResult>::const_iterator mrit;
    for( mrit = results.begin(); mrit != results.end(); mrit++ )
    {
        //First, check that it's the same item.
        //This "magically" includes the hash as item contains the hash value
        if( mrit->item == item )
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
    throw "Big error";
}
