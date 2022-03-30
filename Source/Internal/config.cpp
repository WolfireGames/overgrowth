//-----------------------------------------------------------------------------
//           Name: config.cpp
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
#include "config.h"

#include <Internal/config.h>
#include <Internal/path_utility.h>
#include <Internal/filesystem.h>
#include <Internal/datemodified.h>

#include <Compat/fileio.h>
#include <Logging/logdata.h>
#include <Main/engine.h>

#include <vector>
#include <algorithm>
#include <iostream>

Config config;
Config default_config;

const vec3 kRedBloodColor(0.4f,0.0f,0.0f);
const vec3 kGreenBloodColor(0.0f,0.4f,0.0f);
const vec3 kCyanBloodColor(0.0f,0.4f,0.4f);
const vec3 kBlackBloodColor(0.1f,0.1f,0.1f);

extern bool g_no_reflection_capture;

// Utility function to trim whitespace off the ends of a string
inline std::string trim(std::string source) {
    std::string result = source.erase(source.find_last_not_of(" \t\r") + 1);
    return result.erase(0, result.find_first_not_of(" \t\r"));
}

Config::Config(): has_changed_since_save(false) {
    commonResolutions.push_back(Resolution( 1920, 1080 ));
    commonResolutions.push_back(Resolution( 2560, 1440 ));
    commonResolutions.push_back(Resolution( 1366, 768 ));
    commonResolutions.push_back(Resolution( 3840, 2160 ));
    commonResolutions.push_back(Resolution( 1680, 1050 ));
    commonResolutions.push_back(Resolution( 1600, 900 ));
    commonResolutions.push_back(Resolution( 1440, 900 ));
    commonResolutions.push_back(Resolution( 1920, 1200 ));
    commonResolutions.push_back(Resolution( 2560, 1080 ));
    commonResolutions.push_back(Resolution( 1360, 768 ));
    commonResolutions.push_back(Resolution( 1280, 800 ));
    commonResolutions.push_back(Resolution( 1536, 864 ));
    commonResolutions.push_back(Resolution( 2736, 1824 ));
    commonResolutions.push_back(Resolution( 1280, 720 ));
}

bool Config::Load(const std::string& filename, bool just_filling_blanks, bool shadow_variables) {

    std::string configFile = filename;//pathUtility::localPathToGlobal(filename);
    std::ifstream file;
    my_ifstream_open(file, configFile.c_str(), std::ios_base::in);

    if(!file.is_open())
        return false;

    Load( file, just_filling_blanks, shadow_variables );

    file.close();

    if( just_filling_blanks == false && shadow_variables == false ) {
        date_modified_ = GetDateModifiedInt64(configFile.c_str());
        primary_path_ = configFile;
    }

    return true;
}

bool Config::Load( std::istream& stream, bool just_filling_blanks, bool shadow_variables )
{
    std::string line;
    std::string comment = "//";
    std::string delimiter = ":";

    Map* _map;

    if( shadow_variables )
    {
        _map = &shadow_map_;
        LOGI << "Shadow loading" << std::endl;
    }
    else
    {
        _map = &map_;
    } 
    
    int count = 0;
    while(stream.good())
    {
        getline(stream, line);
        
        // Remove any comments
        size_t commIdx = line.find(comment);
        if(commIdx != std::string::npos)
            line = line.substr(0, commIdx);

        size_t delimIdx = line.find(delimiter);
        if(delimIdx == std::string::npos)
            continue;

        std::string key = trim(line.substr(0, delimIdx));
        std::string value = trim(line.substr(delimIdx + 1));

        ConfigVal val;
        val.data = value;
        val.order = count;
        ++count;

        if(!key.empty()){
            //We compare with global map on purpose here, because that is the "true" state.
            if(!just_filling_blanks || map_.find(key) == map_.end()){
                (*_map)[key] = val;
            }
        }
    }
	return true;
}

static void AddResolution(std::vector<Resolution> &resolutions, int w, int h){
	// Check if resolution is already there
	int num_res = resolutions.size();
	for(int i=0; i<num_res; ++i){
		if(resolutions[i].w == w && resolutions[i].h == h){
			return;
		}
	}

    resolutions.push_back(Resolution(w,h));
}

Resolution::Resolution( int _w, int _h ) : w(_w), h(_h) {
}

int Config::GetMonitorCount() {
    return SDL_GetNumVideoDisplays();
}

