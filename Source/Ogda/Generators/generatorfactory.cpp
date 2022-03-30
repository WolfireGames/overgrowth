//-----------------------------------------------------------------------------
//           Name: generatorfactory.cpp
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
#include "generatorfactory.h"
#include "generator.h"

#include "versionxmlcreator.h"
#include "shortversioncreator.h"
#include "voidcreator.h"
#include "levellistcreator.h"

#include <Logging/logdata.h>

GeneratorFactory::GeneratorFactory()
{
    creators.push_back( new CreatorFactory<VersionXMLCreator>() );
    creators.push_back( new CreatorFactory<ShortVersionCreator>() );
    creators.push_back( new CreatorFactory<LevelListCreator>() );
}

GeneratorFactory::~GeneratorFactory()
{
    std::vector<CreatorFactoryBase*>::iterator factoryit;

    for( factoryit = creators.begin(); factoryit != creators.end(); factoryit++ )
    {
        delete *factoryit;
    }

    creators.clear();
}

bool GeneratorFactory::HasGenerator( const std::string& generator )
{
    std::vector<CreatorFactoryBase*>::iterator factoryit;
    for( factoryit = creators.begin(); factoryit != creators.end(); factoryit++ )
    {
        if( (*factoryit)->GetCreatorName() == generator )
        {
            return true;
        }
    }
    return false;
}

Generator GeneratorFactory::CreateGenerator( const std::string& generator )
{
    std::vector<CreatorFactoryBase*>::iterator factoryit;
    for( factoryit = creators.begin(); factoryit != creators.end(); factoryit++ )
    {
        if( (*factoryit)->GetCreatorName() == generator )
        {
            return Generator((*factoryit)->NewInstance());
        }
    }
    LOGE << "Unable to find generator matching name " << generator << std::endl;
    return Generator( new VoidCreator() );
}
