//-----------------------------------------------------------------------------
//           Name: level.cpp
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
#include "level.h"

#include <Scripting/angelscript/ascontext.h>
#include <Scripting/angelscript/asfuncs.h>
#include <Scripting/angelscript/add_on/scriptarray/scriptarray.h>
#include <Scripting/scriptfile.h>
#include <Scripting/angelscript/ascollisions.h>

#include <Game/characterscript.h>
#include <Game/savefile.h>
#include <Game/savefile_script.h>

#include <Objects/hotspot.h>
#include <Objects/movementobject.h>
#include <Objects/placeholderobject.h>

#include <Main/scenegraph.h>
#include <Main/engine.h>

#include <Internal/comma_separated_list.h>
#include <Internal/common.h>
#include <Internal/filesystem.h>
#include <Internal/levelxml.h>
#include <Internal/profiler.h>
#include <Internal/modloading.h>
#include <Internal/locale.h>

#include <Graphics/pxdebugdraw.h>
#include <GUI/IMUI/imui_script.h>
#include <Logging/logdata.h>
#include <Online/online.h>

extern std::string script_dir_path;

const char* Level::DEFAULT_ENEMY_SCRIPT = "enemycontrol.as";
const char* Level::DEFAULT_PLAYER_SCRIPT = "playercontrol.as";

Level::Level() :
    metaDataDirty( false )
{}

Level::~Level() {
    Dispose();
}

void Level::Dispose() {
    Message("dispose_level");

    for( uint32_t i = 0; i < as_contexts_.size(); i++ ) {
        delete as_contexts_[i].ctx;
        as_contexts_[i].ctx = NULL;
    }
    as_contexts_.clear();

    Engine::Instance()->GetASNetwork()->DeRegisterASNetworkCallback(this);

    old_col_map_.clear();
    for(int i=0; i<kMaxTextElements; ++i){
        text_elements[i].in_use = false;
        text_elements[i].text_canvas_texture.Reset();
    }
}

Path Level::FindScript(const std::string& path) {
    Path out_path = FindFilePath(path, kDataPaths | kModPaths, false);
    if(out_path.isValid() == false){
        out_path = FindFilePath(script_dir_path + path, kDataPaths | kModPaths, false);
    }

    return out_path;
}

void Level::StartDialogue(const std::string& dialogue) const {
    ASArglist args;
    args.AddObject((void*)&dialogue);
    for (unsigned i = 0; i < as_contexts_.size(); i++) {
        if (as_contexts_[i].ctx->HasFunction(as_contexts_[i].as_funcs.start_dialogue)) {
            as_contexts_[i].ctx->CallScriptFunction(as_contexts_[i].as_funcs.start_dialogue);
        }
    }
}

void Level::RegisterMPCallbacks() const {
	for (unsigned i = 0; i < as_contexts_.size(); i++) {
		if (as_contexts_[i].ctx->HasFunction(as_contexts_[i].as_funcs.register_mp_callbacks)) {
			as_contexts_[i].ctx->CallScriptFunction(as_contexts_[i].as_funcs.register_mp_callbacks);
		}
	}
}