std::vector<Resolution> Config::GetPossibleResolutions() {

    int targetMonitor = GetRef("target_monitor").toNumber<int>();
    int displayModeCount = SDL_GetNumDisplayModes(targetMonitor);

	// Populate resolution list
    std::vector<Resolution> resolutions;
    resolutions.reserve(displayModeCount);
 
    SDL_DisplayMode desktopDisplayMode;
    SDL_GetDesktopDisplayMode(targetMonitor, &desktopDisplayMode);

    float desktopAspect = desktopDisplayMode.w / (float)desktopDisplayMode.h;

    for (int i = 0; i < displayModeCount; ++i) {
        SDL_DisplayMode mode;
        SDL_GetDisplayMode(targetMonitor, i, &mode);

        float resolutionAspect = mode.w / (float)mode.h;

		if ((mode.h <= desktopDisplayMode.h && mode.w <= desktopDisplayMode.w && std::fabs(resolutionAspect - desktopAspect) < 0.01f)
            || static_cast<FullscreenMode::Mode>(config["fullscreen"].toNumber<int>()) == FullscreenMode::kFullscreen) {
			bool resolutionFound = false;
			for (size_t i = 0; i < commonResolutions.size(); ++i) {
				if (commonResolutions[i].w == mode.w && commonResolutions[i].h == mode.h) {
					resolutionFound = true;
					break;
				}
			}

			if (!resolutionFound)
				continue;

            AddResolution(resolutions, mode.w, mode.h);
		}
    }
    
    // Add at least this many resolutions to the list in case some user has some weird monitor
    const static int MIN_RESOLUTION_COUNT = 5;
    for (int i = resolutions.size(); resolutions.size() < MIN_RESOLUTION_COUNT && i < displayModeCount; ++i) {
        SDL_DisplayMode mode;
        SDL_GetDisplayMode(targetMonitor, i - resolutions.size(), &mode);

        if (mode.h <= desktopDisplayMode.h && mode.w <= desktopDisplayMode.w) {
            AddResolution(resolutions, mode.w, mode.h);
        }
    }

    // Sort resolutions
    std::sort(resolutions.begin(), resolutions.end(), ResolutionCompare());
	return resolutions;
}

Config * Config::GetPresets(){
	static Config global_settings[4];
    if(global_settings[0].map_.empty()){
        global_settings[0].GetRef("blood") = 1;
        global_settings[0].GetRef("texture_reduce") = 1;
        global_settings[0].GetRef("multisample") = 1;
        global_settings[0].GetRef("anisotropy") = 2;
        global_settings[0].GetRef("simple_fog") = 1;
        global_settings[0].GetRef("depth_of_field") = 0;
        global_settings[0].GetRef("depth_of_field_reduced") = 1;
        global_settings[0].GetRef("detail_objects") = 0;
        global_settings[0].GetRef("detail_object_decals") = 0;
        global_settings[0].GetRef("detail_object_lowres") = 1;
        global_settings[0].GetRef("detail_object_disable_shadows") = 1;
        global_settings[0].GetRef("detail_objects_reduced") = 1;
        global_settings[0].GetRef("simple_shadows") = 1;
        global_settings[0].GetRef("tet_mesh_lighting") = 0;
        global_settings[0].GetRef("motion_blur_amount") = 0;
        global_settings[0].GetRef("no_reflection_capture") = 1;
        global_settings[0].GetRef("particle_field") = 0;
        global_settings[0].GetRef("particle_field_simple") = 1;
        global_settings[0].GetRef("simple_water") = 1;

        global_settings[1].GetRef("blood") = 2;
        global_settings[1].GetRef("texture_reduce") = 0;
        global_settings[1].GetRef("multisample") = 1;
        global_settings[1].GetRef("anisotropy") = 4;
        global_settings[1].GetRef("simple_fog") = 1;
        global_settings[1].GetRef("depth_of_field") = 1;
        global_settings[1].GetRef("depth_of_field_reduced") = 1;
        global_settings[1].GetRef("detail_objects") = 1;
        global_settings[1].GetRef("detail_object_decals") = 0;
        global_settings[1].GetRef("detail_object_lowres") = 1;
        global_settings[1].GetRef("detail_object_disable_shadows") = 1;
        global_settings[1].GetRef("detail_objects_reduced") = 1;
        global_settings[1].GetRef("simple_shadows") = 1;
        global_settings[1].GetRef("tet_mesh_lighting") = 0;
        global_settings[1].GetRef("motion_blur_amount") = 0;
        global_settings[1].GetRef("no_reflection_capture") = 0;
        global_settings[1].GetRef("particle_field") = 1;
        global_settings[1].GetRef("particle_field_simple") = 1;
        global_settings[1].GetRef("simple_water") = 0;

        global_settings[2].GetRef("blood") = 2;
        global_settings[2].GetRef("texture_reduce") = 0;
        global_settings[2].GetRef("multisample") = 1;
        global_settings[2].GetRef("anisotropy") = 4;
        global_settings[2].GetRef("simple_fog") = 1;
        global_settings[2].GetRef("depth_of_field") = 1;
        global_settings[2].GetRef("depth_of_field_reduced") = 0;
        global_settings[2].GetRef("detail_objects") = 1;
        global_settings[2].GetRef("detail_object_decals") = 0;
        global_settings[2].GetRef("detail_object_lowres") = 1;
        global_settings[2].GetRef("detail_object_disable_shadows") = 1;
        global_settings[2].GetRef("detail_objects_reduced") = 0;
        global_settings[2].GetRef("simple_shadows") = 1;
        global_settings[2].GetRef("tet_mesh_lighting") = 0;
        global_settings[2].GetRef("motion_blur_amount") = 0;
        global_settings[2].GetRef("no_reflection_capture") = 0;
        global_settings[2].GetRef("particle_field") = 1;
        global_settings[2].GetRef("particle_field_simple") = 0;
        global_settings[2].GetRef("simple_water") = 0;

        global_settings[3].GetRef("blood") = 2;
        global_settings[3].GetRef("texture_reduce") = 0;
        global_settings[3].GetRef("multisample") = 4;
        global_settings[3].GetRef("anisotropy") = 4;
        global_settings[3].GetRef("simple_fog") = 0;
        global_settings[3].GetRef("depth_of_field") = 1;
        global_settings[3].GetRef("depth_of_field_reduced") = 0;
        global_settings[3].GetRef("detail_objects") = 1;
        global_settings[3].GetRef("detail_object_lowres") = 0;
        global_settings[3].GetRef("detail_object_decals") = 0;
        global_settings[3].GetRef("detail_object_disable_shadows") = 0;
        global_settings[3].GetRef("detail_objects_reduced") = 0;
        global_settings[3].GetRef("simple_shadows") = 0;
        global_settings[3].GetRef("tet_mesh_lighting") = 0;
        global_settings[3].GetRef("motion_blur_amount") = 0;
        global_settings[3].GetRef("no_reflection_capture") = 0;
        global_settings[3].GetRef("particle_field") = 1;
        global_settings[3].GetRef("particle_field_simple") = 0;
        global_settings[3].GetRef("simple_water") = 0;
    }
	return global_settings;
}

