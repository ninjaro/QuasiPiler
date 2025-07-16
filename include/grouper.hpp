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

/**
 * @brief Parses tokens into hierarchical groups and expressions.
 *
 * The grouper is responsible for constructing the AST from a token stream.
 * It handles bracket matching, command separation and expression parsing.
 */
class grouper {
public:
    explicit grouper(reader& r, size_t limit = 64);
    /**
     * @brief Parse a sequence starting at the current reader position.
     * @param kind Expected top-level group kind.
     */
    group_ptr parse(group_kind kind = group_kind::file);

private:
    reader& src;
    size_t limit;
    token current;
    position pos {};
    bool reuse { false };

    void peek();

    [[nodiscard]] group_ptr identify_subgroup(const group_ptr& group) const;
    /**
     * @brief Attach @p inode to the last statement if it is a secondary
     * keyword.
     *
     * Handles constructs like <tt>else</tt> or <tt>catch</tt> by merging them
     * with the previous command group.
     */
    [[nodiscard]] bool
    handle_chain(const group_ptr& result, const group_ptr& inode) const;

    bool append_group(
        const group_ptr& result, const ast_node_ptr& node,
        bool& wait_for_condition, bool& wait_for_body, group_kind kind
    ) const;

    void identify_body(const group_ptr& group) const;

    void identify(const group_ptr& group, const group_ptr& result) const;
    /**
     * @brief Transform token groups representing arithmetic into AST nodes.
     *
     * Runs the expression parser over certain group kinds. If the entire group
     * forms a valid expression, its children are replaced with the resulting
     * expression subtree.
     */
    void parse_arithmetic(const group_ptr& group) const;
    /**
     * @brief Close the current command when a separator is encountered.
     */
    bool
    append_command(group_ptr& group, group_ptr& top, group_kind kind) const;
    /**
     * @brief Begin parsing of a bracketed sub-group.
     *
     * Pushes a new wrapped_node onto @p top when an opening bracket is
     * encountered.
     */
    void append_wrapped(const group_ptr& top);
    /**
     * @brief Finalize a wrapped sub-group when a closing bracket is seen.
     */
    void close_wrapped(const group_ptr& group, group_ptr& top, group_kind kind);
    /**
     * @brief Parse a sequence of tokens into the supplied group.
     *
     * This is the core loop that recognises brackets and separators and
     * builds the initial hierarchical structure.
     */
    void parse_group(group_kind kind, group_ptr& group);
    /**
     * @brief Safely append a node to its parent group.
     */
    void append(
        const group_ptr& parent, const ast_node_ptr& node,
        const std::source_location& location = std::source_location::current()
    ) const;
    /**
     * @brief Create a formatted runtime error describing a parse failure.
     */
    [[nodiscard]] std::runtime_error make_error(
        const std::string& message, const group_ptr& context = {},
        const std::source_location& location = std::source_location::current()
    ) const;
};

#endif // GROUPER_HPP