void Level::Initialize(SceneGraph *scenegraph, GUI* gui) {
    Dispose();

    this->scenegraph = scenegraph;

    Engine::Instance()->GetASNetwork()->RegisterASNetworkCallback(this);

    Path script_path = FindFilePath(script_dir_path+"level.as", kDataPaths|kModPaths);

    HookedASContext hasc;
    hasc.path = script_path;
    hasc.context_name = "main_level_script";
    as_contexts_.push_back(hasc);

    if(!level_specific_script_.empty()){
        Path level_specific_script_path = FindScript(level_specific_script_);
        if(level_specific_script_path.isValid() == false) {
            DisplayError("Couldn't find file", "Couldn't find player script file, default will be used", ErrorType::_ok);
            LOGE << "Could not find file specified in <Script> in level: " << level_specific_script_ << std::endl;
        }

        if( level_specific_script_path.isValid() ) {
            HookedASContext hasc;
            hasc.path = level_specific_script_path;
            hasc.context_name = "level_specific";
            as_contexts_.push_back(hasc);
        }
    }

    if(!pc_script_.empty()){
        Path pc_script_path = FindScript(pc_script_);
        while(!pc_script_path.isValid() && DisplayError("Couldn't find file", "Couldn't find player script file, default will be used instead", ErrorType::_ok_cancel_retry) == ErrorResponse::_retry) {
            pc_script_path = FindScript(pc_script_);
            LOGE << "Could not find file specified in <PCScript> in level: " << pc_script_ << std::endl;
        }

        if(!pc_script_path.isValid())
            pc_script_ = "";
    }

    if(!npc_script_.empty()){
        Path npc_script_path = FindScript(npc_script_);
        while(!npc_script_path.isValid() && DisplayError("Couldn't find file", "Couldn't find enemy script file, default will be used instead", ErrorType::_ok_cancel_retry) == ErrorResponse::_retry) {
            npc_script_path = FindScript(npc_script_);
            LOGE << "Could not find file specified in <NPCScript> in level: " << npc_script_ << std::endl;
        }

        if(!npc_script_path.isValid())
            npc_script_ = "";
    }

    if(!npc_script_.empty()) {
        for(size_t i = 0; i < scenegraph->movement_objects_.size(); ++i) {
            MovementObject* obj = (MovementObject*)scenegraph->movement_objects_[i];
            if(obj->GetNPCObjectScript().empty()) {
                obj->ChangeControlScript(npc_script_);
            }
        }
    }

    std::vector<std::string> mod_level_hooks = ModLoading::Instance().ActiveLevelHooks();
    std::vector<std::string>::iterator modlhit = mod_level_hooks.begin();
    for(; modlhit != mod_level_hooks.end(); modlhit++) {
        HookedASContext hasc;
        Path level_path = FindFilePath(modlhit->c_str(), kDataPaths | kModPaths);
        if(level_path.isValid()){
	        hasc.path = level_path;
            hasc.context_name = "mod_level_hook";
            as_contexts_.push_back(hasc);
        } else {
            LOGE << "Unable to find level script file: " << *modlhit << std::endl;
        }
    }

    ASData as_data;
    as_data.scenegraph = scenegraph;
    as_data.gui = gui;

    as_collisions.reset(new ASCollisions(scenegraph));

    for(unsigned i = 0; i < as_contexts_.size(); i++) {
        ASContext* ctx = new ASContext(as_contexts_[i].context_name.c_str(),as_data);

        AttachASNetwork(ctx);
        AttachUIQueries(ctx);
        AttachActiveCamera(ctx);
        AttachScreenWidth(ctx);
        AttachPhysics(ctx);
        AttachEngine(ctx);
        AttachScenegraph(ctx, scenegraph);
        AttachMessages(ctx);
        AttachStringConvert(ctx);
        AttachTextCanvasTextureToASContext(ctx);
        AttachLevel(ctx);
        AttachInterlevelData(ctx);
        AttachIMUI(ctx);
        AttachPlaceholderObject(ctx);
        AttachTokenIterator(ctx);
        AttachSimpleFile(ctx);
        AttachLocale(ctx);
        AttachUndo(ctx);
        AttachIMGUI(ctx);
        AttachIMGUIModding(ctx);
		AttachOnline(ctx);
        hud_images.AttachToContext(ctx);
        AttachSaveFile(ctx, &Engine::Instance()->save_file_);
        character_script_getter_.AttachToScript(ctx, "character_getter");
        as_collisions->AttachToContext(ctx);

        as_contexts_[i].as_funcs.init                       = ctx->RegisterExpectedFunction("void Init(string level_name)", true);
        as_contexts_[i].as_funcs.update                     = ctx->RegisterExpectedFunction("void Update(int is_paused)", false);
        as_contexts_[i].as_funcs.update_deprecated          = ctx->RegisterExpectedFunction("void Update()", false);

        as_contexts_[i].as_funcs.hotspot_exit               = ctx->RegisterExpectedFunction("void HotspotExit(string event, MovementObject @mo)", false);
        as_contexts_[i].as_funcs.hotspot_enter              = ctx->RegisterExpectedFunction("void HotspotEnter(string event, MovementObject @mo)", false);
        as_contexts_[i].as_funcs.receive_message            = ctx->RegisterExpectedFunction("void ReceiveMessage(string message)", false);
        as_contexts_[i].as_funcs.draw_gui                   = ctx->RegisterExpectedFunction("void DrawGUI()", false);
        as_contexts_[i].as_funcs.draw_gui2                  = ctx->RegisterExpectedFunction("void DrawGUI2()", false);
        as_contexts_[i].as_funcs.draw_gui3                  = ctx->RegisterExpectedFunction("void DrawGUI3()", false);
        as_contexts_[i].as_funcs.has_focus                  = ctx->RegisterExpectedFunction("bool HasFocus()", false);
        as_contexts_[i].as_funcs.dialogue_camera_control    = ctx->RegisterExpectedFunction("bool DialogueCameraControl()", false);
        as_contexts_[i].as_funcs.save_history_state         = ctx->RegisterExpectedFunction("void SaveHistoryState(SavedChunk@ chunk)", false);
        as_contexts_[i].as_funcs.read_chunk                 = ctx->RegisterExpectedFunction("void ReadChunk(SavedChunk@ chunk)", false);
        as_contexts_[i].as_funcs.set_window_dimensions      = ctx->RegisterExpectedFunction("void SetWindowDimensions(int width, int height)", false);
        as_contexts_[i].as_funcs.incoming_tcp_data          = ctx->RegisterExpectedFunction("void IncomingTCPData(uint socket, array<uint8>@ data)", false);

        as_contexts_[i].as_funcs.pre_script_reload          = ctx->RegisterExpectedFunction("void PreScriptReload()", false);
        as_contexts_[i].as_funcs.post_script_reload         = ctx->RegisterExpectedFunction("void PostScriptReload()", false);

        as_contexts_[i].as_funcs.menu                       = ctx->RegisterExpectedFunction("void Menu()", false);
		as_contexts_[i].as_funcs.register_mp_callbacks		= ctx->RegisterExpectedFunction("void RegisterMPCallBacks()", false);
        as_contexts_[i].as_funcs.start_dialogue             = ctx->RegisterExpectedFunction("void StartDialogue(const string &in name", false);
 
        ctx->LoadScript(as_contexts_[i].path);

        as_contexts_[i].ctx = ctx;
    }

    for(unsigned i = 0; i < as_contexts_.size(); i++) {
        LOGI << "Calling void Init(string level_name) for level hook file." << std::endl; 
        ASArglist args;
        args.AddObject(&scenegraph->level_name_);
        as_contexts_[i].ctx->CallScriptFunction(as_contexts_[i].as_funcs.init, &args);

    }

	RegisterMPCallbacks(); 

    PROFILER_ENTER(g_profiler_ctx, "Exporting docs");
    char path[kPathSize];
    FormatString(path, kPathSize, "%saslevel_docs.h", GetWritePath(CoreGameModID).c_str());
    as_contexts_[0].ctx->ExportDocs(path);
    PROFILER_LEAVE(g_profiler_ctx);
}

