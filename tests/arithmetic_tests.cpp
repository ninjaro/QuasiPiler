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

#include "expression.hpp"
#include "grouper.hpp"

#include <gtest/gtest.h>

TEST(ArithmeticTest, ParseBinary) {
    std::string input = "a+b";
    reader r { input };
    grouper g { r };
    auto res = g.parse();
    ASSERT_EQ(res->kind, group_kind::file);
    ASSERT_EQ(res->size(), 1u);
    auto* cmd = dynamic_cast<group_node*>(res->nodes[0].get());
    ASSERT_NE(cmd, nullptr);
    auto* bin = dynamic_cast<binary_node*>(cmd->nodes[0].get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op.word, "+");
}

TEST(ArithmeticTest, ParsePrefixUnary) {
    std::string input = "+a";
    reader r { input };
    grouper g { r };
    auto res = g.parse();
    ASSERT_EQ(res->kind, group_kind::file);
    ASSERT_EQ(res->size(), 1u);
    auto* cmd = dynamic_cast<group_node*>(res->nodes[0].get());
    ASSERT_NE(cmd, nullptr);
    auto* un = dynamic_cast<unary_node*>(cmd->nodes[0].get());
    ASSERT_NE(un, nullptr);
    EXPECT_TRUE(un->is_prefix);
}

TEST(ArithmeticTest, ParsePostfixUnary) {
    std::string input = "a++";
    reader r { input };
    grouper g { r };
    auto res = g.parse();
    ASSERT_EQ(res->kind, group_kind::file);
    ASSERT_EQ(res->size(), 1u);
    auto* cmd = dynamic_cast<group_node*>(res->nodes[0].get());
    ASSERT_NE(cmd, nullptr);
    auto* un = dynamic_cast<unary_node*>(cmd->nodes[0].get());
    ASSERT_NE(un, nullptr);
    EXPECT_FALSE(un->is_prefix);
}

TEST(ArithmeticTest, ParseNestedGroups) {
    std::string input = "++(a--)";
    reader r { input };
    grouper g { r };
    auto res = g.parse();
    ASSERT_EQ(res->kind, group_kind::file);
    ASSERT_EQ(res->size(), 1u);
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

TEST(ExpressionTest, TernaryBranches) {
    std::vector<ast_node_ptr> nodes;
    auto make_tok = [&](std::string w, token_kind k) {
        auto t = std::make_shared<token_node>();
        t->value.word = w;
        t->value.kind = k;
        return t;
    };
    nodes.push_back(make_tok("a", token_kind::keyword));
    nodes.push_back(make_tok("?", token_kind::special_character));
    nodes.push_back(make_tok("b", token_kind::keyword));
    nodes.push_back(make_tok(":", token_kind::separator));
    nodes.push_back(make_tok("c", token_kind::keyword));
    auto items = expression::make_items(nodes);
    size_t idx = 0;
    auto n = expression::parse_expression(items, idx, 0);
    ASSERT_TRUE(std::dynamic_pointer_cast<ternary_node>(n));
    EXPECT_EQ(idx, items.size());

    idx = 0;
    n = expression::parse_expression(items, idx, 3);
    auto tok = std::dynamic_pointer_cast<token_node>(n);
    ASSERT_TRUE(tok);
    EXPECT_EQ(tok->value.word, "a");
    EXPECT_EQ(idx, 1u);

    items.pop_back();
    idx = 0;
    EXPECT_THROW(
        expression::parse_expression(items, idx, 0), std::runtime_error
    );
}

TEST(ExpressionTest, ParsePrefixUnexpectedEnd) {
    std::vector<expression::item> items;
    size_t idx = 0;
    EXPECT_THROW(expression::parse_prefix(items, idx), std::runtime_error);
}
