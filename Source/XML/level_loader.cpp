//-----------------------------------------------------------------------------
//           Name: level_loader.cpp
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

#include <Internal/profiler.h>
#include <Internal/common.h>
#include <Internal/filesystem.h>
#include <Internal/dialogues.h>
#include <Internal/error.h>
#include <Internal/datemodified.h>
#include <Internal/locale.h>
#include <Internal/memwrite.h>
#include <Internal/levelxml.h>
#include <Internal/config.h>

#include <Objects/terrainobject.h>
#include <Objects/cameraobject.h>
#include <Objects/editorcameraobject.h>
#include <Objects/envobject.h>
#include <Objects/movementobject.h>
#include <Objects/group.h>
#include <Objects/hotspot.h>
#include <Objects/decalobject.h>
#include <Objects/hotspot.h>

#include <XML/xml_helper.h>
#include <XML/level_loader.h>

#include <Graphics/models.h>
#include <Graphics/camera.h>
#include <Graphics/sky.h>
#include <Graphics/graphics.h>
#include <Graphics/shaders.h>
#include <Graphics/font_renderer.h>
#include <Graphics/particles.h>
#include <Graphics/text.h>

#include <Asset/Asset/ambientsounds.h>
#include <Asset/Asset/skeletonasset.h>
#include <Asset/Asset/syncedanimation.h>
#include <Asset/Asset/voicefile.h>

#include <Sound/sound.h>
#include <Sound/threaded_sound_wrapper.h>

#include <Main/scenegraph.h>
#include <Math/vec4.h>
#include <Editors/map_editor.h>
#include <Game/level.h>
#include <Logging/logdata.h>
#include <GUI/widgetframework.h>
#include <Online/online.h>

#include <tinyxml.h>

extern std::string script_dir_path;
extern bool g_level_shadows;

void LoadTerrain(SceneGraph& s, const TerrainInfo& ti) {
    AddLoadingText("Loading terrain object...");
    
    TerrainObject* terrain_object = NULL;
    {
        PROFILER_ZONE(g_profiler_ctx, "Creating terrain object");
        terrain_object = new TerrainObject(ti);        
    }
    terrain_object->SetID(0);
    {
        PROFILER_ZONE(g_profiler_ctx, "Adding terrain object to scenegraph");
        s.addObject(terrain_object);
    }
    s.terrain_object_ = terrain_object;
    {
        PROFILER_ZONE(g_profiler_ctx, "Preparing terrain physics");
        terrain_object->PreparePhysicsMesh();
    }
}

void AddCamera(SceneGraph &s) {
    EditorCameraObject* new_object = new EditorCameraObject();
    new_object->SetTranslation(vec3(0,0,0));
    new_object->controlled = true;
    new_object->has_position_initialized = false;
    s.addObject(new_object);
    ActiveCameras::Get()->SetCameraObject((CameraObject*)new_object);
    ActiveCameras::Get()->SetFlags(Camera::kEditorCamera);
}