void Level::Update(bool paused) {
	ASArglist args;
    ASArglist args2;
	args.Add(paused?1:0);
	std::vector<HookedASContext >::iterator cit = as_contexts_.begin();
    PROFILER_ENTER(g_profiler_ctx, "Level script Update()");
    for( unsigned i = 0; i < as_contexts_.size(); i++ ) {
		if( as_contexts_[i].ctx->CallScriptFunction(as_contexts_[i].as_funcs.update, &args) == false) {
            if(paused == false) {
		        as_contexts_[i].ctx->CallScriptFunction(as_contexts_[i].as_funcs.update_deprecated, &args2);
            }
        }
	}

	Online* online = Online::Instance();
	if (online->IsActive()) {
		AngelScriptUpdate update;
		if (online->GetIfPendingAngelScriptUpdates()) {
			update = online->GetAngelScriptUpdate();
			  
			std::vector<char>& temp = update.data;
			
			bool angelscriptCallSuccesfull = false;

			for (unsigned i = 0; i < as_contexts_.size(); i++) {
				if (as_contexts_[i].ctx->CallMPCallBack(update.state, temp) ) { 
					angelscriptCallSuccesfull = true;
				}
			}

			if (angelscriptCallSuccesfull) {
				online->MoveAngelScriptQueueForward();
			}
		}
		
		if (online->GetIfPendingAngelScriptStates()) {

			update = online->GetAngelScriptStates();
 			std::vector<char>& temp = update.data;

			bool angelscriptCallSuccesfull = false;

			for (unsigned i = 0; i < as_contexts_.size(); i++) {
				if (as_contexts_[i].ctx->CallMPCallBack(update.state, temp)) {
					angelscriptCallSuccesfull = true;
				}
			}

			if (angelscriptCallSuccesfull) {
				online->MoveAngelStateQueueForward();
			}
		}
	}
 
    PROFILER_LEAVE(g_profiler_ctx);
}

void Level::LiveUpdateCheck() {
	std::vector<HookedASContext>::iterator cit = as_contexts_.begin();
	for(; cit != as_contexts_.end(); cit++ ) {
        cit->ctx->CallScriptFunction(cit->as_funcs.pre_script_reload);
        cit->ctx->Reload();
        cit->ctx->CallScriptFunction(cit->as_funcs.post_script_reload);
	}
}

