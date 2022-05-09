//-----------------------------------------------------------------------------
//           Name: levelxmlparser.cpp
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
#include "levelxmlparser.h"

#include <XML/xml_helper.h>
#include <Utility/strings.h>
#include <Logging/logdata.h>
#include <Asset/Asset/filehash.h>
#include <Main/engine.h>

#include <tinyxml.h>

uint32_t LevelXMLParser::Load(const std::string& path) {
    Clear();
    TiXmlDocument doc(path.c_str());
    doc.LoadFile();

    if (!doc.Error()) {
        TiXmlElement* eType = doc.FirstChildElement("Type");
        const char* c_type = "";
        if (eType) {
            c_type = eType->GetText();
        }

        if (strmtch(c_type, "level_info_cache")) {
            TiXmlElement* eVersion = doc.FirstChildElement("Version");
            if (eVersion) {
                const char* c_version = eVersion->GetText();
                if (strmtch(c_version, "1") == false) {
                    LOGI << "level info cache file " << path << " is out of date" << std::endl;
                    return false;
                }
            }

            TiXmlElement* eLevelHash = doc.FirstChildElement("LevelHash");
            if (eLevelHash) {
                const char* c_level_hash = eLevelHash->GetText();
                if (c_level_hash) {
                    hash = c_level_hash;
                }
            }
        } else {
            FileHashAssetRef level_hash = Engine::Instance()->GetAssetManager()->LoadSync<FileHashAsset>(path);
            if (level_hash.valid()) {
                hash = level_hash->hash.ToString();
            }
        }

        TiXmlElement* eName = doc.FirstChildElement("Name");
        if (eName) {
            const char* c_name = eName->GetText();
            if (c_name) {
                name = c_name;
            } else {
                LOGE << "Element \"Name\" in " << path << " is null." << std::endl;
            }
        } else {
            LOGE << "Missing element \"Name\" in " << path << std::endl;
        }

        TiXmlElement* eDescription = doc.FirstChildElement("Description");
        if (eDescription) {
            const char* c_description = eDescription->GetText();
            if (c_description) {
                description = c_description;
            }
        }

        TiXmlElement* eShader = doc.FirstChildElement("Shader");
        if (eShader) {
            const char* c_shader = eShader->GetText();
            if (c_shader) {
                shader = c_shader;
            }
        }

        TiXmlElement* eScript = doc.FirstChildElement("Script");
        if (eScript) {
            const char* c_script = eScript->GetText();
            if (c_script) {
                script = c_script;
            }
        }

        eScript = doc.FirstChildElement("PCScript");
        if (eScript) {
            const char* c_script = eScript->GetText();
            if (c_script) {
                player_script = c_script;
            }
        }

        eScript = doc.FirstChildElement("NPCScript");
        if (eScript) {
            const char* c_script = eScript->GetText();
            if (c_script) {
                enemy_script = c_script;
            }
        }

        TiXmlElement* eLoadingScreen = doc.FirstChildElement("LoadingScreen");
        if (eLoadingScreen) {
            TiXmlElement* eImage = eLoadingScreen->FirstChildElement("Image");
            if (eImage) {
                const char* c_loading_screen_image = eImage->GetText();
                if (c_loading_screen_image) {
                    loading_screen.image = c_loading_screen_image;
                } else {
                    LOGI << "Image sub element contents for LoadingScreen is null." << std::endl;
                }
            } else {
                LOGW << "Missing Image sub element for LoadingScreen in level" << std::endl;
            }
        } else {
            LOGW << "Missing LoadingScreen element in level" << std::endl;
        }
    } else {
        LOGE << "Got a doc error " << doc.ErrorDesc() << std::endl;
        return false;
    }

    return true;
}

bool LevelXMLParser::Save(const std::string& path) {
    TiXmlDocument doc;
    TiXmlDeclaration* decl = new TiXmlDeclaration("2.0", "", "");
    doc.LinkEndChild(decl);

    TiXmlElement* e;
    TiXmlElement* e2;

    e = new TiXmlElement("Type");
    e->LinkEndChild(new TiXmlText("level_info_cache"));
    doc.LinkEndChild(e);

    e = new TiXmlElement("Version");
    e->LinkEndChild(new TiXmlText("1"));
    doc.LinkEndChild(e);

    e = new TiXmlElement("LevelHash");
    e->LinkEndChild(new TiXmlText(hash));
    doc.LinkEndChild(e);

    e = new TiXmlElement("Name");
    e->LinkEndChild(new TiXmlText(name));
    doc.LinkEndChild(e);

    e = new TiXmlElement("Description");
    e->LinkEndChild(new TiXmlText(description));
    doc.LinkEndChild(e);

    e = new TiXmlElement("Shader");
    e->LinkEndChild(new TiXmlText(shader));
    doc.LinkEndChild(e);

    e = new TiXmlElement("Script");
    e->LinkEndChild(new TiXmlText(script));
    doc.LinkEndChild(e);

    e = new TiXmlElement("PCScript");
    e->LinkEndChild(new TiXmlText(player_script));
    doc.LinkEndChild(e);

    e = new TiXmlElement("NPCScript");
    e->LinkEndChild(new TiXmlText(enemy_script));
    doc.LinkEndChild(e);

    e = new TiXmlElement("LoadingScreen");
    e2 = new TiXmlElement("Image");
    e2->LinkEndChild(new TiXmlText(loading_screen.image));
    e->LinkEndChild(e2);
    doc.LinkEndChild(e);

    CreateParentDirs(path);
    doc.SaveFile(path.c_str());
    return true;
}

void LevelXMLParser::Clear() {
    name.clear();
    description.clear();
    shader.clear();
    script.clear();
    player_script.clear();
    enemy_script.clear();
}
