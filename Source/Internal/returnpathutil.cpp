//-----------------------------------------------------------------------------
//           Name: returnpathutil.cpp
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
#include "returnpathutil.h"

#include <Asset/Asset/ambientsounds.h>
#include <Asset/Asset/soundgroup.h>
#include <Asset/Asset/syncedanimation.h>
#include <Asset/Asset/attacks.h>
#include <Asset/Asset/character.h>
#include <Asset/Asset/item.h>
#include <Asset/Asset/material.h>
#include <Asset/Asset/reactions.h>
#include <Asset/Asset/actorfile.h>
#include <Asset/Asset/decalfile.h>
#include <Asset/Asset/objectfile.h>
#include <Asset/Asset/hotspotfile.h>

#include <Main/engine.h>
#include <XML/xml_helper.h>

#include <tinyxml.h>

namespace {
std::string GetTypeFromXML(const std::string &path) {
    TiXmlDocument doc;
    LoadXMLRetryable(doc, path, "Unknown");

    if (doc.Error()) {
        LOGE << "Problem loading xml document " << path << " error: " << doc.ErrorDesc() << std::endl;
        return "UNKNOWN";
    }

    TiXmlHandle h_doc(&doc);

    TiXmlElement *root = h_doc.FirstChildElement().ToElement();
    while (root) {
        std::string type = root->Value();
        if (type == "Type") {
            return type;
        }
        root = root->NextSiblingElement();
    }

    root = h_doc.FirstChildElement().ToElement();
    while (root) {
        std::string type = root->Value();

        switch (type[0]) {
            case 'a':
                if (type == "attack") {
                    return "attack";
                }
                break;
            case 'A':
                if (type == "Actor") {
                    return "actor";
                }
                break;
            case 'c':
                if (type == "character") {
                    return "character";
                }
                break;
            case 'D':
                if (type == "DecalObject") {
                    return "decal";
                }
                break;
            case 'H':
                if (type == "Hotspot") {
                    return "hotspot";
                }
                break;
            case 'i':
                if (type == "item") {
                    return "item";
                }
                break;
            case 'L':
                if (type == "Level") {
                    return "level";
                }
                break;
            case 'm':
                if (type == "material") {
                    return "material";
                }
                break;
            case 'O':
                if (type == "Object") {
                    return "envobject";
                }
                break;
            case 'p':
                if (type == "particle") {
                    return "particle";
                }
                break;
            case 'r':
                switch (type[1]) {
                    case 'e':
                        if (type == "reaction") {
                            return "reaction";
                        }
                        break;
                    case 'i':
                        if (type == "rig") {
                            return "rig";
                        }
                        break;
                }
                break;
            case 's':
                switch (type[1]) {
                    case 'a':
                        if (type == "saved") {
                            return "saved";
                        }
                        break;
                    case 'o':
                        switch (type[2]) {
                            case 'n':
                                if (type == "songlist") {
                                    return "songlist";
                                }
                                break;
                            case 'u':
                                if (type == "soundgroup") {
                                    return "soundgroup";
                                }
                                break;
                        }
                        break;
                }
                break;
            case 'S':
                switch (type[1]) {
                    case 'o':
                        if (type == "Sound") {
                            return "ambientsound";
                        }
                        break;
                    case 'y':
                        if (type == "SyncedAnimationGroup") {
                            return "synced_animation";
                        }
                        break;
                }
                break;
        }
        root = root->NextSiblingElement();
    }

    LOGE << "Unable to find a type string for " << path << std::endl;
    return "UNKONWN";
}
}  // namespace

