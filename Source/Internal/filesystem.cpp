//-----------------------------------------------------------------------------
//           Name: filesystem.cpp
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

#include <Internal/modloading.h>
#include <Internal/profiler.h>
#include <Internal/checksum.h>
#include <Internal/datemodified.h>
#include <Internal/filesystem.h>
#include <Internal/common.h>
#include <Internal/error.h>

#include <Utility/assert.h>
#include <Utility/strings.h>

#include <Compat/fileio.h>
#include <Compat/compat.h>

#include <Memory/allocation.h>
#include <Logging/logdata.h>

#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <iterator>
#include <set>
#include <algorithm>
#include <cerrno>

using std::string;
using std::endl;
using std::vector;
using std::ifstream;
using std::ios;
using std::streampos;
using std::istream_iterator;
using std::stringstream;
using std::pair;
using std::size_t;
using std::make_pair;
using std::stringstream;
using std::set;
using std::sort;

Paths vanilla_data_paths;
Paths mod_paths;
char write_path[kPathSize];
static bool write_path_set = false;

Paths::Paths() : num_paths(0) {
    
}

int Paths::AddPath( const char* path ) {
    RemovePath(path);
    string withTrailing = SanitizePath(AssemblePath(string(path),string()));
    if(num_paths < kMaxPaths) {
        FormatString(paths[num_paths], kPathSize, "%s", withTrailing.c_str());
        mod_path_ids[num_paths] = CoreGameModID;
        ++num_paths;
        return num_paths-1;
    } else {

        LOGE << "All preallocated filesystem paths are utilized, this isn't intended to occur, contact developer." << endl;

        LOGE << "Printing all currently registered paths, are there duplicates, empty or invalid entries?" << endl;
        for( int i = 0; i < kMaxPaths; i++ ) {
            LOGE << i << ":" << paths[i] << endl;
        }

        LOG_ASSERT(false);

        return -1;
    }
}

bool Paths::AddModPath( const char* path, ModID modid ) {
    string withTrailing = SanitizePath(AssemblePath(string(path),string()));
    int id = AddPath(withTrailing.c_str());
    if( id >= 0 && id < Paths::kMaxPaths ) {
        mod_path_ids[id] = modid;
        return true;
    } else {
        LOGE << "Problem loading mod" << endl;
        return false;
    }
}

void Paths::RemovePath( const char* path ) {
    string withTrailing = SanitizePath(AssemblePath(string(path),string()));
    bool push_down = false; 
    for( int i = 0; i < num_paths; i++ ) {
        if( push_down )
        {
            memcpy( paths[i-1], paths[i], kPathSize );
            mod_path_ids[i-1] = mod_path_ids[i];
        }

        if( strcmp( withTrailing.c_str(), paths[i] ) == 0 )
        {
            push_down = true;
        }
    }

    //If we found and removed a path by pushing down all after it, we count down.
    if( push_down )
    {
        num_paths--;
    }
}

Path FindImagePath( const char* path, PathFlagsBitfield flags, bool is_necessary )
{
    Path p;

    strcpy( p.original, path );

    if( FindImagePath(p.original, p.resolved, kPathSize, flags, is_necessary, &p.source) == 0 )
    {
        p.valid = true;
    }
    else
    {
        p.valid = false;
    }
    
    return p;

}

Path FindFilePath( const string& path, PathFlagsBitfield flags, bool is_necessary )
{
    return FindFilePath( path.c_str(), flags, is_necessary );
}

Path FindFilePath( const char* path, PathFlagsBitfield flags, bool is_necessary )
{
    Path p;

    strcpy( p.original, path );

    if( FindFilePath(p.original, p.resolved, kPathSize, flags, is_necessary, &p.source, &p.mod_source) == 0 )
    {
        p.valid = true;
    }
    else
    {
        p.valid = false;
    }
    
    return p;
}

const char* overgrowth_dds_cache_intro = "OVERGROWTH_DDS_CACHE";
extern const int overgrowth_dds_cache_version;
const int overgrowth_dds_cache_version = 1;

//The case for looking for both the normal image and the converted became so common I felt the need for this simplification.

