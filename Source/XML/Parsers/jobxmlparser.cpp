//-----------------------------------------------------------------------------
//           Name: jobxmlparser.cpp
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
#include "jobxmlparser.h"

#include <Utility/commonregex.h>
#include <Logging/logdata.h>
#include <XML/xml_helper.h>

#include <tinyxml.h>

JobXMLParser::JobXMLParser() {
}

uint32_t JobXMLParser::Load(const std::string& path) {
    CommonRegex cr;
    Clear();
    TiXmlDocument doc(path.c_str());
    doc.LoadFile();

    if (!doc.Error()) {
        TiXmlElement* pRoot = doc.RootElement();

        if (pRoot) {
            TiXmlHandle hRoot(pRoot);

            TiXmlElement* eInputs = hRoot.FirstChild("Inputs").Element();

            if (eInputs) {
                TiXmlNode* nInputsIt = NULL;

                while ((nInputsIt = eInputs->IterateChildren(nInputsIt))) {
                    if (strcmp(nInputsIt->Value(), "Input") == 0) {
                        TiXmlElement* eInput = nInputsIt->ToElement();

                        if (eInput) {
                            inputs.push_back(eInput->GetText());
                        }
                    } else {
                        if (nInputsIt->ToComment() == NULL)
                            LOGE << "Found invalid tag \"<" << nInputsIt->Value() << ">\" in <Inputs>" << std::endl;
                    }
                }
            }

            TiXmlElement* eSearcher = hRoot.FirstChild("Searchers").Element();

            if (eSearcher) {
                TiXmlNode* nSearcherIt = NULL;

                while ((nSearcherIt = eSearcher->IterateChildren(nSearcherIt))) {
                    if (strcmp(nSearcherIt->Value(), "Searcher") == 0) {
                        TiXmlElement* eSearcher = nSearcherIt->ToElement();

                        if (eSearcher) {
                            Searcher sSearcher;

                            const char* type_pattern_re = eSearcher->Attribute("type_pattern_re");
                            const char* path_ending = eSearcher->Attribute("path_ending");
                            const char* searcher = eSearcher->Attribute("searcher");

                            if (type_pattern_re)
                                sSearcher.type_pattern_re = std::string(type_pattern_re);
                            if (path_ending)
                                sSearcher.path_ending = std::string(path_ending);
                            if (searcher)
                                sSearcher.searcher = std::string(searcher);

                            sSearcher.row = eSearcher->Row();

                            searchers.push_back(sSearcher);
                        }
                    } else {
                        if (nSearcherIt->ToComment() == NULL)
                            LOGE << "Found invalid tag \"<" << nSearcherIt->Value() << ">\" in <Searchers>" << std::endl;
                    }
                }
            }

            TiXmlElement* eItems = hRoot.FirstChild("Items").Element();

            if (eItems) {
                TiXmlNode* nItemsIt = NULL;

                while ((nItemsIt = eItems->IterateChildren(nItemsIt))) {
                    if (strcmp(nItemsIt->Value(), "Item") == 0) {
                        TiXmlElement* eItem = nItemsIt->ToElement();

                        if (eItem) {
                            Item item;

                            const char* path = eItem->GetText();
                            if (path) {
                                item.path = std::string(path);
                            }

                            const char* type = eItem->Attribute("type");
                            if (type) {
                                item.type = std::string(type);
                            }

                            bool recursive = cr.saysTrue(eItem->Attribute("recursive"));
                            item.recursive = recursive;

                            const char* path_ending = eItem->Attribute("path_ending");
                            if (path_ending)
                                item.path_ending = std::string(path_ending);

                            item.row = eItem->Row();
                            items.push_back(item);
                        }
                    } else {
                        if (nItemsIt->ToComment() == NULL)
                            LOGE << "Found invalid tag \"<" << nItemsIt->Value() << ">\" in <Items>" << std::endl;
                    }
                }
            }

            TiXmlElement* eBuilders = hRoot.FirstChild("Builders").Element();

            if (eBuilders) {
                TiXmlNode* nBuilderIt = NULL;

                while ((nBuilderIt = eBuilders->IterateChildren(nBuilderIt))) {
                    if (strcmp(nBuilderIt->Value(), "Builder") == 0) {
                        TiXmlElement* eBuilder = nBuilderIt->ToElement();

                        if (eBuilder) {
                            Builder b;

                            const char* type_pattern_re = eBuilder->Attribute("type_pattern_re");
                            const char* path_ending = eBuilder->Attribute("path_ending");
                            const char* builder = eBuilder->Attribute("builder");

                            if (type_pattern_re)
                                b.type_pattern_re = std::string(type_pattern_re);
                            if (path_ending)
                                b.path_ending = std::string(path_ending);
                            if (builder)
                                b.builder = std::string(builder);

                            b.row = eBuilder->Row();

                            builders.push_back(b);
                        }
                    } else {
                        if (nBuilderIt->ToComment() == NULL)
                            LOGE << "Found invalid tag \"<" << nBuilderIt->Value() << ">\" in <Builders>" << std::endl;
                    }
                }
            }

            TiXmlElement* eGenerators = hRoot.FirstChild("Generators").Element();

            if (eGenerators) {
                TiXmlNode* nGeneratorIt = NULL;

                while ((nGeneratorIt = eGenerators->IterateChildren(nGeneratorIt))) {
                    if (strcmp(nGeneratorIt->Value(), "Generator") == 0) {
                        TiXmlElement* eGenerator = nGeneratorIt->ToElement();

                        if (eGenerator) {
                            Generator b;

                            const char* generator = eGenerator->Attribute("generator");

                            if (generator)
                                b.generator = std::string(generator);

                            b.row = eGenerator->Row();

                            generators.push_back(b);
                        }
                    } else {
                        if (nGeneratorIt->ToComment() == NULL)
                            LOGE << "Found invalid tag \"<" << nGeneratorIt->Value() << ">\" in <Generators>" << std::endl;
                    }
                }
            }

            return true;
        } else {
            LOGE << "Problem loading job file" << path << std::endl;
            return false;
        }
    } else {
        LOGE << "Error parsing job file: \"" << doc.ErrorDesc() << "\"" << std::endl;
        return false;
    }
}

bool JobXMLParser::Save(const std::string& path) {
    return false;
}

void JobXMLParser::Clear() {
    searchers.clear();
    items.clear();
    builders.clear();
}

bool JobXMLParser::Item::operator<(const Item& rhs) const {
    if (row < rhs.row) {
        return true;
    } else if (row == rhs.row) {
        if (path_ending < rhs.path_ending) {
            return true;
        } else if (path_ending == rhs.path_ending) {
            if (path < rhs.path) {
                return true;
            } else if (path == rhs.path) {
                if (type < rhs.type) {
                    return true;
                } else if (type == rhs.type) {
                    if (recursive < rhs.recursive) {
                        return true;
                    } else if (recursive == rhs.recursive) {
                        return false;
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }
            } else {
                return false;
            }
        } else {
            return false;
        }
    } else {
        return false;
    }
}
