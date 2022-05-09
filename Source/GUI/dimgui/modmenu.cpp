//-----------------------------------------------------------------------------
//           Name: modmenu.cpp
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
#include "modmenu.h"

#include <Internal/modloading.h>
#include <Internal/common.h>
#include <Internal/modid.h>
#include <Internal/config.h>

#include <Steam/steamworks.h>
#include <Steam/ugc_item.h>

#include <Main/engine.h>
#include <Graphics/textures.h>
#include <Utility/flat_hash_map.hpp>

#include <imgui.h>
#include <imgui_internal.h>

extern bool show_mod_menu;

static ModID mod_menu_selected_sid = -1;
static ModID upload_to_sid = -1;
static ModID update_to_sid = -1;

static bool show_upload_to_steamworks = false;
static bool show_update_to_steamworks = false;
static bool show_manifest = false;

static void SetDrawUploadToSteamworks(ModID _upload_to_sid) {
    upload_to_sid = _upload_to_sid;
}

static int update_to_currently_selected_visbility = 2;

static void SetDrawUpdateToSteamworks(ModID _update_to_sid) {
    update_to_sid = _update_to_sid;

#if ENABLE_STEAMWORKS
    SteamworksUGC* ugc = Steamworks::Instance()->GetUGC();
    ModInstance* target_modi = ModLoading::Instance().GetMod(update_to_sid);

    if (target_modi) {
        SteamworksUGCItem* ugc_item = target_modi->GetUGCItem();
        if (ugc_item) {
            for (int i = 0; i < update_to_visibilty_options_count; i++) {
                if (mod_visibility_map[i] == ugc_item->visibility) {
                    update_to_currently_selected_visbility = i;
                }
            }
        }
    }
#endif
}

static std::vector<std::pair<const char*, std::vector<ModInstance*> > > GetCategorizedMods() {
    std::vector<std::pair<const char*, std::vector<ModInstance*> > > categorized_mods;

    categorized_mods.push_back(std::pair<const char*, std::vector<ModInstance*> >("Local Mods", std::vector<ModInstance*>()));
    categorized_mods.push_back(std::pair<const char*, std::vector<ModInstance*> >("Subscribed Steam Workshop Mods", std::vector<ModInstance*>()));
    categorized_mods.push_back(std::pair<const char*, std::vector<ModInstance*> >("Unsubscribed Steam Workshop Mods", std::vector<ModInstance*>()));
    categorized_mods.push_back(std::pair<const char*, std::vector<ModInstance*> >("My Steam Workshop Mods", std::vector<ModInstance*>()));
    categorized_mods.push_back(std::pair<const char*, std::vector<ModInstance*> >("My Favorites", std::vector<ModInstance*>()));
    categorized_mods.push_back(std::pair<const char*, std::vector<ModInstance*> >("Core Packages", std::vector<ModInstance*>()));

    const std::vector<ModInstance*> mods = ModLoading::Instance().GetAllMods();

    for (auto mod : mods) {
        if (mod->IsCore()) {
            categorized_mods[5].second.push_back(mod);
        } else if (mod->modsource == ModSourceLocalModFolder) {
            categorized_mods[0].second.push_back(mod);
        } else if (mod->modsource == ModSourceSteamworks) {
            if (mod->IsOwnedByCurrentUser()) {
                categorized_mods[3].second.push_back(mod);
            } else {
                if (mod->IsFavorite()) {
                    categorized_mods[4].second.push_back(mod);
                } else {
                    if (mod->IsSubscribed()) {
                        categorized_mods[1].second.push_back(mod);
                    } else {
                        categorized_mods[2].second.push_back(mod);
                    }
                }
            }
        }
    }
    return categorized_mods;
}

typedef std::vector<std::pair<const char*, std::vector<ModInstance*> > > CategorizedModsList;
static CategorizedModsList categorized_mods_cache_;
static int categorized_mods_iterations_until_next_check = 0;
const int kIterationsPerModListCheck = 30;

static CategorizedModsList& GetCachedCategorizedMods() {
    --categorized_mods_iterations_until_next_check;
    if (categorized_mods_iterations_until_next_check <= 0) {
        categorized_mods_iterations_until_next_check = (int)(kIterationsPerModListCheck + ((float)rand() * (kIterationsPerModListCheck - 1)) / RAND_MAX);
        categorized_mods_cache_ = GetCategorizedMods();
    }
    return categorized_mods_cache_;
}

typedef ska::flat_hash_map<ModInstance*, bool> ModCanActivateCheckCacheMap;
static ModCanActivateCheckCacheMap mod_can_activate_check_cache_map_;

static bool CanActivateModInstance(ModInstance* mod) {
    bool can_activate;
    auto cached_can_activate_check_iter = mod_can_activate_check_cache_map_.find(mod);

    if (cached_can_activate_check_iter == mod_can_activate_check_cache_map_.end()) {
        can_activate = mod->CanActivate();
        mod_can_activate_check_cache_map_.insert(cached_can_activate_check_iter, ModCanActivateCheckCacheMap::value_type(mod, can_activate));
    } else {
        // TODO: If mod gets activated outside of this menu, need to clear this cache
        can_activate = cached_can_activate_check_iter->second;
    }

    return can_activate;
}