//Currently ignoring the modsource, should set it to the correct source if it's not null TODO
int FindImagePath( const char* path, char* buf, int buf_size, PathFlagsBitfield flags, bool is_necessary, PathFlags* resulting_path, bool allow_crn, bool allow_dds, ModID* modsource )
{
    const char* fallback = "Data/Textures/error.tga";
    // We might want a converted image, let's assume it's priority for reasons like performance
    string dds_converted = string(path) + "_converted.dds";
    string crn_converted = string(path) + "_converted.crn";

    const int kMaxPaths = 5;
    char dds_paths[kPathSize * kMaxPaths];
    char orig_paths[kPathSize * kMaxPaths];
    PathFlags dds_flags[kMaxPaths];
    ModID dds_modsources[kMaxPaths];
    PathFlags orig_flags[kMaxPaths];
    ModID orig_modsources[kMaxPaths];
    int num_dds_paths_found = 0;
    if( allow_dds) {
        num_dds_paths_found = FindFilePaths( dds_converted.c_str(), dds_paths, buf_size, kMaxPaths, flags, false, dds_flags, dds_modsources );
    }
    if (num_dds_paths_found == 0 && allow_crn) {
        num_dds_paths_found = FindFilePaths( crn_converted.c_str(), dds_paths, buf_size, kMaxPaths, flags, false, dds_flags, dds_modsources );
    }
    int num_orig_paths_found = FindFilePaths( path, orig_paths, buf_size, kMaxPaths, flags, false, orig_flags, orig_modsources );
    int64_t dds_date_modified[kMaxPaths];
    int64_t orig_date_modified[kMaxPaths];
    int64_t latest_dds_modified = -1;
    int latest_dds_id = -1;
    int64_t latest_orig_modified = -1;
    int latest_orig_id = -1;
    for(int i=0; i<num_dds_paths_found; ++i){
        dds_date_modified[i] = GetDateModifiedInt64(&dds_paths[i*buf_size]);
        if(dds_date_modified[i] > latest_dds_modified){
            latest_dds_modified = dds_date_modified[i];
            latest_dds_id = i;
        }
    }
    for(int i=0; i<num_orig_paths_found; ++i){
        orig_date_modified[i] = GetDateModifiedInt64(&orig_paths[i*buf_size]);
        if(orig_date_modified[i] > latest_orig_modified){
            latest_orig_modified = orig_date_modified[i];
            latest_orig_id = i;
        }
    }

    if(latest_orig_modified > latest_dds_modified && num_orig_paths_found > 0 ){
        FormatString(buf, buf_size, "%s", &orig_paths[latest_orig_id * kPathSize]);
        if(resulting_path) { 
            *resulting_path = orig_flags[latest_orig_id];
        } 
        if(modsource) {
            *modsource = orig_modsources[latest_orig_id];
        }
        if(num_dds_paths_found > 0){
            unsigned short checksum = Checksum(&orig_paths[latest_orig_id * kPathSize]);
            FILE *file = my_fopen(&dds_paths[latest_dds_id * kPathSize], "rb");
            if(file){
                int intro_len = strlen(overgrowth_dds_cache_intro);
                char* intro_check = (char*)OG_MALLOC(intro_len+1);
                intro_check[intro_len] = 0;
                int version_check;
                unsigned short checksum_check;
                fseek(file, -(intro_len + sizeof(version_check) + sizeof(checksum)), SEEK_END);
                fread(intro_check, intro_len, 1, file);
                fread(&version_check, sizeof(version_check), 1, file);
                fread(&checksum_check, sizeof(checksum), 1, file);
                fclose(file);
                if(strcmp(intro_check, overgrowth_dds_cache_intro) == 0 && overgrowth_dds_cache_version == version_check && checksum == checksum_check){
                    FormatString(buf, buf_size, "%s", &dds_paths[latest_dds_id * kPathSize]);
                    if(resulting_path) { 
                        *resulting_path = dds_flags[latest_dds_id];
                    } 
                    if(modsource) {
                        *modsource = dds_modsources[latest_dds_id];
                    }
                }
                OG_FREE(intro_check);
            }
        } else {

        }
        return 0;
    } else if(latest_dds_modified >= latest_orig_modified && num_dds_paths_found > 0) {
        FormatString(buf, buf_size, "%s", &dds_paths[latest_dds_id * kPathSize]);
        if(resulting_path) { 
            *resulting_path = dds_flags[latest_dds_id];
        } 
        if(modsource) {
            *modsource = dds_modsources[latest_dds_id];
        }
        return 0;
        /*
    } else if( strcmp(fallback,path) != 0 ) {
#ifndef NO_ERR
        char err[kPathSize];
        FormatString(err, kPathSize, "Could not find %s, loading fallback", path);
        if( is_necessary )
        {
            DisplayError("Error", err);
        }
        else
        {
            LOGW << path << endl;
        }
#endif
        FindImagePath( fallback, buf, buf_size, flags, true, resulting_path, modsource );
        return -1;
        */
    } else {
        return -1;
    }
}

int FindFilePath(const char* path, char* buf, int buf_size, PathFlagsBitfield flags, bool is_necessary, PathFlags* resulting_path, ModID* modsource) {
	PROFILER_ZONE(g_profiler_ctx, "FindFilePath");
    if(flags & kModPaths){
        for(int i=0; i<mod_paths.num_paths; ++i){
            AssemblePath( mod_paths.paths[i], path, buf, buf_size );
            caseCorrect(buf);
            if(CheckFileAccess(buf)){
                if( resulting_path ) {
                    *resulting_path = kModPaths;
                }

                if( modsource ) {
                    *modsource = mod_paths.mod_path_ids[i];
                }
                return 0;
            }
        }
    }
    if(flags & kDataPaths){
        for(int i=0; i<vanilla_data_paths.num_paths; ++i){
            AssemblePath( vanilla_data_paths.paths[i], path, buf, buf_size );
            caseCorrect(buf);
            if(CheckFileAccess(buf)){
                if( resulting_path ) {
                    *resulting_path = kDataPaths;
                }

                if( modsource ) {
                    *modsource = vanilla_data_paths.mod_path_ids[i];
                }
                return 0;
            }
        }
    }
    if(flags & kModWriteDirs){
        for(int i=0; i<mod_paths.num_paths; ++i) {
            string mod_write_path = GetWritePath(mod_paths.mod_path_ids[i]);
            AssemblePath( mod_write_path.c_str(), path, buf, buf_size );
            // Not case-correcting write dir for now, since it might interact badly with the many uses of the writedir not using FindFilePath
            if(CheckFileAccess(buf)) {
                if( resulting_path ) {
                    *resulting_path = kModWriteDirs;
                }

                if( modsource ) {
                    *modsource = mod_paths.mod_path_ids[i];
                }
                return 0;
            }
        }
    }
    if(flags & kWriteDir){
        AssemblePath( write_path, path, buf, buf_size );
        // Not case-correcting write dir for now, since it might interact badly with the many uses of the writedir not using FindFilePath
        if(CheckFileAccess(buf)){
            if( resulting_path )
            {
                *resulting_path = kWriteDir;
            }

            if( modsource ) {
                *modsource = CoreGameModID;
            }
            return 0;
        }
    }

    //Check if the path given "just works" assuming that it could be an absolute path
    if(flags & kAbsPath) {
        strncpy(buf, path, buf_size);
        caseCorrect(buf);
        if( CheckFileAccess(buf) )
        {
			//LOGW << "Got file via absolute path: " << path << ", was this intended?" << endl;
            if( resulting_path )
            {
                *resulting_path = kAbsPath;
            }
            if( modsource ) {
                *modsource = CoreGameModID;
            }
            return 0;
        }
    }

    if( is_necessary )
    {
        LOGE << "Unable to find \"" << path << "\" in any of the requested locations." << endl;
    }

    if( resulting_path )
    {
        *resulting_path = kNoPath;
    }

    return -1;
}


