//-----------------------------------------------------------------------------
//           Name: settings_screen.cpp
//      Developer: Wolfire Games LLC
//    Description: 
//        License: Read below
//-----------------------------------------------------------------------------
//
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
#include "settings_screen.h"

#include <Graphics/graphics.h>
#include <Graphics/textures.h>
#include <Graphics/sky.h>
#include <Graphics/shaders.h>
#include <Graphics/camera.h>
#include <Graphics/pxdebugdraw.h>

#include <Internal/config.h>
#include <Internal/timer.h>
#include <Internal/filesystem.h>
#include <Internal/common.h>

#include <Asset/Asset/animation.h>
#include <Asset/Asset/levelinfo.h>

#include <Main/scenegraph.h>
#include <Main/engine.h>

#include <Scripting/angelscript/ascrashdump.h>
#include <Scripting/angelscript/ascontext.h>

#include <Logging/logdata.h>
#include <UserInput/input.h>
#include <Editors/map_editor.h>
#include <Sound/sound.h>
#include <Math/vec3math.h>
#include <UserInput/keyTranslator.h>
#include <Compat/fileio.h>

#include <SDL_keycode.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <sstream>

extern float imgui_scale;

extern Config config;
extern AnimationConfig animation_config;
extern bool g_simple_shadows;
extern bool g_level_shadows;
extern bool g_simple_water;
extern bool g_disable_fog;
extern bool g_draw_vr;
extern bool g_albedo_only;
extern bool g_no_reflection_capture;
extern bool g_no_detailmaps;
extern bool g_no_decals;
extern bool g_no_decal_elements;
extern bool g_single_pass_shadow_cascade;
extern bool g_perform_occlusion_query;
extern bool g_gamma_correct_final_output;
extern bool g_attrib_envobj_intancing_support;
extern bool g_attrib_envobj_intancing_enabled;
extern bool g_ubo_batch_multiplier_force_1x;
extern Timer game_timer;

const vec3 kRedBloodColor(0.4f,0.0f,0.0f);
const vec3 kGreenBloodColor(0.0f,0.4f,0.0f);
const vec3 kCyanBloodColor(0.0f,0.4f,0.4f);
const vec3 kBlackBloodColor(0.1f,0.1f,0.1f);

static void UpdateFullscreenMode(FullscreenMode::Mode val) {
    config.GetRef("fullscreen") = static_cast<int>(val);
    Graphics::Instance()->SetFullscreen(static_cast<FullscreenMode::Mode>(config["fullscreen"].toNumber<int>()));
}

static void UpdateMultisampleSetting(int val){
    config.GetRef("multisample") = val;
    Graphics::Instance()->SetFSAA(config["multisample"].toNumber<int>());
}

static void UpdateAnisotropy(int val){
    config.GetRef("anisotropy") = val;
    if(config["anisotropy"].toNumber<int>() != Graphics::Instance()->config_.anisotropy()){
        Graphics::Instance()->SetAnisotropy((float)config["anisotropy"].toNumber<int>());
        Textures::Instance()->ApplyAnisotropy();
    }
}

static void UpdateSimpleFog(bool val){
    config.GetRef("simple_fog") = val;
    Graphics::Instance()->SetSimpleFog(val);
}

static void UpdateDepthOfField(bool val){
    config.GetRef("depth_of_field") = val;
    Graphics::Instance()->SetDepthOfField(val);
}

static void UpdateDepthOfFieldReduced(bool val){
    config.GetRef("depth_of_field_reduced") = val;
    Graphics::Instance()->SetDepthOfFieldReduced(val);
}

static void UpdateDetailObjects(bool val){
    config.GetRef("detail_objects") = val;
    Graphics::Instance()->SetDetailObjects(val);
}

static void UpdateDetailObjectDecals(bool val){
    config.GetRef("detail_object_decals") = val;
    Graphics::Instance()->SetDetailObjectDecals(val);
}

static void UpdateDetailObjectLowres(bool val){
    config.GetRef("detail_object_lowres") = val;
    Graphics::Instance()->SetDetailObjectLowres(val);
}

static void UpdateDetailObjectShadows(bool val){
    config.GetRef("detail_object_disable_shadows") = !val;
    Graphics::Instance()->SetDetailObjectShadows(val);
}

static void UpdateDetailObjectsReduced(bool val){
    config.GetRef("detail_objects_reduced") = val;
    Graphics::Instance()->SetDetailObjectsReduced(val);
}

static void UpdateSimpleShadows(bool val){
    config.GetRef("simple_shadows") = val;
    Graphics::Instance()->setSimpleShadows(val);
}

static void UpdateSimpleWater(bool val) {
    config.GetRef("simple_water") = val;
    Graphics::Instance()->setSimpleWater(config["simple_water"].toNumber<bool>());
}

static void UpdateParticleFieldSimple(bool val){
    config.GetRef("particle_field_simple") = val;
    Graphics::Instance()->SetParticleFieldSimple(val);
}

static void UpdateAttribEnvObjInstancing(bool val){
    config.GetRef("attrib_envobj_instancing") = val;
    Graphics::Instance()->setAttribEnvObjInstancing(val);
}

static void UpdateTetMeshLighting(SceneGraph* scenegraph, bool val){
    if(config["tet_mesh_lighting"] != val){
        config.GetRef("tet_mesh_lighting") = val;
    }
    if(scenegraph){
        scenegraph->light_probe_collection.probe_lighting_enabled = val;
    }
}

static void UpdateLightVolumes(SceneGraph* scenegraph, bool val){
    if(config["light_volume_lighting"] != val){
        config.GetRef("light_volume_lighting") = val;
    }
    if(scenegraph){
        scenegraph->light_probe_collection.light_volume_enabled = val;
    }
}

static void UpdateMotionBlur(float val) {
    config.GetRef("motion_blur_amount") = val;
    Graphics::Instance()->config_.motion_blur_amount_ = val;
}

static void UpdateNoReflectionCapture(bool val ) {
    LOGI << "Setting no reflection capture to " << val << std::endl;
    g_no_reflection_capture = val;
    config.GetRef("no_reflection_capture") = val;
}

