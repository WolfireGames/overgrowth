//-----------------------------------------------------------------------------
//           Name: jobhandler.cpp
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
#include "jobhandler.h"
#include <trex/trex.h>
#include <XML/Parsers/jobxmlparser.h>
#include <Logging/logdata.h>
#include "item.h"
#include <Internal/filesystem.h>
#include <Ogda/Builders/builderfactory.h>
#include <Ogda/Searchers/searcherfactory.h>
#include <Ogda/Generators/generatorfactory.h>
#include "manifest.h"
#include "database_manifest.h"
#include <algorithm>
#include <cstdio>
#include <Utility/hash.h>
#include "jobhandlerthreadpool.h"
#include <cassert>
#include "ogda_config.h"
#include "main.h"
#include <Utility/strings.h>

JobHandler::JobHandler(std::string output_folder, std::string manifest_dest, std::string manifest_source, std::string databasedir, bool perform_removes, bool force_removes, int threads) 
: output_folder(output_folder), manifest_dest(manifest_dest), manifest_source(manifest_source), databasedir(databasedir), perform_removes(perform_removes),force_removes(force_removes), threads(threads)
{

}

bool JobHandler::Run(const std::string& path )
{
    bool ret = true;
    Manifest old_manifest(threads);
    DatabaseManifest database_manifest;

    std::string database_file = AssemblePath(databasedir, "database_manifest.xml");

    if(!manifest_source.empty())
    {
        old_manifest.Load(manifest_source); 
        LOGI << "Calculating hashes of previously built files... [" << threads << "]" << std::endl; 
        {
            old_manifest.PrecalculateCurrentDestinationHashes(output_folder);
        }
    }

    if(!databasedir.empty() )
    {
        if( config_load_from_database || config_save_to_database ) {
            database_manifest.Load(database_file); 
        }
    }
    
    {
        JobXMLParser jobparser;
        if( !jobparser.Load( path ) )
        {
            LOGE << "Error" << std::endl;
            ret = false;
        }
        else
        {
            input_folders = jobparser.inputs;

            LOGI << "Adding Items..." << std::endl;
            {
                size_t item_count = jobparser.items.size();
                size_t cur = 0;
                std::vector<JobXMLParser::Item>::iterator itemit;
                for( itemit = jobparser.items.begin(); itemit != jobparser.items.end(); itemit++ )
                {
                    std::string chosen_input_folder;
                    std::string item_path;
                    bool is_ok = false;

		    LOGD << "Loading Transfer: " << itemit->path << std::endl;
		    for(std::string input_folder : input_folders) 
		    {
			if(is_ok == false) {
                            if( CheckFileAccess(AssemblePath(input_folder,itemit->path).c_str()) ) {
                                item_path = itemit->path;
				chosen_input_folder  = input_folder;
                                is_ok = true;
                            } else {
                                std::string case_corrected = CaseCorrect(AssemblePath(input_folder,itemit->path));
                                if(CheckFileAccess(case_corrected.c_str())) {
                                    item_path = case_corrected.substr(input_folder.size());
				    chosen_input_folder = input_folder;
                                    is_ok = true;
                                    LOGW << "Path \"" << itemit->path << "\" for item had to be case corrected to " << item_path << ". Row: " << itemit->row << std::endl;
                                }
                            }
			}
		    }

                    if( is_ok ) {
                        if( itemit->recursive )
                        {
                            LOGD << "Transfer is recursive" << std::endl;

                            std::vector<std::string> manifest;
                            std::string manifest_path = AssemblePath(chosen_input_folder, item_path);
                            GenerateManifest(manifest_path.c_str(), manifest);

                            LOGD << "Loaded from " << manifest_path << " found " << manifest.size() << " files." << std::endl;

                            std::vector<std::string>::iterator manifestit; 

                            for( manifestit = manifest.begin(); manifestit != manifest.end(); manifestit++ )
                            {
                                std::string full_sub_path = AssemblePath(item_path,*manifestit);
                                if(endswith(full_sub_path.c_str(), itemit->path_ending.c_str()))
                                {
                                    LOGD << "Including " << full_sub_path << std::endl;
                                    AddItem(chosen_input_folder, full_sub_path,itemit->type,*itemit);
                                }
                                else
                                {
                                    LOGD << "Ignoring " << full_sub_path << std::endl;
                                }
                            }
                        }     
                        else
                        {
                            AddItem(chosen_input_folder, item_path,itemit->type,*itemit);
                        }
                    } else {
                        LOGE << "Path \"" << itemit->path << "\" for item is invalid, even after case correction. Row: " << itemit->row << std::endl;
		    }

                    cur++;
                    SetPercent( item_path.c_str(), (int)(100.0f*((float)cur/(float)item_count)) );
                }
            }

            if( config_print_item_list ) 
            {
                LOGI << "Printing item list from deployment file..." << std::endl;
        
                std::vector<Item>::iterator itemit = items.begin();

                for( ;itemit != items.end();itemit++ ) 
                {
                    LOGI << *itemit << std::endl;
                }
            }

            LOGI << "Adding Searchers..." << std::endl;
            {
                SearcherFactory sf;
                std::vector<JobXMLParser::Searcher>::iterator searcherit;
                for( searcherit = jobparser.searchers.begin(); searcherit != jobparser.searchers.end(); searcherit++ )
                {
                    if( sf.HasSearcher(searcherit->searcher))
                    {
                        Searcher searcher = sf.CreateSearcher( searcherit->searcher, searcherit->path_ending, searcherit->type_pattern_re);
                        searchers.push_back(searcher);
                    }
                    else
                    {
                        ret = false;
                        LOGE << "Unable to find searcher matching name " << searcherit->searcher << std::endl;
                    }
                }
            }

            LOGI << "Adding Builders..." << std::endl;
            {
                BuilderFactory bf;
                std::vector<JobXMLParser::Builder>::iterator builderit;
                for( builderit = jobparser.builders.begin(); builderit != jobparser.builders.end(); builderit++ )
                {
                    if( bf.HasBuilder( builderit->builder ) )
                    {
                        Builder builder = bf.CreateBuilder( builderit->builder, builderit->path_ending, builderit->type_pattern_re);
                        builders.push_back(builder);
                    }
                    else
                    {
                        LOGE << "Unable to find builder matching name " << builderit->builder << std::endl;
                        ret = false;
                    }
                }
            }

            LOGI << "Adding Generators..." << std::endl;
            {
                GeneratorFactory bf;
                std::vector<JobXMLParser::Generator>::iterator generatorit;
                for( generatorit = jobparser.generators.begin(); generatorit != jobparser.generators.end(); generatorit++ )
                {
                    if( bf.HasGenerator( generatorit->generator ) )
                    {
                        Generator generator = bf.CreateGenerator( generatorit->generator );
                        generators.push_back(generator);
                    }
                    else
                    {
                        LOGE << "Unable to find generator matching name " << generatorit->generator << std::endl;
                        ret = false;
                    }
                }

            }

            LOGD << jobparser << std::endl;
        }
    }

    LOGI << "Running searchers through Items..." << std::endl;
    {
        std::vector<Item>::iterator itemit;
        for( itemit = items.begin(); itemit != items.end(); itemit++ )
        {
            RunRecursiveSearchOn( *itemit ); 
        }

        LOGI << "Found a total of " << foundItems.size() << " objects when searching" << std::endl;
        items.insert( items.end(), foundItems.begin(), foundItems.end() );
    }

    LOGI << "Marking overshadowed Items" << std::endl;
    {
        std::vector<Item>::iterator itemit;
        for( itemit = items.begin(); itemit != items.end(); itemit++ )
        {
            std::vector<Item>::iterator item2it;
            for( item2it = items.begin(); item2it != items.end(); item2it++ )
            {
                if( itemit->Overshadows(*item2it) )
                {
                    item2it->SetOvershadowed(true);
                }
            }
        }
    }

    if( config_print_duplicates )
    {
        LOGI << "Looking for duplicate items..." << std::endl;
        {
            std::vector<Item>::iterator itemit; 
            std::vector<Item>::iterator itemit2;
            
            std::set<JobXMLParser::Item> first_level_duplicates;

            for( itemit = items.begin(); itemit != items.end(); itemit++ )
            {
                for( itemit2 = itemit + 1; itemit2 != items.end(); itemit2++ )
                {
                    if( itemit->GetAbsPath() == itemit2->GetAbsPath()
                        && itemit->type == itemit2->type )
                    {
                        if( *itemit == itemit->source )
                        {
                            first_level_duplicates.insert( itemit->source );
                            LOGW << "Found duplicated of " << *itemit << " " << "sourced from rows: " << itemit->source.row << " and " << itemit2->source.row << "." << std::endl; 
                        }
                        else if( *itemit2 == itemit2->source )
                        {
                            first_level_duplicates.insert( itemit2->source );
                            LOGW << "Found duplicated of " << *itemit << " " << "sourced from rows: " << itemit->source.row << " and " << itemit2->source.row << "." << std::endl; 
                        }
                    }
                }
            }
        
            LOGI << "Listing all first-level first-time references to duplicates... (For simple removal)" << std::endl;
            {
                std::set<JobXMLParser::Item>::iterator parserit;

                for( parserit = first_level_duplicates.begin(); parserit != first_level_duplicates.end(); parserit++ )
                {
                     fprintf( stderr, "RMLN:%d\n", parserit->row );
                }
            }
        }
    }

    if( config_print_missing )
    {
        LOGI << "Printing files in folder but not in deploy list..." << std::endl;
    
        std::vector<std::string> manifest;

        for(std::string input_folder : input_folders) 
	{
            GenerateManifest( input_folder.c_str(), manifest );
	}

        std::vector<std::string>::iterator manifestit = manifest.begin();

        for( ; manifestit != manifest.end(); manifestit++ )
        {
            if( false == HasItemWithPath( *manifestit ) )
            {
                std::cout << *manifestit << std::endl;
            }
        }
    }

    /*
    LOGI << "Checking what items don't have a searcher..." << std::endl;
    {
        std::map<std::string,int>::iterator searchTypeIt;

        for( searchTypeIt = typeSearcherCount.begin(); searchTypeIt != typeSearcherCount.end(); searchTypeIt++ )
        {
            if( searchTypeIt->second == 0 )
            {
                LOGW << "Item type: " << searchTypeIt->first << " has no assigned searchers." << std::endl; 
            }
        }
    }
    */

    LOGI << "Calculating Item hashes...[" << threads << "]"<< std::endl;
    {
        JobHandlerThreadPool jhtp(threads);
        jhtp.RunHashCalculation(items);
    }

    if( !config_mute_missing )
    {
        LOGI << "Listing referenced items missing from disk..." << std::endl;
        {
            std::vector<Item>::iterator itemit; 

            for( itemit = items.begin(); itemit != items.end(); itemit++ )
            {
                if( itemit->hash.empty() && itemit->IsOvershadowed() == false )
                {
                    LOGE << "Missing item " << *itemit << std::endl;
                } 
            }
        }
    }

    Manifest result_manifest(threads);
    LOGI << "Running Builders through Items..." << std::endl;
    {
        size_t item_count = items.size();
        size_t cur = 0;
        std::vector<Item>::iterator itemit;
        for( itemit = items.begin(); itemit != items.end(); itemit++ )
        {
            if( itemit->IsOvershadowed() )
            {
                LOGI << "Skipping " << *itemit << " because it's overshadowed" << std::endl; 
            }
            else if( itemit->IsOnlySearch() )
            {
                LOGD << "Skipping " << *itemit << " because it's search only" << std::endl; 
            }
            else
            {
                std::vector<Builder>::iterator builderit;
                int count = 0;
                for( builderit = builders.begin(); builderit != builders.end(); builderit++ )
                {
                    if( builderit->IsMatch( *itemit ))
                    {
                        count++;
                        if(builderit->RunEvenOnIdenticalSource() == false && old_manifest.IsUpToDate( *this, *itemit, *builderit ) )
                        {
                            LOGD << "Using cached result on " << *itemit << std::endl;
                            result_manifest.AddResult(old_manifest.GetPreviouslyBuiltResult( *itemit, *builderit ));
                        }
                        else if( config_load_from_database && builderit->RunEvenOnIdenticalSource() == false && builderit->StoreResultInDatabase() && database_manifest.HasBuiltResultFor(*this, *itemit, *builderit) )
                        {
                            LOGI << "Using database value on " << *itemit << std::endl;
                            DatabaseManifestResult dmr = database_manifest.GetPreviouslyBuiltResult(*itemit, *builderit);
                           
                            std::string result_source_path = AssemblePath(databasedir,AssemblePath("files",AssemblePath(dmr.item.hash,dmr.dest_hash)));
                            std::string result_dest_path = AssemblePath(output_folder,dmr.dest);

                            CreateParentDirs(result_dest_path);
                            copyfile(result_source_path,result_dest_path);
                            result_manifest.AddResult(ManifestResult(dmr.dest_hash, *itemit, dmr.dest, true, dmr.name, dmr.version, ManifestResult::DATABASE, dmr.type));
                        }
                        else
                        {
                            //Check if item has source hash, meaning if the source file exists.
                            if( !itemit->hash.empty() )
                            {
                                LOGD << "Running " << builderit->GetBuilderName() << " on " << *itemit << std::endl;
                                result_manifest.AddResult(builderit->Run(*this, *itemit));
                            }
                            else
                            {
                                LOGE << "Unable to run " << builderit->GetBuilderName() << " on " << *itemit << " file missing." << std::endl;
                                ret = false;
                            }
                        }
                    }
                    else
                    {
                        LOGD << "Skipping " <<  builderit->GetBuilderName() << " on " << *itemit << ", doesn't match pattern." << std::endl;
                    }
                }

                if( count == 0 && !itemit->hash.empty()  )
                    LOGW << *itemit << " has no assigned builder" << std::endl;
                if( count > typeBuilderCount[itemit->type] )
                    typeBuilderCount[itemit->type] = count;
            }
            cur++;
            SetPercent( itemit->GetPath().c_str(), (int)(100.0f*((float)cur/(float)item_count)) );
        }
    }

    //Copy manifest to use when running the generators.
    const Manifest generated_manifest = result_manifest;

    LOGI << "Running Generators..." << std::endl;
    {
        std::vector<Generator>::iterator generatorit;
        int count = 0;
        for( generatorit = generators.begin(); generatorit != generators.end(); generatorit++ )
        {
            count++;
            LOGD << "Running " << generatorit->GetGeneratorName() << std::endl;
            result_manifest.AddResult(generatorit->Run(*this,generated_manifest));
        }
    }

    LOGI << "Checking what items don't have a builder..." << std::endl;
    {
        std::map<std::string,int>::iterator builderTypeIt;

        for( builderTypeIt = typeBuilderCount.begin(); builderTypeIt != typeBuilderCount.end(); builderTypeIt++ )
        {
            if( builderTypeIt->second == 0 )
            {
                LOGW << "Item type: " << builderTypeIt->first << " has no assigned builder." << std::endl; 
            }
        }
    }

    if( result_manifest.HasError() )
    {
        LOGE << "Some builder(s) caused an error, see manifest for more info." << std::endl;
        ret = false;
    }

    //Unlinks are serious business, so we do this carefully
    if( ret || force_removes )
    {
        LOGI << "Removing items not listed in the generated manifest" << std::endl;

        std::vector<std::string> destination_file_list;
        GenerateManifest( output_folder.c_str(), destination_file_list );
        {
            std::vector<std::string>::iterator missit;
            for(missit = destination_file_list.begin(); missit != destination_file_list.end(); missit++ )
            {
                LOGD << "In Output Folder: " << *missit << std::endl;
            }
        }

        std::vector<std::string> new_manifest_file_list = result_manifest.GetDestinationFiles();
        {
            std::vector<std::string>::iterator missit;
            for(missit = new_manifest_file_list.begin(); missit != new_manifest_file_list.end(); missit++ )
            {
                LOGD << "New Manifest File: " << *missit << std::endl;
            }
        }

        std::vector<std::string> old_manifest_file_list = old_manifest.GetDestinationFiles();
        {
            std::vector<std::string>::iterator missit;
            for(missit = old_manifest_file_list.begin(); missit != old_manifest_file_list.end(); missit++ )
            {
                LOGD << "Old Manifest File: " << *missit << std::endl;
            }
        }

        std::vector<std::string> unlisted_files;

        std::sort( destination_file_list.begin(), destination_file_list.end() );
        std::sort( new_manifest_file_list.begin(), new_manifest_file_list.end() );
        std::sort( old_manifest_file_list.begin(), old_manifest_file_list.end() );

        std::set_difference( destination_file_list.begin(), destination_file_list.end(),
                          new_manifest_file_list.begin(), new_manifest_file_list.end(),
                          std::back_inserter(unlisted_files) );

        {
            std::vector<std::string>::iterator missit;
            for(missit = unlisted_files.begin(); missit != unlisted_files.end(); missit++ )
            {
                LOGI << "Unlisted: " << *missit << std::endl;
            }
        }

        std::vector<std::string> remove_list;


        //Remove files that are in the folder but not known from previous builds?
        if( config_remove_unlisted_files )
        {
            LOGI << "Adding all unlisted files into the remove list. (--remove-unlisted)" << std::endl;
            remove_list.insert(remove_list.begin(), unlisted_files.begin(), unlisted_files.end() );
        }
        else
        {
            //Only remove unlisted files that are mentioned in the old manifest.
            std::set_intersection( unlisted_files.begin(), unlisted_files.end(),
                                old_manifest_file_list.begin(), old_manifest_file_list.end(),
                                std::back_inserter(remove_list) );
        }

        //If this passes i'm comfortable removing files.
        if( remove_list == unlisted_files || force_removes )
        {
            if( remove_list == unlisted_files )
            {
                LOGI << "Unlisted files match old manifest, removing them" << std::endl;
            }
            else
            {
                LOGI << "Dictated to forcefully remove all found items." << std::endl;
            }

            {
                std::vector<std::string>::iterator missit;
                for(missit = remove_list.begin(); missit != remove_list.end(); missit++ )
                {
                    std::string full_remove_path = AssemblePath(output_folder,*missit);
                    if( perform_removes )
                    {
                        LOGW << "Removing " << full_remove_path << std::endl;
                        remove( full_remove_path.c_str() );
                    }
                    else
                    {
                        LOGI << "Pretending to remove (no --perform-removes): " << full_remove_path << std::endl;
                    }
                }
            }
        }
        else
        {
            LOGF << "Unlisted files and old manifest files don't match, i refuse to try and remove anything because this is a hint that something isn't right in this fully managed directory." << std::endl;
            ret = false;
        }
    }
    else
    {
        LOGE << "Skipping removal due to previous error(s)" << std::endl;
    }

    LOGI << "Removing temporary items..." << std::endl;
    {
        std::vector<Item>::iterator itemit;
        for( itemit = items.begin(); itemit != items.end(); itemit++ )
        {
            if( itemit->IsDeleteOnExit() )
            {
                if( perform_removes )
                {
                    LOGD << "Removing " << *itemit << std::endl; 
                    std::string path = itemit->GetAbsPath(); 
                    remove( path.c_str() );
                }
                else
                {
                    LOGD << "Skipping remove of " << *itemit << " as --perform-removes isn't specified" << std::endl;
                }

            }  
        }
    }

    if( !manifest_dest.empty() )
    {
        LOGI << "Saving resulting manifest to disk: " << manifest_dest << std::endl;
        result_manifest.Save(manifest_dest);
    }

    if(!databasedir.empty() ) {
        if( config_save_to_database ) {
            std::vector<ManifestResult>::const_iterator mareit = result_manifest.ResultsBegin();
            BuilderFactory builder_factory;
            for(; mareit != result_manifest.ResultsEnd(); mareit++ ) {
                if( mareit->mr_type == ManifestResult::BUILT ) {
                    if( mareit->items.size() == 1 ) {
                        if( builder_factory.StoreResultInDatabase(mareit->name) ) {

                            LOGI << "Storing " << *mareit << " in database for future use." << std::endl;
                            std::string result_source_path = AssemblePath(output_folder,mareit->dest);
                            std::string result_dest_path = AssemblePath(databasedir,AssemblePath("files",AssemblePath(mareit->items[0].hash,mareit->dest_hash)));

                            CreateParentDirs(result_dest_path);
                            copyfile(result_source_path,result_dest_path);
                             
                            database_manifest.AddResult(DatabaseManifestResult(
                                mareit->items[0], 
                                mareit->dest_hash, 
                                mareit->dest,
                                mareit->name, 
                                mareit->version,
                                mareit->type 
                            ));
                        }
                    } else { 
                        LOGW << "Database doesn't support multi item sources" << std::endl;
                    }
                }
            }
            CreateParentDirs(database_file);
            database_manifest.Save(database_file); 
        }
    } 

    return ret;
}