int FindFilePaths(const char* path, char* bufs, int buf_size, int num_bufs, PathFlagsBitfield flags, bool is_necessary, PathFlags* resulting_paths, ModID* sourceids) {
    int num_paths_found = 0;
    char* buf = bufs;
    if(flags & kModPaths){
        for(int i=0; i<mod_paths.num_paths; ++i){
            AssemblePath( mod_paths.paths[i], path, buf, buf_size );
            caseCorrect(buf);
            if(CheckFileAccess(buf)){
                if( resulting_paths )
                {
                    resulting_paths[num_paths_found] = kModPaths;
                }
                if( sourceids ) 
                {
                    sourceids[num_paths_found] = mod_paths.mod_path_ids[i];
                }
                ++num_paths_found;
                if(num_paths_found >= num_bufs){
                    return num_paths_found;
                }
                buf += buf_size;
            }
        }
    }
    if(flags & kDataPaths){
        for(int i=0; i<vanilla_data_paths.num_paths; ++i){
            AssemblePath( vanilla_data_paths.paths[i], path, buf, buf_size );
            caseCorrect(buf);
            if(CheckFileAccess(buf)){
                if( resulting_paths )
                {
                    resulting_paths[num_paths_found] = kDataPaths;
                }
                if( sourceids ) 
                {
                    sourceids[num_paths_found] = vanilla_data_paths.mod_path_ids[i];
                }
                ++num_paths_found;
                if(num_paths_found >= num_bufs){
                    return num_paths_found;
                }
                buf += buf_size;
            }
        }
    }
    if(flags & kModWriteDirs) {
        for(int i=0; i<mod_paths.num_paths; ++i) {
            string mod_write_path = GetWritePath(mod_paths.mod_path_ids[i]);
            AssemblePath( mod_write_path.c_str(), path, buf, buf_size );
            // Not case-correcting write dir for now, since it might interact badly with the many uses of the writedir not using FindFilePath
            if(CheckFileAccess(buf)) {
                if(resulting_paths) {
                    resulting_paths[num_paths_found] = kModWriteDirs;
                }
                if( sourceids ) 
                {
                    sourceids[num_paths_found] = mod_paths.mod_path_ids[i];
                }
                ++num_paths_found;
                if(num_paths_found >= num_bufs) {
                    return num_paths_found;
                }
                buf += buf_size;
            }
        }
    }
    if(flags & kWriteDir){
        AssemblePath( write_path, path, buf, buf_size );
        // Not case-correcting write dir for now, since it might interact badly with the many uses of the writedir not using FindFilePath
        if(CheckFileAccess(buf)){
            if( resulting_paths )
            {
                resulting_paths[num_paths_found] = kWriteDir;
            }

            if( sourceids ) 
            {
                sourceids[num_paths_found] = CoreGameModID;
            }

            ++num_paths_found;
            if(num_paths_found >= num_bufs){
                return num_paths_found;
            }
            buf += buf_size;
        }
    }

    //Check if the path given "just works" assuming that it could be an absolute path
    if(flags & kAbsPath){
        if( CheckFileAccess(path) )
        {
            strncpy(buf, path, buf_size);
            if(num_paths_found == 0){
                //LOGW << "Got file via absolute path: " << path << ", was this intended?" << endl;
            }
            if( resulting_paths )
            {
                resulting_paths[num_paths_found] = kAbsPath;
            }
            if( sourceids ) 
            {
                sourceids[num_paths_found] = CoreGameModID;
            }
            ++num_paths_found;
            if(num_paths_found >= num_bufs){
                return num_paths_found;
            }
            buf += buf_size;
        }
    }

    if( is_necessary && num_paths_found == 0 )
    {
        LOGE << "Unable to find \"" << path << "\" in any of the requested locations." << endl;
    }

    return num_paths_found;
}

