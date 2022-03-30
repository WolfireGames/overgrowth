//-----------------------------------------------------------------------------
//           Name: navmeshparameters.cpp
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
#include "navmeshparameters.h"

#include <Utility/assert.h>

#include <tinyxml.h>

#include <string>

using std::string;

NavMeshParameters::NavMeshParameters() :
generate(true),
m_cellSize(0.3f),
m_cellHeight(0.2f),
m_agentHeight(1.7f),
m_agentRadius(0.4f),
m_agentMaxClimb(1.5f),
m_agentMaxSlope(60.0f) {

}

void WriteNavMeshParametersToXML(const NavMeshParameters& nmp, TiXmlElement* elem) {
    LOG_ASSERT(elem);
    //Make sure our assumptions about how many elements there are stays true for future changes.
    // Padding makes the struct 7*4 bytes big rather than 6*4 + 1
    LOG_ASSERT(sizeof(float)*7 == sizeof(NavMeshParameters));

    elem->SetAttribute("generate",              nmp.generate ? "true" : "false");
    elem->SetDoubleAttribute("cell_size",       nmp.m_cellSize);
    elem->SetDoubleAttribute("cell_height",     nmp.m_cellHeight); 
    elem->SetDoubleAttribute("agent_height",    nmp.m_agentHeight); 
    elem->SetDoubleAttribute("agent_radius",    nmp.m_agentRadius); 
    elem->SetDoubleAttribute("agent_max_climb", nmp.m_agentMaxClimb); 
    elem->SetDoubleAttribute("agent_max_slope", nmp.m_agentMaxSlope); 
}

void ReadNavMeshParametersFromXML(NavMeshParameters& nmp, const TiXmlElement* elem) {
    LOG_ASSERT(elem);
    //Make sure our assumptions about how many elements there are stays true for future changes.
    // Padding makes the struct 7*4 bytes big rather than 6*4 + 1
    LOG_ASSERT(sizeof(float)*7 == sizeof(NavMeshParameters));

    string generate_value;
    elem->QueryStringAttribute("generate",       &generate_value);
    if(generate_value == "false") {
        nmp.generate = false;
    } else {
        nmp.generate = true;
    }
    elem->QueryFloatAttribute("cell_size",       &nmp.m_cellSize);
    elem->QueryFloatAttribute("cell_height",     &nmp.m_cellHeight); 
    elem->QueryFloatAttribute("agent_height",    &nmp.m_agentHeight); 
    elem->QueryFloatAttribute("agent_radius",    &nmp.m_agentRadius); 
    elem->QueryFloatAttribute("agent_max_climb", &nmp.m_agentMaxClimb); 
    elem->QueryFloatAttribute("agent_max_slope", &nmp.m_agentMaxSlope); 
}

//adler32 hash
uint32_t HashNavMeshParameters(NavMeshParameters& nmp) {
    LOG_ASSERT(sizeof(float)==sizeof(uint8_t)*4);
    // Padding makes the struct 7*4 bytes big rather than 6*4 + 1
    LOG_ASSERT(sizeof(float)*7 == sizeof(NavMeshParameters));

    // "generate" intentionally not included to not force navmesh regen

    uint32_t s1 = 1;
    uint32_t s2 = 0;

    s1 = (s1 + ((uint8_t*)&nmp.m_cellSize)[0]) % 65521;
    s2 = (s2 + s1) % 65521;
    s1 = (s1 + ((uint8_t*)&nmp.m_cellSize)[1]) % 65521;
    s2 = (s2 + s1) % 65521;
    s1 = (s1 + ((uint8_t*)&nmp.m_cellSize)[2]) % 65521;
    s2 = (s2 + s1) % 65521;
    s1 = (s1 + ((uint8_t*)&nmp.m_cellSize)[3]) % 65521;
    s2 = (s2 + s1) % 65521;

    s1 = (s1 + ((uint8_t*)&nmp.m_cellHeight)[0]) % 65521;
    s2 = (s2 + s1) % 65521;
    s1 = (s1 + ((uint8_t*)&nmp.m_cellHeight)[1]) % 65521;
    s2 = (s2 + s1) % 65521;
    s1 = (s1 + ((uint8_t*)&nmp.m_cellHeight)[2]) % 65521;
    s2 = (s2 + s1) % 65521;
    s1 = (s1 + ((uint8_t*)&nmp.m_cellHeight)[3]) % 65521;
    s2 = (s2 + s1) % 65521;

    s1 = (s1 + ((uint8_t*)&nmp.m_agentHeight)[0]) % 65521;
    s2 = (s2 + s1) % 65521;
    s1 = (s1 + ((uint8_t*)&nmp.m_agentHeight)[1]) % 65521;
    s2 = (s2 + s1) % 65521;
    s1 = (s1 + ((uint8_t*)&nmp.m_agentHeight)[2]) % 65521;
    s2 = (s2 + s1) % 65521;
    s1 = (s1 + ((uint8_t*)&nmp.m_agentHeight)[3]) % 65521;
    s2 = (s2 + s1) % 65521;

    s1 = (s1 + ((uint8_t*)&nmp.m_agentRadius)[0]) % 65521;
    s2 = (s2 + s1) % 65521;
    s1 = (s1 + ((uint8_t*)&nmp.m_agentRadius)[1]) % 65521;
    s2 = (s2 + s1) % 65521;
    s1 = (s1 + ((uint8_t*)&nmp.m_agentRadius)[2]) % 65521;
    s2 = (s2 + s1) % 65521;
    s1 = (s1 + ((uint8_t*)&nmp.m_agentRadius)[3]) % 65521;
    s2 = (s2 + s1) % 65521;

    s1 = (s1 + ((uint8_t*)&nmp.m_agentMaxClimb)[0]) % 65521;
    s2 = (s2 + s1) % 65521;
    s1 = (s1 + ((uint8_t*)&nmp.m_agentMaxClimb)[1]) % 65521;
    s2 = (s2 + s1) % 65521;
    s1 = (s1 + ((uint8_t*)&nmp.m_agentMaxClimb)[2]) % 65521;
    s2 = (s2 + s1) % 65521;
    s1 = (s1 + ((uint8_t*)&nmp.m_agentMaxClimb)[3]) % 65521;
    s2 = (s2 + s1) % 65521;

    s1 = (s1 + ((uint8_t*)&nmp.m_agentMaxSlope)[0]) % 65521;
    s2 = (s2 + s1) % 65521;
    s1 = (s1 + ((uint8_t*)&nmp.m_agentMaxSlope)[1]) % 65521;
    s2 = (s2 + s1) % 65521;
    s1 = (s1 + ((uint8_t*)&nmp.m_agentMaxSlope)[2]) % 65521;
    s2 = (s2 + s1) % 65521;
    s1 = (s1 + ((uint8_t*)&nmp.m_agentMaxSlope)[3]) % 65521;
    s2 = (s2 + s1) % 65521;
 
    return (s2 << 16) | s1;
}