std::string ReturnPathUtil::GetTypeFromFilePath(const std::string &path) {
    std::string::size_type final_dot = path.rfind('.');
    if (final_dot == std::string::npos) {
        FatalError("Error", "File has no extension: %s", path.c_str());
    }
    ++final_dot;
    std::string suffix = path.substr(final_dot, path.length() - final_dot);
    switch (suffix[0]) {
        case 'a':
            switch (suffix[1]) {
                case 'n':
                    if (suffix == "anm") {
                        return "animation";
                    }
                    break;
                case 's':
                    if (suffix == "as") {
                        return "script";
                    }
                    break;
            }
            break;
        case 'd':
            if (suffix == "dds") {
                return "texture";
            }
            break;
        case 'h':
            if (suffix == "html") {
                return "webpage";
            }
            break;
        case 'o':
            switch (suffix[1]) {
                case 'b':
                    if (suffix == "obj") {
                        return "model";
                    }
                    break;
                case 's':
                    if (suffix == "ogg") {
                        return "song";
                    }
                    break;
            }
            break;
        case 'p':
            switch (suffix[1]) {
                case 'h':
                    if (suffix == "png") {
                        return "texture";
                    }
                    break;
                case 'n':
                    if (suffix == "phxbn") {
                        return "skeleton";
                    }
                    break;
            }
            break;
        case 't':
            switch (suffix[1]) {
                case 'g':
                    if (suffix == "tga") {
                        return "texture";
                    }
                    break;
                case 't':
                    if (suffix == "ttf") {
                        return "font";
                    }
                    break;
            }
            break;
        case 'w':
            if (suffix == "wav") {
                return "sound";
            }
            break;
        case 'x':
            if (suffix == "xml") {
                return GetTypeFromXML(path);
            }
            break;
    }
    FatalError("Error", "File suffix unknown: %s", suffix.c_str());
    return "";
}

#define LOAD_SYNC_RETURN_PATHS(string_type, type)                                       \
    if (path == string_type) {                                                          \
        AssetRef<type> a = Engine::Instance()->GetAssetManager()->LoadSync<type>(path); \
        if (a.valid()) {                                                                \
            a->ReturnPaths(path_set);                                                   \
        } else {                                                                        \
            LOGE << "Failed retrieving paths from " #type << path << std::endl;         \
        }                                                                               \
    }

void ReturnPathUtil::ReturnPathsFromPath(const std::string &path, PathSet &path_set) {
    std::string type = GetTypeFromFilePath(path);
    if (path_set.find(type + " " + path) != path_set.end()) {
        return;
    }
    path_set.insert(type + " " + path);
    switch (type[0]) {
        case 'a':
            switch (type[1]) {
                case 'c':
                    LOAD_SYNC_RETURN_PATHS("actor", ActorFile);
                    break;
                case 'm':
                    LOAD_SYNC_RETURN_PATHS("ambientsound", AmbientSound);
                    break;
                case 't':
                    LOAD_SYNC_RETURN_PATHS("attack", Attack);
                    break;
                case 'n':
                    LOAD_SYNC_RETURN_PATHS("animation", Animation);
                    break;
                default:
                    LOGE << "Found no match for the string " << type << std::endl;
                    break;
            }
            break;
        case 'c':
            LOAD_SYNC_RETURN_PATHS("character", Character);
            break;
        case 'd':
            LOAD_SYNC_RETURN_PATHS("decal", DecalFile);
            break;
        case 'e':
            LOAD_SYNC_RETURN_PATHS("envobject", ObjectFile);
            break;
        case 'h':
            LOAD_SYNC_RETURN_PATHS("hotspot", HotspotFile);
            break;
        case 'i':
            LOAD_SYNC_RETURN_PATHS("item", Item);
            break;
        case 'm':
            LOAD_SYNC_RETURN_PATHS("material", Material);
            break;
        case 'p':
            if (type == "particle") {
            }
            break;  // TODO: Implement this
        case 'r':
            switch (type[1]) {
                case 'e':
                    LOAD_SYNC_RETURN_PATHS("reaction", Reaction);
                    break;
                case 'i':
                    if (type == "rig") {
                    }
                    break;  // TODO: Implement this
                default:
                    LOGE << "Found no match for the string " << type << std::endl;
                    break;
            }
            break;
        case 's':
            switch (type[1]) {
                case 'a':
                    if (type == "saved") {
                    }
                    break;
                case 'o':
                    switch (type[2]) {
                        case 'n':
                            if (type == "songlist") {
                            }
                            break;  // TODO: Implement this
                        case 'u':
                            LOAD_SYNC_RETURN_PATHS("soundgroup", SoundGroupInfo);
                            break;
                        default:
                            LOGE << "Found no match for the string " << type << std::endl;
                            break;
                    }
                    break;
                case 'y':
                    LOAD_SYNC_RETURN_PATHS("synced_animation", SyncedAnimationGroup);
                    break;
                default:
                    LOGE << "Found no match for the string " << type << std::endl;
                    break;
            }
            break;
        case 't':
            if (type == "texture") {
            }
            break;
        default:
            LOGE << "Found no match for the string " << type << std::endl;
            break;
    }
}