void AddPath(const char* path, PathFlags type) {
	//LOGI << "Adding path " << path << endl;
    string withTrailing = AssemblePath(string(path),string());
    switch(type){
    case kWriteDir:
        LOGI << "Adding write path " << withTrailing << endl;
        FormatString(write_path, kPathSize, "%s", withTrailing.c_str());
        write_path_set = true;
        break;
    case kModWriteDirs:
        LOGI << "It's invalid to add specific write dir for mods, it's relative to main write path." 
             << endl;
        break;
    case kDataPaths:
        LOGI << "Adding data path " << withTrailing << endl;
        vanilla_data_paths.AddPath(withTrailing.c_str());
        break;
    case kModPaths:
        LOG_ASSERT(false);
        LOGE << "Can't add mod path with this function " << endl;
        break;
    case kAbsPath:
        LOGW << "This can't be done" << endl;
        break;
    case kAnyPath:
        LOGW << "This can't be done" << endl;
        break;
    case kNoPath:
        LOGE << "Error, invalid path set" << endl;
        break;
    }
}

bool AddModPath( const char* path, ModID modid ) {
    return mod_paths.AddModPath(path, modid);
}


void RemovePath( const char* path, PathFlags type )
{
    if( type & kDataPaths )
    {
        vanilla_data_paths.RemovePath(path);
    }

    if( type & kModPaths )
    {
        mod_paths.RemovePath(path);
    }

}

const char* GetDataPath( int i ) {
    if( i < vanilla_data_paths.num_paths ) {
        return vanilla_data_paths.paths[i];
    } else {
        return "";
    }
}

string GetWritePath( const ModID& id ) {
#if defined(OGDA) || defined(OG_WORKER)
    return string(write_path);
#else 
    if( id.Valid() ) {
        if( id != CoreGameModID ) {
            ModInstance *modi = ModLoading::Instance().GetMod(id);
            if( modi ) {
                return AssemblePath(string(write_path),"ModsDataCache",string(modi->id)) + "/";
            } else {
                LOGF << "Got valid mod id to non existant mod" << endl;
            }
        }
    } else {
        LOGF << "Got invalid mod id" << endl;
    }
    return string(write_path);
#endif
}

void GetWritePath( const ModID& id, char * buf, size_t len ) {
    //Fastpath to make the coredump routine a little more robust.
    if( id == CoreGameModID ) {
        strscpy(buf,write_path,len);
    } else {
        string wp = GetWritePath(id); 
        strscpy(buf,wp.c_str(),len);
    }
}

bool FileExists(const Path& path) {
    return path.isValid() && FileExists( path.resolved, kAbsPath );
}

bool FileExists(const string& path, PathFlagsBitfield flags) {
    return FileExists( path.c_str(), flags );
}

void CreateDirsFromPath(const string& base, const string & path) {

    auto temp = SplitPathFileName(path);

    // all dirs in order of occurance
    vector<string> allDirs;
    string pathWithoutFile = temp.first;

    size_t pos = pathWithoutFile.find_first_of("\//");
    while (pos != string::npos) {
        allDirs.push_back(pathWithoutFile.substr(0, pos));
        pathWithoutFile = pathWithoutFile.substr(pos + 1, path.size());
        pos = pathWithoutFile.find_first_of("\//");
    }

    allDirs.push_back(pathWithoutFile); 
    string writtenPath = base;
    for (auto& it : allDirs) { 
        CreateParentDirs(writtenPath + it + string("/"));
        writtenPath += it + string("/");
    }
}