static void LoadAll(const PathSet &path_set, ThreadedSound* sound) {
    std::list<std::string> skeleton_paths;
    for(const auto & entry : path_set) {
        int space_pos = entry.find(' ');
        const std::string &type = entry.substr(0,space_pos);
        const std::string &path = entry.substr(space_pos+1, entry.size()-(space_pos+1));
        static const int kBufSize = 1024;
        char buf[kBufSize];
        FormatString(buf, kBufSize, "Loading %s from %s", type.c_str(), path.c_str());
        PROFILER_ZONE_DYNAMIC_STRING(g_profiler_ctx, buf);
        switch(type[0]){
            case 'a':
                switch(type[1]){
                    case 'm':
                        if(type == "ambient_sound"){Engine::Instance()->GetAssetManager()->LoadSync<AmbientSound>(path);}
                        break;
                    case 'n':
                        if(type == "animation"){Engine::Instance()->GetAssetManager()->LoadSync<Animation>(path);}
                        break;
                    case 't':
                        if(type == "attack"){Engine::Instance()->GetAssetManager()->LoadSync<Attack>(path);}                  
                        break;
                }
                break;
            case 'c':
                if(type == "character"){Engine::Instance()->GetAssetManager()->LoadSync<Character>(path);}
                break;
            case 'd':
                if(type == "decal"){Engine::Instance()->GetAssetManager()->LoadSync<DecalFile>(path);}
                break;
            case 'f':
                if(type == "font"){FontRenderer::Instance()->PreLoadFont(path);}
                break;
            case 'h'://heightmap
                break;
            case 'i':
                if(type == "image_sample"){Engine::Instance()->GetAssetManager()->LoadSync<ImageSampler>(path);}
                break;
            case 'l'://level
                break;
            case 'm':
                switch(type[1]){
                    case 'a':
                        if(type == "material"){Engine::Instance()->GetAssetManager()->LoadSync<Material>(path);}
                        break;
                    case 'o'://model
                        break;
                }
                break;
            case 'o': //object
                break;
            case 'p'://particle
                if(type == "particle"){Engine::Instance()->GetAssetManager()->LoadSync<ParticleType>(path);}
                break;
            case 's':
                switch(type[1]){
                    case 'c'://script
                        break;
                    case 'h'://shader
                        break;
                    case 'k':
                        if(type == "skeleton"){
                            //SkeletonAssets::Instance()->ReturnRef(path);
                            Engine::Instance()->GetAssetManager()->LoadSync<SkeletonAsset>(path);
                            skeleton_paths.push_back(path);
                        }
                        break;
                    case 'o':
                        if(type == "sound"){
                            sound->LoadSoundFile(path);
                        } else if(type == "soundgroup"){
                            //SoundGroupInfoCollection::Instance()->ReturnRef(path);
                            Engine::Instance()->GetAssetManager()->LoadSync<SoundGroupInfo>(path);
                        }
                        break;
                    case 'y':
                         if(type == "synced_animation"){Engine::Instance()->GetAssetManager()->LoadSync<SyncedAnimationGroup>(path);}
                        break;
                }
                break;
            case 't':
                //Textures::Instance()->returnTextureAssetRef(path);
                break;
            case 'v':
                if(type == "voice"){Engine::Instance()->GetAssetManager()->LoadSync<VoiceFile>(path);}
                break;        
        }
    }
    LOGI << "Finished loading path set" << std::endl;
}

struct IDAndType {
	int id;
	EntityType type;
	EntityDescription *desc;
    uint32_t counter; //we use this counter to guarantee the same order on all systems when two objects are otherwise identical.
};

static int IDAndType_CompareID(const void *a, const void *b) {
	IDAndType *a_ptr = (IDAndType*)a;
	IDAndType *b_ptr = (IDAndType*)b;
	int cmp = a_ptr->id - b_ptr->id;
	if(cmp == 0) {
        //We add to the type value, so that we have space to prioritize special types
        int a_val = a_ptr->type + 3;
        int b_val = b_ptr->type + 3;

        //We override group and prefab to be prioritized first, unknown why.
        if(a_ptr->type == _group) {
            a_val = 0;
        }
        if(b_ptr->type == _group) {
            b_val = 0;
        }

        if(a_ptr->type == _prefab) {
            a_val = 1;
        }
        if(b_ptr->type == _prefab) {
            b_val = 1;
        }
        //We prioritize movement_object, because there have been cases where it loses its id, breaking dialogues
        if(a_ptr->type == _movement_object) {
            a_val = 2;
        }
        if(b_ptr->type == _movement_object) {
            b_val = 2;
        }

        cmp = a_val - b_val;

        if( cmp == 0 ) {
            return a_ptr->counter - b_ptr->counter;
        }
	}
	return cmp;
}

// Finds entity IDs that are used more than once, and replaces repeats with -1 so they will be automatically reassigned
static void ExtractFlatList(EntityDescriptionList *desc_list, std::vector<IDAndType> &id_used){
    uint32_t counter = 1; //Zero is assigned to the terrain
	//bool print_results = false;
	for(auto & it : *desc_list)
	{
		id_used.resize(id_used.size()+1);
		IDAndType *id_and_type = &id_used.back();
		id_and_type->desc = &it;
		id_and_type->desc->GetEditableField(EDF_ENTITY_TYPE)->ReadInt((int*)&id_and_type->type);
		id_and_type->desc->GetEditableField(EDF_ID)->ReadInt(&id_and_type->id);
        id_and_type->counter = counter++;
		if(!id_and_type->desc->children.empty()){
			ExtractFlatList(&id_and_type->desc->children, id_used);
		}
	}
}