static void DrawSimpleModMenu() {
    CategorizedModsList& categmods = GetCachedCategorizedMods();
    CategorizedModsList::iterator modcatit = categmods.begin();

    for (; modcatit != categmods.end(); modcatit++) {
        const std::vector<ModInstance*> mods = modcatit->second;
        if (mods.size() > 0 && ImGui::TreeNodeEx(modcatit->first, ImGuiTreeNodeFlags_DefaultOpen)) {
            for (auto mod : mods) {
                bool active = mod->IsActive();
                ModID sid = mod->GetSid();
                ImGui::PushID(sid.id);
                if (CanActivateModInstance(mod)) {
                    if (ImGui::Checkbox(mod->name, &active)) {
                        ModLoading::Instance().GetMod(mod->GetSid())->Activate(active);
                        mod_can_activate_check_cache_map_.clear();
                    }
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                    ImGui::Checkbox(mod->name, &active);
                    ImGui::PopStyleColor(1);
                }
                ImGui::PopID();
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(450.0f);
                    const int kBufSize = 512;
                    char buf[kBufSize];

                    if (mod->GetValidityErrors().size() > 0) {
                        FormatString(
                            buf, kBufSize,
                            "%s %s\nWarnings:\n%s",
                            mod->name.c_str(), mod->version.c_str(), mod->GetValidityErrors().c_str());
                    } else {
                        FormatString(
                            buf, kBufSize,
                            "%s %s\n",
                            mod->name.c_str(), mod->version.c_str());
                    }
                    ImGui::TextUnformatted(buf);
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
            }
            ImGui::TreePop();
        }
    }
}

static void ImGuiYesNo(bool v) {
    ImGui::Text("%s", v ? "Yes" : "No");
}

static void ImguiDrawParameter(ModInstance::Parameter* p, const char* parent_type, int parent_index) {
    const char* name = p->name.c_str();
    const char* type = p->type.c_str();
    const char* value = p->value.c_str();
    ImGui::PushID(parent_index);

    if (strmtch(type, "array") || strmtch(type, "table")) {
        if (strmtch(parent_type, "table")) {
            if (ImGui::TreeNode("p", "[\"%s\"]", name)) {
                for (uint32_t i = 0; i < p->parameters.size(); i++) {
                    ImguiDrawParameter(&p->parameters[i], type, i);
                }
                ImGui::TreePop();
            }
        } else if (strmtch(parent_type, "array")) {
            if (ImGui::TreeNode("p", "[%d]", parent_index)) {
                for (uint32_t i = 0; i < p->parameters.size(); i++) {
                    ImguiDrawParameter(&p->parameters[i], type, i);
                }
                ImGui::TreePop();
            }
        } else {
            if (ImGui::TreeNode("p", "Parameter")) {
                for (uint32_t i = 0; i < p->parameters.size(); i++) {
                    ImguiDrawParameter(&p->parameters[i], type, i);
                }
                ImGui::TreePop();
            }
        }
    } else {
        if (strmtch(parent_type, "table")) {
            ImGui::BulletText("[\"%s\"]: \"%s\"", name, value);
        } else if (strmtch(parent_type, "array")) {
            ImGui::BulletText("[%d]: \"%s\"", parent_index, value);
        } else {
            ImGui::BulletText("\"%s\"", value);
        }
    }

    ImGui::PopID();
}