void Level::HandleCollisions( CollisionPtrMap &col_map, SceneGraph &scenegraph ) {
	exit_call_queue.clear();
	enter_call_queue.clear();
    
    // Convert old collision ids into pointers
	{
		PROFILER_ZONE(g_profiler_ctx, "A");
		old_col_ptr_map.clear();
		{
			CollisionMap &map = old_col_map_;
			for(CollisionMap::iterator iter = map.begin(); iter != map.end(); ++iter){
				int id = iter->first;
				Object* obj_a_ptr = scenegraph.GetObjectFromID(id);
				if(obj_a_ptr){
					CollisionSet &set = iter->second;
					for(CollisionSet::iterator iter = set.begin(); iter != set.end(); ++iter){
						int id = *iter;
						Object* obj_b_ptr = scenegraph.GetObjectFromID(id);
						if(obj_b_ptr){
							old_col_ptr_map[obj_a_ptr].insert(obj_b_ptr);
						}
					}
				}
			}
		}
	}

	{
		PROFILER_ZONE(g_profiler_ctx, "B");
		// Check for collisions that used to be happening, and are now gone
		CollisionPtrMap::iterator oci = old_col_ptr_map.begin();
		for(;oci != old_col_ptr_map.end(); ++oci){
			CollisionPtrMap::iterator ci = col_map.find(oci->first);
			Object* obj_a = (Object*)oci->first;
			CollisionPtrSet::iterator oci2 = oci->second.begin();
			for(;oci2 != oci->second.end(); ++oci2){
				Object* obj_b = (Object*)(*oci2);
				bool not_found = false;
				if(ci == col_map.end()){
					not_found = true;
				} else {
					CollisionPtrSet::iterator ci2 = ci->second.find(*oci2);
					if(ci2 == ci->second.end()){
						not_found = true;
					}
				}
				if(!not_found){
					continue;
				}
				Object* temp_obj_a = obj_a;
				if(obj_b->GetType() == _hotspot_object){
					std::swap(temp_obj_a, obj_b);
				}
				if(temp_obj_a->GetType() == _hotspot_object && obj_b->GetType() == _movement_object){
					Hotspot* hotspot = (Hotspot*)temp_obj_a;
					MovementObject* mo = (MovementObject*)obj_b;

					exit_call_queue.push_back(std::pair<Hotspot*,MovementObject*>(hotspot,mo));
				}
				if(temp_obj_a->GetType() == _hotspot_object && obj_b->GetType() == _item_object){
					Hotspot* hotspot = (Hotspot*)temp_obj_a;
					ItemObject* obj = (ItemObject*)obj_b;
					hotspot->HandleEventItem("exit", obj);
				}
			}
		}
	}

	{
		PROFILER_ZONE(g_profiler_ctx, "C");
		// Check for collisions that now exist
		CollisionPtrMap::iterator ci = col_map.begin();
		for(;ci != col_map.end(); ++ci){
			CollisionPtrMap::iterator oci = old_col_ptr_map.find(ci->first);
			Object* obj_a = (Object*)ci->first;
			std::set<void*>::iterator ci2 = ci->second.begin();
			for(;ci2 != ci->second.end(); ++ci2){
				Object* obj_b = (Object*)(*ci2);
				if(obj_a->GetType() == _movement_object && obj_b->GetType() == _movement_object){
					((MovementObject*)obj_a)->CollideWith((MovementObject*)obj_b);
					continue;
				}

				bool not_found = false;
				if(oci == old_col_ptr_map.end()){
					not_found = true;
				} else {
					std::set<void*>::iterator oci2 = oci->second.find(*ci2);
					if(oci2 == oci->second.end()){
						not_found = true;
					}
				}
				if(!not_found){
					continue;
				}
				Object* temp_obj_a = obj_a;
				if(obj_b->GetType() == _hotspot_object){
					std::swap(temp_obj_a, obj_b);
				}
				if(temp_obj_a->GetType() == _hotspot_object && obj_b->GetType() == _movement_object){
					Hotspot* hotspot = (Hotspot*)temp_obj_a;
					MovementObject* mo = (MovementObject*)obj_b;
					enter_call_queue.push_back(std::pair<Hotspot*,MovementObject*>(hotspot,mo));
				}
				if(temp_obj_a->GetType() == _hotspot_object && obj_b->GetType() == _item_object){
					Hotspot* hotspot = (Hotspot*)temp_obj_a;
					ItemObject* obj = (ItemObject*)obj_b;
					hotspot->HandleEventItem("enter", obj);
				}
			}
		}
	}

	{
		PROFILER_ZONE(g_profiler_ctx, "D");
		// Convert old collision pointers to ids
		CollisionPtrMap &map = col_map;
		old_col_map_.clear();
		for(CollisionPtrMap::iterator iter = map.begin(); iter != map.end(); ++iter){
			int id_a = ((Object*)iter->first)->GetID();
			CollisionPtrSet &set = iter->second;
			for(CollisionPtrSet::iterator iter = set.begin(); iter != set.end(); ++iter){
				int id_b = ((Object*)(*iter))->GetID();
				old_col_map_[id_a].insert(id_b);
			}
		}
	}

	{
		PROFILER_ZONE(g_profiler_ctx, "E");
		//Do the delayed calls
		std::vector<std::pair<Hotspot*,MovementObject*> >::iterator exit_call_queue_it = exit_call_queue.begin();
		for(;exit_call_queue_it != exit_call_queue.end(); exit_call_queue_it++) {
			exit_call_queue_it->first->HandleEvent("exit", exit_call_queue_it->second);
            ASArglist args;
			args.AddObject((void*)&exit_call_queue_it->first->GetScriptFile());
			args.AddAddress((void*)exit_call_queue_it->second);

            for(unsigned i = 0; i < as_contexts_.size(); i++ ) {
			    as_contexts_[i].ctx->CallScriptFunction(as_contexts_[i].as_funcs.hotspot_exit, &args);
            }
		}

		std::vector<std::pair<Hotspot*,MovementObject*> >::iterator enter_call_queue_it = enter_call_queue.begin();
		for(;enter_call_queue_it != enter_call_queue.end(); enter_call_queue_it++) {
			enter_call_queue_it->first->HandleEvent("enter", enter_call_queue_it->second);
            ASArglist args;
			args.AddObject((void*)&enter_call_queue_it->first->GetScriptFile());
			args.AddAddress((void*)enter_call_queue_it->second);

            for(unsigned i = 0; i < as_contexts_.size(); i++ ) {
			    as_contexts_[i].ctx->CallScriptFunction(as_contexts_[i].as_funcs.hotspot_enter, &args);
            }
		}
	}
}

