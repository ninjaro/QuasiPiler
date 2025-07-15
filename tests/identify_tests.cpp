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

TEST(IdentifierTest, IdentifyIfElse) {
    for (std::string input :
         { "if(a){b}else{c}", "if(a)b;else c", "if(a)b;else{c}",
           "if(a){b}else c", "if(a){b}elif(c){d}else{e}",
           "if(a){b}elif(c){d}elif(e){f}else{g}",
           "if(a){b}elif(c)d; else{e}" }) {
        reader r { input };
        grouper g { r };

        auto res = g.parse();
        ASSERT_EQ(res->kind, group_kind::file);
        ASSERT_EQ(res->size(), 1u);

        auto* halt = dynamic_cast<group_node*>(res->nodes[0].get());
        ASSERT_NE(halt, nullptr);
        ASSERT_GE(halt->size(), 2u);

        for (size_t i = 0; i < halt->size() - 1; ++i) {
            auto* cond = dynamic_cast<condition_node*>(halt->nodes[i].get());
            ASSERT_NE(cond, nullptr);
            EXPECT_EQ(cond->value.word, i == 0 ? "if" : "elif");
            EXPECT_TRUE(cond->has_paren);
            EXPECT_TRUE(cond->has_body);
        }

        auto* els = dynamic_cast<control_node*>(halt->nodes.back().get());
        ASSERT_NE(els, nullptr);
        EXPECT_EQ(els->value.word, "else");
        EXPECT_TRUE(els->has_body);
    }
}

TEST(IdentifierTest, IdentifyTryCatch) {
    for (std::string input :
         { "try{b}finally{c}", "try b;finally c", "try b;finally{c}",
           "try{b}finally c", "try{b}catch(c){d}finally{e}",
           "try{b}catch(c){d}catch(e){f}finally{g}",
           "try{b}catch(c)d; finally{e}" }) {
        reader r { input };
        grouper g { r };

        auto res = g.parse();
        ASSERT_EQ(res->kind, group_kind::file);
        ASSERT_EQ(res->size(), 1u);

        auto* halt = dynamic_cast<group_node*>(res->nodes[0].get());
        ASSERT_NE(halt, nullptr);
        ASSERT_GE(halt->size(), 2u);

        auto* tr = dynamic_cast<control_node*>(halt->nodes[0].get());
        ASSERT_NE(tr, nullptr);
        EXPECT_EQ(tr->value.word, "try");
        EXPECT_TRUE(tr->has_body);

        for (size_t i = 1; i < halt->size() - 1; ++i) {
            auto* cond = dynamic_cast<condition_node*>(halt->nodes[i].get());
            ASSERT_NE(cond, nullptr);
            EXPECT_EQ(cond->value.word, "catch");
            EXPECT_TRUE(cond->has_paren);
            EXPECT_TRUE(cond->has_body);
        }

        auto* els = dynamic_cast<control_node*>(halt->nodes.back().get());
        ASSERT_NE(els, nullptr);
        EXPECT_EQ(els->value.word, "finally");
        EXPECT_TRUE(els->has_body);
    }
}

TEST(IdentifierTest, IdentifyCallExpression) {
    std::string input = "main(a,b)";
    reader r { input };
    grouper g { r };

    auto res = g.parse();
    auto* halt = dynamic_cast<group_node*>(res->nodes[0].get());
    ASSERT_NE(halt, nullptr);
    ASSERT_EQ(halt->size(), 1u);

    auto* call = dynamic_cast<callexp_node*>(halt->nodes[0].get());
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->value.word, "main");
    EXPECT_TRUE(call->has_paren);
}

TEST(IdentifierTest, IdentifyFunctionDecl) {
    std::string input = "main(a){b}";
    reader r { input };
    grouper g { r };

    auto res = g.parse();
    auto* halt = dynamic_cast<group_node*>(res->nodes[0].get());
    ASSERT_NE(halt, nullptr);
    ASSERT_EQ(halt->size(), 1u);

    auto* func = dynamic_cast<fundecl_node*>(halt->nodes[0].get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->value.word, "main");
    EXPECT_TRUE(func->has_paren);
    EXPECT_TRUE(func->has_body);
}

TEST(IdentifierTest, IdentifyReturnStatement) {
    std::string input = "return a";
    reader r { input };
    grouper g { r };

    auto res = g.parse();
    auto* halt = dynamic_cast<group_node*>(res->nodes[0].get());
    ASSERT_NE(halt, nullptr);
    ASSERT_EQ(halt->size(), 1u);

    auto* jmp = dynamic_cast<jump_node*>(halt->nodes[0].get());
    ASSERT_NE(jmp, nullptr);
    EXPECT_EQ(jmp->value.word, "return");
    EXPECT_TRUE(jmp->has_body);
}
