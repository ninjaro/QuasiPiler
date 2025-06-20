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

#include "reader.hpp"

#include <gtest/gtest.h>

TEST(ReaderTest, Constructor) {
    std::string str;
    reader r { str };
}

TEST(ReaderTest, GoodWhiteSpaceToken) {
    token t;
    for (std::string str : { "   ", "\t\t\t", "         ", "\n\n\n", "\r\r\r",
                             "  \t    \n\t\t\t\r   " }) {
        reader r { str };
        r.next_token(t);
        EXPECT_EQ(t.kind, token_kind::whitespace);
        r.next_token(t);
        EXPECT_EQ(t.kind, token_kind::eof);
    }
}

TEST(ReaderTest, GoodIntegerToken) {
    token t;
    for (std::string& str :
         std::vector<std::string> { "0", "1", "73", "2147483647", "1234567890",
                                    std::string(1024, '9') }) {
        reader r { str };
        r.next_token(t);
        EXPECT_EQ(t.kind, token_kind::integer);
        r.next_token(t);
        EXPECT_EQ(t.kind, token_kind::eof);
    }
}

TEST(ReaderTest, GoodFloatingToken) {
    token t;
    for (std::string& str : std::vector<std::string> {
             "0.0", "0.0000123456789", "2.71828", "3.141592", "36.6",
             "1234567890.0987654321", std::string(1022, '9') + ".0",
             "0." + std::string(1022, '9'),
             std::string(512, '9') + "." + std::string(511, '9') }) {
        reader r { str };
        r.next_token(t);
        EXPECT_EQ(t.kind, token_kind::floating);
        r.next_token(t);
        EXPECT_EQ(t.kind, token_kind::eof);
    }
    for (std::string& str : std::vector<std::string> {
             "0e123", "1E456", "73e+789", "168E+012", "15e-345", "42E-678",
             "1234567890.0987654321", std::string(1022, '9') + ".0",
             "0." + std::string(1022, '9'),
             std::string(512, '9') + "." + std::string(511, '9') }) {
        reader r { str };
        r.next_token(t);
        EXPECT_EQ(t.kind, token_kind::floating);
        r.next_token(t);
        EXPECT_EQ(t.kind, token_kind::eof);
    }
    for (std::string& str : std::vector<std::string> {
             "0.1e123", "1.2E456", "73.84e+789", "168.861E+012", "15.25e-345",
             "42.42E-678", "1234567890.0987654321",
             std::string(1022, '9') + ".0", "0." + std::string(1022, '9'),
             std::string(512, '9') + "." + std::string(511, '9') }) {
        reader r { str };
        r.next_token(t);
        EXPECT_EQ(t.kind, token_kind::floating);
        r.next_token(t);
        EXPECT_EQ(t.kind, token_kind::eof);
    }
}

TEST(ReaderTest, BadIntegerToken) {
    std::string str = "0123";
    reader r { str };
    token t;
    EXPECT_THROW(r.next_token(t), std::runtime_error);
}

TEST(ReaderTest, BadFloatingToken) {
    token t;
    for (std::string& str :
         std::vector<std::string> { "123.", "123e", "123E", "123e+", "123e-",
                                    "123E+", "123E-", "123eE", "123Ee" }) {
        reader r { str };
        EXPECT_THROW(r.next_token(t), std::runtime_error);
    }
}

TEST(ReaderTest, KeyWordToken) {
    token t;
    for (std::string& str : std::vector<std::string> {
             "_abc123ABC_123cbaCBA___", "abc123ABC_123cbaCBA___", "keyword",
             "a", "b1", "c42", "verybadvareabelename" }) {
        reader r { str };
        r.next_token(t);
        EXPECT_EQ(t.kind, token_kind::keyword);
        r.next_token(t);
        EXPECT_EQ(t.kind, token_kind::eof);
    }
}

TEST(ReaderTest, StringToken) {
    token t;
    for (std::string& str : std::vector<std::string> {
             "\"Hello, world!\"", "'Hello, world!'",
             "'Line 1\nLine2\tTabbed\rCarriage\bBackspace\fFormFeed'",
             R"('Special !@#$%^&*()_+-=[]{};:\"\'\\|,<.>/?`~')",
             R"('All ASCII chars:!"\'#$%&()*+,-./0123456789:;<=>?@[\\]^_`{|}~')",
             "\"Non-ASCII: üñîçødé, 中文, русский, العربية\"",
             R"("The quick brown fox jumps over the lazy dog")",
             R"("EXPECT_EQ(result->to_string(), \\\"EXPECT_EQ(result->to_string(),")",
             R"("\\\\\\\"\\\\\\\");\\\"); isn't it a good test string?")",
             R"("C:\\\\Projects\\\\JSONTest\\\\result.json")",
             R"("First line\nSecond line\rBackspace\b happens  here\nTabbed\/")",
             R"("line:\tTabbed\fEnd of string")",
             R"("\\/First line\\nSecond line\\rBackspace\\b happens here\\nTabbed")",
             R"("line:\\tTabbed\\fEnd of string\\\"")",
             R"("I hate Emoji! 🤣🤫🔥\u1234")" }) {
        reader r { str };
        r.next_token(t);
        EXPECT_EQ(t.kind, token_kind::string);
        r.next_token(t);
        EXPECT_EQ(t.kind, token_kind::eof);
    }
}

TEST(ReaderTest, CommentToken) {
    token t;
    std::string multiline = "/*";
    for (std::string& str : std::vector<std::string> {
             "// \"Hello, world!\"", "// 'Hello, world!'",
             "//'Line 1\\Line2\tTabbed\rCarriage\bBackspace\fFormFeed'",
             R"(// 'Special !@#$%^&*()_+-=[]{};:\"\'\\|,<.>/?`~')",
             R"(//'All ASCII chars:!"\'#$%&()*+,-./0123456789:;<=>?@[\\]^_`{|}~')",
             "//Non-ASCII: üñîçødé, 中文, русский, العربية\"",
             R"(//"The quick brown fox jumps over the lazy dog")",
             R"(//"EXPECT_EQ(result->to_string(), \\\"EXPECT_EQ(result->to_string(),")",
             R"(//\\\\\\\"\\\\\\\");\\\"); isn't it a good test string?")",
             R"(// C:\\\\Projects\\\\JSONTest\\\\result.json")",
             R"(///////First line\nSecond line\rBackspace\b happens  here\nTabbed\/")",
             R"(//line:\tTabbed\fEnd of string")",
             R"(//|\\/First line\\nSecond line\\rBackspace\\b happens here\\nTabbed")",
             R"(///line:\\tTabbed\\fEnd of string\\\"")",
             R"(//I hate Emoji! 🤣🤫🔥\u1234")" }) {
        multiline += str + "\n";
        reader r { str };
        r.next_token(t);
        EXPECT_EQ(t.kind, token_kind::comment);
        r.next_token(t);
        EXPECT_EQ(t.kind, token_kind::eof);
    }
    multiline += "*/";
    reader r { multiline };
    r.next_token(t);
    EXPECT_EQ(t.kind, token_kind::comment);
    r.next_token(t);
    EXPECT_EQ(t.kind, token_kind::eof);
}