bool Level::HasFunction(const std::string& function_definition) {
    bool result = false;

    if(as_contexts_.size() > 0) {
        result = as_contexts_[0].ctx->HasFunction(function_definition);
    }

    return result;
}

int Level::QueryIntFunction( const std::string &func ) {
    ASArglist args;
    int val;
    ASArg return_val;
    return_val.type = _as_int;
    return_val.data = &val;
    if( as_contexts_.size() > 0 ) {
        as_contexts_[0].ctx->CallScriptFunction(func, &args, &return_val);
    }
    return val;
}

void Level::Execute( std::string code ) {
    if( as_contexts_.size() > 0 ) {
        as_contexts_[0].ctx->Execute(code);
    }
}

void Level::Message( const std::string &msg ) {
    Online* online = Online::Instance();
    ASArglist args;
    args.AddObject((void*)&msg);

    if (online->IsHosting()) {
        online->SendLevelMessage(msg);
    }

    for( unsigned i = 0; i < as_contexts_.size(); i++ ) {
        as_contexts_[i].ctx->CallScriptFunction(as_contexts_[i].as_funcs.receive_message, &args);
    }

    for(int i=0, len=level_event_receivers.size(); i<len; ++i){
        Object* obj = scenegraph->GetObjectFromID(level_event_receivers[i]);
        if(obj){
            obj->ReceiveScriptMessage("level_event " + msg);
        } else {
            const int kBufSize = 256;
            char buf[kBufSize];
            FormatString(buf, kBufSize, "Could not find level event receiver with id %d", level_event_receivers[i]);
            DisplayError("Error", buf);
        }
    }
}

int GetNumParamElements(const ScriptParams &sp, const char* name){
    const ScriptParamMap &spm = sp.GetParameterMap();
    ScriptParamMap::const_iterator spm_iter = spm.find(name);
    if(spm_iter == spm.end()){
        return 0;
    }
    CSLIterator iter(spm_iter->second.GetString());
    int num = 0;
    while(iter.GetNext(NULL)){
        ++num;
    }
    return num;
}

int Level::GetNumObjectives() {
    return GetNumParamElements(sp_, "Objectives");
}

int Level::GetNumAchievements() {
    return GetNumParamElements(sp_, "Achievements");
}

void GetParamElement(const ScriptParams &sp, const char* name, int which, std::string &str){
    if(which >= 0) {
        CSLIterator iter(sp.GetStringVal(name));
        int num = 0;
        while(iter.GetNext(&str)) {
            if(num == which){
                return;
            }
            ++num;
        }
    }
    FatalError("Error", "There is no \"%s\" id: %d", name, which);
    return;
}

std::string Level::GetObjective(int which) {
    std::string str;
    GetParamElement(sp_, "Objectives", which, str);
    return str;
}

std::string Level::GetAchievement(int which) {
    std::string str;
    GetParamElement(sp_, "Achievements", which, str);
    return str;
}

// Set's the script for this level (no path)
void Level::SetLevelSpecificScript( std::string& script_name ) {
    metaDataDirty = true;
    level_specific_script_ = script_name;
}

void Level::SetPCScript( std::string& script_name ) {
    metaDataDirty = true;
    pc_script_ = script_name;
}

void Level::SetNPCScript( std::string& script_name ) {
    metaDataDirty = true;
    npc_script_ = script_name;
}

void Level::ClearNPCScript() {
    metaDataDirty = true;
    npc_script_ = "";
}

void Level::ClearPCScript() {
    metaDataDirty = true;
    pc_script_ = "";
}

std::string Level::GetLevelSpecificScript() {
    return level_specific_script_;
}

std::string Level::GetPCScript(const MovementObject* object) {
    if(object) {
        if(!object->GetPCObjectScript().empty()) {
            return object->GetPCObjectScript();
        } else if(!pc_script_.empty()) {
            return pc_script_;
        } else {
            return "playercontrol.as";
        }
    } else {
        return pc_script_.empty() ? DEFAULT_PLAYER_SCRIPT : pc_script_;
    }
}

std::string Level::GetNPCScript(const MovementObject* object) {
    if(object) {
        if(!object->GetNPCObjectScript().empty()) { // First is per-object script
            return object->GetNPCObjectScript();
        } else if(!npc_script_.empty()) { // Then per-level script
            return npc_script_;
        } else if(!object->GetActorScript().empty()) { // Then actor script
            return object->GetActorScript();
        } else { // Finally the default script
            return "enemycontrol.as";
        }
    } else {
        return npc_script_.empty() ? DEFAULT_ENEMY_SCRIPT : npc_script_;
    }
}

