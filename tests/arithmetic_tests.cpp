/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 Yaroslav Riabtsev <yaroslav.riabtsev@rwth-aachen.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "grouper.hpp"
#include <gtest/gtest.h>

TEST(ArithmeticTest, ParseBinary) {
    std::string input = "a+b;";
    reader r { input };
    grouper g { r };
    auto res = g.parse();
    ASSERT_EQ(res->kind, group_kind::file);
    ASSERT_GE(res->size(), 1u);
    auto* cmd = dynamic_cast<group_node*>(res->nodes[0].get());
    ASSERT_NE(cmd, nullptr);
    auto* bin = dynamic_cast<binary_node*>(cmd->nodes[0].get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op.word, "+");
}

TEST(ArithmeticTest, ParsePrefixUnary) {
    std::string input = "+a;";
    reader r { input };
    grouper g { r };
    auto res = g.parse();
    ASSERT_EQ(res->kind, group_kind::file);
    ASSERT_GE(res->size(), 1u);
    auto* cmd = dynamic_cast<group_node*>(res->nodes[0].get());
    ASSERT_NE(cmd, nullptr);
    auto* un = dynamic_cast<unary_node*>(cmd->nodes[0].get());
    ASSERT_NE(un, nullptr);
    EXPECT_TRUE(un->is_prefix);
}

TEST(ArithmeticTest, ParsePostfixUnary) {
    std::string input = "a++;";
    reader r { input };
    grouper g { r };
    auto res = g.parse();
    ASSERT_EQ(res->kind, group_kind::file);
    ASSERT_GE(res->size(), 1u);
    auto* cmd = dynamic_cast<group_node*>(res->nodes[0].get());
    ASSERT_NE(cmd, nullptr);
    auto* un = dynamic_cast<unary_node*>(cmd->nodes[0].get());
    ASSERT_NE(un, nullptr);
    EXPECT_FALSE(un->is_prefix);
}

TEST(ArithmeticTest, ParseNestedGroups) {
    std::string input = "++(a--);";
    reader r { input };
    grouper g { r };
    auto res = g.parse();
    ASSERT_EQ(res->kind, group_kind::file);
    ASSERT_GE(res->size(), 1u);
    auto* cmd = dynamic_cast<group_node*>(res->nodes[0].get());
    ASSERT_NE(cmd, nullptr);
    auto* pre = dynamic_cast<unary_node*>(cmd->nodes[0].get());
    ASSERT_NE(pre, nullptr);
    EXPECT_TRUE(pre->is_prefix);
    auto* paren = dynamic_cast<group_node*>(pre->expr.get());
    ASSERT_NE(paren, nullptr);
    ASSERT_EQ(paren->size(), 1u);
    auto* inner = dynamic_cast<group_node*>(paren->nodes[0].get());
    ASSERT_NE(inner, nullptr);
    ASSERT_EQ(inner->size(), 1u);
    auto* post = dynamic_cast<unary_node*>(inner->nodes[0].get());
    ASSERT_NE(post, nullptr);
    EXPECT_FALSE(post->is_prefix);
}