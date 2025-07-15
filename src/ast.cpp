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

ast_node::~ast_node() = default;

ast_node const* ast_node::get() const noexcept { return this; }

ast_node const* ast_node::first() const { return this; }

const position& ast_node::get_start() const {
    throw std::runtime_error(
        "get_start() is not implemented for ast_node, use derived classes"
    );
}

bool ast_node::empty() const noexcept { return true; }

void ast_node::dump(
    std::ostream& os, const std::string& prefix, const bool is_last, bool
) const {
    os << prefix << (is_last ? "`-" : "|-") << "Null\n";
}

void ast_node::dump(std::ostream& os, const bool full) const {
    dump(os, "", true, full);
}

void ast_node::dump(std::ostream& os) const { dump(os, true); }

bool token_node::empty() const noexcept { return false; }

const position& token_node::get_start() const { return value.pos; }

void token_node::dump(
    std::ostream& os, const std::string& prefix, const bool, bool
) const {
    // os << prefix << (is_last ? "`-" : "|-") << "TokenNode\n";
    // value.dump(os, prefix + (is_last ? "  " : "| "), true);
    value.dump(os, prefix, true);
}

const char* group_kind_name(group_kind k) noexcept {
    static constexpr const char* names[] = {
        "file", "body", "list", "paren", "command", "item", "key", "halt",
    };
    return names[static_cast<size_t>(k)];
}

void placeholder_node::dump(
    std::ostream& os, const std::string& prefix, const bool is_last,
    const bool full
) const {
    if (full && src != nullptr) {
        const auto position = src->get_position();
        src->jump_to_position(start);
        grouper g { *src, limit };
        try {
            auto group = g.parse(kind);
            group->dump(os, prefix, is_last, full);
        } catch (const std::runtime_error& e) {
            src->jump_to_position(start);
            token current;
            src->next_token(current);
            std::ostringstream msg;
            msg << "[PlaceholderNode-Error] during parsing at position <"
                << position.line << ":" << position.column
                << "> with first token: ";
            current.dump(msg);
            msg << prefix << e.what() << "\n";
            throw std::runtime_error(msg.str());
        }
        src->jump_to_position(position);
    } else {
        os << prefix << (is_last ? "`-" : "|-") << "Placeholder("
           << group_kind_name(kind) << ") [" << full_size << " nested nodes]\n";
    }
}

void group_node::append(ast_node_ptr node, const reader& src) {
    size_t exclude = (size() == 0 ? 1 : 0);
    fixed_size += node->fixed_size - exclude;
    full_size += node->full_size - exclude;
    if (node->fixed_size > 1 && std::dynamic_pointer_cast<group_node>(node)) {
        weights.emplace(node->fixed_size, size());
    }
    nodes.push_back(std::move(node));
    while (!weights.empty() && fixed_size > limit) {
        auto [weight, index] = weights.top();
        weights.pop();
        fixed_size += 1 - weight;
        if (!std::dynamic_pointer_cast<placeholder_node>(nodes[index])) {
            squeeze(index, src);
        }
    }
    if (fixed_size > limit) {
        throw std::runtime_error(
            "limit is too small for group node (required "
            + std::to_string(full_size) + ", limit is " + std::to_string(limit)
            + ")"
        );
    }
}

bool group_node::empty() const noexcept { return size() == 0; }

size_t group_node::size() const noexcept { return nodes.size(); }

ast_node const* group_node::get() const noexcept {
    if (size() == 1) {
        return nodes[0]->get();
    }
    return ast_node::get();
}

ast_node const* group_node::first() const {
    if (size() == 0) {
        throw std::runtime_error("group node is empty");
    }
    return nodes[0]->first();
}

void group_node::dump(
    std::ostream& os, const std::string& prefix, const bool is_last,
    const bool full
) const {
    if (kind != group_kind::file) {
        os << prefix << (is_last ? "`-" : "|-");
    }
    os << "Group(" << group_kind_name(kind) << ")";
    if (!full) {
        os << " <" << fixed_size << "/" << full_size << " nodes>";
    }
    os << "\n";
    const std::string child_prefix
        = prefix + (kind != group_kind::file ? (is_last ? "  " : "| ") : "");
    for (size_t i = 0; i < nodes.size(); ++i) {
        nodes[i]->dump(os, child_prefix, i + 1 == nodes.size(), full);
    }
}