// Return the id of an unused text element, or -1 if can't find one
int Level::CreateTextElement() {
    // Return first unused text element, and mark it as in use
    for(int i=0; i<kMaxTextElements; ++i){
        if(!text_elements[i].in_use){
            text_elements[i].in_use = true;
            LOGD << "Allocated Text Element: " << i << std::endl;
            return i;
        }
    }
    FatalError("Error", "Too many level text elements allocated");
    return -1;
}

// Free a text element so someone else can use it, and dispose of its resources
void Level::DeleteTextElement(int which) {
    LOGD << "Freed Text Element: " << which << std::endl; 
    text_elements[which].in_use = false;
    text_elements[which].text_canvas_texture.Reset();
}

// Get a pointer to a given text element canvas texture
TextCanvasTexture* Level::GetTextElement(int which) {
    return &text_elements[which].text_canvas_texture;
}

namespace {
    bool GetLevelBoundaries(Level* level) {
        return level->script_params().ASGetInt("Level Boundaries") == 1;
    }
}

static ScriptParams* ASGetScriptParams(Level* level){
    return &level->script_params();
}

static bool ASHasVar( Level* level, const std::string &name ) {
    bool result = false;
    if( level->as_contexts_.size() > 0 ) {
        result = level->as_contexts_[0].ctx->GetVarPtr(name.c_str()) != NULL;
    }
    return result;
}

static int ASGetIntVar( Level* level, const std::string &name ) {
    if( level->as_contexts_.size() > 0 ) {
	    return *((int*)level->as_contexts_[0].ctx->GetVarPtr(name.c_str()));
    }
    return 0;
}

static int ASGetArrayIntVar( Level* level, const std::string &name, int index ) {
    if( level->as_contexts_.size() > 0 ) {
        return *((int*)level->as_contexts_[0].ctx->GetArrayVarPtr(name, index));
    }
    return 0;
}

static float ASGetFloatVar( Level* level, const std::string &name ) {
    if( level->as_contexts_.size() > 0 ) {
	    return *((float*)level->as_contexts_[0].ctx->GetVarPtr(name.c_str()));
    }
    return 0.0f;
}

static bool ASGetBoolVar( Level* level, const std::string &name ) {
    if( level->as_contexts_.size() > 0 ) {
	    return *((bool*)level->as_contexts_[0].ctx->GetVarPtr(name.c_str()));
    }
    return false;
}

static void ASSetIntVar( Level* level, const std::string &name, int value ) {
    if( level->as_contexts_.size() > 0 ) {
	    *(int*)level->as_contexts_[0].ctx->GetVarPtr(name.c_str()) = value;
    }
}

static void ASSetArrayIntVar( Level* level, const std::string &name, int index, int value ) {
    if( level->as_contexts_.size() > 0 ) {
        *(int*)level->as_contexts_[0].ctx->GetArrayVarPtr(name, index) = value;
    }
}

static void ASSetFloatVar( Level* level, const std::string &name, float value ) {
    if( level->as_contexts_.size() > 0 ) {
	    *(float*)level->as_contexts_[0].ctx->GetVarPtr(name.c_str()) = value;
    }
}

static void ASSetBoolVar( Level* level, const std::string &name, bool value ) {
    if( level->as_contexts_.size() > 0 ) {
	    *(bool*)level->as_contexts_[0].ctx->GetVarPtr(name.c_str()) = value;
    }
}

static bool ASWaitingForInput() {
	return Engine::Instance()->waiting_for_input_;
}

