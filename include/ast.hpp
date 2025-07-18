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

#include "reader.hpp"

struct ast_node {
    size_t fixed_size { 1 }, full_size { 1 };
    virtual ~ast_node();

    [[nodiscard]] virtual ast_node const* get() const noexcept;
    [[nodiscard]] virtual ast_node const* first() const;
    virtual const position& get_start() const;

    [[nodiscard]] virtual bool empty() const noexcept;

    virtual void dump(
        std::ostream& os, const std::string& prefix, bool is_last, bool full
    ) const;
    void dump(std::ostream& os, bool full) const;
    void dump(std::ostream& os) const;
};

using ast_node_ptr = std::shared_ptr<ast_node>;

struct token_node : ast_node {
    token value;
    [[nodiscard]] bool empty() const noexcept override;
    const position& get_start() const override;
    void dump(
        std::ostream& os, const std::string& prefix, bool is_last, bool full
    ) const override;
};

using token_node_ptr = std::shared_ptr<token_node>;

enum class group_kind { file, body, list, paren, command, item, key, halt };

[[nodiscard]] const char* group_kind_name(group_kind k) noexcept;

/**
 * @brief Collection of AST nodes with a configurable size limit.
 *
 * Large sub-groups may be replaced with placeholder nodes to keep
 * @c fixed_size within @c limit.
 */
struct group_node : ast_node {
    size_t limit; ///< Maximum allowed node weight
    group_kind kind { group_kind::halt };
    std::vector<ast_node_ptr> nodes;
    /// queue of heavy child nodes: <node_size, node_index>
    std::priority_queue<std::pair<size_t, size_t>> weights;

    /**
     * @brief Append a child node while respecting the size limit.
     *
     * Nodes contribute their @c fixed_size and @c full_size to the parent. If
     * the accumulated @c fixed_size exceeds @c limit, larger child groups are
     * replaced with ::placeholder_node instances so the tree can be lazily
     * expanded later.
     *
     * @param node Node to append.
     * @param src  Reader used to reconstruct squeezed subtrees on demand.
     */
    void append(ast_node_ptr node, const reader& src);
    [[nodiscard]] bool empty() const noexcept override;
    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] ast_node const* get() const noexcept override;
    [[nodiscard]] ast_node const* first() const override;
    void dump(
        std::ostream& os, const std::string& prefix, bool is_last, bool full
    ) const override;
    [[nodiscard]] const position& get_start() const override;
    /**
     * @brief Replace a child group with a placeholder.
     *
     * The placeholder stores enough information to re-read the original subtree
     * from @p src later. This is used when a group's @c fixed_size would exceed
     * the configured limit and thus needs to be collapsed.
     *
     * @param index Index of the child to replace.
     * @param src   Reader used to recreate the subtree if needed.
     */
    void squeeze(size_t index, const reader& src);
    void pop_back();
};

using group_ptr = std::shared_ptr<group_node>;

struct wrapped_node : group_node {
    position start {};
    const position& get_start() const override;
};

using wrapped_ptr = std::shared_ptr<wrapped_node>;

/**
 * @brief Node standing in place of a squeezed sub-tree.
 *
 * When a group exceeds the configured size limit it can be replaced by a
 * placeholder node. The original reader is stored so the subtree can be
 * reconstructed on demand.
 */
struct placeholder_node final : wrapped_node {
    reader* src { nullptr };
    void dump(
        std::ostream& os, const std::string& prefix, bool is_last, bool full
    ) const override;
};

using placeholder_node_ptr = std::shared_ptr<placeholder_node>;

struct callexp_node : token_node {
    ast_node_ptr paren;
    bool has_paren { false };

    explicit callexp_node(const token& name);
    void set_paren(ast_node_ptr paren);

    void dump(
        std::ostream& os, const std::string& prefix, bool is_last, bool full
    ) const override;
};

using callexp_ptr = std::shared_ptr<callexp_node>;

struct fundecl_node : callexp_node {
    ast_node_ptr body;
    bool has_body { false };

    explicit fundecl_node(const callexp_ptr& proto);
    void set_body(ast_node_ptr body);

    void dump(
        std::ostream& os, const std::string& prefix, bool is_last, bool full
    ) const override;
};

using fundecl_ptr = std::shared_ptr<fundecl_node>;

struct control_node : token_node {
    ast_node_ptr body;
    bool has_body { false };

    explicit control_node(const token& name);
    void set_body(ast_node_ptr body);
    void dump(
        std::ostream& os, const std::string& prefix, bool is_last, bool full
    ) const override;
};

using control_ptr = std::shared_ptr<control_node>;

struct condition_node : control_node {
    bool is_loop { false };
    ast_node_ptr paren;
    bool has_paren { false };

    explicit condition_node(const token& name);
    void set_paren(ast_node_ptr paren);

    void dump(
        std::ostream& os, const std::string& prefix, bool is_last, bool full
    ) const override;
};

using condition_ptr = std::shared_ptr<condition_node>;

struct jump_node : control_node {
    explicit jump_node(const token& name);
    void dump(
        std::ostream& os, const std::string& prefix, bool is_last, bool full
    ) const override;
};

using jump_ptr = std::shared_ptr<jump_node>;

struct unary_node : ast_node {
    token op;
    ast_node_ptr expr;
    bool is_prefix { true };
    int priority { 0 };

    unary_node(
        const token& op, ast_node_ptr expr, bool is_prefix, int priority
    );

    const position& get_start() const override;
    void dump(
        std::ostream& os, const std::string& prefix, bool is_last, bool full
    ) const override;
};

using unary_ptr = std::shared_ptr<unary_node>;

struct binary_node : ast_node {
    token op;
    ast_node_ptr lhs;
    ast_node_ptr rhs;
    int priority { 0 };

    binary_node(
        const token& op, ast_node_ptr lhs, ast_node_ptr rhs, int priority
    );

    const position& get_start() const override;
    void dump(
        std::ostream& os, const std::string& prefix, bool is_last, bool full
    ) const override;
};

using binary_ptr = std::shared_ptr<binary_node>;

struct ternary_node : ast_node {
    token qmark;
    token colon;
    ast_node_ptr cond;
    ast_node_ptr left;
    ast_node_ptr right;
    int priority { 0 };

    ternary_node(
        const token& qmark, const token& colon, ast_node_ptr cond,
        ast_node_ptr left, ast_node_ptr right, int priority
    );

    const position& get_start() const override;
    void dump(
        std::ostream& os, const std::string& prefix, bool is_last, bool full
    ) const override;
};

using ternary_ptr = std::shared_ptr<ternary_node>;

#endif // AST_HPP
