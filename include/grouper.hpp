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

#ifndef GROUPER_HPP
#define GROUPER_HPP

#include "ast.hpp"

class grouper {
public:
    explicit grouper(reader& r, size_t limit = 64);

    group_ptr parse(group_kind kind = group_kind::file);

private:
    reader& src;
    size_t limit;
    token current;
    position pos {};
    bool reuse { false };

    void peek();

    [[nodiscard]] group_ptr identify_subgroup(const group_ptr& group) const;

    [[nodiscard]] bool
    handle_chain(const group_ptr& result, const group_ptr& inode) const;

    bool append_group(
        const group_ptr& result, const ast_node_ptr& node,
        bool& wait_for_condition, bool& wait_for_body, group_kind kind
    ) const;

    void identify_body(const group_ptr& group) const;

    void identify(const group_ptr& group, const group_ptr& result) const;

    bool
    append_command(group_ptr& group, group_ptr& top, group_kind kind) const;

    void append_wrapped(const group_ptr& top);

    void close_wrapped(const group_ptr& group, group_ptr& top, group_kind kind);

    void parse_group(group_kind kind, group_ptr& group);

    void append(
        const group_ptr& parent, const ast_node_ptr& node,
        const std::source_location& location = std::source_location::current()
    ) const;

    [[nodiscard]] std::runtime_error make_error(
        const std::string& message, const group_ptr& context = {},
        const std::source_location& location = std::source_location::current()
    ) const;
};

#endif // GROUPER_HPP
