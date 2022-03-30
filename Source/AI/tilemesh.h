//-----------------------------------------------------------------------------
//           Name: tilemesh.h
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
#pragma once

#include <Math/vec3.h>
#include <AI/navmeshparameters.h>
#include <AI/sample.h>
#include <AI/chunky_tri_mesh.h>

#include <Recast.h>
#include <DetourNavMesh.h>
#include <DetourCrowd.h>

using std::pair;

class TileMesh
{
protected:
	bool m_keepInterResults;
	bool m_buildAll;
	float m_totalBuildTimeMs;

	unsigned char* m_triareas;
	rcHeightfield* m_solid;
	rcCompactHeightfield* m_chf;
	rcContourSet* m_cset;
	rcPolyMesh* m_pmesh;
	rcPolyMeshDetail* m_dmesh;
	rcConfig m_cfg;	

	BuildContext* m_ctx;

	class InputGeom* m_geom;
	class dtNavMesh* m_navMesh;
	class dtNavMeshQuery* m_navQuery;
	class dtCrowd* m_crowd;

	unsigned char m_navMeshDrawFlags;

    float m_cellSize;
	float m_cellHeight;
	float m_agentHeight;
	float m_agentRadius;
	float m_agentMaxClimb;
	float m_agentMaxSlope;
	float m_regionMinSize;
	float m_regionMergeSize;
	bool m_monotonePartitioning;
	float m_edgeMaxLen;
	float m_edgeMaxError;
	float m_vertsPerPoly;
	float m_detailSampleDist;
	float m_detailSampleMaxError;
	
	int m_maxTiles;
	int m_maxPolysPerTile;
	float m_tileSize;
	
	unsigned int m_tileCol;
	float m_tileBmin[3];
	float m_tileBmax[3];
	float m_tileBuildTime;
	float m_tileMemUsage;
	int m_tileTriCount;

    bool m_use_explicit_bounderies;
    vec3 m_Bmin;
    vec3 m_Bmax;

	unsigned char* buildTileMesh(const int tx, const int ty, const float* bmin, const float* bmax, int& dataSize);
	
	void cleanup();
	
	void saveAll(const char* path, const dtNavMesh* mesh);
	dtNavMesh* loadAll(const char* path);

    dtNavMesh* loadAllMem(const char* data, size_t size);
public:
	TileMesh();
	virtual ~TileMesh();
	
	bool handleBuild();
	
	void getTilePos(const float* pos, int& tx, int& ty);
	
	void buildTile(const float* pos);
	void removeTile(const float* pos);
	void buildAllTiles();
	void removeAllTiles();
    void Save(const char *path);
    void Load(const char *path);
    void LoadMem(const char* data, size_t size);

	const rcPolyMesh* getPolyMesh() const;
	class dtNavMeshQuery* getNavMeshQuery() { return m_navQuery; }

	void resetCommonSettings();
    void applySettings(NavMeshParameters& nmp);
	void setContext(BuildContext* ctx) { m_ctx = ctx; }

	void handleMeshChanged(class InputGeom* geom);

    void SetExplicitBounderies(vec3 min, vec3 max );
    void RemoveExplicitBounderies();
    pair<vec3,vec3> GetBoundaries();

	const dtNavMesh* getNavMesh() const;
    void handleSettings();
    void handleCommonSettings();
};
