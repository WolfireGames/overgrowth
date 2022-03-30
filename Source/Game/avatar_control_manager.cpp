//-----------------------------------------------------------------------------
//           Name: avatar_control_manager.cpp
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
#include "avatar_control_manager.h"

#include <Main/engine.h>
#include <Main/scenegraph.h>

#include <Graphics/camera.h>
#include <Editors/map_editor.h>
#include <Objects/cameraobject.h>
#include <Game/level.h>
#include <Online/online.h>

std::vector<Possession> AvatarControlManager::GeneratePossessionList() {
    Engine* engine = Engine::Instance();
    Online* online = Online::Instance();
    SceneGraph* scenegraph_ = Engine::Instance()->GetSceneGraph();

    std::vector<Possession> possessions;

    int local_player_index = 0;
    if(online->IsActive()) {
        for(auto& player_state : online->GetPlayerStates()) {
            if (player_state.object_id == -1) {
                continue;
            }

            bool is_local = online->IsLocalAvatar(player_state.object_id);
            int camera_id = -1;
            int controller_id = -1;

            if(is_local) {
                camera_id = 0;
                controller_id = local_player_index;
                local_player_index++;
            } else {
                //Virtual controller_id, used for syncing input from remote source
                camera_id = player_state.camera_id;
                controller_id = player_state.controller_id;
            }

            MovementObject* mo = nullptr;
            if(scenegraph_ != nullptr ) {
                Object* ob = scenegraph_->GetObjectFromID(player_state.object_id);

                if(ob != nullptr && ob->GetType() == EntityType::_movement_object) {
                    mo = static_cast<MovementObject*>(ob);
                }
            }

            if(mo != nullptr && camera_id != -1 && controller_id != -1) {
                possessions.push_back(Possession(
                    player_state.object_id,
                    controller_id,
                    camera_id,
                    is_local
                ));
            }
        }
    }

    std::vector<ObjectID> unpossessed_avatars = GetUnpossessedAvatars();

    for(int i = 0; i < unpossessed_avatars.size(); i++) {
        int camera_id = 0;

        if(Engine::Instance()->GetSplitScreen()) {
            camera_id = i;
        }

        possessions.push_back(Possession(unpossessed_avatars[i], local_player_index, camera_id, !Online::Instance()->IsActive() || Online::Instance()->IsHosting()));
        local_player_index++;
    }

    return possessions;
}

std::vector<ObjectID> AvatarControlManager::GetUnpossessedAvatars() {
    Online* online = Online::Instance();
    SceneGraph* scenegraph_ = Engine::Instance()->GetSceneGraph();

    std::vector<ObjectID> unpossessed_avatars;
    
    int avatar_ids[Engine::kMaxAvatars];
    int num_avatars;
    if(scenegraph_ != nullptr) {
        scenegraph_->GetPlayerCharacterIDs(&num_avatars, avatar_ids, Engine::kMaxAvatars);

        for(int i = 0; i < num_avatars; i++) {
            bool found = online->IsAvatarPossessed(avatar_ids[i]);

            if(found == false) {
                unpossessed_avatars.push_back(avatar_ids[i]);
            }
        }
    }

    return unpossessed_avatars;
}

