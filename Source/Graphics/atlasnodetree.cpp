//-----------------------------------------------------------------------------
//           Name: atlasnodetree.cpp
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
#include "atlasnodetree.h"

#include <Math/ivec2math.h>
#include <Utility/math_macro.h>
#include <Logging/logdata.h>

#include <cstdlib>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <climits>

static int c = 1;

AtlasNodeTree::AtlasNode::AtlasNode():
allocated(false),
suballocated(false),
parent(NULL),
pos(0),
dim(0),
id(-1),
uv_start(0),
uv_end(0)
{
    subnodes[0] = NULL;
    subnodes[1] = NULL;
    subnodes[2] = NULL;
    subnodes[3] = NULL;
}

AtlasNodeTree::AtlasNode::AtlasNode(ivec2 _pos, ivec2 _dim, ivec2 _top_dim, AtlasNode* mem, int memcount, AtlasNode* _parent, AtlasNode *self):
allocated(false),
suballocated(false),
parent(_parent),
pos(_pos),
dim(_dim),
id(c++)
{
    assert(IS_PWR2(dim[0]));
    assert(IS_PWR2(dim[1]));

    uv_start = vec2(pos) / vec2(_top_dim);
    uv_end = vec2(dim) / vec2(_top_dim);

    if( memcount == 0 )
    {
        subnodes[0] = NULL;
        subnodes[1] = NULL;
        subnodes[2] = NULL;
        subnodes[3] = NULL;
    }
    else if( memcount >= 4 )
    {
        ivec2 halfwidth = dim/2;
        int step = (memcount-4)/4;

        subnodes[0] = &mem[(step+1)*0];
        subnodes[1] = &mem[(step+1)*1];
        subnodes[2] = &mem[(step+1)*2];
        subnodes[3] = &mem[(step+1)*3];

        *(subnodes[0]) = AtlasNode(pos                        , halfwidth, _top_dim, subnodes[0]+1, step, self,subnodes[0]);
        *(subnodes[1]) = AtlasNode(pos + ivec2(halfwidth[0],0), halfwidth, _top_dim, subnodes[1]+1, step, self,subnodes[1]);
        *(subnodes[2]) = AtlasNode(pos + ivec2(0,halfwidth[1]), halfwidth, _top_dim, subnodes[2]+1, step, self,subnodes[2]);
        *(subnodes[3]) = AtlasNode(pos + halfwidth            , halfwidth, _top_dim, subnodes[3]+1, step, self,subnodes[3]);
    }
    else
    {
        assert(false);
    }
}

AtlasNodeTree::AtlasNodeRef::AtlasNodeRef():
treeid(-1),
node(NULL)
{
}

AtlasNodeTree::AtlasNodeRef::AtlasNodeRef( int _treeid, AtlasNodeTree::AtlasNode* _node ) : treeid(_treeid), node(_node)
{
}

bool AtlasNodeTree::AtlasNodeRef::valid()
{
    return node != NULL && treeid != -1;
}

AtlasNodeTree::AtlasNodeRef AtlasNodeTree::RetrieveNode(ivec2 dim)
{
    int needed_area = dim[0]*dim[1];

    int best_area = INT_MAX;
    AtlasNode* best_node = NULL;

    for( int i = 0; i < nodecount; i++ )
    { 
        AtlasNode* cur_node = &data[i];
        int cur_area;

        //See if the node is a potential candidate
        if( cur_node->allocated == false 
            && cur_node->suballocated == false )
        {
            if( dim[0] <= cur_node->dim[0] && dim[1] <= cur_node->dim[1] )
            {
                cur_area = cur_node->dim[0] * cur_node->dim[1];

                if( cur_area < best_area )
                {
                    best_area = cur_area;
                    best_node = cur_node; 

                    //Fast path out, we already have a perfect match
                    if( cur_area == needed_area )
                    {
                        break;
                    }
                }
            }
        }
    }

    if( best_node != NULL)
    {
        //Mark this and all subnodes as allocated, because they are.
        best_node->SinkAllocation();

        //Mark all node above that they have a subnode with allocation 
        best_node->LiftSubAllocation();  
    }

    return AtlasNodeRef( this->treeid, best_node );
}

void AtlasNodeTree::FreeNode(const AtlasNodeTree::AtlasNodeRef& ref)
{
    if( ref.treeid == this->treeid)
    {
        if(ref.node != NULL)
        {
            ref.node->allocated = false;
            ref.node->suballocated = false;
        }
        else
        {
            LOGE << "Trying to free node already free" << std::endl;
        }
    }
    else
    {
        LOGE << "You're trying to free a node that did not come from this tree" << std::endl;
    }
}

void AtlasNodeTree::Clear()
{
    for( int i = 0; i < nodecount; i++ )
    {
        AtlasNode* cur_node = &data[i];

        cur_node->allocated = false;
        cur_node->suballocated = false;
    }
}

void AtlasNodeTree::AtlasNode::LiftSubAllocation()
{
    this->suballocated = true; 

    if( this->parent != NULL)
    {
        this->parent->LiftSubAllocation();
    }
}

void AtlasNodeTree::AtlasNode::SinkAllocation()
{
    this->allocated = true;

    if( subnodes[0] != NULL )
        subnodes[0]->SinkAllocation();

    if( subnodes[1] != NULL )
        subnodes[1]->SinkAllocation();

    if( subnodes[2] != NULL )
        subnodes[2]->SinkAllocation();

    if( subnodes[3] != NULL )
        subnodes[3]->SinkAllocation();
}

bool AtlasNodeTree::AtlasNode::IsUsed()
{
    //If a given node has both set, it's the "starting point" for a allocation propagation,
    //meaning it's the one returned by RetrieveNode function at some point.
    return this->suballocated && this->allocated;
}

AtlasNodeTree::AtlasNodeTree( ivec2 dim, unsigned smallest_size )
{
    static int treeidc = 0;
    this->treeid = treeidc++;

    //The smallest allowed size also must be power of 2 as dimensions are
    assert(IS_PWR2(smallest_size));
    //Dimensions have to be a power of two to allow some assumptions
    //mainly that each level is a power of two down.
    assert(IS_PWR2(dim[0]) && IS_PWR2(dim[1]));
    //We currently demand that the area is squared
    assert(dim[0] == dim[1]);

    unsigned minv = std::min( dim[0], dim[1] );
    int level_count = 0;

    while( minv > smallest_size ) 
    {
        minv = minv >> 1; 
        level_count++;
    }

    nodecount = 1;
    for( int i = 1; i <= level_count; i++ )
    {
        nodecount += (int) pow(4,i);
    }

    data = new AtlasNode[nodecount];

    data[0] = AtlasNode(ivec2(0), dim, dim, data+1, nodecount-1, NULL, &data[0]);
}

AtlasNodeTree::~AtlasNodeTree()
{
    delete[](data);
}
