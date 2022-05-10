//-----------------------------------------------------------------------------
//           Name: objectseeker.cpp
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
#include "objectseeker.h"

#include <tinyxml.h>

#include <Logging/logdata.h>
#include <Utility/strings.h>

std::vector<Item> ObjectSeeker::SearchXML(const Item& item, TiXmlDocument& doc) {
    const char* elems[] =
        {
            "Model",
            "ColorMap",
            "NormalMap",
            "PaletteMap",
            "WeightMap",
            "TranslucencyMap",
            "WindMap",
            "MaterialPath",
            "SharpnessMap"};

    const char* elems_type[] =
        {
            "model",
            "texture",
            "texture",
            "texture",
            "texture",
            "texture",
            "texture",
            "material",
            "texture"};

    assert(ARRLEN(elems_type) == ARRLEN(elems));

    const char* ignored[] =
        {
            "Shader",
            "ShaderName",
            "ShaderPath",
            "flags",
            "GroundOffset",
            "avg_color",
            "label"};

    std::vector<Item> items;

    TiXmlHandle hRoot(&doc);

    TiXmlElement* eElem = hRoot.FirstChildElement("Object").FirstChildElement().Element();

    if (!eElem) {
        LOGE << "Cant find right root node in " << item << std::endl;
    }

    while (eElem) {
        const char* name = eElem->Value();
        const char* text = eElem->GetText();
        if (name) {
            int id;
            if ((id = FindStringInArray(elems, ARRLEN(elems), name)) >= 0) {
                if (text && strlen(text) > 0) {
                    items.push_back(Item(item.input_folder, text, elems_type[id], item.source));
                } else {
                    LOGW << "String value in " << item << " for element " << elems[id] << " is empty" << std::endl;
                }
            } else if ((id = FindStringInArray(ignored, ARRLEN(ignored), name)) >= 0) {
                LOGD << "Ignored " << ignored[id] << " in " << item << std::endl;
            } else if (strmtch("DetailObjects", name)) {
                TiXmlElement* eDetailObject = eElem->FirstChildElement();

                while (eDetailObject) {
                    {
                        const char* obj_path = eDetailObject->Attribute("obj_path");
                        if (obj_path) {
                            items.push_back(Item(item.input_folder, obj_path, "object", item.source));
                        }
                    }

                    {
                        const char* weight_path = eDetailObject->Attribute("weight_path");
                        if (weight_path) {
                            items.push_back(Item(item.input_folder, weight_path, "texture", item.source));
                        }
                    }

                    eDetailObject = eDetailObject->NextSiblingElement();
                }
            } else if (strmtch("DetailMaps", name)) {
                TiXmlElement* eDetailMap = eElem->FirstChildElement();

                while (eDetailMap) {
                    {
                        const char* colorpath = eDetailMap->Attribute("colorpath");
                        if (colorpath) {
                            items.push_back(Item(item.input_folder, colorpath, "texture", item.source));
                        }
                    }

                    {
                        const char* normalpath = eDetailMap->Attribute("normalpath");
                        if (normalpath) {
                            items.push_back(Item(item.input_folder, normalpath, "texture", item.source));
                        }
                    }

                    {
                        const char* materialpath = eDetailMap->Attribute("materialpath");
                        if (materialpath) {
                            items.push_back(Item(item.input_folder, materialpath, "material", item.source));
                        }
                    }

                    eDetailMap = eDetailMap->NextSiblingElement();
                }
            } else {
                LOGE << "Unahandled subvalue in Object from " << item << " called " << name << std::endl;
            }
        } else {
            LOGE << "Generic warning" << std::endl;
        }

        eElem = eElem->NextSiblingElement();
    }

    return items;
}