static void SetDescID(EntityDescription *desc, int id){
	EntityDescriptionField* edf = desc->GetEditableField(EDF_ID);
    if( edf )
    {
        edf->data.clear();
        edf->WriteInt(id);
    }
    else
    {
        LOGE << "EDF_ID object returned is NULL, unexpected behaviour" << std::endl;
    }
}

static void FixDuplicateIDs(EntityDescriptionList *desc_list, bool has_terrain){
	std::vector<IDAndType> id_used;
	ExtractFlatList(desc_list, id_used);
    if(!id_used.empty()){
        EntityDescription terrain_desc;
        if(has_terrain){
            IDAndType terrain_id_and_type;
            terrain_id_and_type.id = 0;
            terrain_id_and_type.counter = 0;
            terrain_id_and_type.type = _terrain_type;
            terrain_id_and_type.desc = &terrain_desc;
            id_used.push_back(terrain_id_and_type);
        }
	    qsort(&id_used[0], id_used.size(), sizeof(id_used[0]), IDAndType_CompareID);
	    int free_index = 1;
	    int free_id = -1;
	    for(int i=1, len=id_used.size(); i<len; ++i){
		    if(id_used[i-1].id == id_used[i].id){
			    while(free_index != len && (id_used[free_index].id < id_used[free_index-1].id+2 || id_used[free_index].id < free_id+2)){
				    free_id = id_used[free_index].id;
				    ++free_index;
			    }
			    ++free_id;
			    if(id_used[i-1].type == _group || id_used[i-1].type == _prefab){
                    LOGW << "Object loaded of type " << id_used[i-1].type << " is changing id from " << id_used[i-1].id << " to " << free_id << " due to collision." << std::endl;
				    SetDescID(id_used[i-1].desc, free_id);
			    } else {
                    LOGW << "Object loaded of type " << id_used[i].type << " is changing id from " << id_used[i].id << " to " << free_id << " due to collision." << std::endl;
				    SetDescID(id_used[i].desc, free_id);
			    }
		    }
	    }
    }
}

extern TextAtlasRenderer g_text_atlas_renderer;
extern TextAtlas g_text_atlas[kNumTextAtlas];
extern ASTextContext g_as_text_context;
extern const char* font_path;

void AnalyzeForLineBreaks(char* str, int len){
	FontRenderer* font_renderer = FontRenderer::Instance();
	int font_size = int(max(18, min(Graphics::Instance()->window_dims[1] / 30, Graphics::Instance()->window_dims[0] / 50)));
	TextMetrics metrics = g_as_text_context.ASGetTextAtlasMetrics( font_path, font_size, 0, str);
	float threshold = (float) std::min(font_size*40, Graphics::Instance()->window_dims[0]-100);
	std::string final;
	std::string first_line = str;
	std::string second_line;
	while(first_line.length() > 0){
		while(metrics.bounds[2] > threshold){
			int last_space = first_line.find_last_of(' ');
			second_line.insert(0, first_line.substr(last_space));
			first_line.resize(last_space);
			metrics = g_as_text_context.ASGetTextAtlasMetrics( font_path, font_size, 0, first_line.c_str());
		}
		final += first_line + "\n";
		if(!second_line.empty()){
			first_line = second_line.substr(1);
			second_line = "";
		} else {
			first_line.clear();
		}
		metrics = g_as_text_context.ASGetTextAtlasMetrics( font_path, font_size, 0, first_line.c_str());
	}
	FormatString(str, len, "%s", final.c_str());
}

