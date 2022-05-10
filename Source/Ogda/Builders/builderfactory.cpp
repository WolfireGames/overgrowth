//-----------------------------------------------------------------------------
//           Name: builderfactory.cpp
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
#include "builderfactory.h"
#include "builder.h"

#include "copyaction.h"
#include "voidaction.h"
#include "dxt5action.h"
#include "crunchaction.h"

BuilderFactory::BuilderFactory() {
    actions.push_back(new ActionFactory<CopyAction>());
    actions.push_back(new ActionFactory<DXT5Action>());
    actions.push_back(new ActionFactory<CrunchAction>());
    actions.push_back(new ActionFactory<VoidAction>());
}

BuilderFactory::~BuilderFactory() {
    std::vector<ActionFactoryBase*>::iterator factoryit;

    for (factoryit = actions.begin(); factoryit != actions.end(); factoryit++) {
        delete *factoryit;
    }

    actions.clear();
}

bool BuilderFactory::HasBuilder(const std::string& builder) {
    std::vector<ActionFactoryBase*>::iterator factoryit;
    for (factoryit = actions.begin(); factoryit != actions.end(); factoryit++) {
        if ((*factoryit)->GetActionName() == builder) {
            return true;
        }
    }
    return false;
}

Builder BuilderFactory::CreateBuilder(const std::string& builder, const std::string& ending, const std::string& type_pattern_re) {
    std::vector<ActionFactoryBase*>::iterator factoryit;
    for (factoryit = actions.begin(); factoryit != actions.end(); factoryit++) {
        if ((*factoryit)->GetActionName() == builder) {
            return Builder((*factoryit)->NewInstance(), ending, type_pattern_re);
        }
    }
    LOGE << "Unable to find builder matching name " << builder << std::endl;
    return Builder(new VoidAction(), ending, type_pattern_re);
}

bool BuilderFactory::StoreResultInDatabase(std::string builder) {
    std::vector<ActionFactoryBase*>::iterator factoryit;
    for (factoryit = actions.begin(); factoryit != actions.end(); factoryit++) {
        if ((*factoryit)->GetActionName() == builder) {
            return (*factoryit)->StoreResultInDatabase();
        }
    }
    LOGW << "Couldn't find builder with name " << builder << std::endl;
    return false;
}
