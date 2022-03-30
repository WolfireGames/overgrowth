//-----------------------------------------------------------------------------
//           Name: item.cpp
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

#include "item.h"
#include <iostream>

#include <Utility/hash.h>
#include <Internal/filesystem.h>
#include <Logging/logdata.h>
#include <Internal/datemodified.h>
#include <Ogda/ogda_hash.h>

Item::Item( const std::string& input_folder, const std::string& path, const std::string& type, const JobXMLParser::Item& source ) : 
	input_folder(input_folder), 
	type(type), 
	hash(""), 
	source(source), 
	only_parse(false), 
	delete_on_exit(false), 
	is_overshadowed(false)
{
    SetPath(path);
}

Item::Item( const std::string& input_folder, const std::string& path, const std::string& type, const std::string& hash, const JobXMLParser::Item& source ) : 
	input_folder(input_folder), 
	type(type), 
	hash(hash), 
	source(source), 
	only_parse(false), 
	delete_on_exit(false), 
	is_overshadowed(false)
{
    SetPath(path);
}
    
void Item::SetPath( std::string path )
{
    //This is a bit of a hack, to look for Data, but it's almost part of the standard by now.
    if( path.substr(0,5) == std::string("Data/") )
    {
        this->path = path.substr(5);  
    }
    else if( path.substr(0,5) == std::string("Data\\") )
    {
        this->path = path.substr(5);  
    }
    else if( path.substr(0,5) == std::string("data\\") )
    {
        this->path = path.substr(5);  
    }
    else if( path.substr(0,5) == std::string("data/") )
    {
        this->path = path.substr(5);  
    }
    else
    {
        this->path = path;
    }
}

bool Item::operator==(const JobXMLParser::Item& rhs) const
{
    return this->type == rhs.type && this->path == rhs.path;
}

bool Item::operator==(const Item& rhs) const
{
    return this->hash == rhs.hash && this->path == rhs.path && this->type == rhs.type && this->only_parse == rhs.only_parse; 
}

bool Item::operator<(const Item& rhs) const
{
    if( hash < rhs.hash  )
    {
        return true;
    }
    else if( hash == rhs.hash )
    {
        if( path < rhs.path )
        {
            return true;
        }
        else if( path == rhs.path )
        {
            if( type < rhs.type )
            {
                return true;
            }
            else if( type == rhs.type )
            {
                if( only_parse < rhs.only_parse )
                {
                    return true;
                }
                else if( only_parse == rhs.only_parse )
                {
                    return false;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

bool Item::FileAccess( )
{
    std::string fullpath = GetAbsPath();
    return CheckFileAccess(fullpath.c_str());
}

void Item::CalculateHash()
{
    hash = OgdaGetFileHash(GetAbsPath());
}

std::string Item::GetPath() const
{
    return path;
}

void Item::VerifyPath()
{
    std::string assembled = GetAbsPath();
    if(!CheckFileAccess(assembled.c_str()))
    {
        std::string new_path = CaseCorrect(assembled);

        SetPath(new_path.substr(input_folder.size()));
        LOGD << "Case Corrected path to " << path << std::endl;

        std::string assembled = GetAbsPath();
        if( !CheckFileAccess(assembled.c_str()) )
        {
            LOGE << "File still missing after case correction: " << assembled << std::endl;
        }
    }
}

std::string Item::GetAbsPath() const
{
    if( path[0] == '/' ) //Don't have to correct abs paths because they refer to a generated file.
    {
        return path;
    }
    else
    {
        return AssemblePath(input_folder, path);
    }
}

bool Item::IsOnlySearch()
{
    return only_parse;
}

void Item::SetOnlySearch(bool value)
{
    only_parse = value;
}

bool Item::IsOvershadowed()
{
    return is_overshadowed;
}

void Item::SetOvershadowed(bool value)
{
    is_overshadowed = value;
}

bool Item::Overshadows( const Item& item )
{
    if( false == overshadows.empty() && overshadows == item.GetPath() )
    {
        return true;
    }
    return false;
}

void Item::SetOvershadows( const Item& item )
{
    overshadows = item.GetPath();
}

void Item::SetDeleteOnExit(bool value)
{
    delete_on_exit = value;
}

bool Item::IsDeleteOnExit()
{
    return delete_on_exit;
}

std::ostream& operator<<(std::ostream& out, const Item& item)
{
    return (out << "Item(" << item.path << "," << item.type << "," << item.hash << "," << item.only_parse << "," << item.source << ")");
}

uint64_t Item::GetSubHash() const{ 
    char val[8];
    if( hash.size() > 16 ) {
        for( int i = 0; i < 8; i++ ) {
            val[i] = hash[i*2] * 16 + hash[i*2+1];
        }
    } else {
        LOGW << "Original hash is too small or nonexistant. " << *this << std::endl;
        return 0;
    }
    return *reinterpret_cast<uint64_t*>(val);
}