const position& group_node::get_start() const {
    if (size() == 0) {
        throw std::runtime_error(
            "group node is empty, cannot get start position"
        );
    }
    return nodes[0]->get_start();
}

void group_node::squeeze(const size_t index, const reader& src) {
    if (index >= nodes.size()) {
        throw std::out_of_range("index out of range for group node");
    }
    auto group = std::dynamic_pointer_cast<group_node>(nodes[index]);
    if (!group) {
        std::stringstream ss;
        nodes[index]->dump(ss, "\t", true, true);
        throw std::runtime_error(
            "node at index " + std::to_string(index)
            + " is not a group node: \n" + ss.str()
        );
    }
    if (group->nodes.empty()) {
        throw std::runtime_error(
            "cannot squeeze empty group node at index " + std::to_string(index)
        );
    }
    const auto ph = std::make_shared<placeholder_node>();
    ph->src = const_cast<reader*>(&src);
    ph->limit = group->limit;
    ph->kind = group->kind;
    if (auto wn = std::dynamic_pointer_cast<wrapped_node>(group)) {
        ph->start = wn->nodes[0]->get_start();
    } else {
        ph->start = group->get_start();
    }
    ph->full_size = group->full_size;
    ph->fixed_size = 1;
    nodes[index] = ph;
}

void group_node::pop_back() {
    if (nodes.empty()) {
        throw std::runtime_error("cannot pop from empty group node");
    }
    fixed_size -= nodes.back()->fixed_size;
    full_size -= nodes.back()->full_size;
    nodes.pop_back();
    if (nodes.empty()) {
        fixed_size = 1;
        full_size = 1;
    }
}

const position& wrapped_node::get_start() const { return start; }

callexp_node::callexp_node(const token& name) { value = name; }

void callexp_node::set_paren(ast_node_ptr p) {
    paren = std::move(p);
    has_paren = true;
    fixed_size += paren->fixed_size;
    full_size += paren->full_size;
}

void callexp_node::dump(
    std::ostream& os, const std::string& prefix, bool is_last, bool full
) const {
    os << prefix << (is_last ? "`-" : "|-") << "CallExpr\n";
    const std::string child_prefix = prefix + (is_last ? "  " : "| ");
    value.dump(os, child_prefix, !has_paren);
    if (has_paren) {
        paren->dump(os, child_prefix, true, full);
    }
}

fundecl_node::fundecl_node(const callexp_ptr& proto)
    : callexp_node(proto ? proto->value : token {}) {
    if (proto) {
        has_paren = proto->has_paren;
        paren = proto->paren;
        fixed_size = proto->fixed_size;
        full_size = proto->full_size;
    }
}

void fundecl_node::set_body(ast_node_ptr b) {
    body = std::move(b);
    has_body = true;
    fixed_size += body->fixed_size;
    full_size += body->full_size;
}

void fundecl_node::dump(
    std::ostream& os, const std::string& prefix, bool is_last, bool full
) const {
    os << prefix << (is_last ? "`-" : "|-") << "FunctionDecl\n";
    const std::string child_prefix = prefix + (is_last ? "  " : "| ");
    value.dump(os, child_prefix, !(has_paren || has_body));
    if (has_paren) {
        paren->dump(os, child_prefix, !has_body, full);
    }
    if (has_body) {
        body->dump(os, child_prefix, true, full);
    }
}

control_node::control_node(const token& name) { value = name; }

void control_node::set_body(ast_node_ptr b) {
    body = std::move(b);
    has_body = true;
    fixed_size += body->fixed_size;
    full_size += body->full_size;
}

void control_node::dump(
    std::ostream& os, const std::string& prefix, bool is_last, bool full
) const {
    os << prefix << (is_last ? "`-" : "|-") << "Control(" << value.word << ")";
    if (!full) {
        os << " <" << fixed_size << "/" << full_size << " nodes>";
    }
    os << '\n';
    if (has_body) {
        const std::string child_prefix = prefix + (is_last ? "  " : "| ");
        body->dump(os, child_prefix, true, full);
    }
}

