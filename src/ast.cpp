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

void token_node::dump(
    std::ostream& os, const std::string& prefix, const bool is_last, bool
) const {
    os << prefix << (is_last ? "`-" : "|-") << "TokenNode\n";
    value.dump(os, prefix + (is_last ? "  " : "| "), true);
}

void placeholder_node::dump(
    std::ostream& os, const std::string& prefix, bool is_last, bool full
) const {
    if (full && src != nullptr) {
        const auto position = src->get_position();
        src->jump_to_position(value.pos);
        grouper g { *src };
        auto group = g.parse_group(kind);
        group->dump(os, prefix, is_last, full);
        src->jump_to_position(position);
    } else {
        os << prefix << (is_last ? "`-" : "|-") << "PlaceholderNode ["
           << full_size << " nested nodes]\n";
    }
}

static const char* group_kind_name(const group_kind k) noexcept {
    static constexpr const char* names[]
        = { "file", "body", "list", "paren", "command", "item", "key", "halt" };
    return names[static_cast<size_t>(k)];
}

void group_node::append(ast_node_ptr node) {
    fixed_size += node->fixed_size;
    full_size += node->full_size;
    if (node->fixed_size > 1) {
        weights.emplace(node->fixed_size, size());
    }
    nodes.push_back(std::move(node));
    while (!weights.empty() && full_size > limit) {
        auto [weight, index] = weights.top();
        weights.pop();
        fixed_size += 1 - weight;
        squeeze(index);
    }
    if (full_size > limit) {
        throw std::runtime_error("limit is too small for group node");
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
) const noexcept {
    if (kind != group_kind::file) {
        os << prefix << (is_last ? "`-" : "|-");
    }
    os << "Group(" << group_kind_name(kind) << ")";
    // if (placeholder) {
    //     os << " <placeholder with " << full_size << " nested nodes>";
    //     if (full) {
    //         // extract squeezed
    //     }
    // }
    // if (!full) {
    //     os << " <" << fixed_size << "/" << full_size << " nested nodes>";
    // }
    os << "\n";
    const std::string child_prefix
        = prefix + (kind != group_kind::file ? (is_last ? "  " : "| ") : "");
    for (size_t i = 0; i < nodes.size(); ++i) {
        nodes[i]->dump(os, child_prefix, i + 1 == nodes.size(), full);
    }
}

void group_node::squeeze(const size_t index) {
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
    const auto* first = dynamic_cast<const token_node*>(group->first());
    if (!first) {
        throw std::runtime_error(
            "cannot squeeze group node with non-token first element"
        );
    }
    const auto ph = std::make_shared<placeholder_node>();
    ph->kind = group->kind;
    ph->value = first->value;
    ph->full_size = group->full_size;
    ph->fixed_size = 1;
    nodes[index] = ph;
}