void Level::DefineLevelTypePublic(ASContext *as_context) {
    as_context->RegisterObjectType("Level", 0, asOBJ_REF | asOBJ_NOCOUNT); 
    as_context->RegisterObjectMethod("Level", "bool HasFunction(const string &in function_definition)", asMETHOD(Level,HasFunction), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Level", "int QueryIntFunction(const string &in function)", asMETHOD(Level,QueryIntFunction), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Level", "int GetNumObjectives()", asMETHOD(Level,GetNumObjectives), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Level", "int GetNumAchievements()", asMETHOD(Level,GetNumAchievements), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Level", "string GetObjective(int which)", asMETHOD(Level,GetObjective), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Level", "string GetAchievement(int which)", asMETHOD(Level,GetAchievement), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Level", "void SendMessage(const string& in message)", asMETHOD(Level,Message), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Level", "void Execute(string code)", asMETHOD(Level,Execute), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Level", "const string& GetPath(const string& in key)", asMETHOD(Level,GetPath), asCALL_THISCALL, "Get path from paths xml file");
    as_context->RegisterObjectMethod("Level", "bool HasFocus()", asMETHOD(Level,HasFocus), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Level", "bool DialogueCameraControl()", asMETHOD(Level,DialogueCameraControl), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Level", "bool LevelBoundaries()", asFUNCTION(GetLevelBoundaries), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("Level", "int CreateTextElement()", asMETHOD(Level,CreateTextElement), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Level", "void DeleteTextElement(int which)", asMETHOD(Level,DeleteTextElement), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Level", "TextCanvasTexture@ GetTextElement(int which)", asMETHOD(Level,GetTextElement), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Level", "void GetCollidingObjects(int id, array<int> @array)", asMETHOD(Level,GetCollidingObjects), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Level", "void ReceiveLevelEvents(int id)", asMETHOD(Level, ReceiveLevelEvents), asCALL_THISCALL);
	as_context->RegisterObjectMethod("Level", "void StopReceivingLevelEvents(int id)", asMETHOD(Level, StopReceivingLevelEvents), asCALL_THISCALL);
	as_context->RegisterObjectMethod("Level", "bool HasVar(const string &in name)", asFUNCTION(ASHasVar), asCALL_CDECL_OBJFIRST);
	as_context->RegisterObjectMethod("Level", "int GetIntVar(const string &in name)", asFUNCTION(ASGetIntVar), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("Level", "int GetArrayIntVar(const string &in name, int index)", asFUNCTION(ASGetArrayIntVar), asCALL_CDECL_OBJFIRST);
	as_context->RegisterObjectMethod("Level", "float GetFloatVar(const string &in name)", asFUNCTION(ASGetIntVar), asCALL_CDECL_OBJFIRST);
	as_context->RegisterObjectMethod("Level", "bool GetBoolVar(const string &in name)", asFUNCTION(ASGetIntVar), asCALL_CDECL_OBJFIRST);
	as_context->RegisterObjectMethod("Level", "void SetIntVar(const string &in name, int value)", asFUNCTION(ASSetIntVar), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("Level", "void SetArrayIntVar(const string &in name, int index, int value)", asFUNCTION(ASSetArrayIntVar), asCALL_CDECL_OBJFIRST);
	as_context->RegisterObjectMethod("Level", "void SetFloatVar(const string &in name, float value)", asFUNCTION(ASSetIntVar), asCALL_CDECL_OBJFIRST);
	as_context->RegisterObjectMethod("Level", "void SetBoolVar(const string &in name, bool value)", asFUNCTION(ASSetIntVar), asCALL_CDECL_OBJFIRST);
	as_context->RegisterObjectMethod("Level", "bool WaitingForInput()", asFUNCTION(ASWaitingForInput), asCALL_CDECL_OBJFIRST);
	as_context->RegisterObjectMethod("Level", "ScriptParams@ GetScriptParams()", asFUNCTION(ASGetScriptParams), asCALL_CDECL_OBJFIRST);
    as_context->DocsCloseBrace();
}


void Level::GetCollidingObjects( int id, CScriptArray *array ) {
    int collision[2];
    for(CollisionMap::iterator iter = old_col_map_.begin(); iter != old_col_map_.end(); ++iter){
        collision[0] = iter->first;
        CollisionSet &set = iter->second;
        for(CollisionSet::iterator iter = set.begin(); iter != set.end(); ++iter){
            collision[1] = *iter;
            if(collision[0] == id){
                array->InsertLast(&collision[1]);
            } else if(collision[1] == id){
                array->InsertLast(&collision[0]);
            }
        }
    }
}

void Level::Draw() {
	PROFILER_GPU_ZONE(g_profiler_ctx, "Level::Draw");
    for(unsigned i = 0; i < as_contexts_.size(); i++ ) {
        as_contexts_[i].ctx->CallScriptFunction(as_contexts_[i].as_funcs.draw_gui);
    }
	hud_images.Draw();
    for(unsigned i = 0; i < as_contexts_.size(); i++ ) {
		as_contexts_[i].ctx->CallScriptFunction(as_contexts_[i].as_funcs.draw_gui2);
	}
    hud_images.Draw();
    for(unsigned i = 0; i < as_contexts_.size(); i++ ) {
        as_contexts_[i].ctx->CallScriptFunction(as_contexts_[i].as_funcs.draw_gui3);
    }
    hud_images.Draw();
}

void Level::SetFromLevelInfo( const LevelInfo &li ) {
    level_specific_script_ = li.script_;
    pc_script_ = li.pc_script_;
    npc_script_ = li.npc_script_;
    level_script_paths.clear();
    for(int i=0, len=li.script_paths_.size(); i<len; ++i){
        const LevelInfo::StrPair &script_path = li.script_paths_[i];
        level_script_paths[script_path.first] = script_path.second;
    }
    recently_created_items_ = li.recently_created_items_; 
    nav_mesh_parameters_ = li.nav_mesh_parameters_;
    loading_screen_ = li.loading_screen_;
}

std::string Level::GetAchievementsString() const {
    return sp_.GetStringVal("Achievements");
}

bool Level::HasFocus() {
    ASArglist args;
    bool has_focus = false;
    ASArg return_arg;
    return_arg.data = &has_focus;
    return_arg.type = _as_bool;
    for(unsigned i = 0; i < as_contexts_.size(); i++ ) {
        as_contexts_[i].ctx->CallScriptFunction(as_contexts_[i].as_funcs.has_focus, &args, &return_arg);
        if(has_focus) {
            return true;
        }
    }
    return false;
}

bool Level::DialogueCameraControl() {
    bool dialogue_control = false;
    ASArglist args;
    ASArg return_arg;
    return_arg.data = &dialogue_control;
    return_arg.type = _as_bool;
    for(unsigned i = 0; i < as_contexts_.size(); i++ ) {
        as_contexts_[i].ctx->CallScriptFunction(as_contexts_[i].as_funcs.dialogue_camera_control, &args, &return_arg);
        if(dialogue_control) {
            return true;
        }
    }
    return false;
}

ScriptParams& Level::script_params() {
    return sp_;
}

void Level::SetScriptParams(const ScriptParamMap& spm) {
    sp_.SetParameterMap(spm);
}

const std::string & Level::GetPath(const std::string& key) const {
    StringMap::const_iterator iter = level_script_paths.find(key);
    if(iter == level_script_paths.end()){
        FatalError("Error", "No path found with key: %s", key.c_str());
    }
    return iter->second;
}

void Level::SaveHistoryState(std::list<SavedChunk> & chunks, int state_id) {
    SavedChunk saved_chunk;
    saved_chunk.obj_id = 0;
    saved_chunk.type = ChunkType::LEVEL;

    ASArglist args;
    args.AddAddress(&saved_chunk);
    for(unsigned i = 0; i < as_contexts_.size(); i++ ) {
        as_contexts_[i].ctx->CallScriptFunction(as_contexts_[i].as_funcs.save_history_state, &args);
    }
    AddChunkToHistory(chunks, state_id, saved_chunk);
}

void Level::ReadChunk(const SavedChunk &the_chunk) {
    ASArglist args;
    args.AddAddress((void*)&the_chunk);

    for(unsigned i = 0; i < as_contexts_.size(); i++ ) {
        as_contexts_[i].ctx->CallScriptFunction(as_contexts_[i].as_funcs.read_chunk, &args);
    }
}

bool Level::isMetaDataDirty() {
    if(metaDataDirty) {
        return true;
    }

    for(size_t i = 0; i < scenegraph->objects_.size(); ++i) {
        if(scenegraph->objects_[i]->GetType() == _placeholder_object) {
            PlaceholderObject* obj = (PlaceholderObject*)scenegraph->objects_[i];
            if(obj->GetScriptParams()->HasParam("Dialogue")) {
                if(obj->unsaved_changes) {
                    return true;
                }
            }
        }
    }

    return false;
}

void Level::setMetaDataClean() {
    metaDataDirty = false;
    for(size_t i = 0; i < scenegraph->objects_.size(); ++i) {
        if(scenegraph->objects_[i]->GetType() == _placeholder_object) {
            ((PlaceholderObject*)scenegraph->objects_[i])->unsaved_changes = false;
        }
    }
}

void Level::setMetaDataDirty() {
    metaDataDirty = true;
}

void Level::ReceiveLevelEvents(int id) {
    level_event_receivers.push_back(id);
}

void Level::StopReceivingLevelEvents(int id) {
    for(int i=0; i<(int)level_event_receivers.size(); ++i){
        while(i<(int)level_event_receivers.size() && level_event_receivers[i] == id){
            level_event_receivers[i] = level_event_receivers.back();
            level_event_receivers.resize(level_event_receivers.size()-1);
        }
    }
}

void Level::WindowResized(ivec2 value ) {
    ASArglist args;
    args.Add(value[0]);
    args.Add(value[1]);
    for(unsigned i = 0; i < as_contexts_.size(); i++ ) {
        as_contexts_[i].ctx->CallScriptFunction(as_contexts_[i].as_funcs.set_window_dimensions, &args);
    }
}

void Level::PushSpawnerItemRecent(const SpawnerItem& item) {
    std::vector<SpawnerItem>::iterator spawner_it = recently_created_items_.begin();
    while( spawner_it != recently_created_items_.end()) {
        if( *spawner_it == item ) {
            recently_created_items_.erase(spawner_it);
            spawner_it = recently_created_items_.begin();
        } else {
            ++spawner_it;
        }
    }
    recently_created_items_.push_back(item);
}

std::vector<SpawnerItem> Level::GetRecentlyCreatedItems() {
    return recently_created_items_;
}

void Level::IncomingTCPData(SocketID socket, uint8_t* data, size_t len) {
    for(unsigned i = 0; i < as_contexts_.size(); i++ ) {
        asIScriptEngine *engine = as_contexts_[i].ctx->GetEngine();
        asITypeInfo *arrayType = engine->GetTypeInfoByDecl("array<uint8>");

        CScriptArray* array = CScriptArray::Create(arrayType);
        array->Reserve(len);

        for( unsigned i = 0; i < len; i++ ) {
            array->InsertLast(&data[i]);
        }

        ASArglist args;
        args.Add(socket);
        args.AddAddress((void*)array);

        as_contexts_[i].ctx->CallScriptFunction(as_contexts_[i].as_funcs.incoming_tcp_data, &args, NULL); 

        array->Release();
    }
}
