//-----------------------------------------------------------------------------
//           Name: scriptfile.cpp
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
#include "scriptfile.h"

#include <Internal/error.h>
#include <Internal/filesystem.h>
#include <Internal/stopwatch.h>
#include <Internal/returnpathutil.h>
#include <Internal/checksum.h>
#include <Internal/config.h>
#include <Internal/profiler.h>

#include <Scripting/scriptlogging.h>
#include <Compat/fileio.h>
#include <Main/engine.h>

#include <errno.h>
#include <algorithm>

using std::min;
using std::max;

extern std::string script_dir_path;

const int _num_script_open_tries = 10;

std::string ScriptFile::GetModPollutionInformation() const {
    std::stringstream ss;
    for( unsigned i = 0; i < dependencies.size(); i++ ) {
        if( dependencies[i].path.GetModsource() != CoreGameModID ) {
            ss << "Includes " << dependencies[i].path << " from mod " << ModLoading::Instance().GetModName(dependencies[i].path.GetModsource()) << "." << std::endl;
        }
    }
    return ss.str();
}

static bool AlreadyAddedIncludeFileInParentHierarchy(const ScriptFile* parent_script_file, const Path &path) {
    bool already_loaded = false;
    while(parent_script_file != NULL){
        if(path == parent_script_file->file_path){
            already_loaded = true;
            break;
        }
        parent_script_file = parent_script_file->parent;
    }
    return already_loaded;
}

bool ScriptFile::AlreadyAddedIncludeFile(const Path &path) {
    bool already_loaded = false;
    for(unsigned i=0; i<dependencies.size(); ++i){
        if(path == dependencies[i].path){
            already_loaded = true;
            break;
        }
    }
    return already_loaded;
}

unsigned GetLineNumber(std::string &script, unsigned char_pos){
    unsigned line_number = 1;
    for(unsigned i=0; i<char_pos; i++){
        if(script[i] == '\n'){
            line_number++;
        }
    }
    return line_number;
}

unsigned GetNumLines(std::string &script){
    unsigned line_number = 1;
    for(unsigned i=0; i<script.size(); i++){
        if(script[i] == '\n'){
            line_number++;
        }
    }
    return line_number;
}

FileRangeList::iterator GetFileRange(FileRangeList& ranges, unsigned line) {
    FileRangeList::iterator iter = ranges.begin();
    FileRangeList::iterator good_iter = ranges.end();
    for(;iter != ranges.end(); ++iter){
        IncludeFileRange& range = (*iter);
        if(line >= range.start){
            good_iter = iter;
        }
    }
    return good_iter;
}

FileRangeList::const_iterator GetFileRange(const FileRangeList& ranges, 
                                           unsigned line) {
    FileRangeList::const_iterator iter = ranges.begin();
    FileRangeList::const_iterator good_iter = ranges.end();
    for(;iter != ranges.end(); ++iter){
        const IncludeFileRange& range = (*iter);
        if(line >= range.start){
            good_iter = iter;
        }
    }
    return good_iter == ranges.end() ? ranges.begin() : good_iter;
}

static FILE *robust_fopen(const char* path, const char* mode, int max_tries = 10) {
    int tries = 0;
    FILE *file = my_fopen(path, mode);
    while(file == NULL && tries < max_tries){
        file = my_fopen(path, mode);
        if(file == NULL){
            tries++;
            BusyWaitMilliseconds(50);
        }
    }
    return file;
}

static std::string ReadScriptFile(const Path &path) {
    int max_tries = 10;
    int tries = 0;
    int len = 0;
    bool file_exists = false;
    FILE *file;

    while(len == 0 && tries < max_tries){
        file = robust_fopen(path.GetFullPath(), "rb");

        if( file == NULL ) {
            FatalError("Error","Failed to open the script file: %s\n%s",
                       path.GetFullPath(), strerror(errno));
        }

        file_exists = true;

        // Determine the size of the file    
        fseek(file, 0, SEEK_END);
        len = ftell(file);
        fseek(file, 0, SEEK_SET);

        if(len == 0){
            fclose(file);
            tries++;
            BusyWaitMilliseconds(50);
        }
    }

    // Read the entire file
    std::string script;
    script.resize(len);
    int c =    fread(&script[0], len, 1, file);

    if(file_exists) {
        if( len == 0 ) {
            FatalError("Error","Script file was present, but empty:\n%s\n%s",
                                path.GetFullPath(), strerror(errno));
        } else if( c == 0 ) {
            FatalError("Error","Failed to load script file:\n%s\n%s",
                                path.GetFullPath(), strerror(errno));
        }
    } else {
        FatalError("Error","Failed to load script file:\n%s\n%s",
                            path.GetFullPath(), strerror(errno));
    }
    
    fclose(file);

    return script;
}