condition_node::condition_node(const token& name)
    : control_node(name) {
    if (name.word == "for" || name.word == "while") {
        is_loop = true;
    }
}

void condition_node::set_paren(ast_node_ptr p) {
    paren = std::move(p);
    has_paren = true;
    fixed_size += paren->fixed_size;
    full_size += paren->full_size;
}

void condition_node::dump(
    std::ostream& os, const std::string& prefix, bool is_last, bool full
) const {
    os << prefix << (is_last ? "`-" : "|-") << (is_loop ? "Loop" : "Condition")
       << '(' << value.word << ")";
    if (!full) {
        os << " <" << fixed_size << "/" << full_size << " nodes>";
    }
    os << '\n';
    const std::string child_prefix = prefix + (is_last ? "  " : "| ");
    if (has_paren) {
        paren->dump(os, child_prefix, !has_body, full);
    }
    if (has_body) {
        body->dump(os, child_prefix, true, full);
    }
}

jump_node::jump_node(const token& name)
    : control_node(name) { }

void jump_node::dump(
    std::ostream& os, const std::string& prefix, bool is_last, bool full
) const {
    control_node::dump(os, prefix, is_last, full);
}

unary_node::unary_node(
    const token& op, ast_node_ptr expr, bool is_prefix, int priority
)
    : op(op)
    , expr(std::move(expr))
    , is_prefix(is_prefix)
    , priority(priority) {
    fixed_size += this->expr->fixed_size;
    full_size += this->expr->full_size;
}

const position& unary_node::get_start() const { return op.pos; }

void unary_node::dump(
    std::ostream& os, const std::string& prefix, bool is_last, bool full
) const {
    os << prefix << (is_last ? "`-" : "|-") << "Unary(" << op.word
       << (is_prefix ? ", prefix" : ", postfix") << ", prio=" << priority
       << ")\n";
    const std::string child_prefix = prefix + (is_last ? "  " : "| ");
    expr->dump(os, child_prefix, true, full);
}

binary_node::binary_node(
    const token& op, ast_node_ptr lhs, ast_node_ptr rhs, int priority
)
    : op(op)
    , lhs(std::move(lhs))
    , rhs(std::move(rhs))
    , priority(priority) {
    fixed_size += this->lhs->fixed_size + this->rhs->fixed_size;
    full_size += this->lhs->full_size + this->rhs->full_size;
}

const position& binary_node::get_start() const { return op.pos; }

void binary_node::dump(
    std::ostream& os, const std::string& prefix, bool is_last, bool full
) const {
    os << prefix << (is_last ? "`-" : "|-") << "Binary(" << op.word
       << ", prio=" << priority << ")\n";
    const std::string child_prefix = prefix + (is_last ? "  " : "| ");
    lhs->dump(os, child_prefix, false, full);
    rhs->dump(os, child_prefix, true, full);
}

ternary_node::ternary_node(
    const token& qmark, const token& colon, ast_node_ptr cond,
    ast_node_ptr left, ast_node_ptr right, int priority
)
    : qmark(qmark)
    , colon(colon)
    , cond(std::move(cond))
    , left(std::move(left))
    , right(std::move(right))
    , priority(priority) {
    fixed_size += this->cond->fixed_size + this->left->fixed_size
        + this->right->fixed_size;
    full_size += this->cond->full_size + this->left->full_size
        + this->right->full_size;
}

const position& ternary_node::get_start() const { return qmark.pos; }

void ternary_node::dump(
    std::ostream& os, const std::string& prefix, bool is_last, bool full
) const {
    os << prefix << (is_last ? "`-" : "|-") << "Ternary(?:) prio=" << priority
       << "\n";
    const std::string child_prefix = prefix + (is_last ? "  " : "| ");
    cond->dump(os, child_prefix, false, full);
    left->dump(os, child_prefix, false, full);
    right->dump(os, child_prefix, true, full);
}