void Config::SetSettingsToPreset(std::string preset_name )
{
	int index = 0;
	if(preset_name == "Low"){
		index = 0;
    }else if(preset_name == "Reduced"){
        index = 1;
	}else if(preset_name == "Medium"){
		index = 2;
	}else if(preset_name == "High"){
		index = 3;
	}else{
		return;
	}
	Config::Map& map = GetPresets()[index].map_;
	for(Config::Map::iterator iter = map.begin(); iter != map.end(); ++iter ){
		config.GetRef(iter->first) = iter->second.data;
	}
}

std::string Config::GetSettingsPreset(){
	int preset = 0;

	// Determine if we match any of the global_settings presets
	for(int i=0; i<4; ++i){
		Config::Map& map = GetPresets()[i].map_;
		for(Config::Map::iterator iter = map.begin(); iter != map.end(); ++iter ){
			if(config.GetRef(iter->first) != iter->second.data){
				preset = i + 1;
				break;
			}
		}   
		if(preset == i){
			break;
		}
	}
	if(preset == 4){
		preset = 0;
	} else {
		++preset;
	}
	switch(preset) {
		case 0: 
			return "Custom";
		case 1:
			return "Low";
        case 2:
            return "Reduced";
		case 3:
			return "Medium";
		case 4:
			return "High";
		default:
			return "Error";
	}
}

std::vector<std::string> Config::GetSettingsPresets() {
    std::vector<std::string> presets;
    //presets.push_back("Custom");
    presets.push_back("Low");
    presets.push_back("Reduced");
    presets.push_back("Medium");
    presets.push_back("High");
    return presets;
}

static const char* difficulty_presets[] = {
    "Casual",
    "Hardcore",
    "Expert",
    NULL
};

static const float difficulty_preset_values[] = {
    0.0f, 0.8f,
    0.5f, 1.0f,
    1.0f, 1.0f
};

static const bool difficulty_tutorials_preset_values[] = {
    true,
    true,
    false
};

static const bool difficulty_ledge_grab_preset_values[] = {
    true,
    true,
    false
};

