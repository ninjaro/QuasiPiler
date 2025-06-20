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

#include "ast.hpp"
#include "grouper.hpp"
#include <gtest/gtest.h>

TEST(GrouperTest, ParsesSimpleBody) {
    std::string input = "{a;b}";
    reader r { input };
    grouper g { r };

    auto res = g.parse_group();
    ASSERT_EQ(res->kind, group_kind::file);
    ASSERT_EQ(res->size(), 1u);

    auto* halt = dynamic_cast<group_node*>(res->nodes[0].get());
    ASSERT_NE(halt, nullptr);
    EXPECT_EQ(halt->kind, group_kind::halt);
    ASSERT_EQ(halt->size(), 1u);

    auto* body = dynamic_cast<group_node*>(halt->nodes[0].get());
    ASSERT_NE(body, nullptr);
    EXPECT_EQ(body->kind, group_kind::body);
    ASSERT_EQ(body->size(), 2u);

    auto* cmd = dynamic_cast<group_node*>(body->nodes[0].get());
    ASSERT_NE(cmd, nullptr);
    EXPECT_EQ(cmd->kind, group_kind::command);
    auto* a = dynamic_cast<token_node*>(cmd->nodes[0].get());
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->value.word, "a");

    auto* trailing = dynamic_cast<group_node*>(body->nodes[1].get());
    ASSERT_NE(trailing, nullptr);
    auto* b = dynamic_cast<token_node*>(trailing->nodes[0].get());
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(b->value.word, "b");
}

TEST(GrouperTest, ParsesNestedListBody) {
    std::string input = "[a,{b;c}]";
    reader r { input };
    grouper g { r };

    auto res = g.parse_group();
    ASSERT_EQ(res->kind, group_kind::file);
    ASSERT_EQ(res->size(), 1u);

    auto* list = dynamic_cast<const group_node*>(res->first());
    ASSERT_NE(list, nullptr);
    EXPECT_EQ(list->kind, group_kind::list);
    ASSERT_EQ(list->size(), 2u);

    auto* item = dynamic_cast<group_node*>(list->nodes[0].get());
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->kind, group_kind::item);
    auto* a = dynamic_cast<token_node*>(item->nodes[0].get());
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->value.word, "a");

    auto* next = dynamic_cast<group_node*>(list->nodes[1].get());
    ASSERT_NE(next, nullptr);
    auto* body = dynamic_cast<group_node*>(next->nodes[0].get());
    ASSERT_NE(body, nullptr);
    EXPECT_EQ(body->kind, group_kind::body);
}

TEST(GrouperTest, MissingClosingThrows) {
    std::string input = "[a";
    reader r { input };
    grouper g { r };
    EXPECT_THROW(g.parse_group(), std::runtime_error);
}

TEST(GrouperTest, ConstructorEnforcesLimit) {
    std::string input = "a";
    reader r { input };
    EXPECT_THROW(grouper(r, 1);, std::runtime_error);
}

TEST(GrouperTest, LimitTooSmallThrows) {
    for (auto [str, lim] : std::vector<std::pair<std::string, size_t>> {
             { "{a;[b,c,d];e}", 14 },
             { "a,b,c,d,e,f", 12 },
             { "{[a,a,a,a,a],[b,b,b,b]}", 24 } }) {
        reader r { str };
        grouper g { r, lim };
        EXPECT_THROW(g.parse_group(), std::runtime_error);
    }
}