static void DrawAdvancedModMenu(Engine* engine) {
    const std::vector<ModInstance*>& mods = ModLoading::Instance().GetMods();

    // Automatic size
    ImGui::BeginChild("Sub1", ImVec2(300, 0), false, ImGuiWindowFlags_HorizontalScrollbar & ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Columns(1);

#if ENABLE_STEAMWORKS
    if (Steamworks::Instance()->UserNeedsToAcceptWorkshopAgreement()) {
        ImGui::TextWrapped("%s", "Note: You need to go to steampowered.com and accept the Workshop Agreement before you're able to fully publish mods.");
        ImGui::Separator();
    }
#endif

    CategorizedModsList& categmods = GetCachedCategorizedMods();
    CategorizedModsList::iterator modcatit = categmods.begin();

    for (; modcatit != categmods.end(); modcatit++) {
        const std::vector<ModInstance*> mods = modcatit->second;
        if (mods.size() > 0 && ImGui::TreeNodeEx(modcatit->first, ImGuiTreeNodeFlags_DefaultOpen)) {
            for (auto mod : mods) {
                bool active = mod->IsActive();
                ModID sid = mod->GetSid();
                ImGui::PushID(sid.id);
                if (CanActivateModInstance(mod)) {
                    if (mod->IsActive()) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                    }

                    if (ImGui::Selectable(mod->name, mod_menu_selected_sid == sid, ImGuiSelectableFlags_SpanAllColumns)) {
                        mod_menu_selected_sid = sid;
                    }

                    if (mod->IsActive()) {
                        ImGui::PopStyleColor(1);
                    }
                    /*
                    if(ImGui::Checkbox(mod->name.c_str(), &active)){
                        ModLoading::Instance().GetMod(mod->GetSid())->Activate(active);
                        mod_can_activate_check_cache_map_.clear();
                    }
                    */
                } else {
                    if (mod->IsInstalled()) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.3f, 0.1f, 1.0f));
                    }

                    if (ImGui::Selectable(mod->name, mod_menu_selected_sid == sid, ImGuiSelectableFlags_SpanAllColumns)) {
                        mod_menu_selected_sid = sid;
                    }
                    ImGui::PopStyleColor(1);
                    /*
                    ImGui::Checkbox(mod->name.c_str(), &active);
                    //ImGui::MenuItem(mod->name.c_str(), NULL, false, false);
                    */
                }
                ImGui::PopID();
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(450.0f);
                    const int kBufSize = 512;
                    char buf[kBufSize];
                    if (mod->GetValidityErrors().size() > 0) {
                        FormatString(
                            buf, kBufSize,
                            "%s %s\nWarnings:\n%s",
                            mod->name.c_str(), mod->version.c_str(),
                            mod->GetValidityErrors().c_str());
                    } else {
                        FormatString(
                            buf, kBufSize,
                            "%s %s\n",
                            mod->name.c_str(), mod->version.c_str());
                    }
                    ImGui::TextUnformatted(buf);
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
            }
            ImGui::TreePop();
        }
    }

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
    ImGui::BeginChild("Sub2", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar & ImGuiWindowFlags_AlwaysAutoResize);

    ModInstance* modi = ModLoading::Instance().GetMod(mod_menu_selected_sid);

    if (modi != NULL) {
        Path thumbnail_path = FindFilePath(AssemblePath(modi->path.c_str(), modi->thumbnail), kAbsPath, false);
        if (strlen(modi->thumbnail) > 0 && thumbnail_path.isValid()) {
            Engine::Instance()->spawner_thumbnail = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(thumbnail_path.GetFullPath(), PX_NOMIPMAP | PX_NOREDUCE | PX_NOCONVERT, 0x0);
            Textures::Instance()->EnsureInVRAM(Engine::Instance()->spawner_thumbnail->GetTextureRef());
            ImGui::Image((ImTextureID)(uintptr_t)Textures::Instance()->returnTexture(Engine::Instance()->spawner_thumbnail), ImVec2(400, 225), ImVec2(0, 0), ImVec2(1, 1));
            ImGui::Separator();
        }

        ImGui::Columns(2);
        ImGui::Text("ID:");
        ImGui::NextColumn();
        ImGui::Text("%s", modi->id.c_str());
        ImGui::NextColumn();
        if (modi->modsource == ModSourceSteamworks) {
#if ENABLE_STEAMWORKS
            SteamworksUGCItem* item = modi->GetUGCItem();
            if (item) {
                ImGui::Text("Steamworks ID:");
                ImGui::NextColumn();
                ImGui::TextWrapped("%s", item->GetSteamworksIDString().c_str());
                ImGui::NextColumn();
            }
#endif
        }
        ImGui::Text("Version:");
        ImGui::NextColumn();
        ImGui::Text("%s", modi->version.c_str());
        ImGui::NextColumn();
        ImGui::Text("Type:");
        ImGui::NextColumn();
        ImGui::Text("%s", modi->GetModsourceString());
        ImGui::NextColumn();
        ImGui::Text("Name:");
        ImGui::NextColumn();
        ImGui::Text("%s", modi->name.c_str());
        ImGui::NextColumn();
        ImGui::Text("Category:");
        ImGui::NextColumn();
        ImGui::Text("%s", modi->category.c_str());
        ImGui::NextColumn();
        ImGui::Text("Author:");
        ImGui::NextColumn();
        ImGui::Text("%s", modi->author.c_str());
        ImGui::NextColumn();
        ImGui::Text("Tags:");
        ImGui::NextColumn();
        ImGui::TextWrapped("%s", modi->GetTagsListString().c_str());
        ImGui::NextColumn();

        if (modi->modsource == ModSourceSteamworks) {
#if ENABLE_STEAMWORKS
            SteamworksUGCItem* item = modi->GetUGCItem();
            if (item) {
                ImGui::Text("Visibility");
                ImGui::NextColumn();
                int visibility_index = 0;
                for (int i = 0; i < update_to_visibilty_options_count; i++) {
                    if (mod_visibility_map[i] == item->visibility) {
                        visibility_index = i;
                    }
                }
                ImGui::Text("%s", mod_visibility_options[visibility_index]);
                ImGui::NextColumn();
            }
#endif
        }

        ImGui::Separator();

        ImGui::Text("Valid:");
        ImGui::NextColumn();
        ImGuiYesNo(modi->IsValid());
        ImGui::NextColumn();

        ImGui::Text("Activated:");
        ImGui::NextColumn();
        ImGuiYesNo(modi->IsActive());
        ImGui::NextColumn();

        if (modi->modsource == ModSourceSteamworks) {
#if ENABLE_STEAMWORKS
            ImGui::Text("Personal Favorite:");
            ImGui::NextColumn();
            ImGuiYesNo(modi->IsFavorite());
            ImGui::NextColumn();

            ImGui::Text("Personal Vote:");
            ImGui::NextColumn();
            const char* vote = "";

            switch (modi->GetUserVote()) {
                case ModInstance::k_VoteUnknown:
                    vote = "Unknown";
                    break;
                case ModInstance::k_VoteNone:
                    vote = "None";
                    break;
                case ModInstance::k_VoteUp:
                    vote = "Up";
                    break;
                case ModInstance::k_VoteDown:
                    vote = "Down";
                    break;
            }
            ImGui::Text("%s", vote);
            ImGui::NextColumn();

            SteamworksUGCItem* item = modi->GetUGCItem();
            if (item) {
                ImGui::Separator();

                ImGui::Text("Subscribed:");
                ImGui::NextColumn();
                ImGuiYesNo(item->IsSubscribed());
                ImGui::NextColumn();

                ImGui::Text("Installed:");
                ImGui::NextColumn();
                ImGuiYesNo(item->IsInstalled());
                ImGui::NextColumn();

                ImGui::Text("Downloading:");
                ImGui::NextColumn();
                ImGuiYesNo(item->IsDownloading());
                ImGui::NextColumn();

                ImGui::Text("Download Pending:");
                ImGui::NextColumn();
                ImGuiYesNo(item->IsDownloadPending());
                ImGui::NextColumn();

                ImGui::Text("Needs Update:");
                ImGui::NextColumn();
                ImGuiYesNo(item->NeedsUpdate());
                ImGui::NextColumn();

                ImGui::Text("Steam Error:");
                ImGui::NextColumn();
                ImGui::TextWrapped("%s", item->GetLastResultError());
                ImGui::NextColumn();
            }
#else
            ImGui::Text("Game wasn't compiled with Steamworks Support.");
#endif
        }
        ImGui::Separator();

        ImGui::Text("Path:");
        ImGui::NextColumn();
        ImGui::TextWrapped("%s", modi->path.c_str());
        ImGui::NextColumn();

        ImGui::Separator();

        std::vector<std::string> validity_errors = modi->GetValidityErrorsArr();
        ImGui::Text("Errors:");
        for (auto& validity_error : validity_errors) {
            ImGui::NextColumn();
            ImGui::Text("%s", validity_error.c_str());
            ImGui::NextColumn();
        }
        ImGui::Separator();
        ImGui::Text("Mod Dependencies:");
        for (auto& mod_dependencie : modi->mod_dependencies) {
            ImGui::NextColumn();
            ImGui::Text("%s", mod_dependencie.id.c_str());
            ImGui::NextColumn();
        }
        ImGui::Separator();
        ImGui::Text("Supported Version:");
        for (auto& supported_version : modi->supported_versions) {
            ImGui::NextColumn();
            ImGui::Text("%s", supported_version.c_str());
            ImGui::NextColumn();
        }
        ImGui::Separator();

        ImGui::Text("User Control");
        ImGui::NextColumn();

        if (modi->CanActivate()) {
            if (modi->IsActive()) {
                if (ImGui::Button("Deactivate")) {
                    modi->Activate(false);
                    mod_can_activate_check_cache_map_.clear();
                }
            } else {
                if (ImGui::Button("Activate")) {
                    modi->Activate(true);
                    mod_can_activate_check_cache_map_.clear();
                }
            }
        }
#if ENABLE_STEAMWORKS
        SteamworksUGC* ugc = Steamworks::Instance()->GetUGC();
        if (ugc) {
            if (modi->modsource == ModSourceSteamworks) {
                if (ImGui::Button("Open Workshop Page")) {
                    ModID id = modi->GetSid();
                    Steamworks::Instance()->OpenWebPageToMod(id);
                }

                if (ImGui::Button("Open Author Page")) {
                    ModID id = modi->GetSid();
                    Steamworks::Instance()->OpenWebPageToModAuthor(id);
                }

                if (modi->IsSubscribed()) {
                    if (ImGui::Button("Unsubscribe")) {
                        modi->RequestUnsubscribe();
                    }
                } else {
                    if (ImGui::Button("Subscribe")) {
                        modi->RequestSubscribe();
                    }
                }

                if (modi->IsFavorite()) {
                    if (ImGui::Button("Remove Favorite")) {
                        modi->RequestFavoriteSet(false);
                    }
                } else {
                    if (ImGui::Button("Make Favorite")) {
                        modi->RequestFavoriteSet(true);
                    }
                }
                if (modi->GetUserVote() != ModInstance::k_VoteUp) {
                    if (ImGui::Button("Vote Up")) {
                        modi->RequestVoteSet(true);
                    }
                }
                if (modi->GetUserVote() != ModInstance::k_VoteDown) {
                    if (ImGui::Button("Vote Down")) {
                        modi->RequestVoteSet(false);
                    }
                }
            }
        }
#endif
        ImGui::NextColumn();

        ImGui::Separator();

        ImGui::Text("Owner Control");
        ImGui::NextColumn();

        if (ImGui::Button("Manifest...")) {
            show_manifest = true;
        }

        if (modi->IsActive() && (!modi->levels.empty() || !modi->campaigns.empty()) && ImGui::Button("Generate cache")) {
            engine->GenerateLevelCache(modi);
        }

#if ENABLE_STEAMWORKS
        if (ugc) {
            if (modi->modsource == ModSourceLocalModFolder && modi->IsCore() == false) {
                if (ImGui::Button("Upload To Steamworks...")) {
                    show_upload_to_steamworks = true;
                    SetDrawUploadToSteamworks(mod_menu_selected_sid);
                }
            }

            if (modi->modsource == ModSourceSteamworks) {
                if (modi->IsOwnedByCurrentUser()) {
                    if (ImGui::Button("Upload Update...")) {
                        show_update_to_steamworks = true;
                        SetDrawUpdateToSteamworks(mod_menu_selected_sid);
                    }
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                    ImGui::Button("Upload Update...");
                    ImGui::PopStyleColor(1);
                }
            }
        }
#endif

        ImGui::NextColumn();

        ImGui::Separator();

        ImGui::Text("Items:");
        for (auto& item : modi->items) {
            ImGui::NextColumn();
            ImGui::Text("%s", item.title.c_str());
            ImGui::NextColumn();
        }
        ImGui::Separator();

        ImGui::Text("Levels:");
        for (auto& level : modi->levels) {
            ImGui::NextColumn();
            ImGui::Text("%s", level.title.c_str());
            ImGui::NextColumn();
        }
        ImGui::Separator();

        ImGui::Text("Campaigns:");
        for (auto& campaign : modi->campaigns) {
            ImGui::NextColumn();
            ImGui::Text("%s", campaign.title.c_str());
            ImGui::NextColumn();
        }

        ImGui::Separator();

        for (unsigned i = 0; i < modi->campaigns.size(); i++) {
            ImGui::Text("%s levels:", modi->campaigns[i].title.c_str());

            for (auto& level : modi->campaigns[i].levels) {
                ImGui::NextColumn();
                ImGui::Text("%s", level.title.c_str());
                ImGui::NextColumn();
            }

            if (i != modi->campaigns.size() - 1) {
                ImGui::Separator();
            }
        }
    }

    ImGui::Columns(1);

    if (modi != NULL) {
        ImGui::Separator();
        ImGui::Text("Parameter Data");
        ImGui::Separator();

        for (auto& level : modi->levels) {
            if (ImGui::TreeNode(level.title.c_str(), "%s level", level.title.c_str())) {
                ImguiDrawParameter(&level.parameter, "", 0);
                ImGui::TreePop();
            }
        }

        for (auto& campaign : modi->campaigns) {
            ImGui::PushID("campaign_params");
            if (ImGui::TreeNode(campaign.id.c_str(), "%s campaign", campaign.title.c_str())) {
                ImguiDrawParameter(&campaign.parameter, "", 0);
                for (unsigned j = 0; j < campaign.levels.size(); j++) {
                    if (ImGui::TreeNode(campaign.levels[j].title.c_str(), "%s level", campaign.levels[j].title.c_str())) {
                        ImguiDrawParameter(&campaign.levels[j].parameter, "", 0);
                        ImGui::TreePop();
                    }
                }

                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
}

static void DrawUploadToSteamworks() {
    UGCID upload_ugcid;

    ImGui::SetNextWindowSize(ImVec2(1024.0f, 768.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Steamworks New Upload", &show_upload_to_steamworks);

#if ENABLE_STEAMWORKS
    SteamworksUGC* ugc = Steamworks::Instance()->GetUGC();
    ModInstance* modi = ModLoading::Instance().GetMod(upload_to_sid);

    if (ugc && modi) {
        ImGui::Columns(2);
        ImGui::Text("ID:");
        ImGui::NextColumn();
        ImGui::Text("%s", modi->id.c_str());
        ImGui::NextColumn();
        ImGui::Text("Version:");
        ImGui::NextColumn();
        ImGui::Text("%s", modi->version.c_str());
        ImGui::NextColumn();
        ImGui::Text("Name:");
        ImGui::NextColumn();
        ImGui::Text("%s", modi->name.c_str());
        ImGui::NextColumn();
        ImGui::Text("Category:");
        ImGui::NextColumn();
        ImGui::Text("%s", modi->category.c_str());
        ImGui::NextColumn();
        ImGui::Text("Author:");
        ImGui::NextColumn();
        ImGui::Text("%s", modi->author.c_str());
        ImGui::NextColumn();
        ImGui::Text("Tags:");
        ImGui::NextColumn();
        ImGui::TextWrapped("%s", modi->GetTagsListString().c_str());
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();

        static char change_desc[k_cchPublishedDocumentChangeDescriptionMax] = "Initial upload";

        ImGui::Text("Update Message");
        ImGui::InputTextMultiline("##source", change_desc, IM_ARRAYSIZE(change_desc), ImVec2(-1.0f, ImGui::GetTextLineHeight() * 12), ImGuiInputTextFlags_AllowTabInput);

        uint32_t upload_status = modi->GetUploadValidity();

        if (upload_status == ModInstance::k_UploadValidityOk) {
            ImGui::Separator();

            ImGui::Columns(2);

            std::vector<SteamworksUGCItem*>::iterator uploaded_item = ugc->GetItem(upload_ugcid);
            if (uploaded_item != ugc->GetItemEnd()) {
                ImGui::Text("Upload Status:");
                ImGui::NextColumn();
                ImGui::Text("%s", (*uploaded_item)->UpdateStatusString());
                ImGui::NextColumn();
                ImGui::Text("Progress:");
                ImGui::NextColumn();
                ImGui::ProgressBar((*uploaded_item)->ItemUploadProgress());
                ImGui::NextColumn();
                ImGui::Text("Steam Error:");
                ImGui::NextColumn();
                ImGui::TextWrapped("%s", (*uploaded_item)->GetLastResultError());
                ImGui::NextColumn();
            } else {
                ImGui::Text("Upload Status:");
                ImGui::NextColumn();
                ImGui::Text("Waiting");
                ImGui::NextColumn();
                ImGui::Text("Progress:");
                ImGui::NextColumn();
                ImGui::ProgressBar(0.0f);
                ImGui::NextColumn();
                ImGui::Text("Steam Error:");
                ImGui::NextColumn();
                ImGui::TextWrapped("%s", "");
                ImGui::NextColumn();
            }

            ImGui::Columns(1);
            ImGui::Separator();

            ImGui::TextWrapped("Before uploading, ensure you have the rights to publish this mod. Abuse and copyright infringement is likely to result in a ban from steam workshop. In some instances local and international law might apply.");

            if (ModLoading::Instance().GetSteamModsMatchingID(modi->id.str()).size() > 0) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0, 0, 1.0f));
                ImGui::TextWrapped("There is already an instance of this id, are you sure you don't wish to update an existing mod rather than upload a new one?");
                ImGui::PopStyleColor(1);
            }

            static bool verify_upload = false;
            ImGui::Checkbox("I have fully read all the above and agree", &verify_upload);

            if (verify_upload) {
                if (ImGui::Button("Upload")) {
                    if (verify_upload) {
                        verify_upload = false;
                        upload_ugcid = ugc->TryUploadMod(upload_to_sid);

                        if (upload_ugcid.Valid() == false) {
                            LOGE << "Upload failed" << std::endl;
                        }
                    }
                }
            }
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
            ImGui::TextWrapped("%s", "The following errors prevents you from uploading your mod.");
            ImGui::PopStyleColor();

            if (upload_status & ModInstance::k_UploadValidityInvalidPreviewImage) {
                ImGui::Separator();
                ImGui::TextWrapped("%s", "A referenced <PreviewImage> file is invalid in the source mod.");
            }

            if (upload_status & ModInstance::k_UploadValidityMissingPreview) {
                ImGui::Separator();
                ImGui::TextWrapped("%s", "The source mod is missing a valid <PreviewImage> file, atleast one is required for steamworks upload.");
            }

            if (upload_status & ModInstance::k_UploadValidityInvalidModFolder) {
                ImGui::Separator();
                ImGui::TextWrapped("%s", "The mod source path to folder is invalid.");
            }

            if (upload_status & ModInstance::k_UploadValidityBrokenXml) {
                ImGui::Separator();
                ImGui::TextWrapped("%s", "The mod.xml file is malformed.");
            }

            if (upload_status & ModInstance::k_UploadValidityInvalidID) {
                ImGui::Separator();
                ImGui::TextWrapped("%s", "The id field is invalid.");
            }

            if (upload_status & ModInstance::k_UploadValidityInvalidVersion) {
                ImGui::Separator();
                ImGui::TextWrapped("%s", "The version field is invalid.");
            }

            if (upload_status & ModInstance::k_UploadValidityMissingReadRights) {
                ImGui::Separator();
                ImGui::TextWrapped("%s", "Missing read rights in the mod folder.");
            }

            if (upload_status & ModInstance::k_UploadValidityMissingXml) {
                ImGui::Separator();
                ImGui::TextWrapped("%s", "Missing mod.xml file");
            }

            if (upload_status & ModInstance::k_UploadValidityInvalidThumbnail) {
                ImGui::Separator();
                ImGui::TextWrapped("%s", "Missing a valid <Thumbnail> file");
            }

            if (upload_status & ModInstance::k_UploadValidityOversizedPreviewImage) {
                ImGui::Separator();
                ImGui::TextWrapped("%s", "<PreviewImage> file size exceeds 1MB limit");
            }

            if (upload_status & ModInstance::k_UploadValidityGenericValidityError) {
                ImGui::Separator();
                ImGui::TextWrapped(
                    "%s",
                    ModInstance::GenerateValidityErrors(modi->GetValidity() & ModInstance::kValidityUploadSteamworksBlockingMask).c_str());

                std::vector<std::string> invalid_item_paths = modi->GetInvalidItemPaths();

                if (invalid_item_paths.size() > 0) {
                    ImGui::Separator();
                    ImGui::TextWrapped("%s", "The following <Item> paths are invalid");

                    for (unsigned int i = 0; i < invalid_item_paths.size(); i++) {
                        ImGui::TextWrapped("%s", invalid_item_paths[i].c_str());
                    }
                }
            }
        }
    }
#else
    ImGui::Text("Game was not compiled with Steamworks Support.");
#endif
    ImGui::End();
}

static void DrawUpdateToSteamworks() {
    ModInstance* target_modi = ModLoading::Instance().GetMod(update_to_sid);
    ModInstance* source_modi = NULL;

    static int currently_selected_source = 0;

    ImGui::SetNextWindowSize(ImVec2(1024.0f, 768.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Steamworks Mod Update Upload", &show_update_to_steamworks);
#if ENABLE_STEAMWORKS
    SteamworksUGC* ugc = Steamworks::Instance()->GetUGC();
    if (ugc && target_modi) {
        SteamworksUGCItem* uploaded_ugc_item = target_modi->GetUGCItem();
        if (uploaded_ugc_item) {
            std::vector<ModInstance*> possible_sources;

            if (strlen(target_modi->id) == 0) {
                possible_sources = ModLoading::Instance().GetLocalMods();
            } else {
                possible_sources = ModLoading::Instance().GetLocalModsMatchingID(target_modi->id.str());
            }

            if ((unsigned)currently_selected_source > possible_sources.size()) {
                currently_selected_source = possible_sources.size();
            }

            std::vector<const char*> dropdown_names;

            for (unsigned i = 0; i < possible_sources.size(); i++) {
                dropdown_names.push_back(possible_sources[i]->name);
            }

            if (possible_sources.size() > 0 && currently_selected_source < (int)possible_sources.size()) {
                source_modi = possible_sources[currently_selected_source];
            }

            if (source_modi) {
                uploaded_ugc_item->SetIntendedUpdateModSource(source_modi->GetSid());
            } else {
                uploaded_ugc_item->SetIntendedUpdateModSource(ModID());
            }

            uint32_t source_upload_status = 0ULL;
            if (source_modi) {
                source_upload_status = source_modi->GetUploadValidity();
            }

            uint32_t target_upload_status = uploaded_ugc_item->VerifyForUpload();

            ImGui::Columns(1);

            ImGui::Combo("Local Source Mod", &currently_selected_source, &dropdown_names[0], dropdown_names.size(), 10);

            ImGui::Separator();

            ImGui::Text("Source Mod");

            ImGui::Separator();

            ImGui::Columns(2);

            if (source_modi) {
                ImGui::Text("ID:");
                ImGui::NextColumn();
                ImGui::Text("%s", source_modi->id.c_str());
                ImGui::NextColumn();
                ImGui::Text("Version:");
                ImGui::NextColumn();
                if (target_upload_status & SteamworksUGCItem::k_UploadVerifyUnchangedVersion) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
                }
                ImGui::Text("%s", source_modi->version.c_str());
                ImGui::NextColumn();
                if (target_upload_status & SteamworksUGCItem::k_UploadVerifyUnchangedVersion) {
                    ImGui::PopStyleColor();
                }
                ImGui::Text("Name:");
                ImGui::NextColumn();
                ImGui::Text("%s", source_modi->name.c_str());
                ImGui::NextColumn();
                ImGui::Text("Category:");
                ImGui::NextColumn();
                ImGui::Text("%s", source_modi->category.c_str());
                ImGui::NextColumn();
                ImGui::Text("Author:");
                ImGui::NextColumn();
                ImGui::Text("%s", source_modi->author.c_str());
                ImGui::NextColumn();
                ImGui::Text("Tags:");
                ImGui::NextColumn();
                ImGui::TextWrapped("%s", source_modi->GetTagsListString().c_str());
                ImGui::NextColumn();
                ImGui::Text("Visibility");
                ImGui::NextColumn();
                ImGui::Combo("", &update_to_currently_selected_visbility, mod_visibility_options, 3, 3);
            } else {
                ImGui::Text("ID:");
                ImGui::NextColumn();
                ImGui::NextColumn();
                ImGui::Text("Version:");
                ImGui::NextColumn();
                ImGui::NextColumn();
                ImGui::Text("Name:");
                ImGui::NextColumn();
                ImGui::NextColumn();
                ImGui::Text("Category:");
                ImGui::NextColumn();
                ImGui::NextColumn();
                ImGui::Text("Author:");
                ImGui::NextColumn();
                ImGui::NextColumn();
                ImGui::Text("Tags:");
                ImGui::NextColumn();
                ImGui::NextColumn();
                ImGui::Text("Visibility");
                ImGui::NextColumn();
                ImGui::NextColumn();
            }

            ImGui::Columns(1);

            ImGui::Separator();

            ImGui::Text("Destination Mod");

            ImGui::Separator();

            ImGui::Columns(2);

            ImGui::Text("ID:");
            ImGui::NextColumn();
            ImGui::Text("%s", target_modi->id.c_str());
            ImGui::NextColumn();
            ImGui::Text("Version:");
            ImGui::NextColumn();
            ImGui::Text("%s", target_modi->version.c_str());
            ImGui::NextColumn();
            ImGui::Text("Name:");
            ImGui::NextColumn();
            ImGui::Text("%s", target_modi->name.c_str());
            ImGui::NextColumn();
            ImGui::Text("Category:");
            ImGui::NextColumn();
            ImGui::Text("%s", target_modi->category.c_str());
            ImGui::NextColumn();
            ImGui::Text("Author:");
            ImGui::NextColumn();
            ImGui::Text("%s", target_modi->author.c_str());
            ImGui::NextColumn();
            ImGui::Text("Tags:");
            ImGui::NextColumn();
            ImGui::TextWrapped("%s", target_modi->GetTagsListString().c_str());
            ImGui::NextColumn();
            ImGui::Text("Visibility");
            ImGui::NextColumn();

            int visibility_index = 0;
            for (int i = 0; i < update_to_visibilty_options_count; i++) {
                if (mod_visibility_map[i] == uploaded_ugc_item->visibility) {
                    visibility_index = i;
                }
            }
            ImGui::Text("%s", mod_visibility_options[visibility_index]);
            ImGui::NextColumn();

            ImGui::Columns(1);

            ImGui::Separator();

            static char change_desc[k_cchPublishedDocumentChangeDescriptionMax] = "Generic update";

            ImGui::Text("Update Message");
            ImGui::InputTextMultiline("##source", change_desc, IM_ARRAYSIZE(change_desc), ImVec2(-1.0f, ImGui::GetTextLineHeight() * 12), ImGuiInputTextFlags_AllowTabInput);

            /*
            static bool only_metadata = false;
            ImGui::Checkbox( "Only upload meta information", &only_metadata );

            if( ImGui::IsItemHovered() ) {
                ImGui::BeginTooltip();
                ImGui::Text("%s", "Limit upload only to mod meta information, including visibility.");
                ImGui::Text("%s", "This excludes Data folder and mod.xml file.");
                ImGui::EndTooltip();
            }
            */

            ImGui::Separator();

            ImGui::Columns(2);

            ImGui::Text("Upload Status:");
            ImGui::NextColumn();
            ImGui::Text("%s", uploaded_ugc_item->UpdateStatusString());
            ImGui::NextColumn();
            ImGui::Text("Progress:");
            ImGui::NextColumn();
            ImGui::ProgressBar(uploaded_ugc_item->ItemUploadProgress());
            ImGui::NextColumn();
            ImGui::Text("Upload Error:");
            ImGui::NextColumn();
            ImGui::TextWrapped("%s", uploaded_ugc_item->GetLastResultError());
            ImGui::NextColumn();

            if (source_modi) {
                if (uploaded_ugc_item->GetUpdateStatus() == k_EItemUpdateStatusInvalid) {
                    if (target_upload_status == SteamworksUGCItem::k_UploadVerifyOk && source_upload_status == ModInstance::k_UploadValidityOk) {
                        ImGui::Columns(1);
                        ImGui::Separator();

                        ImGui::TextWrapped("Before uploading, ensure you have the rights to publish this mod. Abuse and copyright infringement is likely to result in a ban from steam workshop. In some instances local and international law might apply.");

                        static bool verify_upload = false;
                        ImGui::Checkbox("I have fully read all the above and agree", &verify_upload);

                        if (verify_upload) {
                            if (ImGui::Button("Upload")) {
                                if (verify_upload) {
                                    verify_upload = false;
                                    target_modi->RequestUpdate(source_modi->GetSid(), change_desc, static_cast<ModVisibilityOptions>(update_to_currently_selected_visbility));
                                }
                            }
                        }
                    } else {
                        ImGui::Columns(1);
                        ImGui::Separator();

                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
                        ImGui::TextWrapped("%s", "The following errors prevents you from updating your mod.");
                        ImGui::PopStyleColor();

                        if (source_upload_status & ModInstance::k_UploadValidityInvalidPreviewImage) {
                            ImGui::Separator();
                            ImGui::TextWrapped("%s", "A referenced <PreviewImage> file is invalid in the source mod.");
                        }

                        if (source_upload_status & ModInstance::k_UploadValidityMissingPreview) {
                            ImGui::Separator();
                            ImGui::TextWrapped("%s", "The source mod is missing a valid <PreviewImage> file, atleast one is required for steamworks upload.");
                        }

                        if (source_upload_status & ModInstance::k_UploadValidityInvalidModFolder) {
                            ImGui::Separator();
                            ImGui::TextWrapped("%s", "The mod source path to folder is invalid.");
                        }

                        if (source_upload_status & ModInstance::k_UploadValidityBrokenXml) {
                            ImGui::Separator();
                            ImGui::TextWrapped("%s", "The mod.xml file is malformed.");
                        }
                        if (source_upload_status & ModInstance::k_UploadValidityInvalidID) {
                            ImGui::Separator();
                            ImGui::TextWrapped("%s", "The id field is invalid.");
                        }

                        if (source_upload_status & ModInstance::k_UploadValidityInvalidVersion) {
                            ImGui::Separator();
                            ImGui::TextWrapped("%s", "The version field is invalid.");
                        }

                        if (source_upload_status & ModInstance::k_UploadValidityMissingReadRights) {
                            ImGui::Separator();
                            ImGui::TextWrapped("%s", "Missing read rights in the mod folder.");
                        }

                        if (source_upload_status & ModInstance::k_UploadValidityGenericValidityError) {
                            ImGui::Separator();
                            ImGui::TextWrapped(
                                "%s",
                                ModInstance::GenerateValidityErrors(source_modi->GetValidity() & ModInstance::kValidityUploadSteamworksBlockingMask).c_str());
                        }

                        if (source_upload_status & ModInstance::k_UploadValidityMissingXml) {
                            ImGui::Separator();
                            ImGui::TextWrapped("%s", "Missing mod.xml file");
                        }

                        if (source_upload_status & ModInstance::k_UploadValidityInvalidThumbnail) {
                            ImGui::Separator();
                            ImGui::TextWrapped("%s", "Missing a valid <Thumbnail> file");
                        }

                        if (source_upload_status & ModInstance::k_UploadValidityOversizedPreviewImage) {
                            ImGui::Separator();
                            ImGui::TextWrapped("%s", "<PreviewImage> file size exceeds 1MB limit");
                        }

                        std::vector<std::string> invalid_item_paths = source_modi->GetInvalidItemPaths();

                        if (invalid_item_paths.size() > 0) {
                            ImGui::Separator();
                            ImGui::TextWrapped("%s", "The following <Item> paths are invalid");

                            for (unsigned int i = 0; i < invalid_item_paths.size(); i++) {
                                ImGui::TextWrapped("%s", invalid_item_paths[i].c_str());
                            }
                        }

                        if (target_upload_status & SteamworksUGCItem::k_UploadVerifyUnchangedVersion) {
                            ImGui::Separator();
                            ImGui::TextWrapped("%s", "The version is the same in source and target");
                        }

                        if (target_upload_status & SteamworksUGCItem::k_UploadVerifyMissingModSource) {
                            ImGui::Separator();
                            ImGui::TextWrapped("%s", "No valid mod source selected");
                        }

                        if (target_upload_status & SteamworksUGCItem::k_UploadVerifyInvalidPreviewImage) {
                            ImGui::Separator();
                            ImGui::TextWrapped("%s", "Preview image path is invalid");
                        }
                    }
                } else {
                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::TextWrapped("%s", "An upload is currently underway...");
                }
            }
        }
    }
#else
    ImGui::Text("Game was not compiled with Steamworks support.");
#endif
    ImGui::End();
}

static void ModActivationChanged(const ModInstance* mod);
class ImGuiModMenuLoadingCallback : public ModLoadingCallback {
   public:
    void ModActivationChange(const ModInstance* mod) override {
        ModActivationChanged(mod);
    }
};
static ImGuiModMenuLoadingCallback imgui_mod_loading_callback_;

static bool advanced_menu = false;
void InitializeModMenu() {
    advanced_menu = config["advanced_mod_menu"].toBool();
    ModLoading::Instance().RegisterCallback(&imgui_mod_loading_callback_);
}

void DrawModMenu(Engine* engine) {
    ImGui::SetNextWindowSize(ImVec2(1024.0f, 768.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Mods", &show_mod_menu, ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Menu")) {
#if ENABLE_STEAMWORKS
            if (Steamworks::Instance()->UserCanAccessWorkshop()) {
                if (ImGui::MenuItem("Steam Workshop", NULL, false, Steamworks::Instance()->IsConnected())) {
                    Steamworks::Instance()->OpenWebPageToWorkshop();
                }
            }
#endif

            if (ImGui::MenuItem("Advanced", NULL, &advanced_menu)) {
                config.GetRef("advanced_mod_menu") = advanced_menu;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    if (advanced_menu) {
        DrawAdvancedModMenu(engine);
    } else {
        DrawSimpleModMenu();
    }

    ImGui::End();

#if ENABLE_STEAMWORKS
    SteamworksUGC* ugc = Steamworks::Instance()->GetUGC();
    if (ugc) {
        ModInstance* modi = ModLoading::Instance().GetMod(upload_to_sid);
        if (modi) {
            if (modi->modsource == ModSourceLocalModFolder) {
                if (show_upload_to_steamworks) {
                    DrawUploadToSteamworks();
                }
            }
        }

        modi = ModLoading::Instance().GetMod(update_to_sid);
        if (modi) {
            if (modi->modsource == ModSourceSteamworks) {
                if (show_update_to_steamworks) {
                    DrawUpdateToSteamworks();
                }
            }
        }
    }
#endif

    if (show_manifest) {
        ModInstance* modi = ModLoading::Instance().GetMod(mod_menu_selected_sid);
        ImGui::SetNextWindowSize(ImVec2(1024.0f, 768.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Manifest", &show_manifest);
        if (modi) {
            for (auto& i : modi->manifest) {
                ImGui::Text("%s", i.c_str());
            }
        }
        ImGui::End();
    }
}

void CleanupModMenu() {
    mod_can_activate_check_cache_map_.clear();
    categorized_mods_cache_.clear();
    categorized_mods_iterations_until_next_check = 0;
}

static void ModActivationChanged(const ModInstance* mod) {
    // TODO: Can mods get added or removed during runtime?
    mod_can_activate_check_cache_map_.clear();
}