bool LevelLoader::LoadLevel(const Path& level_path, SceneGraph& s) {
    Graphics* gi = Graphics::Instance();

    if(level_path.isValid() == false) {
        FatalError("Error", "Could not find level file: %s", level_path.GetFullPath());
    }

    LevelInfo li;
    {
        PROFILER_ZONE(g_profiler_ctx, "Parsing level xml");
        ParseLevelXML(level_path.GetFullPath(), li);
        g_level_shadows = li.shadows_;
        const char* localized_load_tip = GetLevelTip(config["language"].str().c_str(), FindShortestPath(level_path.GetFullPath()).c_str());
        char temp_load_screen_tip[kPathSize] = {'\0'};
        if(localized_load_tip) {
			FormatString(temp_load_screen_tip, kPathSize, "%s", localized_load_tip);
        } else {
            // Simple fallback to en_us in case there is one available
            const char* localized_load_tip = GetLevelTip("en_us", FindShortestPath(level_path.GetFullPath()).c_str());
            if(localized_load_tip) {
                FormatString(temp_load_screen_tip, kPathSize, "%s", localized_load_tip);
            } else if (li.spm_.find("Load Tip") != li.spm_.end()){
                FormatString(temp_load_screen_tip, kPathSize, li.spm_["Load Tip"].GetString().c_str());
            }
        }
        AnalyzeForLineBreaks(temp_load_screen_tip, kPathSize);
        FormatString(Engine::Instance()->load_screen_tip, kPathSize, "%s", temp_load_screen_tip);
        if(!li.script_.empty()){
            std::string path = li.script_.substr(0, li.script_.size()-3) + "_paths.xml";
            if(!FileExists(path.c_str(), kAnyPath)){
                path = script_dir_path + path;
            }
            Path level_script_path;
            level_script_path = FindFilePath(path.c_str(), kAnyPath, false);
            if(level_script_path.isValid()){
                TiXmlDocument doc;
                if (!doc.LoadFile(level_script_path.GetFullPath())) {
                    FatalError("Error", "Bad xml data in level script path file: %s", path.c_str());
                }
                const TiXmlElement* root = doc.RootElement();

                LevelInfo::StrPair script_path;
                for(const TiXmlElement* field = root; field; field = field->NextSiblingElement()){
                    const char* val = field->Value();
                    if(strcmp(val, "path") == 0) {
                        script_path.first.clear();
                        script_path.second.clear();
                        const TiXmlAttribute* attrib = field->FirstAttribute();
                        while(attrib){
                            const char* name = attrib->Name();
                            if(strcmp(name, "path") == 0){
                                script_path.second = attrib->Value();
                            } else if(strcmp(name, "key") == 0){
                                script_path.first = attrib->Value();
                            }
                            attrib = attrib->Next();
                        }
                        li.script_paths_.push_back(script_path);
                    }
                }
            }
        }
    }
    
    s.level_path_ = FindShortestPath2(level_path.GetFullPath());
    s.level_has_been_previously_saved_ = false;
    s.level_name_ = li.level_name_;
    s.level_visible_name_ = li.visible_name_;
    s.level_visible_description_ = li.visible_description_;
    gi->post_shader_name = li.shader_;

    bool has_terrain = !li.terrain_info_.colormap.empty();
    if(has_terrain){
        PROFILER_ZONE(g_profiler_ctx, "Loading terrain");
        LoadTerrain(s, li.terrain_info_);
    }

	FixDuplicateIDs(&li.desc_list_, has_terrain);
    AddLoadingText("Adding loaded objects to scene...");
    {
        PROFILER_ZONE(g_profiler_ctx, "Adding objects");

        static const int kBufSize = 256;
        char buf[kBufSize];
        for(auto & i : li.desc_list_){
            buf[0] = '\0';
            const EntityDescriptionField* path_field = i.GetField(EDF_FILE_PATH);
            if(path_field && !path_field->data.empty()){
                FormatString(buf, kBufSize, "Adding object: %s", std::string(path_field->data.begin(), path_field->data.end()).c_str());
                PROFILER_ENTER_DYNAMIC_STRING(g_profiler_ctx, buf);    
            } else {
                PROFILER_ENTER(g_profiler_ctx, "Adding object: unknown");
            }
            Object * obj = MapEditor::AddEntityFromDesc(&s, i, true);
            if( obj == NULL ) {
                LOGE << "Failed to construct object \"" << buf << "\" for level load" << std::endl;
            }
            PROFILER_LEAVE(g_profiler_ctx);
            if( Engine::Instance()->RequestedInterruptLoading() ) {
                return false;
            }
        }
    }
    s.map_editor->SetUpSky(li.sky_info_);
    
    if(ActiveCameras::Get()->m_camera_object == NULL) {
        AddCamera(s);
    }

    {
        PROFILER_ZONE(g_profiler_ctx, "Setting up level info");
        s.level->SetFromLevelInfo(li);
    }
    {
        gi->nav_mesh_out_of_date = li.out_of_date_info_.nav_mesh;
        gi->nav_mesh_out_of_date_chunk = -1;
    }
    {
        PROFILER_ZONE(g_profiler_ctx, "Loading nav mesh");
        AddLoadingText("Loading nav mesh...");
        if( s.LoadNavMesh() )
        {
            AddLoadingText("Nav mesh loaded!");
        }
        else
        {
			if(li.nav_mesh_parameters_.generate && config["no_auto_nav_mesh"].toBool() == false){
				AddLoadingText("Navmesh needs to be rebuilt for some reason");
				AddLoadingText("Generating new nav mesh...");
				s.CreateNavMesh();
				s.SaveNavMesh();
			}
        }
    }
    s.SendMessageToAllObjects(OBJECT_MSG::FINALIZE_LOADED_CONNECTIONS);

    AddLoadingText("Loading ambient sounds and music...");
    for(auto & ambient_sound : li.ambient_sounds_){
        Engine::Instance()->GetSound()->AddAmbientTriangle(ambient_sound);
    }
    AddLoadingText("Getting path set...");
    PathSet path_set;
    {
        PROFILER_ZONE(g_profiler_ctx, "Returning paths to path set");
        li.ReturnPaths(path_set);
    }
    AddLoadingText("Loading all data in advance...");
    {
        PROFILER_ZONE(g_profiler_ctx, "Preloading all paths");
        LoadAll(path_set, Engine::Instance()->GetSound());
    }

    PROFILER_ZONE(g_profiler_ctx, "Applying level script params");
    // Add defaults if needed
    ScriptParams sp;
    sp.SetParameterMap(li.spm_);
    sp.ASAddString("Objectives", "destroy_all");
    sp.ASAddString("Achievements", "flawless, no_injuries, no_kills");
    sp.ASAddInt("Level Boundaries", 1);
    sp.ASAddInt("Shared Camera", 0);
    sp.ASAddFloat("HDR White point", 0.7f);
    sp.ASAddFloat("HDR Black point", 0.005f);
    sp.ASAddFloat("HDR Bloom multiplier", 1.0f);
    SceneGraph::ApplyScriptParams(&s, sp.GetParameterMap());

    return true;
}