std::vector<std::string> Config::GetDifficultyPresets() {
    std::vector<std::string> presets; 
    //presets.push_back("Custom");
    for( int i = 0; difficulty_presets[i] != NULL; i++ ) {
        presets.push_back(difficulty_presets[i]);
    }
    return presets;
}

std::string Config::GetDifficultyPreset() {
    for(int i=0; difficulty_presets[i] != NULL; ++i){
        if(GetRef("game_difficulty").toNumber<float>() == difficulty_preset_values[i*2] &&
           GetRef("global_time_scale_mult").toNumber<float>() == difficulty_preset_values[i*2+1] && 
           GetRef("tutorial").toBool() == difficulty_tutorials_preset_values[i] &&
           GetRef("auto_ledge_grab").toBool() == difficulty_ledge_grab_preset_values[i])
        {
            return difficulty_presets[i];
        }			
    }
    return "Custom";
}

//Used to validate progress, rounds down in difficulty steps
std::string Config::GetClosestDifficulty() {
    std::string difficulty = "Custom";
    for(int i=0; difficulty_presets[i] != NULL; ++i){
        if( GetRef("game_difficulty").toNumber<float>() >= difficulty_preset_values[i*2] &&
           GetRef("global_time_scale_mult").toNumber<float>() >= difficulty_preset_values[i*2+1]) {
            difficulty = difficulty_presets[i];
        } else {
            break;
        }
    }
    return difficulty;
}

void Config::SetDifficultyPreset( std::string name ) {
    for(int i = 0; difficulty_presets[i] != NULL; i++ ) {
        if( strmtch(difficulty_presets[i],name.c_str())){
            GetRef("game_difficulty") = difficulty_preset_values[i*2]; 
            GetRef("global_time_scale_mult") = difficulty_preset_values[i*2+1];
            GetRef("tutorials") = difficulty_tutorials_preset_values[i];
            GetRef("auto_ledge_grab") = difficulty_ledge_grab_preset_values[i];
        }
    }
}

void Config::ReloadDynamicSettings(){
    Engine::Instance()->SetGameSpeed(config["global_time_scale_mult"].toNumber<float>(), true);
	Engine::Instance()->GetSound()->SetMusicVolume(config.GetRef("music_volume").toNumber<float>());
	Engine::Instance()->GetSound()->SetMasterVolume(config.GetRef("master_volume").toNumber<float>());
	Graphics::Instance()->config_.motion_blur_amount_ = (float)config["motion_blur_amount"].toNumber<int>();
	Input::Instance()->SetMouseSensitivity(config["mouse_sensitivity"].toNumber<float>());
    Input::Instance()->UpdateGamepadLookSensitivity();
    Input::Instance()->UpdateGamepadDeadzone();
}

void Config::ReloadStaticSettings(){
	Input::Instance()->SetFromConfig(config);
    Graphics::Instance()->SetFromConfig(config, true);
    Graphics::Instance()->SetTargetMonitor(config["target_monitor"].toNumber<int>());
	Graphics::Instance()->SetResolution(config["screenwidth"].toNumber<int>(), config["screenheight"].toNumber<int>(), false);
	Graphics::Instance()->SetFSAA(config["multisample"].toNumber<int>());
	if(config["anisotropy"].toNumber<int>() != Graphics::Instance()->config_.anisotropy()){
        Graphics::Instance()->SetAnisotropy((float)config["anisotropy"].toNumber<int>());     
        Textures::Instance()->ApplyAnisotropy();
    }
	Graphics::Instance()->SetSimpleFog(config["simple_fog"].toBool());
	Graphics::Instance()->SetDepthOfField(config["depth_of_field"].toBool());
    Graphics::Instance()->SetDepthOfFieldReduced(config["depth_of_field_reduced"].toBool());
	Graphics::Instance()->SetDetailObjects(config["detail_objects"].toBool());
	Graphics::Instance()->SetDetailObjectDecals(config["detail_object_decals"].toBool());
	Graphics::Instance()->SetDetailObjectLowres(config["detail_object_lowres"].toBool());
	Graphics::Instance()->SetDetailObjectShadows(!config["detail_object_disable_shadows"].toBool());
    Graphics::Instance()->SetDetailObjectsReduced(config["detail_objects_reduced"].toBool());
    Graphics::Instance()->setAttribEnvObjInstancing(config["attrib_envobj_instancing"].toBool());
	//Makes the game crash
	//the_scenegraph->light_probe_collection.light_volume_enabled = config["light_volume_lighting"].toBool();
    Graphics::Instance()->SetFullscreen(static_cast<FullscreenMode::Mode>(config["fullscreen"].toNumber<int>()));
	Graphics::Instance()->SetVsync(config["vsync"].toBool());
	Graphics::Instance()->config_.SetBlood(config.GetRef("blood").toNumber<int>());
	Graphics::Instance()->config_.SetBloodColor(GraphicsConfig::BloodColorFromString(config["blood_color"].str()));
	Graphics::Instance()->config_.SetSplitScreen(config["split_screen"].toNumber<bool>());
	Graphics::Instance()->SetSeamlessCubemaps(config["seamless_cubemaps"].toNumber<bool>());
    g_no_reflection_capture = config["no_reflection_capture"].toNumber<bool>();
	
	Input::Instance()->SetInvertXMouseLook(config["invert_x_mouse_look"].toNumber<bool>());
	Input::Instance()->SetInvertYMouseLook(config["invert_y_mouse_look"].toNumber<bool>());
	Input::Instance()->UseRawInput(config["use_raw_input"].toNumber<bool>());
	int num_cameras = ActiveCameras::NumCameras();
    int curr_id = ActiveCameras::GetID();
    for(int i=0; i<num_cameras; ++i){
        ActiveCameras::Set(i);
        ActiveCameras::Get()->SetAutoCamera(config["auto_camera"].toNumber<bool>());
    }
	ActiveCameras::Set(curr_id);

    SceneGraph* scene_graph = Engine::Instance()->GetSceneGraph();
    if(scene_graph)
        scene_graph->PreloadShaders();
}

