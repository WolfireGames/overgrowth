//-----------------------------------------------------------------------------
//           Name: string_test.cpp
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

#include <Utility/strings.h>
#include <Utility/serialize.h>

#include <cmath>
#include <cstdlib>
#include <set>

#include <gtest/gtest.h>

static uint32_t hexflip(uint32_t v) {
    char str[9];
    uint32_t vout;
    flags_to_string(str, v);
    string_flags_to_uint32(&vout, str);
    return vout;
}

TEST(Utility, String)
{
    EXPECT_TRUE(endswith("my/cool/string.png", ".png"));
    EXPECT_FALSE(endswith("my/cool/string.png", ".pnk"));

    char buffer[6];
    EXPECT_EQ(strscpy(buffer, "false", 6), 0);
    EXPECT_EQ(strscpy(buffer, "false", 5), SOURCE_TOO_LONG);
    EXPECT_EQ(strscpy(buffer, NULL, 5), SOURCE_IS_NULL);
    EXPECT_EQ(strscpy(buffer, NULL, 0), DESTINATION_IS_ZERO_LENGTH);

    EXPECT_EQ(saysTrue("true"), 1);
    EXPECT_EQ(saysTrue("false"), 0);
    EXPECT_EQ(saysTrue("falseeeee"), -1);
    EXPECT_EQ(saysTrue(NULL), -2);

    EXPECT_EQ(hexflip(0), 0);
    for (uint32_t i = 1; i < 32; i++) {
        EXPECT_EQ(hexflip(1U << i), (1U << i));
    }

    for (uint32_t i = 1; i < 0xFF; i++) {
        EXPECT_EQ(hexflip(i), i);
    }

    EXPECT_EQ(hexflip(0x12345678), 0x12345678);
    EXPECT_EQ(hexflip(0xFEDCBA98), 0xFEDCBA98);
    EXPECT_EQ(hexflip(0xDEADBEEF), 0xDEADBEEF);
}