bool FileExists(const char* path, PathFlagsBitfield flags) {
    char temp_path[kPathSize];
    if(FindFilePath(path, temp_path, kPathSize, flags, false) != -1) {
        if( isFile( temp_path )) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

string GetConfigPath()
{
    char config_path[kPathSize];
    GetConfigPath(config_path, kPathSize);
    return string(config_path);
}

void GetConfigPath( char* buf, size_t len )
{
	FormatString(buf, len, "%sData/config.txt", GetWritePath(CoreGameModID).c_str());
}

string GetLogfilePath( )
{
	char logfile_path[kPathSize];
	GetLogfilePath(logfile_path, kPathSize);
	return string(logfile_path);
}

void GetLogfilePath( char * buf, size_t size )
{
	FormatString( buf, size, "%slogfile.txt", GetWritePath(CoreGameModID).c_str());
}

void GetScenGraphDumpPath( char* buf, size_t size ) 
{
	FormatString( buf, size, "%sscene_dump.txt", GetWritePath(CoreGameModID).c_str());
}

void GetASDumpPath( char* buf, size_t size ) 
{
	FormatString( buf, size, "%sas_dump.txt", GetWritePath(CoreGameModID).c_str());
}

string GetHWReportPath()
{
	char hwreport_path[kPathSize];
	GetHWReportPath( hwreport_path, kPathSize );
	return string(hwreport_path);
}

void GetHWReportPath( char * buf, size_t size )
{
	FormatString(buf, size, "%sData/hwreport.txt", GetWritePath(CoreGameModID).c_str());
}

bool has_suffix(const string &str, const string &suffix)
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

string GuessMime( string path )
{
    if( has_suffix( path, ".html" ) || has_suffix( path, ".htm" ) )
    {
        return "text/html";
    }

    if( has_suffix( path, ".js" ) )
    {
        return "application/javascript";
    }

    if( has_suffix( path, ".jpg" ) || has_suffix( path, ".jpeg" ) )
    {
        return "image/jpeg";
    }

    if( has_suffix( path, ".png" ) )
    {
        return "image/png";
    }

    if( has_suffix( path, ".gif" ) )
    {
        return "image/gif";
    }

    if( has_suffix( path, ".json" ) )
    {
        return "application/json";
    }

    if( has_suffix( path, ".ttf" ) )
    {
        return "application/x-font-ttf";
    }

    if( has_suffix( path, ".css" ) )
    {
        return "text/css";
    }

    if( has_suffix( path, ".svg" ) )
    {
        return "image/svg+xml";
    }

    LOGE << "Unknown mime type for " << path << endl;  
    return "text/plain";
    
}

string StripDriveletter( string path )
{
	if( isupper(path[0]) && path[1] == ':' )
	{
		return path.substr(4);
	}
	return path;
}

vector<unsigned char> readFile(const char* filename)
{
    /*
    TODO: Fix this faster variant
	vector<unsigned char> vec;
    ifstream in(filename, ios::in | ios::binary);
    if (in)
    {
        string contents;
        in.seekg(0, ios::end);
        vec.resize(in.tellg());
        in.seekg(0, ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
    }
	else
	{
		LOGF << "Unable to read file" << filename << endl;
	}
    return vec;
     */

    // open the file:
	vector<unsigned char> vec;

    if( isFile( filename ) )
    {
        ifstream file;
        my_ifstream_open(file, filename, ios::binary);
        
        if( file.good() )
        {
            // Stop eating new lines in binary mode!!!
            file.unsetf(ios::skipws);

            // get its size:
            streampos fileSize;

            file.seekg(0, ios::end);
            fileSize = file.tellg();
            file.seekg(0, ios::beg);

            // reserve capacity		
            vec.reserve(fileSize);

            // read the data:
            vec.insert(vec.begin(),
                       istream_iterator<unsigned char>(file),
                       istream_iterator<unsigned char>());
        }
        else
        {
            LOGE << "Unable to read file " << filename << endl;
        }
        file.close();
    }

    return vec;
}    

void CreateParentDirs( const string& abs_path )
{ 
    CreateParentDirs(abs_path.c_str());
}

bool CheckFileAccess(const char* path){ 
    return checkFileAccess(path);
}

void CreateParentDirs(const char* abs_path) { 
	createParentDirs(abs_path);
}

void GenerateManifest( const char* path, vector<string>& files )
{
    getDeepManifest( path, "", files ); 
}

const string GetPathFlagsStr( PathFlagsBitfield f )
{
    stringstream ss;
    if( f & kDataPaths ) {
        ss << "kDataPaths ";
    }
    if( f & kWriteDir ) {
        ss << "kWriteDir ";
    }
    if( f & kModPaths ) {
        ss << "kModPaths ";
    }
    if( f & kModWriteDirs ) {
        ss << "kModWriteDirs ";
    }
    if( f & kAbsPath ) {
        ss << "kAbsPath ";
    }
    return ss.str();
}

string AssemblePath( const char* first, const char* second ) 
{
    return AssemblePath(string(first), string(second));
}

void AssemblePath( const char* first, const char* second, char* out, size_t outsize )
{
    if ( strlen(first) == 0 )
    {
        LOGE << "AssemblePath was provided with a zero length string" << endl;
        FormatString(out, outsize, "%s", second);
        return;
    }

    if( first[strlen(first)-1] == '/' || second[0] == '/' )
    {
        FormatString(out, outsize, "%s%s", first, second);
    }
    else
    {
        FormatString(out, outsize, "%s/%s", first, second);
    }
}

string AssemblePath( const string& first, const string& second )
{
    if ( first.empty() )
    {
        LOGW << "AssemblePath was provided with a zero length string" << endl;
        return second;
    }

    if( first[first.length()-1] == '/' || second[0] == '/' )
    {
        return first + second;
    }
    else
    {
        return first + "/" + second;
    }
}

string AssemblePath( const string& first, const string& second, const string& third )
{
    return AssemblePath(AssemblePath(first,second),third);
}

pair< string, string > SplitPathFileName( char* fullPath )
{
    string fP( fullPath );
    return SplitPathFileName( fP );
}

pair< string, string > SplitPathFileName( string const& fullPath )
{
    size_t pos = fullPath.find_last_of("/\\");
    
    if( pos == string::npos )
    {
        return make_pair( "", fullPath );
    }
    else 
	{
        return make_pair( fullPath.substr(0,pos), fullPath.substr(pos+1) );
    }
}

int CheckWritePermissions(const char* dir)
{
    char buffer[kPathSize];
    sprintf(buffer, "%s/ogtestfile.tmp", dir);
    NormalizePathSeparators(buffer);
    int error = createfile(buffer);
    if(error == 0) {
        deletefile(buffer);
        return 0;
    } else {
        return error;
    }
}

int copyfile( const string& source, const string& dest )
{
    return os_copyfile( source.c_str(), dest.c_str() );
}

int copyfile( const char* source, const char* dest )
{
    return os_copyfile( source, dest );
}

int movefile( const char* source, const char* dest )
{
    return os_movefile(source, dest);
}

int deletefile(const char* filename)
{
    return os_deletefile(filename);
}

int createfile(const char* filename)
{
    return os_createfile(filename);
}

int fileexists(const char* filename)
{
    return os_fileexists(filename);
}

string DumpIntoFile( const void *buf, size_t nbyte )
{
    return dumpIntoFile( buf, nbyte );
}

string CaseCorrect(const string& v)
{
    char new_path[kPathSize];
    strncpy(new_path,v.c_str(),kPathSize);
    caseCorrect(new_path);
    return string(new_path);
}

string ApplicationPathSeparators( const string& v )
{
	size_t slash_pos = 0;
	string res = v;

	while( (slash_pos = res.find_first_of("\\")) != string::npos )
	{
		res[slash_pos] = '/';
	}

	return res;
}

void ApplicationPathSeparators( char* string )
{
	for( int i = 0; string[i] != '\0'; i++ )
	{
		if( string[i] == '\\' )
			string[i] = '/';
	}
}

string NormalizePathSeparators( const string& v)
{
	size_t slash_pos = 0;
	string res = v;
#ifdef WIN32
	while( (slash_pos = res.find_first_of("/")) != string::npos )
	{
		res[slash_pos] = '\\';
	}
#else
	while( (slash_pos = res.find_first_of("\\")) != string::npos )
	{
		res[slash_pos] = '/';
	}
#endif
	return res;
}

void NormalizePathSeparators( char* string )
{
	for( int i = 0; string[i] != '\0'; i++ )
	{
#ifdef WIN32
		if( string[i] == '/' )
			string[i] = '\\';
#else
		if( string[i] == '\\' )
			string[i] = '/';
#endif
	}
}

bool AreSame( const char* path1, const char* path2 )
{
	return areSame(path1,path2);
}

Path FindShortestPath2(const string& p1)
{
    string v = SanitizePath(p1);

	Path match;
	string current_path = v;
	size_t next_slash = 0;
    int path = kAnyPath;
    bool try_again = false;

	do
	{
        try_again = false;
        Path new_path = FindFilePath( current_path.c_str(), path, false );
		if( new_path.isValid() )
		{
			if( AreSame(new_path.GetFullPath(), v.c_str() ) )
			{
				match = new_path;
				LOGI << "Found a match for " << v << " that is " << current_path << " in " << new_path << endl;
			}
            else
            {
                try_again = true;
                path &= ~new_path.source;
            }
		}

        if(!try_again)
        {
            next_slash = current_path.find_first_of("/");
            if( next_slash != string::npos )
            {
                current_path = current_path.substr(next_slash+1);
            }
        }
	}
	while(next_slash != string::npos);

	LOGI << "Chose " << match << endl;
	return match;
}

string FindShortestPath(const string& string)
{
    return FindShortestPath2(string).GetOriginalPathStr();
}

static bool string_sort_by_length_r( string a, string b )
{
    return a.length() > b.length();
}

/* 
 * Smart-ish cache clearer, removes files of known types from the local write dir. Intended to function as a sort of sanitation fallback.
 */
void ClearCache( bool dry_run )
{
    if( write_path_set == false )
    {
        LOGE << "Write path is not set, will not clear cache" << endl;
        return;
    }

    if( !dry_run )
    {
        LOGI << "Clearing Cache Data (For Real)..." << endl;
    }
    else
    {
        LOGI << "Clearing Cache Data (Dry Run)..." << endl;
    }

    vector<ModID> modids;
#if !(defined(OGDA) || defined(OG_WORKER))
    modids = ModLoading::Instance().GetModsSid();
#endif
    modids.push_back(CoreGameModID);
    for(auto & modid : modids) 
    {
        stringstream write_data_path;
        write_data_path << GetWritePath(modid);
        write_data_path << "Data/";

        vector<string> write_dir_files;

        vector<pair<string,string> > exclude_with_ending;
        
        exclude_with_ending.push_back(pair<string,string>("Mods",""));

        vector<pair<string,string> > remove_with_ending;
        //Clearly cache files
        remove_with_ending.push_back(pair<string,string>("",".mcache"));
        remove_with_ending.push_back(pair<string,string>("",".cache"));
        remove_with_ending.push_back(pair<string,string>("",".lod_cache"));
        remove_with_ending.push_back(pair<string,string>("",".tga_image_sample_cache"));
        remove_with_ending.push_back(pair<string,string>("",".tga_avg_color_cache"));
        remove_with_ending.push_back(pair<string,string>("",".png_image_sample_cache"));
        remove_with_ending.push_back(pair<string,string>("",".png_hmcache"));
        remove_with_ending.push_back(pair<string,string>("",".png_hmcache_scaled"));

        //Probably cache files
        remove_with_ending.push_back(pair<string,string>("",".tga_converted.dds"));
        remove_with_ending.push_back(pair<string,string>("",".png_converted.dds"));
        remove_with_ending.push_back(pair<string,string>("",".jpg_converted.dds"));

        //Specific subcategories
        remove_with_ending.push_back(pair<string,string>("Prototypes","weights.png"));
        remove_with_ending.push_back(pair<string,string>("Textures","weights.png"));

        remove_with_ending.push_back(pair<string,string>("Prototypes","cube.dds"));
        remove_with_ending.push_back(pair<string,string>("Textures","cube.dds"));

        remove_with_ending.push_back(pair<string,string>("Prototypes","cube_blur.dds"));
        remove_with_ending.push_back(pair<string,string>("Textures","cube_blur.dds"));

        remove_with_ending.push_back(pair<string,string>("Prototypes",".png_normal.png"));
        remove_with_ending.push_back(pair<string,string>("Textures",".png_normal.png"));

        remove_with_ending.push_back(pair<string,string>("Prototypes",".png.obj"));
        remove_with_ending.push_back(pair<string,string>("Textures",".png.obj"));

        remove_with_ending.push_back(pair<string,string>("ScriptBytecode",".bytecode"));

        //Older level cache files.
        remove_with_ending.push_back(pair<string,string>("Levels",".nav"));
        remove_with_ending.push_back(pair<string,string>("Levels",".nav.obj"));
        remove_with_ending.push_back(pair<string,string>("Levels",".nav.xml"));

        //Newer cache files
        remove_with_ending.push_back(pair<string,string>("LevelNavmeshes",".nav"));
        remove_with_ending.push_back(pair<string,string>("LevelNavmeshes",".nav.obj"));
        remove_with_ending.push_back(pair<string,string>("LevelNavmeshes",".nav.xml"));

        remove_with_ending.push_back(pair<string,string>("Temp","navmesh.obj"));
        remove_with_ending.push_back(pair<string,string>("Temp","terrainlow.obj"));

        remove_with_ending.push_back(pair<string,string>("Models",".hull"));

        remove_with_ending.push_back(pair<string,string>("BattleJSONDumps",".json"));

        remove_with_ending.push_back(pair<string,string>("CompiledShaders",".frag"));
        remove_with_ending.push_back(pair<string,string>("CompiledShaders",".vert"));
        remove_with_ending.push_back(pair<string,string>("CompiledShaders",".geom"));
        remove_with_ending.push_back(pair<string,string>("CompiledShaders",".glsl"));

        GenerateManifest( write_data_path.str().c_str(), write_dir_files );

        vector<string> keep_files;
        vector<string> remove_files;

        for(auto & write_dir_file : write_dir_files) 
        {
            bool found = false;
            for(auto & fit : remove_with_ending)
            {
                if( hasBeginning( write_dir_file, fit.first) && hasEnding( write_dir_file, fit.second ) )
                {
                    found = true;
                }
            }

            if( found ) 
            {
                /* Second pass with exclusion, if there's one match, regret inclusion, exclusion overrides */
                for(auto & fit : exclude_with_ending)
                {
                    if( hasBeginning( write_dir_file, fit.first) && ( fit.second.empty() || hasEnding( write_dir_file, fit.second ) ) )
                    {
                        found = false;
                    }
                }
            }

            if( found )
            {
                remove_files.push_back( write_dir_file );
            }
            else
            {
                keep_files.push_back( write_dir_file );
            }
        }

        for(auto & remove_file : remove_files)
        {
            string path = write_data_path.str() + remove_file;
            LOGI << "Removing: " << path  << endl;
            if( dry_run == false )
            {
                if( deletefile(path.c_str()) != 0 )
                {
                    LOGE << "Error removing: " << path << " reason:" << strerror(errno) << endl;
                }
            }
        }  

        //Now we want to remove all the empty folders resulting from this purge.
        if( dry_run == false )
        {
            //Create a set with all unique folder paths
            set<string> folder_paths; 
            for(auto & remove_file : remove_files)
            {
                string base = SplitPathFileName( remove_file ).first;

                while( base.empty() == false )
                {
                    folder_paths.insert( base );
                    base = SplitPathFileName( base ).first;
                }
            }

            vector<string> empty_folder_paths;
            set<string>::iterator folder_path_it = folder_paths.begin();
            for( ;folder_path_it != folder_paths.end(); folder_path_it++ )
            {
                vector<string> temp_l_1;
                GenerateManifest((write_data_path.str()+(*folder_path_it)).c_str(), temp_l_1);

                if( temp_l_1.size() == 0 )
                {
                    empty_folder_paths.push_back(*folder_path_it); 
                }
            }

            sort(empty_folder_paths.begin(), empty_folder_paths.end(), string_sort_by_length_r );

            for(auto & empty_folder_path : empty_folder_paths)
            {
                string path = write_data_path.str() + empty_folder_path;
                LOGI << "Removing Empty Folder:" << path << endl;
                if( deletefile(path.c_str()) != 0 )
                {
                    LOGE << "Error removing: " << path << " reason:" << strerror(errno) << endl;
                    
                }
            }
        }

        for(auto & keep_file : keep_files)
        {
            string path = write_data_path.str() + keep_file;
            LOGI << "Keeping: " << path << endl;
        }
    }
}

string GetMorphPath(const string &base_model, const string &morph){
    return base_model.substr(0, base_model.size()-4) +  "_" + morph + ".obj";
}

bool IsFile(Path& p) {
    return isFile(p.GetAbsPathStr().c_str());
}

bool IsImageFile(Path &image) {
    static const char* ending_arr[] = {
        ".png",
        ".dds",
        ".crn",
        ".jpg",
        ".jpeg",
        ".PNG",
        ".JPG",
        ".JPEG",
        ".DDS",
        ".CRN",
        NULL
    };
   
    bool has_match = false;
    const char * str = image.GetOriginalPath();
    unsigned c = 0;
    while( ending_arr[c] != NULL ) {
        if( endswith(str,ending_arr[c]) ){
            has_match = true; 
            break;
        }
        c++;
    }

    return has_match && IsFile(image);
}


string SanitizePath( const string& path ) {
    return SanitizePath(path.c_str());
}

string SanitizePath( const char* path ) {
    if( path == NULL ) {
        LOGE << "Told to sanitize null pointer path, returning empty" << endl;
        return string();
    } else if(IsPathSane(path)) {
        return string(path);
    } else  {
        const string app_path = ApplicationPathSeparators(string(path));
        string ret; 
        ret.reserve(app_path.size());
        for( unsigned i = 0; i < app_path.size(); i++ ) {
            if(i == 0 || ((app_path[i-1] == '/' && app_path[i] == '/') == false)) {
                ret.push_back(app_path[i]);
            }
        }
        return ret;
    }
}

bool IsPathSane( const char* path ) {
    bool ret = true;
    for( unsigned i = 0; i < strlen(path); i++ ) {
        if(path[i] == '\\') {
            ret = false;
        }
        if( i != 0 ) {
            if( path[i-1] == '/' && path[i] == '/' ) {
                ret = false;
            }
        }
    } 
    return ret;
}

string GenerateParallelPath( const char* base, const char* target, const char* postfix, Path asset ) {
    LOG_ASSERT(strlen(base)>0);
    LOG_ASSERT(strlen(target)>0);
    LOG_ASSERT(base[0] != '/'); 
    LOG_ASSERT(base[strlen(base)-1] != '/');
    LOG_ASSERT(target[0] != '/'); 
    LOG_ASSERT(target[strlen(target)-1] != '/');
    LOG_ASSERT(IsPathSane(base));
    LOG_ASSERT(IsPathSane(target));
    LOG_ASSERT(asset.isValid());
     
    Path shortest_path = FindShortestPath2(asset.GetFullPath());
    string shortest_path_orig_folder = SplitPathFileName(shortest_path.GetOriginalPath()).first;
    string filename = RemoveFileEnding(SplitPathFileName(shortest_path.GetOriginalPath()).second);

    if( pstrmtch(shortest_path_orig_folder.c_str(), base, strlen(base)) ) {
        if( shortest_path_orig_folder.size() > strlen(base)+1 ) {
            if( shortest_path_orig_folder[strlen(base)] == '/' ) {
                shortest_path_orig_folder = &shortest_path_orig_folder.c_str()[strlen(base)+1];
            } else {
                shortest_path_orig_folder = "unknown_source";
            }
        } else {
            shortest_path_orig_folder = "";
        }
    } else {
        shortest_path_orig_folder = "unknown_source";
    }
    
	char nav_path[kPathSize];
    if( shortest_path_orig_folder.size() > 0 ) {
        FormatString(nav_path, kPathSize, "%s/%s/%s%s", target, shortest_path_orig_folder.c_str(), filename.c_str(), postfix);
    } else {
        FormatString(nav_path, kPathSize, "%s/%s%s",target, filename.c_str(), postfix);
    }

    return string(nav_path);
}

uint64_t GetFileSize( const Path& file ) {
    if( file.isValid() ) {
        FILE* f = my_fopen(file.GetFullPath(),"rb");
        if(f) {
            fseek(f,0L,SEEK_END);
            long size_b = ftell(f);
            fclose(f);

            return (uint64_t)size_b;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

void CreateBackup(const char* path, int max_backup_count/*= 10*/)
{
    pair<string, string> path_file_name = SplitPathFileName(path);
    path_file_name.first = ApplicationPathSeparators(path_file_name.first);

    char backup_dir[kPathSize];
    size_t find_index = path_file_name.first.find("Data/");
    if(find_index != 0) {
        find_index = path_file_name.first.find("/Data/");
    }
    if(find_index != string::npos) {
        if(find_index == 0) {
            FormatString(backup_dir, kPathSize, "%sData/backup/%s/", GetWritePath(CoreGameModID).c_str(), path_file_name.first.c_str() + strlen("Data/"));
        } else {
            FormatString(backup_dir, kPathSize, "%sData/backup/%s/", GetWritePath(CoreGameModID).c_str(), path_file_name.first.substr(find_index + strlen("/Data/")).c_str());
        }
    } else {
        FormatString(backup_dir, kPathSize, "%s/Data/backup/", GetWritePath(CoreGameModID).c_str());
    }
    ApplicationPathSeparators(backup_dir);
    CreateParentDirs(backup_dir);

    char backup_path[kPathSize];
    int backup_number = 1;
    for(int max = max_backup_count; backup_number < max; ++backup_number) {
        FormatString(backup_path, kPathSize, "%s%s%d", backup_dir, path_file_name.second.c_str(), backup_number);
        if(fileexists(backup_path) != 0)
            break;
    }

    for(int i = backup_number; i > 1; --i) {
        char target_path[kPathSize];
        FormatString(backup_path, kPathSize, "%s%s%d", backup_dir, path_file_name.second.c_str(), i - 1);
        FormatString(target_path, kPathSize, "%s%s%d", backup_dir, path_file_name.second.c_str(), i);

        if(movefile(backup_path, target_path) != 0) {
            LOGW << "Couldn't move file from " << backup_path << " to " << target_path << " when backing up, trying to copy it instead" << endl;
            if(copyfile(backup_path, target_path) != 0) {
                LOGW << "Couldn't copy file from " << backup_path << " to " << target_path << " when backing up, it will be skipped" << endl;
            }
        }
    }

    LOGI << "Saving file backup to " << backup_path << endl;
    if(movefile(path, backup_path) != 0) {
        if(copyfile(path, backup_path) != 0) {
            LOGW << "Couldn't copy file from " << path << " to " << backup_path << " when backing up, it will be skipped" << endl;
        }
    }
}
