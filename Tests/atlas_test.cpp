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

#include <gtest/gtest.h>

#include <cmath>
#include <cstdlib>

#include <set>

TEST(Graphics, AtlasVerifyIds)
{
    AtlasNodeTree tree(ivec2(1024, 1024), 64);

    /*
    LOGI << "Size: " << tree.nodecount << std::endl;
    for( int i = 0; i < tree.nodecount; i++ )
    {
        LOGI << tree.data[i] << std::endl;
    }
    */

    for (int i = 1; i < tree.GetNodeCount() - 1; i++) {
        EXPECT_EQ(tree.GetNodeRoot()[i].id + 1, tree.GetNodeRoot()[i + 1].id);
    }
}

TEST(Graphics, AtlasAlloc1) {
    AtlasNodeTree tree(ivec2(1024, 1024), 64);

    EXPECT_TRUE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Expected valid element";
    EXPECT_FALSE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Last element should be invalid";
}

TEST(Graphics, AtlasAlloc2) {
    AtlasNodeTree tree(ivec2(1024, 1024), 64);

    EXPECT_TRUE(tree.RetrieveNode(ivec2(256, 256)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(256, 256)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Expected valid element";
    EXPECT_FALSE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Last element should be invalid";
}

TEST(Graphics, AtlasAlloc3) {
    AtlasNodeTree tree(ivec2(1024, 1024), 64);

    EXPECT_TRUE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(256, 256)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(256, 256)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Expected valid element";
    EXPECT_FALSE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Last element should be invalid";
}

TEST(Graphics, AtlasAlloc4)
{
    AtlasNodeTree tree(ivec2(1024, 1024), 64);

    EXPECT_TRUE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(128, 128)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(128, 128)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(256, 256)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Expected valid element";
    EXPECT_FALSE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Last element should be invalid";
}

TEST(Graphics, AtlasAlloc5) {
    AtlasNodeTree tree(ivec2(1024, 1024), 64);

    EXPECT_TRUE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(256, 256)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(256, 256)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(256, 256)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(256, 256)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Expected valid element";
    EXPECT_FALSE(tree.RetrieveNode(ivec2(64, 64)).valid()) << "Last element should be invalid";
}

TEST(Graphics, AtlasAlloc6) {
    AtlasNodeTree tree(ivec2(1024, 1024), 64);

    EXPECT_TRUE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(64, 128)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(128, 64)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(128, 128)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Expected valid element";
    EXPECT_FALSE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Last element should be invalid";
}

TEST(Graphics, AtlasAlloc7) {

    AtlasNodeTree tree(ivec2(1024, 1024), 64);

    EXPECT_TRUE(tree.RetrieveNode(ivec2(500, 512)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(64, 128)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(100, 64)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(128, 100)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(512, 500)).valid()) << "Expected valid element";
    EXPECT_TRUE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Expected valid element";
    EXPECT_FALSE(tree.RetrieveNode(ivec2(512, 512)).valid()) << "Last element should be invalid";
}

TEST(Graphics, AtlasInsertCheck) {
    AtlasNodeTree tree(ivec2(1024, 1024), 64);

    std::set<AtlasNodeTree::AtlasNode*> pset;

    pset.insert(tree.RetrieveNode(ivec2(512, 512)).node);
    pset.insert(tree.RetrieveNode(ivec2(64, 128)).node);
    pset.insert(tree.RetrieveNode(ivec2(128, 64)).node);
    pset.insert(tree.RetrieveNode(ivec2(128, 128)).node);
    pset.insert(tree.RetrieveNode(ivec2(512, 512)).node);
    pset.insert(tree.RetrieveNode(ivec2(512, 512)).node);

    EXPECT_EQ(pset.size(), 6);
}