void JobHandler::RunRecursiveSearchOn( const Item& item )
{
    std::vector<Item> foundSum;
    if( std::find(searchedItems.begin(),searchedItems.end(),item) == searchedItems.end() )
    {
        std::vector<Searcher>::iterator searcherit;
        int count = 0;
        for( searcherit = searchers.begin(); searcherit != searchers.end(); searcherit++ )
        {
            std::vector<Item> f = searcherit->TrySearch(*this,item,&count);

            foundSum.insert(foundSum.end(),f.begin(),f.end());
        }
        assert((count > 0 && foundSum.size() > 0) || foundSum.size() == 0);

        if( count == 0 )
        {
            LOGW << item << " has no assigned searcher" << std::endl;
        }

        if( count > typeSearcherCount[item.type] )
            typeSearcherCount[item.type] = count;

        searchedItems.push_back(item);

        std::vector<Item>::iterator foundSumIt;

        for( foundSumIt = foundSum.begin(); foundSumIt != foundSum.end(); foundSumIt++ )
        {
            //We sometimes get broken paths from the files (bad case etc)
            foundSumIt->VerifyPath();
            RunRecursiveSearchOn( *foundSumIt );
        }

        if( foundSum.size() > 0 )
            LOGD << "Found " << foundSum.size() << " in " << item << std::endl;
        foundItems.insert(foundSum.begin(), foundSum.end());
    }
}

void JobHandler::AddItem(const std::string& input_folder, const std::string& path, const std::string& type, const JobXMLParser::Item& source)
{
    LOGD << "Adding " << type << ": " << path << std::endl; 
    items.push_back(Item(input_folder,path,type,source)); 
}

std::vector<Item>::const_iterator JobHandler::ItemsBegin() const
{
    return items.begin();
}

std::vector<Item>::const_iterator JobHandler::ItemsEnd() const
{
    return items.end();
}

bool JobHandler::HasItemWithPath( const std::string& path )
{
    std::vector<Item>::iterator itemit = items.begin();
    for( ; itemit != items.end(); itemit++ )
    {
        if( itemit->path == path )
        {
            return true;
        }
    }
    return false;
}
