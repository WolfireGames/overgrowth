//-----------------------------------------------------------------------------
//           Name: debuglevelload.h
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
#pragma once

#include <Internal/config.h>

#include <string>

extern Config config;

std::string DebugLoadLevel() {
    //return "ShaleGuard2.xml";
    //return "TheCave.xml";
    //return "DesertFort_IGF.xml";
    //return "TerrainSculpt2_IGF.xml";
    //return "Project60/10_eroded_plateau.xml";
    //return "Project60/5_painted_desert.xml";
    //return "Project60/5_painted_desert_night.xml";
    //return "Project60/16_red_desert.xml";
    //return "Project60/16_red_desert_platforming.xml";
    //return "Project60/16_red_desert_bridge.xml";
    //return "Project60/16_red_desert_super_empty.xml";
    //return "LugaruStory/Village.xml";
    //return "Proto/arrival.xml";
    //return "Project60/stealth_1.xml";
    //return "ogLevels/up_high.xml";
    //return "ogLevels/pillars.xml";
    //return "terrain/dead_hills_terrain.xml";
    //return "Project60/16_red_desert_super_empty_script.xml";
    //return "Project60/16_red_desert_challenge1.xml";
    //return "Project60/16_red_desert_challenge2.xml;"
    //return "Project60/16_red_desert_challenge3.xml";
    //return "Project60/16_red_desert_challenge4.xml";
    //return "Project60/16_red_desert_challenge5.xml";
    //return "Project60/16_red_desert_challenge6.xml";
    //return "Project60/16_red_desert_challenge7.xml";
    //return "Project60/16_red_desert_challenge8.xml";
    //return "Project60/16_red_desert_challenge9.xml";
    //return "Project60/16_red_desert_challenge10.xml";
    //return "Project60/16_red_desert_challenge11.xml";
    //return "Project60/16_red_desert_challenge12.xml";
    //return "Project60/16_red_desert_challenge13.xml";
    //return "Project60/16_red_desert_challenge14.xml";
    //return "lugaru_snow.xml";
    //return "Project60/1_dead_hills.xml";
    //return "Project60/11_forest_creek.xml";
    //return "Project60/24_patchy_highlands.xml";
    //return "forest_hills.xml";
    //return "PlainsHills_IGF.xml";
    //return "arenas/construction_temple_arena.xml";
    //return "arenas/stucco_courtyard_arena.xml";
    //return "arenas/stucco_courtyard_arena_test.xml";
    
    return config["debug_load_level"].str();

    //return "nothing.xml";
    //return "map.xml";
    
    //return "Project60/2_impressive_mountains_4player.xml";
    //return "Project60/2_impressive_mountains.xml";
    //return "Project60/8_dead_volcano.xml";
    //return "Project60/14_gentle_slopes.xml";
    //return "Project60/22_grass_beach.xml";
    //return "Project60/4_red_hills.xml";
    //return "sea_cliffs.xml";
    //return "Challenge/tree_canyon_summer.xml";
    //return "challenge/scrubby_hills.xml";
    //return "challenge/scrubby_hills_fall.xml";
    //return "challenge/shielded_stands.xml";
}
