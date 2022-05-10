//-----------------------------------------------------------------------------
//           Name: tilemesh.cpp
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
#define _USE_MATH_DEFINES

#include "tilemesh.h"

#include <AI/input_geom.h>
#include <Logging/logdata.h>
#include <Utility/assert.h>
#include <Compat/fileio.h>
#include <GUI/widgetframework.h>

#include <Recast.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <iomanip>
#include <iostream>

#ifdef WIN32
#define snprintf _snprintf
#endif

using std::endl;
using std::pair;
using std::string;

inline unsigned int nextPow2(unsigned int v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

inline unsigned int ilog2(unsigned int v) {
    unsigned int r;
    unsigned int shift;
    r = (v > 0xffff) << 4;
    v >>= r;
    shift = (v > 0xff) << 3;
    v >>= shift;
    r |= shift;
    shift = (v > 0xf) << 2;
    v >>= shift;
    r |= shift;
    shift = (v > 0x3) << 1;
    v >>= shift;
    r |= shift;
    r |= (v >> 1);
    return r;
}

TileMesh::TileMesh() : m_keepInterResults(false),
                       m_buildAll(true),
                       m_totalBuildTimeMs(0),
                       m_triareas(0),
                       m_solid(0),
                       m_chf(0),
                       m_cset(0),
                       m_pmesh(0),
                       m_dmesh(0),
                       m_maxTiles(0),
                       m_maxPolysPerTile(0),
                       m_tileSize(0),
                       m_tileBuildTime(0),
                       m_tileMemUsage(0),
                       m_tileTriCount(0),
                       // m_Bmin(-100,-1000,-100),
                       // m_Bmax(100,1000,100),
                       m_use_explicit_bounderies(false) {
    resetCommonSettings();
    m_navQuery = dtAllocNavMeshQuery();
    m_crowd = dtAllocCrowd();
    m_navMesh = dtAllocNavMesh();

    memset(m_tileBmin, 0, sizeof(m_tileBmin));
    memset(m_tileBmax, 0, sizeof(m_tileBmax));
}

TileMesh::~TileMesh() {
    dtFreeNavMeshQuery(m_navQuery);
    dtFreeNavMesh(m_navMesh);
    dtFreeCrowd(m_crowd);
    cleanup();
    m_navMesh = 0;
}

void TileMesh::resetCommonSettings() {
    // m_cellSize = 0.3f;
    // m_cellHeight = 0.2f;
    // m_agentHeight = 2.0f;
    // m_agentRadius = 0.6f;
    // m_agentMaxClimb = 0.9f;
    // m_agentMaxSlope = 45.0f;
    m_regionMinSize = 8;
    m_regionMergeSize = 20;
    m_monotonePartitioning = false;
    m_edgeMaxLen = 12.0f;
    m_edgeMaxError = 1.3f;
    m_vertsPerPoly = 6.0f;
    m_detailSampleDist = 6.0f;
    m_detailSampleMaxError = 1.0f;
    m_tileSize = 128;

    NavMeshParameters nmpd;
    applySettings(nmpd);
}

void TileMesh::applySettings(NavMeshParameters& nmp) {
    // Check if we are assuming right in how the struct is constructed internally
    //  Padding makes the struct 7*4 bytes big rather than 6*4 + 1
    LOG_ASSERT(sizeof(float) * 7 == sizeof(NavMeshParameters));

    m_cellSize = nmp.m_cellSize;
    m_cellHeight = nmp.m_cellHeight;
    m_agentHeight = nmp.m_agentHeight;
    m_agentRadius = nmp.m_agentRadius;
    m_agentMaxClimb = nmp.m_agentMaxClimb;
    m_agentMaxSlope = nmp.m_agentMaxSlope;
}

void TileMesh::cleanup() {
    delete[] m_triareas;
    m_triareas = 0;
    rcFreeHeightField(m_solid);
    m_solid = 0;
    rcFreeCompactHeightfield(m_chf);
    m_chf = 0;
    rcFreeContourSet(m_cset);
    m_cset = 0;
    rcFreePolyMesh(m_pmesh);
    m_pmesh = 0;
    rcFreePolyMeshDetail(m_dmesh);
    m_dmesh = 0;
}

static const int NAVMESHSET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T';  //'MSET';
static const int NAVMESHSET_VERSION = 1;

struct NavMeshSetHeader {
    int magic;
    int version;
    int numTiles;
    dtNavMeshParams params;
};

struct NavMeshTileHeader {
    dtTileRef tileRef;
    int dataSize;
};

void TileMesh::saveAll(const char* path, const dtNavMesh* mesh) {
    if (!mesh) return;

    FILE* fp = my_fopen(path, "wb");
    if (!fp)
        return;

    // Store header.
    NavMeshSetHeader header;
    header.magic = NAVMESHSET_MAGIC;
    header.version = NAVMESHSET_VERSION;
    header.numTiles = 0;
    for (int i = 0; i < mesh->getMaxTiles(); ++i) {
        const dtMeshTile* tile = mesh->getTile(i);
        if (!tile || !tile->header || !tile->dataSize) continue;
        header.numTiles++;
    }
    memcpy(&header.params, mesh->getParams(), sizeof(dtNavMeshParams));
    fwrite(&header, sizeof(NavMeshSetHeader), 1, fp);

    // Store tiles.
    for (int i = 0; i < mesh->getMaxTiles(); ++i) {
        const dtMeshTile* tile = mesh->getTile(i);
        if (!tile || !tile->header || !tile->dataSize) continue;

        NavMeshTileHeader tileHeader;
        tileHeader.tileRef = mesh->getTileRef(tile);
        tileHeader.dataSize = tile->dataSize;
        fwrite(&tileHeader, sizeof(tileHeader), 1, fp);

        fwrite(tile->data, tile->dataSize, 1, fp);
    }

    fclose(fp);
}

dtNavMesh* TileMesh::loadAllMem(const char* data, size_t size) {
    if (data && size > 0) {
        const char* pos = data;

        NavMeshSetHeader header;
        LOG_ASSERT(pos + sizeof(NavMeshSetHeader) <= data + size);
        memcpy(&header, pos, sizeof(NavMeshSetHeader));
        pos += sizeof(NavMeshSetHeader);

        if (header.magic != NAVMESHSET_MAGIC) {
            return 0;
        }
        if (header.version != NAVMESHSET_VERSION) {
            return 0;
        }

        dtNavMesh* mesh = dtAllocNavMesh();
        if (!mesh) {
            return 0;
        }
        dtStatus status = mesh->init(&header.params);
        if (dtStatusFailed(status)) {
            return 0;
        }

        // Read tiles.
        for (int i = 0; i < header.numTiles; ++i) {
            NavMeshTileHeader tileHeader;

            LOG_ASSERT(pos + sizeof(tileHeader) <= data + size);
            memcpy(&tileHeader, pos, sizeof(tileHeader));
            pos += sizeof(tileHeader);
            if (!tileHeader.tileRef || !tileHeader.dataSize)
                break;

            unsigned char* tile_data = (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
            if (!data) break;
            memset(tile_data, 0, tileHeader.dataSize);

            LOG_ASSERT(pos + tileHeader.dataSize <= data + size);
            memcpy(tile_data, pos, tileHeader.dataSize);
            pos += tileHeader.dataSize;

            mesh->addTile(tile_data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, 0);
        }

        return mesh;
    } else {
        LOGE << "Invalid data to load for tilemesh" << endl;
        return NULL;
    }
}

dtNavMesh* TileMesh::loadAll(const char* path) {
    FILE* fp = my_fopen(path, "rb");
    if (!fp) return 0;

    // Read header.
    NavMeshSetHeader header;
    fread(&header, sizeof(NavMeshSetHeader), 1, fp);
    if (header.magic != NAVMESHSET_MAGIC) {
        fclose(fp);
        return 0;
    }
    if (header.version != NAVMESHSET_VERSION) {
        fclose(fp);
        return 0;
    }

    dtNavMesh* mesh = dtAllocNavMesh();
    if (!mesh) {
        fclose(fp);
        return 0;
    }
    dtStatus status = mesh->init(&header.params);
    if (dtStatusFailed(status)) {
        fclose(fp);
        return 0;
    }

    // Read tiles.
    for (int i = 0; i < header.numTiles; ++i) {
        NavMeshTileHeader tileHeader;
        fread(&tileHeader, sizeof(tileHeader), 1, fp);
        if (!tileHeader.tileRef || !tileHeader.dataSize)
            break;

        unsigned char* data = (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
        if (!data) break;
        memset(data, 0, tileHeader.dataSize);
        fread(data, tileHeader.dataSize, 1, fp);

        mesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, 0);
    }

    fclose(fp);

    return mesh;
}

void TileMesh::Save(const char* path) {
    saveAll(path, m_navMesh);
}

void TileMesh::Load(const char* path) {
    dtFreeNavMesh(m_navMesh);
    m_navMesh = loadAll(path);
    m_navQuery->init(m_navMesh, 2048);
}

void TileMesh::LoadMem(const char* data, size_t size) {
    dtFreeNavMesh(m_navMesh);
    m_navMesh = loadAllMem(data, size);
    m_navQuery->init(m_navMesh, 2048);
}

bool TileMesh::handleBuild() {
    if (!m_geom || !m_geom->getMesh()) {
        LOGE << "buildTiledNavigation: No vertices and triangles." << endl;
        return false;
    }

    dtFreeNavMesh(m_navMesh);

    m_navMesh = dtAllocNavMesh();
    if (!m_navMesh) {
        LOGE << "buildTiledNavigation: Could not allocate navmesh." << endl;
        return false;
    }

    dtNavMeshParams params;

    pair<vec3, vec3> meshBounds = GetBoundaries();
    const float* bmin = meshBounds.first.entries;

    rcVcopy(params.orig, bmin);
    params.tileWidth = m_tileSize * m_cellSize;
    params.tileHeight = m_tileSize * m_cellSize;
    params.maxTiles = m_maxTiles;
    params.maxPolys = m_maxPolysPerTile;

    dtStatus status;

    status = m_navMesh->init(&params);
    if (dtStatusFailed(status)) {
        LOGE << "buildTiledNavigation: Could not init navmesh." << endl;
        return false;
    }

    status = m_navQuery->init(m_navMesh, 2048);
    if (dtStatusFailed(status)) {
        LOGE << "buildTiledNavigation: Could not init Detour navmesh query" << endl;
        return false;
    }

    if (m_buildAll)
        buildAllTiles();

    return true;
}

// Analyze the mesh and flag each vertex if it belongs to a watertight mesh
// island that must be filled in.
// This method no longer seems necessary since i updated recast, as the functions don't exist.
// A search for the function name online doesn't return anything, and the
// new reference implementation has no mention of a mesh filling.
/*
static bool* GetVertsNeedFillFromMesh(const rcMeshLoaderObj* mesh){
    const int nverts = mesh->getVertCount();
    const int ntris = mesh->getTriCount();
    const int* tris = mesh->getTris();

    bool *verts_fill = new bool[nverts];
    memset(verts_fill, false, sizeof(bool)*nverts);
    rcGetVertsNeedFill(tris, ntris, verts_fill);
    return verts_fill;
}
*/

void TileMesh::buildTile(const float* pos) {
    if (!m_geom) return;
    if (!m_navMesh) return;

    pair<vec3, vec3> meshBounds = GetBoundaries();
    const float* bmin = meshBounds.first.entries;
    const float* bmax = meshBounds.second.entries;

    const float ts = m_tileSize * m_cellSize;
    const int tx = (int)((pos[0] - bmin[0]) / ts);
    const int ty = (int)((pos[2] - bmin[2]) / ts);

    m_tileBmin[0] = bmin[0] + tx * ts;
    m_tileBmin[1] = bmin[1];
    m_tileBmin[2] = bmin[2] + ty * ts;

    m_tileBmax[0] = bmin[0] + (tx + 1) * ts;
    m_tileBmax[1] = bmax[1];
    m_tileBmax[2] = bmin[2] + (ty + 1) * ts;

    int dataSize = 0;
    unsigned char* data = buildTileMesh(tx, ty, m_tileBmin, m_tileBmax, dataSize);

    if (data) {
        // Remove any previous data (navmesh owns and deletes the data).
        m_navMesh->removeTile(m_navMesh->getTileRefAt(tx, ty, 0), 0, 0);

        // Let the navmesh own the data.
        dtStatus status = m_navMesh->addTile(data, dataSize, DT_TILE_FREE_DATA, 0, 0);
        if (dtStatusFailed(status))
            dtFree(data);
    }
}

void TileMesh::getTilePos(const float* pos, int& tx, int& ty) {
    if (!m_geom) return;

    pair<vec3, vec3> meshBounds = GetBoundaries();
    const float* bmin = meshBounds.first.entries;

    const float ts = m_tileSize * m_cellSize;
    tx = (int)((pos[0] - bmin[0]) / ts);
    ty = (int)((pos[2] - bmin[2]) / ts);
}

void TileMesh::removeTile(const float* pos) {
    if (!m_geom) return;
    if (!m_navMesh) return;

    pair<vec3, vec3> meshBounds = GetBoundaries();
    const float* bmin = meshBounds.first.entries;
    const float* bmax = meshBounds.second.entries;

    const float ts = m_tileSize * m_cellSize;
    const int tx = (int)((pos[0] - bmin[0]) / ts);
    const int ty = (int)((pos[2] - bmin[2]) / ts);

    m_tileBmin[0] = bmin[0] + tx * ts;
    m_tileBmin[1] = bmin[1];
    m_tileBmin[2] = bmin[2] + ty * ts;

    m_tileBmax[0] = bmin[0] + (tx + 1) * ts;
    m_tileBmax[1] = bmax[1];
    m_tileBmax[2] = bmin[2] + (ty + 1) * ts;

    m_navMesh->removeTile(m_navMesh->getTileRefAt(tx, ty, 0), 0, 0);
}

void TileMesh::buildAllTiles() {
    if (!m_geom) return;
    if (!m_navMesh) return;

    pair<vec3, vec3> meshBounds = GetBoundaries();
    const float* bmin = meshBounds.first.entries;
    const float* bmax = meshBounds.second.entries;
    int gw = 0, gh = 0;
    rcCalcGridSize(bmin, bmax, m_cellSize, &gw, &gh);
    const int ts = (int)m_tileSize;
    const int tw = (gw + ts - 1) / ts;
    const int th = (gh + ts - 1) / ts;
    const float tcs = m_tileSize * m_cellSize;

    LOGI << "Navmesh: tile dimensions are " << tw << "x" << th << endl;

    int last_percent = 0;
    time_t last_time = time(0);
    time_t start_time = last_time;
    for (int y = 0; y < th; ++y) {
        for (int x = 0; x < tw; ++x) {
            m_tileBmin[0] = bmin[0] + x * tcs;
            m_tileBmin[1] = bmin[1];
            m_tileBmin[2] = bmin[2] + y * tcs;

            m_tileBmax[0] = bmin[0] + (x + 1) * tcs;
            m_tileBmax[1] = bmax[1];
            m_tileBmax[2] = bmin[2] + (y + 1) * tcs;

            int dataSize = 0;
            unsigned char* data = buildTileMesh(x, y, m_tileBmin, m_tileBmax, dataSize);
            if (data) {
                // Remove any previous data (navmesh owns and deletes the data).
                m_navMesh->removeTile(m_navMesh->getTileRefAt(x, y, 0), 0, 0);
                // Let the navmesh own the data.
                dtStatus status = m_navMesh->addTile(data, dataSize, DT_TILE_FREE_DATA, 0, 0);
                if (dtStatusFailed(status))
                    dtFree(data);
            }
        }

        float percent = y / (float)th * 100.0f;
        time_t current_time = time(0);
        int seconds = (int)difftime(current_time, last_time);
        if (((int)percent != last_percent && (int)percent % 5 == 0) || seconds >= 15) {
            char buffer[512];
            sprintf(buffer, "Navmesh: %d seconds elapsed, %.2f%% done", (int)difftime(current_time, start_time), percent);
            last_percent = (int)percent;
            last_time = current_time;
            AddLoadingText(string(buffer));
        }
    }
}

void TileMesh::removeAllTiles() {
    pair<vec3, vec3> meshBounds = GetBoundaries();
    const float* bmin = meshBounds.first.entries;
    const float* bmax = meshBounds.second.entries;
    int gw = 0, gh = 0;
    rcCalcGridSize(bmin, bmax, m_cellSize, &gw, &gh);
    const int ts = (int)m_tileSize;
    const int tw = (gw + ts - 1) / ts;
    const int th = (gh + ts - 1) / ts;

    for (int y = 0; y < th; ++y)
        for (int x = 0; x < tw; ++x)
            m_navMesh->removeTile(m_navMesh->getTileRefAt(x, y, 0), 0, 0);
}

unsigned char* TileMesh::buildTileMesh(const int tx, const int ty, const float* bmin, const float* bmax, int& dataSize) {
    if (!m_geom || !m_geom->getMesh() || !m_geom->getChunkyMesh()) {
        LOGE << "buildNavigation: Input mesh is not specified." << endl;
        return 0;
    }

    m_tileMemUsage = 0;
    m_tileBuildTime = 0;

    cleanup();

    const float* verts = m_geom->getMesh()->getVerts();
    const int nverts = m_geom->getMesh()->getVertCount();
    const rcChunkyTriMesh* chunkyMesh = m_geom->getChunkyMesh();

    // Init build configuration from GUI
    memset(&m_cfg, 0, sizeof(m_cfg));
    m_cfg.cs = m_cellSize;
    m_cfg.ch = m_cellHeight;
    m_cfg.walkableSlopeAngle = m_agentMaxSlope;
    m_cfg.walkableHeight = (int)ceilf(m_agentHeight / m_cfg.ch);
    m_cfg.walkableClimb = (int)floorf(m_agentMaxClimb / m_cfg.ch);
    m_cfg.walkableRadius = (int)ceilf(m_agentRadius / m_cfg.cs);
    m_cfg.maxEdgeLen = (int)(m_edgeMaxLen / m_cellSize);
    m_cfg.maxSimplificationError = m_edgeMaxError;
    m_cfg.minRegionArea = (int)rcSqr(m_regionMinSize);      // Note: area = size*size
    m_cfg.mergeRegionArea = (int)rcSqr(m_regionMergeSize);  // Note: area = size*size
    m_cfg.maxVertsPerPoly = (int)m_vertsPerPoly;
    m_cfg.tileSize = (int)m_tileSize;
    m_cfg.borderSize = m_cfg.walkableRadius + 3;  // Reserve enough padding.
    m_cfg.width = m_cfg.tileSize + m_cfg.borderSize * 2;
    m_cfg.height = m_cfg.tileSize + m_cfg.borderSize * 2;
    m_cfg.detailSampleDist = m_detailSampleDist < 0.9f ? 0 : m_cellSize * m_detailSampleDist;
    m_cfg.detailSampleMaxError = m_cellHeight * m_detailSampleMaxError;

    rcVcopy(m_cfg.bmin, bmin);
    rcVcopy(m_cfg.bmax, bmax);
    m_cfg.bmin[0] -= m_cfg.borderSize * m_cfg.cs;
    m_cfg.bmin[2] -= m_cfg.borderSize * m_cfg.cs;
    m_cfg.bmax[0] += m_cfg.borderSize * m_cfg.cs;
    m_cfg.bmax[2] += m_cfg.borderSize * m_cfg.cs;

    // LOGI <<  "Building navigation:" << endl;
    // LOGI <<  m_cfg.width << " x " <<  m_cfg.height << " cells" << endl;
    // LOGI.Format( " - %.1fK verts, %.1fK tris\n", nverts/1000.0f, ntris/1000.0f);

    // Allocate voxel heightfield where we rasterize our input data to.
    m_solid = rcAllocHeightfield();
    if (!m_solid) {
        LOGE << "buildNavigation: Out of memory 'solid'." << endl;
        return 0;
    }
    if (!rcCreateHeightfield(m_ctx, *m_solid, m_cfg.width, m_cfg.height, m_cfg.bmin, m_cfg.bmax, m_cfg.cs, m_cfg.ch)) {
        LOGE << "buildNavigation: Could not create solid heightfield." << endl;
        return 0;
    }

    // Allocate array that can hold triangle flags.
    // If you have multiple meshes you need to process, allocate
    // and array which can hold the max number of triangles you need to process.
    m_triareas = new unsigned char[chunkyMesh->maxTrisPerChunk];
    if (!m_triareas) {
        LOGE.Format("buildNavigation: Out of memory 'm_triareas' (%d).", chunkyMesh->maxTrisPerChunk);
        return 0;
    }

    float tbmin[2], tbmax[2];
    tbmin[0] = m_cfg.bmin[0];
    tbmin[1] = m_cfg.bmin[2];
    tbmax[0] = m_cfg.bmax[0];
    tbmax[1] = m_cfg.bmax[2];

    const int kMaxCID = 4096;
    int cid[kMaxCID];  // TODO: Make grow when returning too many items.
    const int ncid = rcGetChunksOverlappingRect(chunkyMesh, tbmin, tbmax, cid, kMaxCID);
    if (!ncid)
        return 0;
    if (ncid == kMaxCID) {
        printf("Need to increase kMaxCID.\n");
    }

    m_tileTriCount = 0;

    // Count the total number of triangles in the relevant chunky mesh nodes,
    // and then put them in an array so that FillTriangles can process them
    // all at once.
    int n_total_tris = 0;
    for (int i = 0; i < ncid; ++i) {
        const rcChunkyTriMeshNode& node = chunkyMesh->nodes[cid[i]];
        const int nctris = node.n;
        n_total_tris += nctris;
    }

    int* total_tris = new int[n_total_tris * 3];
    n_total_tris = 0;

    for (int i = 0; i < ncid; ++i) {
        const rcChunkyTriMeshNode& node = chunkyMesh->nodes[cid[i]];
        const int* ctris = &chunkyMesh->tris[node.i * 3];
        const int nctris = node.n;

        memcpy(&total_tris[n_total_tris * 3], ctris, nctris * sizeof(int) * 3);
        n_total_tris += nctris;
    }

    delete[] total_tris;

    for (int i = 0; i < ncid; ++i) {
        const rcChunkyTriMeshNode& node = chunkyMesh->nodes[cid[i]];
        const int* ctris = &chunkyMesh->tris[node.i * 3];
        const int nctris = node.n;

        m_tileTriCount += nctris;

        memset(m_triareas, 0, nctris * sizeof(unsigned char));
        rcMarkWalkableTriangles(m_ctx, m_cfg.walkableSlopeAngle,
                                verts, nverts, ctris, nctris, m_triareas);

        rcRasterizeTriangles(m_ctx, verts, nverts, ctris, m_triareas, nctris, *m_solid, m_cfg.walkableClimb);
    }

    if (!m_keepInterResults) {
        delete[] m_triareas;
        m_triareas = 0;
    }

    // Once all geometry is rasterized, we do initial pass of filtering to
    // remove unwanted overhangs caused by the conservative rasterization
    // as well as filter spans where the character cannot possibly stand.
    rcFilterLowHangingWalkableObstacles(m_ctx, m_cfg.walkableClimb, *m_solid);
    rcFilterLedgeSpans(m_ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid);
    rcFilterWalkableLowHeightSpans(m_ctx, m_cfg.walkableHeight, *m_solid);

    // Compact the heightfield so that it is faster to handle from now on.
    // This will result more cache coherent data as well as the neighbours
    // between walkable cells will be calculated.
    m_chf = rcAllocCompactHeightfield();
    if (!m_chf) {
        LOGE.Format("buildNavigation: Out of memory 'chf'.\n");
        return 0;
    }
    if (!rcBuildCompactHeightfield(m_ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid, *m_chf)) {
        LOGE.Format("buildNavigation: Could not build compact data.\n");
        return 0;
    }

    if (!m_keepInterResults) {
        rcFreeHeightField(m_solid);
        m_solid = 0;
    }

    // Erode the walkable area by agent radius.
    if (!rcErodeWalkableArea(m_ctx, m_cfg.walkableRadius, *m_chf)) {
        LOGE.Format("buildNavigation: Could not erode.\n");
        return 0;
    }

    // (Optional) Mark areas.
    const ConvexVolume* vols = m_geom->getConvexVolumes();
    for (int i = 0; i < m_geom->getConvexVolumeCount(); ++i)
        rcMarkConvexPolyArea(m_ctx, vols[i].verts, vols[i].nverts, vols[i].hmin, vols[i].hmax, (unsigned char)vols[i].area, *m_chf);

    if (m_monotonePartitioning) {
        // Partition the walkable surface into simple regions without holes.
        if (!rcBuildRegionsMonotone(m_ctx, *m_chf, m_cfg.borderSize, m_cfg.minRegionArea, m_cfg.mergeRegionArea)) {
            LOGE.Format("buildNavigation: Could not build regions.\n");
            return 0;
        }
    } else {
        // Prepare for region partitioning, by calculating distance field along the walkable surface.
        if (!rcBuildDistanceField(m_ctx, *m_chf)) {
            LOGE.Format("buildNavigation: Could not build distance field.\n");
            return 0;
        }

        // Partition the walkable surface into simple regions without holes.
        if (!rcBuildRegions(m_ctx, *m_chf, m_cfg.borderSize, m_cfg.minRegionArea, m_cfg.mergeRegionArea)) {
            LOGE.Format("buildNavigation: Could not build regions.\n");
            return 0;
        }
    }

    // Create contours.
    m_cset = rcAllocContourSet();
    if (!m_cset) {
        LOGE.Format("buildNavigation: Out of memory 'cset'.\n");
        return 0;
    }
    if (!rcBuildContours(m_ctx, *m_chf, m_cfg.maxSimplificationError, m_cfg.maxEdgeLen, *m_cset)) {
        LOGE << "buildNavigation: Could not create contours." << endl;
        return 0;
    }

    if (m_cset->nconts == 0) {
        return 0;
    }

    // Build polygon navmesh from the contours.
    m_pmesh = rcAllocPolyMesh();
    if (!m_pmesh) {
        LOGE << "buildNavigation: Out of memory 'pmesh'." << endl;
        return 0;
    }
    if (!rcBuildPolyMesh(m_ctx, *m_cset, m_cfg.maxVertsPerPoly, *m_pmesh)) {
        LOGE << "buildNavigation: Could not triangulate contours." << endl;
        return 0;
    }

    // Build detail mesh.
    m_dmesh = rcAllocPolyMeshDetail();
    if (!m_dmesh) {
        LOGE << "buildNavigation: Out of memory 'dmesh'." << endl;
        return 0;
    }

    if (!rcBuildPolyMeshDetail(m_ctx, *m_pmesh, *m_chf,
                               m_cfg.detailSampleDist, m_cfg.detailSampleMaxError,
                               *m_dmesh)) {
        LOGE << "buildNavigation: Could build polymesh detail." << endl;
        return 0;
    }

    if (!m_keepInterResults) {
        rcFreeCompactHeightfield(m_chf);
        m_chf = 0;
        rcFreeContourSet(m_cset);
        m_cset = 0;
    }

    unsigned char* navData = 0;
    int navDataSize = 0;
    if (m_cfg.maxVertsPerPoly <= DT_VERTS_PER_POLYGON) {
        if (m_pmesh->nverts >= 0xffff) {
            // The vertex indices are ushorts, and cannot point to more than 0xffff vertices.
            LOGE.Format("Too many vertices per tile %d (max: %d).\n", m_pmesh->nverts, 0xffff);
            return 0;
        }

        // Update poly flags from areas.
        for (int i = 0; i < m_pmesh->npolys; ++i) {
            if (m_pmesh->areas[i] == RC_WALKABLE_AREA)
                m_pmesh->areas[i] = SAMPLE_POLYAREA_GROUND;

            if (m_pmesh->areas[i] == SAMPLE_POLYAREA_GROUND ||
                m_pmesh->areas[i] == SAMPLE_POLYAREA_GRASS ||
                m_pmesh->areas[i] == SAMPLE_POLYAREA_ROAD) {
                m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK;
            } else if (m_pmesh->areas[i] == SAMPLE_POLYAREA_WATER) {
                m_pmesh->flags[i] = SAMPLE_POLYFLAGS_SWIM;
            } else if (m_pmesh->areas[i] == SAMPLE_POLYAREA_DOOR) {
                m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK | SAMPLE_POLYFLAGS_DOOR;
            }
        }

        dtNavMeshCreateParams params;
        memset(&params, 0, sizeof(params));
        params.verts = m_pmesh->verts;
        params.vertCount = m_pmesh->nverts;
        params.polys = m_pmesh->polys;
        params.polyAreas = m_pmesh->areas;
        params.polyFlags = m_pmesh->flags;
        params.polyCount = m_pmesh->npolys;
        params.nvp = m_pmesh->nvp;
        params.detailMeshes = m_dmesh->meshes;
        params.detailVerts = m_dmesh->verts;
        params.detailVertsCount = m_dmesh->nverts;
        params.detailTris = m_dmesh->tris;
        params.detailTriCount = m_dmesh->ntris;
        params.offMeshConVerts = m_geom->getOffMeshConnectionVerts();
        params.offMeshConRad = m_geom->getOffMeshConnectionRads();
        params.offMeshConDir = m_geom->getOffMeshConnectionDirs();
        params.offMeshConAreas = m_geom->getOffMeshConnectionAreas();
        params.offMeshConFlags = m_geom->getOffMeshConnectionFlags();
        params.offMeshConUserID = m_geom->getOffMeshConnectionId();
        params.offMeshConCount = m_geom->getOffMeshConnectionCount();
        params.walkableHeight = m_agentHeight;
        params.walkableRadius = m_agentRadius;
        params.walkableClimb = m_agentMaxClimb;
        params.tileX = tx;
        params.tileY = ty;
        params.tileLayer = 0;
        rcVcopy(params.bmin, m_pmesh->bmin);
        rcVcopy(params.bmax, m_pmesh->bmax);
        params.cs = m_cfg.cs;
        params.ch = m_cfg.ch;
        params.buildBvTree = true;

        if (!dtCreateNavMeshData(&params, &navData, &navDataSize)) {
            LOGE << "Could not build Detour navmesh." << endl;
            return 0;
        }
    }
    m_tileMemUsage = navDataSize / 1024.0f;

    // LOGI.Format(">> Polymesh: %d vertices  %d polygons\n", m_pmesh->nverts, m_pmesh->npolys);

    dataSize = navDataSize;
    return navData;
}

const rcPolyMesh* TileMesh::getPolyMesh() const {
    return m_pmesh;
}

void TileMesh::handleMeshChanged(class InputGeom* geom) {
    m_geom = geom;

    cleanup();

    dtFreeNavMesh(m_navMesh);
    m_navMesh = 0;
}

void TileMesh::SetExplicitBounderies(vec3 min, vec3 max) {
    m_use_explicit_bounderies = true;
    m_Bmin = min;
    m_Bmax = max;
}

pair<vec3, vec3> TileMesh::GetBoundaries() {
    if (m_use_explicit_bounderies) {
        return pair<vec3, vec3>(m_Bmin, m_Bmax);
    } else {
        const float* bmin = m_geom->getMeshBoundsMin();
        const float* bmax = m_geom->getMeshBoundsMax();

        return pair<vec3, vec3>(
            vec3(bmin[0], bmin[1], bmin[2]),
            vec3(bmax[0], bmax[1], bmax[2]));
    }
}

void TileMesh::RemoveExplicitBounderies() {
    m_use_explicit_bounderies = false;
}

const dtNavMesh* TileMesh::getNavMesh() const {
    return m_navMesh;
}

void TileMesh::handleSettings() {
    handleCommonSettings();

    if (m_geom) {
        const float* bmin = m_geom->getMeshBoundsMin();
        const float* bmax = m_geom->getMeshBoundsMax();
        char text[64];
        int gw = 0, gh = 0;
        rcCalcGridSize(bmin, bmax, m_cellSize, &gw, &gh);
        const int ts = (int)m_tileSize;
        const int tw = (gw + ts - 1) / ts;
        const int th = (gh + ts - 1) / ts;
        snprintf(text, 64, "Tiles  %d x %d", tw, th);

        // Max tiles and max polys affect how the tile IDs are caculated.
        // There are 22 bits available for identifying a tile and a polygon.
        int tileBits = rcMin((int)ilog2(nextPow2(tw * th)), 14);
        if (tileBits > 14) tileBits = 14;
        int polyBits = 22 - tileBits;
        m_maxTiles = 1 << tileBits;
        m_maxPolysPerTile = 1 << polyBits;
        snprintf(text, 64, "Max Tiles  %d", m_maxTiles);
        snprintf(text, 64, "Max Polys  %d", m_maxPolysPerTile);
    } else {
        m_maxTiles = 0;
        m_maxPolysPerTile = 0;
    }

    char msg[64];
    snprintf(msg, 64, "Build Time: %.1fms", m_totalBuildTimeMs);
}

void TileMesh::handleCommonSettings() {
    if (m_geom) {
        const float* bmin = m_geom->getMeshBoundsMin();
        const float* bmax = m_geom->getMeshBoundsMax();
        int gw = 0, gh = 0;
        rcCalcGridSize(bmin, bmax, m_cellSize, &gw, &gh);
        char text[64];
        snprintf(text, 64, "Voxels  %d x %d", gw, gh);
    }
}