void LevelLoader::SaveTerrain(TiXmlNode* root, SceneGraph* s) {
    TerrainObject *to = s->terrain_object_;
    if(to){
        const TerrainInfo &ti = to->terrain_info();

        TiXmlElement* terrain_el = new TiXmlElement("Terrain");  
        root->LinkEndChild(terrain_el);

        TiXmlElement* terrain_sub_el;
        terrain_sub_el = new TiXmlElement("Heightmap");
        terrain_sub_el->LinkEndChild( new TiXmlText(ti.heightmap.c_str()) );
        terrain_el->LinkEndChild(terrain_sub_el);
        terrain_sub_el = new TiXmlElement("ShaderExtra");
        terrain_sub_el->LinkEndChild( new TiXmlText(ti.shader_extra.c_str()) );
        terrain_el->LinkEndChild(terrain_sub_el);
        terrain_sub_el = new TiXmlElement("DetailMap");
        terrain_sub_el->LinkEndChild( new TiXmlText("") );
        terrain_el->LinkEndChild(terrain_sub_el);
        terrain_sub_el = new TiXmlElement("ColorMap");
        terrain_sub_el->LinkEndChild( new TiXmlText(ti.colormap.c_str()) );
        terrain_el->LinkEndChild(terrain_sub_el);
        if(!ti.weightmap.empty()){
            terrain_sub_el = new TiXmlElement("WeightMap");
            terrain_sub_el->LinkEndChild( new TiXmlText(ti.weightmap.c_str()) );
            terrain_el->LinkEndChild(terrain_sub_el);
        }
        if(!ti.model_override.empty()){
            terrain_sub_el = new TiXmlElement("ModelOverride");
            terrain_sub_el->LinkEndChild( new TiXmlText(ti.model_override.c_str()) );
            terrain_el->LinkEndChild(terrain_sub_el);
        }

        terrain_sub_el = new TiXmlElement("DetailMaps");
        for(unsigned i=0; i<4; ++i){
            TiXmlElement* detail_map_el = new TiXmlElement("DetailMap");
            detail_map_el->SetAttribute("colorpath",ti.detail_map_info[i].colorpath.c_str());
            detail_map_el->SetAttribute("normalpath",ti.detail_map_info[i].normalpath.c_str());
            detail_map_el->SetAttribute("materialpath",ti.detail_map_info[i].materialpath.c_str());
            terrain_sub_el->LinkEndChild(detail_map_el);
        }
        terrain_el->LinkEndChild(terrain_sub_el);

        TiXmlElement *does = new TiXmlElement("DetailObjects");
        for(const auto & dol : ti.detail_object_info){
            TiXmlElement *doe = new TiXmlElement("DetailObject");
            doe->SetAttribute("obj_path", dol.obj_path.c_str());
            doe->SetAttribute("weight_path", dol.weight_path.c_str());
            doe->SetDoubleAttribute("normal_conform", (double)dol.normal_conform);
            doe->SetDoubleAttribute("density", (double)dol.density);
            doe->SetDoubleAttribute("min_embed", (double)dol.min_embed);
            doe->SetDoubleAttribute("max_embed", (double)dol.max_embed);
            doe->SetDoubleAttribute("min_scale", (double)dol.min_scale);
            doe->SetDoubleAttribute("max_scale", (double)dol.max_scale);
            doe->SetDoubleAttribute("view_distance", (double)dol.view_dist);
            doe->SetDoubleAttribute("jitter_degrees", (double)dol.jitter_degrees);
            doe->SetDoubleAttribute("overbright", (double)dol.overbright);
            doe->SetDoubleAttribute("tint_weight", (double)dol.tint_weight);
            if(dol.collision_type == _static){
                doe->SetAttribute("collision_type", "static");
            } else if(dol.collision_type == _plant){
                doe->SetAttribute("collision_type", "plant");
            }
            does->LinkEndChild(doe);
        }
        terrain_el->LinkEndChild(does);
    }
}