static void AddResolution(std::vector<Resolution> &resolutions, int w, int h){
    if(w < 640 || h < 480){
        return;
    }
    // Check if resolution is already there
    int num_res = resolutions.size();
    for(int i=0; i<num_res; ++i){
        if(resolutions[i].w == w && resolutions[i].h == h){
            return;
        }
    }
    // Add resolution
    resolutions.push_back(Resolution(w,h));
}

static void UpdateEnableLayeredSoundtrackLimiter(bool val) {
    Engine::Instance()->GetSound()->EnableLayeredSoundtrackLimiter(val);
    config.GetRef("use_soundtrack_limiter") = val;
}

void AddResolutionDropdownImGui() {
    std::vector<Resolution> resolutions = config.GetPossibleResolutions();
    // Add current resolution
    const int *render_dims = Graphics::Instance()->render_dims;
    AddResolution(resolutions, render_dims[0], render_dims[1]);
    // Sort resolutions
    std::sort(resolutions.begin(), resolutions.end(), ResolutionCompare());
    // Add dropdown list
    static int item = 0;
    for(int i=0, len=resolutions.size(); i<len; ++i){
        if(render_dims[0] == resolutions[i].w && render_dims[1] == resolutions[i].h){
            item = i+1;
        }
    }
    const int kMaxResolutions = 30;
    const int kMaxResolutionLen = 30;
    const int kMaxStrLen = kMaxResolutions*kMaxResolutionLen;
    char resolution_items[kMaxStrLen];
    char temp[kMaxResolutionLen];
    int num_resolutions = min((int)resolutions.size(), kMaxResolutions);
    int index = 0;
    FormatString(temp, kMaxResolutionLen, "Custom");
    for(int j=0,len=strlen(temp); j<len; ++j){
        resolution_items[index] = temp[j];
        ++index;
    }
    resolution_items[index] = '\0';
    ++index;
    for(int i=0; i<num_resolutions; ++i){
        FormatString(temp, kMaxResolutionLen, "%dx%d", resolutions[i].w, resolutions[i].h);
        for(int j=0,len=strlen(temp); j<len; ++j){
            resolution_items[index] = temp[j];
            ++index;
        }
        resolution_items[index] = '\0';
        ++index;
    }
    resolution_items[index] = '\0';
    ++index;
    static int custom_res[2];
    int set_new_res[] = {-1,-1};
    if(ImGui::Combo("Resolution", &item, resolution_items)){
        if(item == 0){
            ImGui::OpenPopup("custom_resolution");
            custom_res[0] = render_dims[0];
            custom_res[1] = render_dims[1];
        } else {
            set_new_res[0] = resolutions[item-1].w;
            set_new_res[1] = resolutions[item-1].h;
        }
    }
    if (ImGui::BeginPopup("custom_resolution")) {
        ImGui::Text("Set custom resolution:");
        ImGui::InputInt("Width", &custom_res[0], 0);
        ImGui::InputInt("Height", &custom_res[1], 0);
        if(ImGui::Button("Apply")){
            set_new_res[0] = custom_res[0];
            set_new_res[1] = custom_res[1];
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if(set_new_res[0] != -1){
        set_new_res[0] = max(set_new_res[0], 640);
        set_new_res[1] = max(set_new_res[1], 480);
        config.GetRef("screenwidth") = set_new_res[0];
        config.GetRef("screenheight") = set_new_res[1];
        Graphics::Instance()->SetResolution(set_new_res[0], set_new_res[1], false);
    }
}

static void SetGameSpeed(float val, bool hard) {
    Engine::Instance()->SetGameSpeed(val, hard);
}

void DrawSettingsImGui(SceneGraph* scenegraph, ImGuiSettingsType type){
    Graphics* graphics = Graphics::Instance();
    static Config* global_settings = Config::GetPresets();

    if (ImGui::BeginMenu("Graphics")) {
        bool checkbox_val;
        int dropdown_item;

        // Determine if we match any of the global_settings presets
        dropdown_item = 0;
        for(int i=0; i<3; ++i){
            Config::Map& map = global_settings[i].map_;
            for(Config::Map::iterator iter = map.begin(); iter != map.end(); ++iter ){
                if(config.GetRef(iter->first) != iter->second.data){
                    dropdown_item = i + 1;
                    break;
                }
            }
            if(dropdown_item == i){
                break;
            }
        }
        if(dropdown_item == 3){
            dropdown_item = 0;
        } else {
            ++dropdown_item;
        }

        if(ImGui::Combo("Overall", &dropdown_item, "Custom\0Low\0Medium\0High\0\0")){
            if(dropdown_item != 0){
                int curr_global_setting = dropdown_item-1;
                Config::Map& map = global_settings[curr_global_setting].map_;
                for(Config::Map::iterator iter = map.begin(); iter != map.end(); ++iter ){
                    config.GetRef(iter->first) = iter->second.data;
                }
                UpdateMultisampleSetting(config.GetRef("multisample").toNumber<int>());
                UpdateAnisotropy(config.GetRef("anisotropy").toNumber<int>());
                UpdateSimpleFog(config.GetRef("simple_fog").toNumber<bool>());
                UpdateDepthOfField(config.GetRef("depth_of_field").toNumber<bool>());
                UpdateDepthOfFieldReduced(config.GetRef("depth_of_field_reduced").toNumber<bool>());
                UpdateDetailObjects(config.GetRef("detail_objects").toNumber<bool>());
                UpdateDetailObjectDecals(config.GetRef("detail_object_decals").toNumber<bool>());
                UpdateDetailObjectLowres(config.GetRef("detail_object_lowres").toNumber<bool>());
                UpdateDetailObjectShadows(!config.GetRef("detail_object_disable_shadows").toNumber<bool>());
                UpdateSimpleShadows(config.GetRef("simple_shadows").toNumber<bool>());
                UpdateSimpleWater(config.GetRef("simple_water").toNumber<bool>());
                UpdateAttribEnvObjInstancing(config.GetRef("attrib_envobj_instancing").toNumber<bool>());
                UpdateTetMeshLighting(scenegraph, config.GetRef("tet_mesh_lighting").toNumber<bool>());
                UpdateMotionBlur(config.GetRef("motion_blur_amount").toNumber<float>());
                UpdateNoReflectionCapture(config.GetRef("no_reflection_capture").toBool());
            }
        }

        AddResolutionDropdownImGui();

        dropdown_item = static_cast<int>(graphics->config_.full_screen());
        const char* fullscreen_choices[] = {
            "Windowed",
            "Fullscreen",
            "Windowed borderless",
            "Fullscreen borderless"
        };
        if (ImGui::Combo("Fullscreen mode", &dropdown_item, fullscreen_choices, IM_ARRAYSIZE(fullscreen_choices))){
            UpdateFullscreenMode(static_cast<FullscreenMode::Mode>(dropdown_item));
        }

        switch(graphics->config_.FSAA_samples()) {
        case 2: dropdown_item=1; break;
        case 4: dropdown_item=2; break;
        case 8: dropdown_item=3; break;
        default: dropdown_item=0; break;
        }
        const char* aa_choices[] = {
            "None",
            "2x",
            "4x",
            "8x"
        };
        if(ImGui::Combo("Anti-aliasing", &dropdown_item, aa_choices, IM_ARRAYSIZE(aa_choices))){
            switch(dropdown_item){
            case 0:
                UpdateMultisampleSetting(1); break;
            case 1:
                UpdateMultisampleSetting(2); break;
            case 2:
                UpdateMultisampleSetting(4); break;
            case 3:
                UpdateMultisampleSetting(8); break;
            }
        }

        switch(config["anisotropy"].toNumber<int>()) {
        case 2: dropdown_item=1; break;
        case 4: dropdown_item=2; break;
        case 8: dropdown_item=3; break;
        default: dropdown_item=0; break;
        }
        if(ImGui::Combo("Anisotropy", &dropdown_item, aa_choices, IM_ARRAYSIZE(aa_choices))){
            switch(dropdown_item){
            case 0:
                UpdateAnisotropy(1); break;
            case 1:
                UpdateAnisotropy(2); break;
            case 2:
                UpdateAnisotropy(4); break;
            case 3:
                UpdateAnisotropy(8); break;
            }
        }

        dropdown_item = config.GetRef("texture_reduce").toNumber<int>();
        const char* texture_detail_choices[] = {
            "Full",
            "1/2",
            "1/4",
            "1/8"
        };
        if(ImGui::Combo("Texture Detail (requires restart)", &dropdown_item, texture_detail_choices, IM_ARRAYSIZE(texture_detail_choices))){
            config.GetRef("texture_reduce") = dropdown_item;
        }


        dropdown_item = config.GetRef("imgui_scale").toNumber<int>();
        const char* imgui_scale_choices[] = {
            "100%",
            "125%",
            "150%",
            "175%",
            "200%",
            "225%",
            "250%",
            "275%",
            "300%"
        };
        if (ImGui::Combo("Debug Menu Scale", &dropdown_item, imgui_scale_choices, IM_ARRAYSIZE(imgui_scale_choices))) {
            config.GetRef("imgui_scale") = dropdown_item;
            imgui_scale = 1.0f + static_cast<float>(dropdown_item) / 4.0f;
        }


        checkbox_val = graphics->config_.vSync();
        if(ImGui::Checkbox("VSync", &checkbox_val)){
            config.GetRef("vsync") = checkbox_val;
            graphics->SetVsync(checkbox_val);
        }

        checkbox_val = graphics->config_.limit_fps_in_game();
        if(ImGui::Checkbox("Limit FPS In Game", &checkbox_val)){
            config.GetRef("limit_fps_in_game") = checkbox_val;
            graphics->SetLimitFpsInGame(checkbox_val);
        }

        {
            int max_fps_val = graphics->config_.max_frame_rate();
            if(ImGui::DragInt("Max FPS", &max_fps_val, 5.0f, 15, 300)){
                if(max_fps_val < 15){
                    max_fps_val = 15;
                }
                if(max_fps_val > 500){
                    max_fps_val = 500;
                }
                config.GetRef("max_frame_rate") = max_fps_val;
                graphics->SetMaxFrameRate(max_fps_val);
            }
        }

        checkbox_val = g_simple_shadows;
        if(ImGui::Checkbox("Simple shadows", &checkbox_val)){
            UpdateSimpleShadows(checkbox_val);
        }

        checkbox_val = config["simple_fog"].toNumber<bool>();
        if(ImGui::Checkbox("Simple fog", &checkbox_val)){
            UpdateSimpleFog(checkbox_val);
        }

        checkbox_val = config["simple_water"].toNumber<bool>();
        if(ImGui::Checkbox("Simple Water", &checkbox_val)){
            UpdateSimpleWater(checkbox_val);
        }

        bool depth_of_field = config["depth_of_field"].toNumber<bool>();
        if(ImGui::Checkbox("Depth of field", &depth_of_field)){
            UpdateDepthOfField(depth_of_field);
        }

        ImGui::BeginDisabled(!depth_of_field);
        checkbox_val = config["depth_of_field_reduced"].toNumber<bool>();
        if(ImGui::Checkbox("Depth of field reduce amount", &checkbox_val)){
            UpdateDepthOfFieldReduced(checkbox_val);
        }
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!g_attrib_envobj_intancing_support);
        checkbox_val = g_attrib_envobj_intancing_enabled;
        if(ImGui::Checkbox("Vertex Attrib Instancing", &checkbox_val)){
            UpdateAttribEnvObjInstancing(checkbox_val);
        }
        ImGui::EndDisabled();

        checkbox_val = config.GetRef("tet_mesh_lighting").toNumber<bool>();
        if(ImGui::Checkbox("Use tet mesh lighting", &checkbox_val)){
            UpdateTetMeshLighting(scenegraph, checkbox_val);
        }

        checkbox_val = config["light_volume_lighting"].toNumber<bool>();
        if(ImGui::Checkbox("Use ambient light volumes", &checkbox_val)){
            UpdateLightVolumes(scenegraph, checkbox_val);
        }

        bool detail_objects = graphics->config_.detail_objects();
        if(ImGui::Checkbox("Detail objects", &detail_objects)){
            UpdateDetailObjects(detail_objects);
        }

        ImGui::BeginDisabled(!detail_objects);
        checkbox_val = config["detail_objects_reduced"].toNumber<bool>();
        if(ImGui::Checkbox("Detail objects reduce count", &checkbox_val)) {
            UpdateDetailObjectsReduced(checkbox_val);
        }

        checkbox_val = config["detail_object_decals"].toNumber<bool>();
        if(ImGui::Checkbox("Detail object decals", &checkbox_val)) {
            UpdateDetailObjectDecals(checkbox_val);
        }

        checkbox_val = config["detail_object_lowres"].toNumber<bool>();
        if(ImGui::Checkbox("Lowres detail objects (needs restart)", &checkbox_val)) {
            UpdateDetailObjectLowres(checkbox_val);
        }

        checkbox_val = !config["detail_object_disable_shadows"].toNumber<bool>();
        if(ImGui::Checkbox("Detail object shadows", &checkbox_val)) {
            UpdateDetailObjectShadows(checkbox_val);
        }
        ImGui::EndDisabled();

		checkbox_val = config.GetRef("particle_field").toNumber<bool>();
		if(ImGui::Checkbox("GPU particle field", &checkbox_val)){
			config.GetRef("particle_field") = checkbox_val;
		}

        checkbox_val = config.GetRef("particle_field_simple").toNumber<bool>();
        if(ImGui::Checkbox("GPU particle field (simplify)", &checkbox_val)){
            config.GetRef("particle_field_simple") = checkbox_val;
        }

		checkbox_val = config.GetRef("custom_level_shaders").toNumber<bool>();
		if(ImGui::Checkbox("Custom level shaders", &checkbox_val)){
			config.GetRef("custom_level_shaders") = checkbox_val;
		}

        checkbox_val = config["no_reflection_capture"].toNumber<bool>();
        if(ImGui::Checkbox("No reflection capture", &checkbox_val)){
            UpdateNoReflectionCapture(checkbox_val);
        }

        int slider_val = (int)( graphics->config_.motion_blur_amount_ * 100.0f + 0.5f );
        if(ImGui::SliderInt("Motion blur", &slider_val, 0, 100, "%.0f%%")){
            UpdateMotionBlur(slider_val * 0.01f);
		}

		{
			int slider_val = (int)( config.GetRef("brightness").toNumber<float>() * 100.0f + 0.5f );
			if(ImGui::SliderInt("Brightness", &slider_val, 10, 200, "%.0f%%")){
				config.GetRef("brightness") = slider_val * 0.01f;
			}
		}

        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Audio")) {
        int slider_val = (int)(config.GetRef("music_volume").toNumber<float>() * 100.0f + 0.5f);
        if(ImGui::SliderInt("Music Volume", &slider_val, 0, 100, "%.0f%%")){
            float val = slider_val / 100.0f;
            config.GetRef("music_volume") = val;
            Engine::Instance()->GetSound()->SetMusicVolume(val);
        }

        slider_val = (int)(config.GetRef("master_volume").toNumber<float>() * 100.0f + 0.5f);
        if(ImGui::SliderInt("Master Volume", &slider_val, 0, 100, "%.0f%%")){
            float val = slider_val / 100.0f;
            config.GetRef("master_volume") = val;
            Engine::Instance()->GetSound()->SetMasterVolume(val);
        }

        static int device_selection = -1;
        std::vector<std::string> available_devices = Engine::Instance()->GetSound()->GetAvailableDevices();
        if( device_selection == -1 ) {
            for( unsigned i = 0; i < available_devices.size(); i++ ) {
                if( strmtch( available_devices[i], config["preferred_audio_device"].str() ) ) {
                    device_selection = i;
                }
            }
        }
        std::string used_device = Engine::Instance()->GetSound()->GetUsedDevice();
        const char* devices[16];
        int audio_device_count = 0;
        for( unsigned i = 0; i < 16 && i < available_devices.size(); i++ ) {
            devices[i] = available_devices[i].c_str();
            if( device_selection == -1 ) {
                if( strmtch(used_device, available_devices[i]) ) {
                    device_selection = i;
                }
            }
            audio_device_count++;
        }
        if( device_selection == -1 ) {
            device_selection = 0;
        }
        ImGui::Text("Used Device: %s", used_device.c_str());
        ImGui::Text("You will need to restart to apply new audio device usage.");
        if(ImGui::Combo("Preferred Audio Device", &device_selection, devices, audio_device_count)) {
            if( device_selection < audio_device_count ) {
                LOGI << "Setting new preferred audio device: " << devices[device_selection] << std::endl;
                config.GetRef("preferred_audio_device") = devices[device_selection];
            }
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Game")) {
        int dropdown_val;
        dropdown_val = 2 - config.GetRef("blood").toNumber<int>();
        if(ImGui::Combo("Blood amount", &dropdown_val, "Full\0No dripping\0None\0\0")){
            int val = 2 -dropdown_val;
            config.GetRef("blood") = val;
            Graphics::Instance()->config_.SetBlood(val);
        }

        dropdown_val = 0;
        const float kBloodColorThreshold = 0.01f;
        vec3 blood_color = GraphicsConfig::BloodColorFromString(config["blood_color"].str());
        if(distance_squared(blood_color, kGreenBloodColor) < kBloodColorThreshold){
            dropdown_val = 1;
        } else if(distance_squared(blood_color, kCyanBloodColor) < kBloodColorThreshold){
            dropdown_val = 2;
        } else if(distance_squared(blood_color, kBlackBloodColor) < kBloodColorThreshold){
            dropdown_val = 3;
        }
        if(ImGui::Combo("Blood color", &dropdown_val, "Red\0Green\0Cyan\0Black\0\0")){
            vec3 col;
            switch(dropdown_val){
            case 0: col = kRedBloodColor; break;
            case 1: col = kGreenBloodColor; break;
            case 2: col = kCyanBloodColor; break;
            case 3: col = kBlackBloodColor; break;
            }
            std::ostringstream oss;
            oss << col[0] << " " << col[1] << " " << col[2];
            config.GetRef("blood_color") = oss.str();
            Graphics::Instance()->config_.SetBloodColor(col);
        }

        bool checkbox_val = config["split_screen"].toNumber<bool>();
        if(ImGui::Checkbox("Split screen", &checkbox_val)){
            config.GetRef("split_screen") = checkbox_val;
            Graphics::Instance()->config_.SetSplitScreen(checkbox_val);
        }

        bool tutorials = config["tutorials"].toBool();
        if(ImGui::Checkbox("Tutorials", &tutorials)) {
            config.GetRef("tutorials") = tutorials;
        }

		checkbox_val = config["auto_ledge_grab"].toNumber<bool>();
		if(ImGui::Checkbox("Automatic ledge grab", &checkbox_val)){
			config.GetRef("auto_ledge_grab") = checkbox_val;
		}

        std::vector<std::string> diff_presets = config.GetDifficultyPresets();
        const char* custom_fallback = "Custom";
        std::vector<const char*> diff_presets_c;

        std::string diff_current = config.GetClosestDifficulty();

		int dropdown_item = -1;

        for( size_t i = 0; i < diff_presets.size(); i++ ) {
            if( diff_presets[i] == diff_current ) {
                dropdown_item = i;
            }
            diff_presets_c.push_back(diff_presets[i].c_str());
        }

        if(dropdown_item == -1) {
            diff_presets_c.push_back(custom_fallback);
            dropdown_item = diff_presets_c.size()-1;
        }

		if(ImGui::Combo("Difficulty Preset", &dropdown_item, &diff_presets_c[0], diff_presets.size())){
            config.SetDifficultyPreset(diff_presets[dropdown_item]);
            config.ReloadDynamicSettings();
		}

        float global_time_scale_mult = config["global_time_scale_mult"].toNumber<float>();
        if(ImGui::SliderFloat("Game Speed", &global_time_scale_mult, 0.5f, 1.0f)){
			SetGameSpeed(global_time_scale_mult, true);
        }

        float game_difficulty = config["game_difficulty"].toNumber<float>();
        if(ImGui::SliderFloat("Game Difficulty", &game_difficulty, 0.0f, 1.0f)){
            config.GetRef("game_difficulty") = game_difficulty;
		}

        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Controls")) {
        int slider_val;
        slider_val = (int)(config["mouse_sensitivity"].toNumber<float>()*40.0f + 0.5f);
        if(ImGui::SliderInt("Mouse sensitivity", &slider_val, 0, 100, "%.0f%%")){
            float val = slider_val / 40.0f;
            config.GetRef("mouse_sensitivity") = val;
            Input::Instance()->SetMouseSensitivity(val);
        }

        bool checkbox_val;
        checkbox_val = config["invert_x_mouse_look"].toNumber<bool>();
        if(ImGui::Checkbox("Invert mouse X", &checkbox_val)){
            config.GetRef("invert_x_mouse_look") = checkbox_val;
            Input::Instance()->SetInvertXMouseLook(checkbox_val);
        }

        checkbox_val = config["invert_y_mouse_look"].toNumber<bool>();
        if(ImGui::Checkbox("Invert mouse Y", &checkbox_val)){
            config.GetRef("invert_y_mouse_look") = checkbox_val;
            Input::Instance()->SetInvertYMouseLook(checkbox_val);
        }

        checkbox_val = config["use_raw_input"].toNumber<bool>();
        if(ImGui::Checkbox("Raw mouse input", &checkbox_val)){
            config.GetRef("use_raw_input") = checkbox_val;
            Input::Instance()->UseRawInput(checkbox_val);
        }

        checkbox_val = config["auto_camera"].toNumber<bool>();
        if(ImGui::Checkbox("Automatic camera", &checkbox_val)){
            config.GetRef("auto_camera") = checkbox_val;
            int num_cameras = ActiveCameras::NumCameras();
            int curr_id = ActiveCameras::GetID();
            for(int i=0; i<num_cameras; ++i){
                ActiveCameras::Set(i);
                ActiveCameras::Get()->SetAutoCamera(checkbox_val);
            }
            ActiveCameras::Set(curr_id);
        }

        static int key_selected = -1;
        const char* commands[] = {
            "Forwards",
            "key[up]",
            "Backwards",
            "key[down]",
            "Left",
            "key[left]",
            "Right",
            "key[right]",
            "Jump",
            "key[jump]",
            "Crouch",
            "key[crouch]",
            "Slow motion",
            "key[slow]",
            "Equip/sheathe item",
            "key[item]",
            "Throw/pick-up item",
            "key[drop]",
            "\0"
        };

        ImGui::Columns(2);
        int id=0;
        for(int i=0; commands[i][0] != '\0'; i+=2){
            ImGui::PushItemWidth(-1);
            if(i == key_selected){
                ImGui::Text("Enter input");
            } else {
                ImGui::PushID(id);
                if(ImGui::Button(config[commands[i+1]].str().c_str())){
                    key_selected = i;
                }
                ImGui::PopID();
                ++id;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();
            ImGui::PushID(id);
            ImGui::Text("%s", commands[i]);
            ImGui::PopID();
            ++id;
            ImGui::NextColumn();
        }
        if(key_selected != -1){
            ImGui::CaptureKeyboardFromApp();
            ImGui::CaptureMouseFromApp();
            for(int i=0; i<512; ++i){
                if(ImGui::IsKeyDown(i)){
                    int val = i;
                    if(val > SDLK_z){
                        val = val + SDLK_CAPSLOCK - 1 - SDLK_z;
                    }
                    config.GetRef(commands[key_selected+1]) = SDLKeycodeToString((SDL_Keycode)val);
                    Input::Instance()->SetFromConfig(config);
                    key_selected = -1;
                }
            }
        }
        ImGui::Columns(1);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Camera")) {
        int slider_val;
        slider_val = static_cast<int>(config["chase_camera_fov"].toNumber<float>());

        if (slider_val == 0) {
            slider_val = 90;
        }

        if (ImGui::SliderInt("Chase Camera FOV", &slider_val, 1, 180, "%.0f degrees")) {
            config.GetRef("chase_camera_fov") = static_cast<float>(slider_val);
        }

        slider_val = static_cast<int>(config["editor_camera_fov"].toNumber<float>());

        if (slider_val == 0) {
            slider_val = 90;
        }

        if (ImGui::SliderInt("Editor Camera FOV", &slider_val, 1, 180, "%.0f degrees")) {
            config.GetRef("editor_camera_fov") = static_cast<float>(slider_val);
        }

        ImGui::Text("+/- key to change current FOV");
        ImGui::Text("0 to reset to 90");

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Debug")) {
        bool checkbox_val;

        checkbox_val = config["debug_keys"].toNumber<bool>();
        if(ImGui::Checkbox("Debug keys", &checkbox_val)){
            config.GetRef("debug_keys") = checkbox_val;
            Input::Instance()->debug_keys = checkbox_val;
        }

		checkbox_val = config["skip_loading_pause"].toNumber<bool>();
		if(ImGui::Checkbox("Skip Loading Pause 'press any key'", &checkbox_val)){
			config.GetRef("skip_loading_pause") = checkbox_val;
		}

        ImGui::Checkbox("Dynamic Character Cubemap", &Graphics::Instance()->config_.dynamic_character_cubemap_);

        checkbox_val = config["decal_normals"].toNumber<bool>();
        if(ImGui::Checkbox("Decal normal maps", &checkbox_val)){
            config.GetRef("decal_normals") = checkbox_val;
        }

        checkbox_val = config["visible_raycasts"].toNumber<bool>();
        if(ImGui::Checkbox("Visible Raycasts", &checkbox_val)){
            config.GetRef("visible_raycasts") = checkbox_val;
        }

        checkbox_val = config["visible_sound_spheres"].toNumber<bool>();
        if(ImGui::Checkbox("Visible Sound Spheres", &checkbox_val)){
            config.GetRef("visible_sound_spheres") = checkbox_val;
            DebugDrawAux::Instance()->visible_sound_spheres = checkbox_val;
        }

        checkbox_val = config["sound_label"].toNumber<bool>();
        if(ImGui::Checkbox("Sound Display", &checkbox_val)){
            config.GetRef("sound_label") = checkbox_val;
        }

        checkbox_val = config["fps_label"].toNumber<bool>();
        if(ImGui::Checkbox("FPS Display", &checkbox_val)){
            config.GetRef("fps_label") = checkbox_val;
        }

        checkbox_val = config["editor_mode"].toNumber<bool>();
        if(ImGui::Checkbox("Start Level In Editor", &checkbox_val)){
            config.GetRef("editor_mode") = checkbox_val;
        }

        checkbox_val = config["main_menu"].toNumber<bool>();
        if(ImGui::Checkbox("Start In Main Menu", &checkbox_val)){
            config.GetRef("main_menu") = checkbox_val;
        }

        const int kBufSize = 256;
        char buf[kBufSize];
        FormatString(buf, kBufSize, "%s", config["debug_load_level"].str().c_str());
        if(ImGui::InputText("Level to load", buf, kBufSize)){
            config.GetRef("debug_load_level") = buf;
        }

        checkbox_val = config["seamless_cubemaps"].toNumber<bool>();
        if(ImGui::Checkbox("Seamless Cubemaps", &checkbox_val)){
            config.GetRef("seamless_cubemaps") = checkbox_val;
            Graphics::Instance()->SetSeamlessCubemaps(checkbox_val);
        }

        /*
        checkbox_val = config["background_process_pool"].toNumber<bool>();
        if(ImGui::Checkbox("Background Process Pool", &checkbox_val)){
            config.GetRef("background_process_pool") = checkbox_val;
            Textures::Instance()->SetProcessPoolsEnabled(checkbox_val);
        }
        */

        if (ImGui::BeginMenu("Animation")) {
            ImGui::Checkbox("Disable IK", &animation_config.kDisableIK);
            ImGui::Checkbox("Disable Modifiers", &animation_config.kDisableModifiers);
            ImGui::Checkbox("Disable Soft Animation", &animation_config.kDisableSoftAnimation);
            ImGui::Checkbox("Disable Interpolation", &animation_config.kDisableInterpolation);
            ImGui::Checkbox("Disable Animation Layers", &animation_config.kDisableAnimationLayers);
            ImGui::Checkbox("Disable Animation Mix", &animation_config.kDisableAnimationMix);
            ImGui::Checkbox("Disable Animation Transition", &animation_config.kDisableAnimationTransition);
            ImGui::Checkbox("Disable Physics Interpolation", &animation_config.kDisablePhysicsInterpolation);
            ImGui::Checkbox("Force Idle Anim", &animation_config.kForceIdleAnim);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Movement")) {
            checkbox_val = config["debug_draw_collision_spheres"].toNumber<bool>();
            if(ImGui::Checkbox("Show Collision Spheres", &checkbox_val)){
                config.GetRef("debug_draw_collision_spheres") = checkbox_val;
            }

            checkbox_val = config["debug_draw_combat_debug"].toNumber<bool>();
            if(ImGui::Checkbox("Show Combat State", &checkbox_val)){
                config.GetRef("debug_draw_combat_debug") = checkbox_val;
            }

            checkbox_val = config["debug_draw_vis_lines"].toNumber<bool>();
            if(ImGui::Checkbox("Draw Vis Lines", &checkbox_val)){
                config.GetRef("debug_draw_vis_lines") = checkbox_val;
            }

            checkbox_val = config["debug_draw_bones"].toNumber<bool>();
            if(ImGui::Checkbox("Draw Bones", &checkbox_val)){
                config.GetRef("debug_draw_bones") = checkbox_val;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("AI")) {
            checkbox_val = config["debug_show_ai_state"].toNumber<bool>();
            if(ImGui::Checkbox("Show AI State", &checkbox_val)){
                config.GetRef("debug_show_ai_state") = checkbox_val;
            }

            checkbox_val = config["debug_draw_stealth_debug"].toNumber<bool>();
            if(ImGui::Checkbox("Show AI Awareness State", &checkbox_val)){
                config.GetRef("debug_draw_stealth_debug") = checkbox_val;
            }

            checkbox_val = config["debug_show_ai_path"].toNumber<bool>();
            if(ImGui::Checkbox("Show AI Path", &checkbox_val)){
                config.GetRef("debug_show_ai_path") = checkbox_val;
            }

            checkbox_val = config["debug_show_ai_jump"].toNumber<bool>();
            if(ImGui::Checkbox("Show AI Jump", &checkbox_val)){
                config.GetRef("debug_show_ai_jump") = checkbox_val;
            }

            checkbox_val = config["debug_show_ai_investigate"].toNumber<bool>();
            if(ImGui::Checkbox("Show AI Investigate", &checkbox_val)){
                config.GetRef("debug_show_ai_investigate") = checkbox_val;
            }

            checkbox_val = config["debug_mouse_path_test"].toNumber<bool>();
            if(ImGui::Checkbox("AI Mouse Path Testing", &checkbox_val)){
                config.GetRef("debug_mouse_path_test") = checkbox_val;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Graphics")) {
            checkbox_val = config["albedo_only"].toNumber<bool>();
            if(ImGui::Checkbox("Albedo Only", &checkbox_val)){
                config.GetRef("albedo_only") = checkbox_val;
                g_albedo_only = checkbox_val;
            }

            checkbox_val = config["no_decals"].toNumber<bool>();
            if(ImGui::Checkbox("No Light/Decal system", &checkbox_val)){
                config.GetRef("no_decals") = checkbox_val;
                g_no_decals = checkbox_val;
            }

            checkbox_val = config["no_decal_elements"].toNumber<bool>();
            if(ImGui::Checkbox("No Lights or Decals", &checkbox_val)){
                config.GetRef("no_decal_elements") = checkbox_val;
                g_no_decal_elements = checkbox_val;
            }

            checkbox_val = config["occlusion_query"].toNumber<bool>();
            if(ImGui::Checkbox("Occlusion Queries", &checkbox_val)){
                config.GetRef("occlusion_query") = checkbox_val;
                g_perform_occlusion_query = checkbox_val;
            }

			checkbox_val = config["gamma_correct_final_output"].toNumber<bool>();
			if(ImGui::Checkbox("Gamma Correct Final Output", &checkbox_val)){
				config.GetRef("gamma_correct_final_output") = checkbox_val;
				g_gamma_correct_final_output = checkbox_val;
			}

            // if (GLEW_VERSION_4_0) {
            //     checkbox_val = config["single_pass_shadow_cascade"].toNumber<bool>();
            //     if(ImGui::Checkbox("Single-Pass Shadow Cascade", &checkbox_val)){
            //         config.GetRef("single_pass_shadow_cascade") = checkbox_val;
            //         g_single_pass_shadow_cascade = checkbox_val;
            //     }
            // }

            checkbox_val = config["no_detailmaps"].toNumber<bool>();
            if(ImGui::Checkbox("Disable detail maps", &checkbox_val)){
                config.GetRef("no_detailmaps") = checkbox_val;
                g_no_detailmaps = checkbox_val;
            }

            checkbox_val = g_ubo_batch_multiplier_force_1x;
            if(ImGui::Checkbox("Disable UBO Batch Multiplier", &checkbox_val)){
                // Not saved to config (at least for now)
                g_ubo_batch_multiplier_force_1x = checkbox_val;
            }

            checkbox_val = config["volume_shadows"].toNumber<bool>();
            if(ImGui::Checkbox("Volume Shadows", &checkbox_val)){
                config.GetRef("volume_shadows") = checkbox_val;
            }

            checkbox_val = config["ssao"].toNumber<bool>();
            if(ImGui::Checkbox("SSAO", &checkbox_val)){
                config.GetRef("ssao") = checkbox_val;
            }

			checkbox_val = config["disable_fog"].toNumber<bool>();
			if(ImGui::Checkbox("Disable Fog", &checkbox_val)){
				config.GetRef("disable_fog") = checkbox_val;
				g_disable_fog = config["disable_fog"].toNumber<bool>();
			}

            checkbox_val = graphics->config_.gpu_skinning();
            if(ImGui::Checkbox("GPU Skinning", &checkbox_val)){
                config.GetRef("gpu_skinning") = checkbox_val;
                graphics->config_.SetGPUSkinning(checkbox_val);
                if( scenegraph ) {
                    scenegraph->map_editor->UpdateGPUSkinning();
                }
            }

			checkbox_val = config["opengl_callback_errors"].toNumber<bool>();
			if(ImGui::Checkbox("OpenGL Callbacks Errors", &checkbox_val)){
				config.GetRef("opengl_callback_errors") = checkbox_val;
			}

			checkbox_val = config["opengl_callback_error_dialog"].toNumber<bool>();
			if(ImGui::Checkbox("Dialog on OpenGL error", &checkbox_val)){
				config.GetRef("opengl_callback_error_dialog") = checkbox_val;
			}

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Sound")) {
            checkbox_val = config["use_soundtrack_limiter"].toBool();
            if(ImGui::Checkbox("Use Soundtrack Limiter", &checkbox_val)) {
                UpdateEnableLayeredSoundtrackLimiter(checkbox_val);
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Game")) {
            checkbox_val = config["block_cheating_progress"].toBool();
            if(ImGui::Checkbox("Block Cheating Progress", &checkbox_val)) {
				config.GetRef("block_cheating_progress") = checkbox_val;
            }

            ImGui::EndMenu();
        }

        checkbox_val = config["allow_game_dir_save"].toNumber<bool>();
        if(ImGui::Checkbox("Allow Game Directory Save", &checkbox_val)){
            config.GetRef("allow_game_dir_save") = checkbox_val;
        }

		checkbox_val = config["no_auto_nav_mesh"].toNumber<bool>();
		if(ImGui::Checkbox("Disable Auto Navmesh Regen", &checkbox_val)){
			config.GetRef("no_auto_nav_mesh") = checkbox_val;
		}

		checkbox_val = config["no_texture_convert"].toNumber<bool>();
		if(ImGui::Checkbox("Disable Auto Texture Conversion", &checkbox_val)){
			config.GetRef("no_texture_convert") = checkbox_val;
		}

        checkbox_val = config.HasKey("list_include_files_when_logging_mod_errors")
            ? config["list_include_files_when_logging_mod_errors"].toNumber<bool>()
            : true;
        if(ImGui::Checkbox("List Include Files When Logging Mod Errors", &checkbox_val)){
            config.GetRef("list_include_files_when_logging_mod_errors") = checkbox_val;
        }

        ImGui::Separator();

        if(ImGui::Button("Clear local write cache (dangerous)")){
            ClearCache(false);
        }

        if(ImGui::Button("List files that would be cleared from cache")){
            ClearCache(true);
        }

        ImGui::Separator();

        if(ImGui::Button("Show test error message")){
            DisplayError("Error test title", "Error test body");
        }

        if( ImGui::Button("Dump state" ) ) {
             SceneGraph* s = Engine::Instance()->GetSceneGraph();
             if(s) {
                s->DumpState();
             }
             DumpAngelscriptStates();
             LogSystem::Flush();
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("Campaign Progress (Dangerous)")) {
            if(ImGui::Button("Unlock all campaign levels")){
                std::vector<ModInstance::Campaign> campaigns = ModLoading::Instance().GetCampaigns();
                for( size_t i = 0; i < campaigns.size(); i++ ) {
                    ModInstance::Campaign camp = campaigns[i];
                    SavedLevel& s = Engine::Instance()->save_file_.GetSave(camp.id.str(),"linear_campaign","");
                    for( size_t k = 0; k < camp.levels.size(); k++ ) {
                        s.SetArrayValue("unlocked_levels",k,camp.levels[k].id.str());
                    }
                }
                Engine::Instance()->save_file_.QueueWriteInPlace();
            }

            if(ImGui::BeginMenu("Unlock level in campaign")) {
                std::vector<ModInstance::Campaign> campaigns = ModLoading::Instance().GetCampaigns();
                for( size_t i = 0; i < campaigns.size(); i++ ) {
                    ModInstance::Campaign camp = campaigns[i];
                    if(ImGui::BeginMenu(camp.title.c_str())) {
                        for( size_t k = 0; k < camp.levels.size(); k++ ) {
                            if(ImGui::Button(camp.levels[k].title.c_str())) {
                                SavedLevel& s = Engine::Instance()->save_file_.GetSave(camp.id.str(),"linear_campaign","");
                                s.AppendArrayValueIfUnique("unlocked_levels",camp.levels[k].id.str());
                                Engine::Instance()->save_file_.QueueWriteInPlace();
                            }
                        }
                        ImGui::EndMenu();
                    }
                }
                ImGui::EndMenu();
            }

            if(ImGui::Button("Clear all campaign progress")){
                std::vector<ModInstance::Campaign> campaigns = ModLoading::Instance().GetCampaigns();
                for( size_t i = 0; i < campaigns.size(); i++ ) {
                    ModInstance::Campaign camp = campaigns[i];
                    SavedLevel& s = Engine::Instance()->save_file_.GetSave(camp.id.str(),"linear_campaign","");
                    s.ClearData();
                }
                Engine::Instance()->save_file_.QueueWriteInPlace();
            }

            std::vector<std::string> difficulties = config.GetDifficultyPresets();

            if(ImGui::BeginMenu("Finish All Levels On Difficulty")) {
                for( size_t k = 0; k < difficulties.size(); k++ ) {
                    if( ImGui::Button(difficulties[k].c_str())) {
                        std::vector<ModInstance::Campaign> campaigns = ModLoading::Instance().GetCampaigns();
                        for( size_t i = 0; i < campaigns.size(); i++ ) {
                            ModInstance::Campaign camp = campaigns[i];
                            for( size_t j = 0; j < camp.levels.size(); j++ ) {
                                SavedLevel& s = Engine::Instance()->save_file_.GetSave(camp.id.str(),"linear_campaign",camp.levels[j].id.str());
                                if( false == s.ArrayContainsValue("finished_difficulties",difficulties[k]) ) {
                                    s.AppendArrayValue("finished_difficulties",difficulties[k]);
                                }
                            }
                        }
                        Engine::Instance()->save_file_.QueueWriteInPlace();
                    }
                }
                ImGui::EndMenu();
            }

            if(ImGui::BeginMenu("Finish Level On Difficulty")) {
                std::vector<ModInstance::Campaign> campaigns = ModLoading::Instance().GetCampaigns();
                for( size_t i = 0; i < campaigns.size(); i++ ) {
                    ModInstance::Campaign camp = campaigns[i];
                    if(ImGui::BeginMenu(camp.title)) {
                        for( size_t j = 0; j < camp.levels.size(); j++ ) {
                            ModInstance::Level level = camp.levels[j];
                            if(ImGui::BeginMenu(level.title)) {
                                for( size_t k = 0; k < difficulties.size(); k++ ) {
                                    if( ImGui::Button(difficulties[k].c_str())) {
                                        SavedLevel& s = Engine::Instance()->save_file_.GetSave(camp.id.str(),"linear_campaign",level.id.str());
                                        if( false == s.ArrayContainsValue("finished_difficulties",difficulties[k]) ) {
                                            s.AppendArrayValue("finished_difficulties",difficulties[k]);
                                        }
                                        Engine::Instance()->save_file_.QueueWriteInPlace();
                                    }
                                }
                                ImGui::EndMenu();
                            }
                        }
                        ImGui::EndMenu();
                    }
                }
                ImGui::EndMenu();
            }

            if(ImGui::Button("Clear All Linear Level Data")) {
                std::vector<ModInstance::Campaign> campaigns = ModLoading::Instance().GetCampaigns();
                for( size_t i = 0; i < campaigns.size(); i++ ) {
                    ModInstance::Campaign camp = campaigns[i];
                    for( size_t j = 0; j < camp.levels.size(); j++ ) {
                        SavedLevel& s = Engine::Instance()->save_file_.GetSave(camp.id.str(),"linear_campaign",camp.levels[j].id.str());
                        s.ClearData();
                    }
                }
                Engine::Instance()->save_file_.QueueWriteInPlace();
            }

            if(ImGui::BeginMenu("Clear Save Data For Level")) {
                std::vector<ModInstance::Campaign> campaigns = ModLoading::Instance().GetCampaigns();
                for( size_t i = 0; i < campaigns.size(); i++ ) {
                    ModInstance::Campaign camp = campaigns[i];
                    if(ImGui::BeginMenu(camp.title)) {
                        for( size_t j = 0; j < camp.levels.size(); j++ ) {
                            ModInstance::Level level = camp.levels[j];
                            if(ImGui::Button(level.title)) {
                                SavedLevel& s = Engine::Instance()->save_file_.GetSave(camp.id.str(),"linear_campaign",level.id.str());
                                s.ClearData();
                                Engine::Instance()->save_file_.QueueWriteInPlace();
                            }
                        }
                        ImGui::EndMenu();
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        checkbox_val = config.GetRef("player_invincible").toNumber<bool>();
        if(ImGui::Checkbox("Make player invincible", &checkbox_val)) {
            config.GetRef("player_invincible") = checkbox_val;
        }

        ImGui::EndMenu();
    }
}
