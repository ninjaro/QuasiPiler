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

#ifndef AST_HPP
#define AST_HPP

#include <memory>
#include <queue>
#include <string>
#include <vector>

enum class token_kind {
    eof,
    open_bracket,
    close_bracket,
    separator,
    keyword,
    string,
    comment,
    whitespace,
    integer,
    floating,
    special_character
};

struct token {
    token_kind kind;
    int line;
    int column;
    std::streamoff file_offset;
    std::string word;

    virtual ~token();

    virtual void dump(std::ostream& os, const std::string& prefix, bool is_last)
        const noexcept;

    void dump(std::ostream& os) const noexcept;
};

using token_ptr = std::shared_ptr<token>;

struct ast_node {
    size_t fixed_size { 1 }, full_size { 1 };
    virtual ~ast_node();

    virtual ast_node const* first() const noexcept;

    virtual bool empty() const noexcept;

    virtual void dump(
        std::ostream& os, const std::string& prefix, bool is_last, bool full
    ) const noexcept;
    void dump(std::ostream& os, bool full) const noexcept;
    void dump(std::ostream& os) const noexcept;

    virtual void placeholde();
};

using ast_node_ptr = std::shared_ptr<ast_node>;

struct token_node : ast_node {
    token value;
    bool empty() const noexcept override;

    void dump(
        std::ostream& os, const std::string& prefix, bool is_last, bool full
    ) const noexcept override;
};

using token_node_ptr = std::shared_ptr<token_node>;

enum class group_kind { file, body, list, paren, command, item, key, halt };

struct group_node : ast_node {
    size_t limit;
    group_kind kind { group_kind::halt };
    std::vector<ast_node_ptr> nodes;
    std::priority_queue<std::pair<size_t, size_t>>
        weights; /// node_size -> node_index

    bool placeholder { false };
    void append(ast_node_ptr node);
    bool empty() const noexcept override;
    size_t size() const noexcept;
    ast_node const* first() const noexcept override;
    void dump(
        std::ostream& os, const std::string& prefix, bool is_last, bool full
    ) const noexcept override;
    void placeholde() override;
};

using group_ptr = std::shared_ptr<group_node>;
#endif // AST_HPP
