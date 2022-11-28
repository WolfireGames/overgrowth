//-----------------------------------------------------------------------------
//           Name: vec4_test.cpp
//      Developer: Wolfire Games LLC
//         Author: Stephan Vedder
//    Description: Test validity of our own vector class
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
//---------------------------------------------

#include <Math/vec4.h>
#include <Math/vec4math.h>

#include <gtest/gtest.h>

TEST(Vec4, Initialized)
{
    // Default init
    vec4 zero_vec;
    EXPECT_EQ(zero_vec[0], 0.0f);
    EXPECT_EQ(zero_vec[1], 0.0f);
    EXPECT_EQ(zero_vec[2], 0.0f);
    EXPECT_EQ(zero_vec[3], 1.0f);

    EXPECT_EQ(zero_vec.x(), 0.0f);
    EXPECT_EQ(zero_vec.y(), 0.0f);
    EXPECT_EQ(zero_vec.z(), 0.0f);
    EXPECT_EQ(zero_vec.w(), 1.0f);

    // Uniform init
    vec4 uni_vec(1.0f);
    EXPECT_EQ(uni_vec[0], 1.0f);
    EXPECT_EQ(uni_vec[1], 1.0f);
    EXPECT_EQ(uni_vec[2], 1.0f);
    EXPECT_EQ(uni_vec[3], 1.0f);

    EXPECT_EQ(uni_vec.x(), 1.0f);
    EXPECT_EQ(uni_vec.y(), 1.0f);
    EXPECT_EQ(uni_vec.z(), 1.0f);
    EXPECT_EQ(uni_vec.w(), 1.0f);

    // Member init
    {
        vec4 mem_vec(1.0f, 2.0f, 3.0f);
        EXPECT_EQ(mem_vec[0], 1.0f);
        EXPECT_EQ(mem_vec[1], 2.0f);
        EXPECT_EQ(mem_vec[2], 3.0f);
        EXPECT_EQ(mem_vec[3], 1.0f);

        EXPECT_EQ(mem_vec.x(), 1.0f);
        EXPECT_EQ(mem_vec.y(), 2.0f);
        EXPECT_EQ(mem_vec.z(), 3.0f);
        EXPECT_EQ(mem_vec.w(), 1.0f);
    }

    {
        vec4 mem_vec(1.0f, 2.0f, 3.0f, 4.0f);
        EXPECT_EQ(mem_vec[0], 1.0f);
        EXPECT_EQ(mem_vec[1], 2.0f);
        EXPECT_EQ(mem_vec[2], 3.0f);
        EXPECT_EQ(mem_vec[3], 4.0f);

        EXPECT_EQ(mem_vec.x(), 1.0f);
        EXPECT_EQ(mem_vec.y(), 2.0f);
        EXPECT_EQ(mem_vec.z(), 3.0f);
        EXPECT_EQ(mem_vec.w(), 4.0f);
    }
}

TEST(Vec4, Operators)
{
    // Default init
    vec4 a(1.0f, 2.0f, 3.0f, 4.0f), b(2.0f, 4.0f, 6.0f, 8.0f);
    vec4 c = a + b;
    EXPECT_EQ(c[0], 3.0f);
    EXPECT_EQ(c[1], 6.0f);
    EXPECT_EQ(c[2], 9.0f);
    EXPECT_EQ(c[3], 4.0f); // taken from A
    c = a - b;
    EXPECT_EQ(c[0], -1.0f);
    EXPECT_EQ(c[1], -2.0f);
    EXPECT_EQ(c[2], -3.0f);
    EXPECT_EQ(c[3], 4.0f); // taken from A
}