void AvatarControlManager::Update() {
    Engine* engine = Engine::Instance();
    Online* online = Online::Instance();
    SceneGraph* scenegraph_ = Engine::Instance()->GetSceneGraph();

    scenegraph_->GetPlayerCharacterIDs(&engine->num_avatars, engine->avatar_ids, Engine::kMaxAvatars);

    if (scenegraph_->map_editor->state_ != MapEditor::kInGame) { // We probably shouldn't do this in mp at all, not even our own controlable char as host
        for(int i=0; i < engine->num_avatars; ++i){
            Object* obj = scenegraph_->GetObjectFromID(engine->avatar_ids[i]);
            if(obj->GetType() == _movement_object){
                MovementObject* avatar = (MovementObject*)obj;

                //If we're in a multiplayer game, don't switch other players characters to NPC mode, only do it to ours.
                if (online->IsHosting() == false || online->IsLocalAvatar(engine->avatar_ids[i])) {
                    avatar->ChangeControlScript(scenegraph_->level->GetNPCScript(avatar));
                    avatar->controlled = false;
                    avatar->camera_id = 0;
                }
            }
        }
        Camera& active_camera = *ActiveCameras::Get();
        CameraObject& camera_object = *active_camera.getCameraObject();
        if(!camera_object.controlled) {
            camera_object.controlled = true;
            camera_object.SetTranslation(active_camera.GetPos());
        }
    } else {
        //If there is no controllable avatars in the scene, create one at the camera position and use it.
        if (engine->num_avatars == 0) {
            int id = scenegraph_->map_editor->CreateObject("Data/Objects/IGF_Characters/IGF_TurnerActor.xml");
            MovementObject* mo = static_cast<MovementObject*>(scenegraph_->GetObjectFromID(id));
            mo->SetTranslation(ActiveCameras::Get()->getCameraObject()->GetTranslation());
            mo->is_player = true;
            scenegraph_->GetPlayerCharacterIDs(&engine->num_avatars, engine->avatar_ids, Engine::kMaxAvatars);
        }

        //If we're running in multiplayer, disable multiple local avatars for now.
        //This might change in the future if we want to support local split-screen, then
        //this has to be called with the number of local players on _this_ client/host, 
        //because this function sets up the bindings between controllers and the indexes of controller
        //input from 0 to 3.
        if(online->IsActive()) {
            Input::Instance()->SetUpForXPlayers(1);
        } else {
            Input::Instance()->SetUpForXPlayers(engine->num_avatars);
        }

        bool focused_character_set = false;
		/*
		for (uint32_t i = 0; i < num_npc_avatars; i++) {

			MovementObject* avatar = static_cast<MovementObject*>(scenegraph_->GetObjectFromID(avatar_ids[i]));
			if (Engine::Instance()->multiplayer.IsActive() && !Engine::Instance()->multiplayer.IsHosting()) {
				avatar->ChangeControlScript(scenegraph_->level->GetNPCMPScript());
			}
		}
		*/

        std::vector<Possession> possessions = GeneratePossessionList();

        for(Possession possession : possessions) {
            MovementObject* avatar = static_cast<MovementObject*>(scenegraph_->GetObjectFromID(possession.avatar));	

			if (avatar != nullptr) {
				avatar->ChangeControlScript(scenegraph_->level->GetPCScript(avatar));

				avatar->controlled = true;
				avatar->controller_id = possession.controller_id;
				avatar->remote = !possession.is_local;

				if (possession.is_local) {
					//Changing the focused value should only occur if we're playing in split-screen, as it's the only
					//situation where it's relevant to dynamically shift the audio source, and the underlying reason why
					//we have this concept in the engine in the first place.
					if (engine->GetSplitScreen()) {
						//TODO:
						//Getting the knocked_out value here from the angel script env is pretty rough.
						//We should reconsider this approach. /Max
						if (!focused_character_set && avatar->ASGetIntVar("knocked_out") == MovementObject::_awake) {
							avatar->focused_character = true;
							focused_character_set = true;
						}
						else {
							avatar->focused_character = false;
						}
					}
					else {
                        if (!avatar->focused_character && !focused_character_set && !avatar->remote && Online::Instance()->IsHosting()) {
                            Online::Instance()->PossessAvatar(Online::Instance()->online_session->local_player_id, avatar->GetID());
                        }

						//Set the first player controlled avatar as the focused one, this works
						//in single player because we only expect there to be one controllable
						//avatar.
						avatar->focused_character = !focused_character_set;
						focused_character_set = true;
					}
				}

				avatar->camera_id = possession.camera_id;
			}
        }
        ActiveCameras::Get()->getCameraObject()->controlled = false;
    }
}