void LevelLoader::SaveLevel(SceneGraph &s, SaveLevelType type) {
    /*if(s.VerifySanity() == false ) {
        SDL_MessageBoxData message_box_data;
        message_box_data.title = "Overgrowth";
        message_box_data.message = "Your level has some sanity warnings that should be fixed, are you sure you wish to save?";
        message_box_data.colorScheme = NULL;
        message_box_data.window = NULL;
        message_box_data.flags = SDL_MESSAGEBOX_WARNING;
        message_box_data.numbuttons = 2;
        SDL_MessageBoxButtonData buttons[3];
        buttons[0].text = "Yes";
        buttons[0].flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
        buttons[0].buttonid = 0;
        buttons[1].text = "No";
        buttons[1].flags = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
        buttons[1].buttonid = 1;
        message_box_data.buttons = buttons;
        int button_id;
        SDL_ShowMessageBox(&message_box_data, &button_id);
        if(button_id == 1){
            return;
        }
    }*/

    std::string save_path;
    std::string old_path;
    if(s.level_path_.isValid()) {
        save_path = s.level_path_.GetFullPath();
        old_path = save_path;
    }

    if(type == kSaveAs || s.level_path_.isValid() == false || (((s.level_path_.source & kModPaths) || (s.level_path_.source & kDataPaths)) && (!s.level_has_been_previously_saved_ && config["allow_game_dir_save"].toBool() == false))) {
        std::string start_dir;
        if( config["allow_game_dir_save"].toBool() ) {
            if( save_path.empty() ) {
                start_dir = std::string(GetDataPath(0)) + "/Data/";
            } else {
                start_dir = SplitPathFileName( save_path ).first;
            }
        } else {
            start_dir = std::string(GetWritePath(CoreGameModID).c_str()) + "/Data/";
        }

        bool valid = false;
        
        while(!valid) {
            const int BUF_SIZE = 512;
            char buf[BUF_SIZE];
            Dialog::DialogErr err = Dialog::writeFile("xml",1,start_dir.c_str(),buf,BUF_SIZE);
            if(err){
                LOGE << "Cancelling level save due to dialog close" << std::endl;
                return;
            } else {
                save_path = buf;
                old_path = save_path;

                std::string temp = save_path.substr(0, save_path.find_last_of("\\/"));
                if(CheckWritePermissions(temp.c_str()) != 0) {
                    LOGW << "Couldn't write to directory at " << temp << std::endl;
                } else {
                    valid = true;
                }
            }
        }
    }

    if(fileexists(old_path.c_str()) == 0) {
        CreateBackup(old_path.c_str(), config["level_backup_count"].toNumber<int>());
    }
    createfile(save_path.c_str());

    int slash_position = save_path.find_last_of("\\/")+1;
    int dot_position = save_path.rfind('.');

    s.level_path_ = FindShortestPath2(save_path);
    s.level_has_been_previously_saved_ = true;
    s.level_name_ = save_path.substr(slash_position, dot_position-slash_position);
    s.level_visible_name_ = s.level_name_; // This isn't really used

    Graphics* graphics = Graphics::Instance();
    LOGI << "Saving level: " << s.level_name_ << std::endl;

    TiXmlDocument doc;
    
    TiXmlDeclaration* decl = new TiXmlDeclaration( "2.0", "", "" );
    doc.LinkEndChild( decl );

    //
    //
    TiXmlNode* eRoot = static_cast<TiXmlNode*>(&doc);
    /* This should be activated in the future like alpha 213 or 214
    TiXmlElement* eRoot = new TiXmlElement("Level");
    doc.LinkEndChild( eRoot );
    */
     

    // write file type
    TiXmlElement* version = new TiXmlElement("Type");
    eRoot->LinkEndChild(version);
    version->LinkEndChild(new TiXmlText("saved"));

    TiXmlElement* name_el = new TiXmlElement("Name");  
    eRoot->LinkEndChild(name_el);
    name_el->LinkEndChild(new TiXmlText(s.level_visible_name_.c_str()));
    
    TiXmlElement* desc_el = new TiXmlElement("Description");  
    eRoot->LinkEndChild(desc_el);
    desc_el->LinkEndChild(new TiXmlText(s.level_visible_description_.c_str()));
    
    TiXmlElement* shader_el;
    shader_el = new TiXmlElement("Shader");
    shader_el->LinkEndChild( new TiXmlText(graphics->post_shader_name.c_str()) );
    eRoot->LinkEndChild(shader_el);

    TiXmlElement* shadow_el;
    shadow_el = new TiXmlElement("Shadows");
    shadow_el->LinkEndChild( new TiXmlText(g_level_shadows ? "true" : "false") );
    eRoot->LinkEndChild(shadow_el);

    TiXmlElement* loading_screen_el;
    loading_screen_el = new TiXmlElement("LoadingScreen");
    TiXmlElement* loading_screen_image_el = new TiXmlElement("Image"); 
    loading_screen_image_el->LinkEndChild(new TiXmlText(s.level->loading_screen_.image.c_str()));
    loading_screen_el->LinkEndChild( loading_screen_image_el );
    eRoot->LinkEndChild(loading_screen_el);
    
    LOGI << "Saving terrain and sky..." << std::endl;
    SaveTerrain(eRoot, &s);

    {
        TiXmlElement* element = new TiXmlElement("OutOfDate");  
        eRoot->LinkEndChild(element);
        element->SetAttribute("NavMesh",graphics->nav_mesh_out_of_date?"true":"false");
        element->SetAttribute("NavMeshParamHash", (int)HashNavMeshParameters(s.level->nav_mesh_parameters_));
    }
    
    LOGI << "Saving script and spawn points..." << std::endl;
    {
        TiXmlElement* ambient_sound_element = new TiXmlElement("AmbientSounds");  
        eRoot->LinkEndChild(ambient_sound_element);

        const std::vector<AmbientTriangle>& ambient_triangles =
            Engine::Instance()->GetSound()->GetAmbientTriangles();
        for(const auto & ambient_triangle : ambient_triangles){
            TiXmlElement* ambient;
            ambient = new TiXmlElement("Ambient");
            ambient_sound_element->LinkEndChild(ambient);
            ambient->SetAttribute("path", ambient_triangle.path.c_str());
        }
    }

    {
        TiXmlElement* element = new TiXmlElement("Script");
        element->LinkEndChild(new TiXmlText(s.level->GetLevelSpecificScript().c_str()));
        eRoot->LinkEndChild(element);
    }
    {
        if(s.level->GetPCScript(NULL) != Level::DEFAULT_PLAYER_SCRIPT) {
            TiXmlElement* element = new TiXmlElement("PCScript");
            element->LinkEndChild(new TiXmlText(s.level->GetPCScript(NULL).c_str()));
            eRoot->LinkEndChild(element);
        }
    }
    {
        if(s.level->GetNPCScript(NULL) != Level::DEFAULT_ENEMY_SCRIPT) {
            TiXmlElement* element = new TiXmlElement("NPCScript");
            element->LinkEndChild(new TiXmlText(s.level->GetNPCScript(NULL).c_str()));
            eRoot->LinkEndChild(element);
        }
    }
    {
        TiXmlElement* element = new TiXmlElement("LevelScriptParameters");
        eRoot->LinkEndChild(element);
        WriteScriptParamsToXML(s.level->script_params().GetParameterMap(), element);
    }
    {
        TiXmlElement* element = new TiXmlElement("NavMeshParameters");
        eRoot->LinkEndChild(element);
        WriteNavMeshParametersToXML(s.level->nav_mesh_parameters_, element);
    }

    LOGI << "Saving entities..." << std::endl;
    s.map_editor->SaveEntities(eRoot);

    {
        LOGI << "Saving recent spawner item list" << std::endl;
        TiXmlElement* elements = new TiXmlElement("RecentItems");

        std::vector<SpawnerItem> recent_items = s.level->GetRecentlyCreatedItems();

        for(auto & recent_item : recent_items) {
            SpawnerItem* si = &recent_item; 

            TiXmlElement* element = new TiXmlElement("SpawnerItem");
            element->SetAttribute("display_name", si->display_name.c_str());
            element->SetAttribute("path", si->path.c_str());
            element->SetAttribute("thumbnail_path", si->thumbnail_path.c_str());

            elements->LinkEndChild(element);
        }
        eRoot->LinkEndChild(elements); 
    }

    LOGI << "Full Level Save Path: "  << save_path <<  std::endl;
    doc.SaveFile(save_path);
    LOGI << "Short Level Save Path: " << s.level_path_ << std::endl;
}

EntityType CheckGenericType(TiXmlDocument& doc) {
    TiXmlHandle hDoc(&doc);
    TiXmlNode* pNode;

    pNode = hDoc.FirstChild("Object").ToNode();
    if (pNode != NULL) return _env_object;

    pNode = hDoc.FirstChild("Actor").ToNode();
    if (pNode != NULL) return _movement_object;

    pNode = hDoc.FirstChild("Sound").ToNode();
    if (pNode != NULL) return _ambient_sound_object;

    pNode = hDoc.FirstChild("item").ToNode();
    if (pNode != NULL) return _item_object;

    pNode = hDoc.FirstChild("DecalObject").ToNode();
    if (pNode != NULL) return _decal_object;

    pNode = hDoc.FirstChild("Hotspot").ToNode();
    if (pNode != NULL) return _hotspot_object;

    return _no_type;
}
