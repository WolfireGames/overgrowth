//-----------------------------------------------------------------------------
//           Name: atlastest.cpp
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
#include <Graphics/atlasnodetree.h>
#include <Logging/logdata.h>

#include <tut/tut.hpp>

#include <cmath>
#include <cstdlib>

#include <set>

namespace tut 
{ 
    struct data //
    { 
        int hej;
    };
    
    typedef test_group<data> tg;
    tg test_groupt("AtlasNodeTree tests");
    
    typedef tg::object testobject;
    
    template<> 
    template<> 
    void testobject::test<1>() 
    { 
        AtlasNodeTree tree(ivec2(1024,1024), 64);

        /*
        LOGI << "Size: " << tree.nodecount << std::endl;
        for( int i = 0; i < tree.nodecount; i++ )
        {
            LOGI << tree.data[i] << std::endl;
        }
        */

        for( int i = 1; i < tree.GetNodeCount()-1; i++ )
        {
            ensure_equals("Verify depth first memory", tree.GetNodeRoot()[i].id+1, tree.GetNodeRoot()[i+1].id);
        }
    }

    template<> 
    template<> 
    void testobject::test<2>() 
    { 
        AtlasNodeTree tree(ivec2(1024,1024),64);

        ensure("Verify correct flat allocation",tree.RetrieveNode(ivec2(512,512)).valid());   
        ensure("Verify correct flat allocation",tree.RetrieveNode(ivec2(512,512)).valid());   
        ensure("Verify correct flat allocation",tree.RetrieveNode(ivec2(512,512)).valid());   
        ensure("Verify correct flat allocation",tree.RetrieveNode(ivec2(512,512)).valid());   
        ensure("Verify correct flat allocation",tree.RetrieveNode(ivec2(512,512)).valid()==false);   
        
    }

    template<> 
    template<> 
    void testobject::test<3>() 
    { 
        AtlasNodeTree tree(ivec2(1024,1024),64);

        ensure("Verify allocation",tree.RetrieveNode(ivec2(256,256)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(256,256)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(512,512)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(512,512)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(512,512)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(512,512)).valid()==false);   
        
    }

    template<> 
    template<> 
    void testobject::test<4>() 
    { 
        AtlasNodeTree tree(ivec2(1024,1024),64);

        ensure("Verify allocation",tree.RetrieveNode(ivec2(512,512)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(256,256)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(256,256)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(512,512)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(512,512)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(512,512)).valid()==false);   
    }

    template<> 
    template<> 
    void testobject::test<5>() 
    { 
        AtlasNodeTree tree(ivec2(1024,1024),64);

        ensure("Verify allocation, mixxed sizes",tree.RetrieveNode(ivec2(512,512)).valid());   
        ensure("Verify allocation, mixxed sizes",tree.RetrieveNode(ivec2(128,128)).valid());   
        ensure("Verify allocation, mixxed sizes",tree.RetrieveNode(ivec2(128,128)).valid());   
        ensure("Verify allocation, mixxed sizes",tree.RetrieveNode(ivec2(256,256)).valid());   
        ensure("Verify allocation, mixxed sizes",tree.RetrieveNode(ivec2(512,512)).valid());   
        ensure("Verify allocation, mixxed sizes",tree.RetrieveNode(ivec2(512,512)).valid());   
        ensure("Verify allocation, mixxed sizes",tree.RetrieveNode(ivec2(512,512)).valid()==false);   
    }

    template<> 
    template<> 
    void testobject::test<6>() 
    { 
        AtlasNodeTree tree(ivec2(1024,1024),64);

        ensure("Verify allocation",tree.RetrieveNode(ivec2(512,512)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(256,256)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(256,256)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(256,256)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(256,256)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(512,512)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(512,512)).valid());   
        ensure("Verify last is invalid",tree.RetrieveNode(ivec2(64,64)).valid()==false);   
    }

    template<> 
    template<> 
    void testobject::test<7>() 
    { 
        AtlasNodeTree tree(ivec2(1024,1024),64);

        ensure("Verify allocation",tree.RetrieveNode(ivec2(512,512)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(64,128)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(128,64)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(128,128)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(512,512)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(512,512)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(512,512)).valid() == false);   
    }

    template<> 
    template<> 
    void testobject::test<8>() 
    { 
        AtlasNodeTree tree(ivec2(1024,1024),64);

        ensure("Verify allocation",tree.RetrieveNode(ivec2(500,512)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(64,128)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(100,64)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(128,100)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(512,500)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(512,512)).valid());   
        ensure("Verify allocation",tree.RetrieveNode(ivec2(512,512)).valid()==false);   
    }

    template<> 
    template<> 
    void testobject::test<9>() 
    { 
        AtlasNodeTree tree(ivec2(1024,1024),64);

        std::set<AtlasNodeTree::AtlasNode*> pset;

        pset.insert(tree.RetrieveNode(ivec2(512,512)).node);   
        pset.insert(tree.RetrieveNode(ivec2(64,128)).node);   
        pset.insert(tree.RetrieveNode(ivec2(128,64)).node);   
        pset.insert(tree.RetrieveNode(ivec2(128,128)).node);   
        pset.insert(tree.RetrieveNode(ivec2(512,512)).node);   
        pset.insert(tree.RetrieveNode(ivec2(512,512)).node);   

        ensure("Verify that we don't get the same object twice", pset.size()==6);
    }
}
