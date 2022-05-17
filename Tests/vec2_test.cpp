//-----------------------------------------------------------------------------
//           Name: vec2_test.cpp
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

#include <Math/vec2.h>
#include <Math/vec2math.h>

#include <gtest/gtest.h>

TEST(Vec2, Initialized)
{
    // Default init
    vec2 zero_vec;
    EXPECT_EQ(zero_vec[0], 0.0f);
    EXPECT_EQ(zero_vec[1], 0.0f);

    EXPECT_EQ(zero_vec.x(), 0.0f);
    EXPECT_EQ(zero_vec.y(), 0.0f);

    // Uniform init
    vec2 uni_vec(1.0f);
    EXPECT_EQ(uni_vec[0], 1.0f);
    EXPECT_EQ(uni_vec[1], 1.0f);

    EXPECT_EQ(uni_vec.x(), 1.0f);
    EXPECT_EQ(uni_vec.y(), 1.0f);

    // Member init
    vec2 mem_vec(1.0f, 2.0f);
    EXPECT_EQ(mem_vec[0], 1.0f);
    EXPECT_EQ(mem_vec[1], 2.0f);

    EXPECT_EQ(mem_vec.x(), 1.0f);
    EXPECT_EQ(mem_vec.y(), 2.0f);
}

TEST(Vec2, Operators)
{
    // Default init
    vec2 a(1.0f, 2.0f), b(2.0f, 4.0f);
    vec2 c = a + b;
    EXPECT_EQ(c[0], 3.0f);
    EXPECT_EQ(c[1], 6.0f);
    c = a - b;
    EXPECT_EQ(c[0], -1.0f);
    EXPECT_EQ(c[1], -2.0f);
    c = a / b;
    EXPECT_EQ(c[0], 0.5f);
    EXPECT_EQ(c[1], 0.5f);
    c = a * b;
    EXPECT_EQ(c[0], 2.0f);
    EXPECT_EQ(c[1], 8.0f);
}