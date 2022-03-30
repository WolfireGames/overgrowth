//-----------------------------------------------------------------------------
//           Name: searcherfactory.cpp
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
#include <vector>

#include "searcherfactory.h"

#include "searcher.h"

#include "Seekers/voidseeker.h"
#include "Seekers/actorobjectlevelseeker.h"
#include "Seekers/decalseeker.h"
#include "Seekers/terrainlevelseeker.h"
#include "Seekers/skylevelseeker.h"
#include "Seekers/ambientsoundlevelseeker.h"
#include "Seekers/hotspotseeker.h"
#include "Seekers/objectseeker.h"
#include "Seekers/syncedanimationgroupseeker.h"
#include "Seekers/attackseeker.h"
#include "Seekers/characterseeker.h"
#include "Seekers/actorseeker.h"
#include "Seekers/skeletonseeker.h"
#include "Seekers/animationretargetseeker.h"
#include "Seekers/itemseeker.h"
#include "Seekers/materialseeker.h"
#include "Seekers/particleseeker.h"
#include "Seekers/prefabseeker.h"
#include "Seekers/spawnerlistseeker.h"
#include "Seekers/preconvertedddsshadowseeker.h"
#include "Seekers/objcolseeker.h"
#include "Seekers/objhullseeker.h"
#include "Seekers/levelnormseeker.h"

SearcherFactory::SearcherFactory()
{
    seekers.push_back( new SeekerFactory<VoidSeeker>() );
    seekers.push_back( new SeekerFactory<ActorObjectLevelSeeker>() );
    seekers.push_back( new SeekerFactory<TerrainLevelSeeker>() );
    seekers.push_back( new SeekerFactory<DecalSeeker>() );
    seekers.push_back( new SeekerFactory<SkyLevelSeeker>() );
    seekers.push_back( new SeekerFactory<AmbientSoundLevelSeeker>() );
    seekers.push_back( new SeekerFactory<HotspotSeeker>() );
    seekers.push_back( new SeekerFactory<ObjectSeeker>() );
    seekers.push_back( new SeekerFactory<SyncedAnimationGroupSeeker>() );
    seekers.push_back( new SeekerFactory<AttackSeeker>() );
    seekers.push_back( new SeekerFactory<CharacterSeeker>() );
    seekers.push_back( new SeekerFactory<ActorSeeker>() );
    seekers.push_back( new SeekerFactory<SkeletonSeeker>() );
    seekers.push_back( new SeekerFactory<AnimationRetargetSeeker>() );
    seekers.push_back( new SeekerFactory<ItemSeeker>() );
    seekers.push_back( new SeekerFactory<MaterialSeeker>() );
    seekers.push_back( new SeekerFactory<ParticleSeeker>() );
    seekers.push_back( new SeekerFactory<PrefabSeeker>() );
    seekers.push_back( new SeekerFactory<SpawnerListSeeker>() );
    seekers.push_back( new SeekerFactory<PreConvertedDDSSeeker>() );
    seekers.push_back( new SeekerFactory<ObjColSeeker>() );
    seekers.push_back( new SeekerFactory<ObjHullSeeker>() );
    seekers.push_back( new SeekerFactory<LevelNormSeeker>() );
}

SearcherFactory::~SearcherFactory()
{
    std::vector<SeekerFactoryBase*>::iterator factoryit;

    for( factoryit = seekers.begin(); factoryit != seekers.end(); factoryit++ )
    {
        delete *factoryit;
    }

    seekers.clear();
}

bool SearcherFactory::HasSearcher( const std::string& searcher )
{
    std::vector<SeekerFactoryBase*>::iterator factoryit;
    for( factoryit = seekers.begin(); factoryit != seekers.end(); factoryit++ )
    {
        if( (*factoryit)->GetSeekerName() == searcher )
        {
            return true;
        }
    }
    return false;
}

Searcher SearcherFactory::CreateSearcher( const std::string& searcher, const std::string& path_ending, const std::string& type_pattern_re )
{
    std::vector<SeekerFactoryBase*>::iterator factoryit;
    for( factoryit = seekers.begin(); factoryit != seekers.end(); factoryit++ )
    {
        if( (*factoryit)->GetSeekerName() == searcher )
        {
            return Searcher((*factoryit)->NewInstance(), path_ending, type_pattern_re);
        }
    }

    LOGE << "Unable to find searcher matching name " << searcher << std::endl;

    return Searcher( new VoidSeeker(), path_ending, type_pattern_re );
}
