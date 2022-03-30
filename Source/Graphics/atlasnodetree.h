//-----------------------------------------------------------------------------
//           Name: atlasnodetree.h
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

#include <Math/ivec2.h>
#include <Math/vec2.h>

#include <iostream>

class AtlasNodeTree
{
public:
    class AtlasNode
    {
    private:
        bool allocated;
        bool suballocated;
        AtlasNode* subnodes[4];
        AtlasNode* parent;
    public:
        ivec2 pos;
        ivec2 dim;

        int id;

        vec2 uv_start;
        vec2 uv_end;
    
        AtlasNode();
        AtlasNode(ivec2 _pos, ivec2 _dim, ivec2 _top_dim, AtlasNode* mem, int memcount, AtlasNode* parent, AtlasNode* self);
        
        /*
            Lift suballocation flag from this node and upwards 
        */
        void LiftSubAllocation();

        /*
            Mark self and all subnodes as allocated, in depth first order (because memory layout)
        */
        void SinkAllocation();

        /*
            Is this function the one returned to be used by a given item?
        */
        bool IsUsed();

        friend class AtlasNodeTree;
        friend std::ostream& operator<<(std::ostream& os, const AtlasNodeTree::AtlasNode &v );
    };

public:
    class AtlasNodeRef
    {
    public:
        AtlasNodeRef();
        AtlasNodeRef(int treeid, AtlasNode * node);

        bool valid();

        int treeid;
        AtlasNode * node;
    };
public:
    AtlasNodeTree( ivec2 _dim, unsigned smallest_size );
    ~AtlasNodeTree();

    /*
        Find the node that wastes least surface of the atlas to fit
        the given dimensions and mark it as allocated, returns null if there are no fitting slots.
    */
    AtlasNodeRef RetrieveNode(ivec2 _dim);
    void FreeNode(const AtlasNodeRef& ref);
    void Clear();

    inline int GetNodeCount(){return nodecount;}
    inline AtlasNode* GetNodeRoot(){return data;}

private:
    int nodecount;
    AtlasNode* data;
    int treeid;
    /*
        Disable dangerous functions
    */
    AtlasNodeTree( AtlasNodeTree &rhs );
    AtlasNodeTree& operator=( AtlasNodeTree &rhs);

};

inline std::ostream& operator<<(std::ostream& os, const AtlasNodeTree::AtlasNode &v )
{
    if( v.parent != NULL )
        os << "p:" << v.parent->id;
    else
        os << "p:no parent";

    os << " a:" << v.allocated << " sv:" << v.suballocated << " id:" << v.id << " " << v.pos << " " << v.dim;
    return os;

}