class ConfigValCompare {
public:
    bool operator()(const std::pair<std::string, ConfigVal> &a, 
                    const std::pair<std::string, ConfigVal> &b) 
    {
        return a.second.order < b.second.order;
    }
};

bool Config::Save(const std::string& filename) {
    LOGI << "Saving config to: " << filename << std::endl;
    CreateParentDirs(filename.c_str());
    std::ofstream file;
    my_ofstream_open(file, filename);

    if(!file.is_open())
        return false;

    std::vector<std::pair<std::string, ConfigVal> > vec(map_.begin(), map_.end());
    std::sort(vec.begin(), vec.end(), ConfigValCompare());

    for(unsigned i=0; i<vec.size(); ++i)
    {
        file << vec[i].first << ": " << vec[i].second.data.str() << "\n";
    }

    file.close();

    has_changed_since_save = false;

    if( AreSame(filename.c_str(), primary_path_.c_str()) ) {
        date_modified_ = GetDateModifiedInt64(primary_path_.c_str());
    }

    return true;
}

const StringVariant& Config::operator[](const std::string& keyName) const {
    std::map<std::string, ConfigVal>::const_iterator iter = shadow_map_.find(keyName);

    if(iter != shadow_map_.end())
    {
        return iter->second.data;
    }

    iter = map_.find(keyName);
    if(iter != map_.end())
    {
        return iter->second.data;
    }

    static StringVariant empty;
    return empty;
}

bool Config::HasKey( const char* key )
{
    std::string s = std::string(key);
    return HasKey(s);
}

bool Config::HasKey( std::string& key )
{
    std::map<std::string, ConfigVal>::const_iterator iter = shadow_map_.find(key);

    if(iter != shadow_map_.end())
    {
        return true;
    }

    iter = map_.find(key);

    if(iter != map_.end())
    {
        return true;
    }

    return false;
}

void Config::RemoveConfig( std::string key ) {
    std::map<std::string, ConfigVal>::iterator iter = shadow_map_.find(key);

    if(iter != shadow_map_.end())
    {
        shadow_map_.erase(iter);
    }

    iter = map_.find(key);

    if(iter != map_.end())
    {
        map_.erase(iter);
    }
}

bool Config::HasChangedSinceLastSave()
{
    return has_changed_since_save;
}

bool Config::operator!=( const Config& other ) const
{
    return !(*this == other);
}

bool Config::operator==( const Config& other ) const
{
    return map_ == other.map_;
}

bool Config::PrimarySourceModified() {
    if( primary_path_.empty() == false ) {
        return GetDateModifiedInt64(primary_path_.c_str()) != date_modified_;
    } else {
        return false;
    }
}

std::vector<Resolution>& Config::GetCommonResolutions()
{
	return commonResolutions;
}

bool ConfigVal::operator==( const ConfigVal &other ) const
{
    return data == other.data;
}

bool StringVariant::operator==( const StringVariant &other ) const
{
    return data == other.data;
}

bool StringVariant::operator!=( const StringVariant &other ) const
{
    return !((*this) == other);
}