static void UpdateFile( const ScriptFile* parent_script_file, ScriptFile &file, const std::string &source, const Path& path ) {
    file.parent = parent_script_file;
    file.unexpanded_contents = source;
    file.contents = source;
    file.file_path = path;
    file.dependencies.clear();
    file.dependencies.resize(1);
    file.dependencies[0].path = path;
    file.dependencies[0].modified = GetDateModifiedInt64(path);
    file.ExpandIncludePaths();
    file.hash = djb2_string_hash(file.contents.c_str());
    std::vector<Dependency>::iterator iter = file.dependencies.begin();
    file.latest_modification = (*iter).modified;
    ++iter;
    for(; iter != file.dependencies.end(); ++iter){
        file.latest_modification = max(file.latest_modification, (*iter).modified);
    }
}

static const ScriptFile* GetScriptFileRecursive(const ScriptFile* parent_script_file, const Path& path) {
    // Look in script file cache for desired script file
    ScriptFileMap& file_map = ScriptFileCache::Instance()->files;
    ScriptFileMap::iterator iter;
    iter = file_map.find(path.GetFullPathStr());
    ModID modsource;
    if(iter == file_map.end()){
        // Desired script file not found, so load it
        std::string source = ReadScriptFile(path);
        if (!source.empty()) {
            ScriptFile& file = file_map[path.GetFullPathStr()];
            UpdateFile(parent_script_file, file, source, path);
            return &file;
        }
    } else {
        // Script file found -- make sure it hasn't changed
        ScriptFile &script_file = (*iter).second;
        if(ScriptFileUtil::DetectScriptFileChanged(path)){
            std::string source = ReadScriptFile(path);
            if (!source.empty()) {
                UpdateFile(parent_script_file, script_file, source, path);
                return &script_file;
            }
        } else {
            return &script_file;
        }
    }

    return NULL;
}

static void InsertFileRange(FileRangeList& ranges, 
                     unsigned start, 
                     unsigned length, 
                     const Path& path)
{
    FileRangeList::iterator parent_iter = GetFileRange(ranges, start);
    
    // Split parent in half
    IncludeFileRange& parent = (*parent_iter);
    unsigned local_cut = start - parent.start;
    
    if(parent.length-local_cut>0){
        FileRangeList::iterator after_parent_iter = parent_iter;
        after_parent_iter++;
        ranges.insert(after_parent_iter,
                      IncludeFileRange(parent.start+local_cut,
                                       parent.length-local_cut,
                                       parent.offset,
                                       parent.file_path));
        parent.length = local_cut;
    }

    // Shift all elements to the right of the cut
    {
        FileRangeList::iterator shift_iter = parent_iter;
        shift_iter++;
        for(;shift_iter != ranges.end(); ++shift_iter){
            IncludeFileRange& range = (*shift_iter);
            range.start += length;
            range.offset += length-1;
        }
    }

    // Add new range in between parent halves
    {
        FileRangeList::iterator after_parent_iter = parent_iter;
        after_parent_iter++;    
        ranges.insert(after_parent_iter,
                      IncludeFileRange(start,length,start-1,path));
    }

    // Delete original parent half if length is now zero
    if(parent.length == 0){
        ranges.erase(parent_iter);
    }
}

void ScriptFile::ExpandIncludePaths(){
    std::string &script = contents;

    file_range.clear();
    file_range.push_back(IncludeFileRange(1,GetNumLines(script),0,file_path));

    // Get the position of the first letter of the first "#include"
    // directive in the script
    size_t found_pos = script.find("#include");
    while (found_pos != std::string::npos){        
        bool disabled_include = false;
        for( int i = found_pos-1; i >= 0; i-- ) {
            if(script[i] != ' ' && script[i] != '\t') {
                if( script[i] == '\n' || script[i] == '\r' ) {
                    break;
                } else {
                    if( script[i] != '/' ) {
                        LOGE << "Unexpected character \'" << script[i] << "\' before #include in " << file_path << std::endl;
                        int line_count = 0;
                        for( int k = 0; k < i; k++ ){
                            if( script[k] == '\n' ){
                                line_count++;
                            }
                        }
                        LOGE << " line " << line_count << std::endl;
                    } else {
                        disabled_include = true;
                    }
                }
            }
        }
        size_t path_start = found_pos + 10; // Get pos of first letter of path
        size_t path_end = script.find('\"',path_start); // Find end of path
        std::string path = std::string(script_dir_path) +
                           script.substr(path_start, path_end-path_start);
        
        // If include file has not already been added, then replace #include
        // directive with contents of included file. Otherwise, just remove
        // the directive.
        Path include_path = FindFilePath(path, kDataPaths | kModPaths);
        if(disabled_include == false && !AlreadyAddedIncludeFileInParentHierarchy(this, include_path) && !AlreadyAddedIncludeFile(include_path)){
            dependencies.resize(dependencies.size()+1);
            dependencies.back().path = include_path;
            if( include_path.isValid() ) {
                dependencies.back().modified = GetDateModifiedInt64(include_path.GetFullPath());
                
                std::string new_script = GetScriptFileRecursive(this, include_path)->unexpanded_contents;
                
                if( config["dump_include_scripts"].toBool() )
                {
                    new_script = 
                    + "/*include - START - " + path + " */\n"
                    + new_script 
                    + "/*include - END   - " + path + " */\n";
                }

                script.replace(found_pos,path_end+1-found_pos,new_script);
                
                unsigned line_number = GetLineNumber(script, found_pos);
                unsigned include_length = GetNumLines(new_script);
                InsertFileRange(file_range,line_number,include_length,include_path);
            } else {
                LOGE << "Could not resolve script include: " << path << std::endl;
            }
        } else {
            script.replace(found_pos,path_end+1-found_pos,"");
        }

        found_pos = script.find("#include",found_pos);
    }
}

LineFile ScriptFile::GetCorrectedLine( unsigned line ) const
{
    LOGD << "Getting corrected line A." << std::endl;
    FileRangeList::const_iterator iter = GetFileRange(file_range, line);
    LOGD << "Getting corrected line B." << std::endl;
    const IncludeFileRange& range = (*iter);
    LOGD << "Getting corrected line C." << std::endl;

    return LineFile(line - range.offset, range.file_path);
}

bool ScriptFileUtil::DetectScriptFileChanged(const Path &path) {
    ScriptFileMap& file_map = ScriptFileCache::Instance()->files;
    ScriptFileMap::iterator iter;
    iter = file_map.find(path.GetFullPathStr());
    if(iter == file_map.end()){
        return true;
    } else {
        ScriptFile &file = (*iter).second;
        return GetLatestModification(path) > file.latest_modification;
    }
}

int64_t ScriptFileUtil::GetLatestModification(const Path &path) {
	PROFILER_ZONE(g_profiler_ctx, "GetLatestModification");
    ScriptFileMap& file_map = ScriptFileCache::Instance()->files;
    ScriptFileMap::iterator iter;
    iter = file_map.find(path.GetFullPathStr());
    if(iter == file_map.end()){
        DisplayError("Error", "Getting latest modification of unloaded script");
        return 0;
    } else {
        ScriptFile &file = (*iter).second;
        std::vector<Dependency>::iterator iter = file.dependencies.begin();
        int64_t latest_modification = GetDateModifiedInt64((*iter).path);
        ++iter;
        for(; iter != file.dependencies.end(); ++iter){
            latest_modification = max(latest_modification, GetDateModifiedInt64((*iter).path));
        }
        return latest_modification;
    }
}

const ScriptFile* ScriptFileUtil::GetScriptFile(const Path& path) {
    return GetScriptFileRecursive(NULL, path);
}

void ScriptFileUtil::ReturnPaths(const Path &script_path, PathSet &path_set) {
    path_set.insert("script "+script_path.GetOriginalPathStr());
    const ScriptFile &script_file = *ScriptFileUtil::GetScriptFile(script_path);
    const std::string &script = script_file.contents;
    std::string load_path;
    std::string type;
    std::string::size_type index = 0;
    while(1){
        index = script.find("\"Data/", index); // Set index to first quote of "Data/" path
        if(index == std::string::npos){
            break;
        }
        ++index; // Set index to first character of Data/ path
        std::string::size_type end_quote = script.find('\"', index); // Find last quote
        load_path = script.substr(index, end_quote - index);
        ReturnPathUtil::ReturnPathsFromPath(load_path, path_set);
    }
}

void ScriptFileCache::Dispose()
{
    files.clear();
}

