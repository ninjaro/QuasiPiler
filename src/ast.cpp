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

token::~token() = default;

static const char* token_kind_name(const token_kind k) noexcept {
    static constexpr const char* names[]
        = { "eof",     "open_bracket", "close_bracket",    "separator",
            "keyword", "string",       "comment",          "whitespace",
            "integer", "floating",     "special_character" };
    return names[static_cast<size_t>(k)];
}

void token::dump(
    std::ostream& os, const std::string& prefix, const bool is_last
) const noexcept {
    os << prefix << (is_last ? "`-" : "|-") << "Token(" << token_kind_name(kind)
       << ") <" << line << ":" << column << ">(\"" << word << "\")\n";
}

void token::dump(std::ostream& os) const noexcept { dump(os, "", true); }

ast_node::~ast_node() = default;

ast_node const* ast_node::first() const noexcept { return this; }

bool ast_node::empty() const noexcept { return true; }

void ast_node::dump(
    std::ostream& os, const std::string& prefix, const bool is_last, bool
) const noexcept {
    os << prefix << (is_last ? "`-" : "|-") << "Null\n";
}

void ast_node::dump(std::ostream& os, const bool full) const noexcept {
    dump(os, "", true, full);
}

void ast_node::dump(std::ostream& os) const noexcept { dump(os, true); }

void ast_node::placeholde() {
    throw std::runtime_error(
        "cannot placeholde a base ast_node, use derived classes"
    );
}

bool token_node::empty() const noexcept { return false; }

void token_node::dump(
    std::ostream& os, const std::string& prefix, const bool is_last, bool
) const noexcept {
    os << prefix << (is_last ? "`-" : "|-") << "TokenNode\n";
    value.dump(os, prefix + (is_last ? "  " : "| "), true);
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
        nodes[index]->placeholde();
    }
    if (full_size > limit) {
        throw std::runtime_error("limit is too small for group node");
    }
}

bool group_node::empty() const noexcept { return size() == 0; }

size_t group_node::size() const noexcept { return nodes.size(); }

ast_node const* group_node::first() const noexcept {
    if (size() == 1) {
        return nodes[0]->first();
    }
    return ast_node::first();
}

void group_node::dump(
    std::ostream& os, const std::string& prefix, const bool is_last,
    const bool full
) const noexcept {
    if (kind != group_kind::file) {
        os << prefix << (is_last ? "`-" : "|-");
    }
    os << "Group(" << group_kind_name(kind) << ")";
    if (placeholder) {
        os << " <placeholder with " << full_size << " nested nodes>";
        if (full) {
            // extract squeezed
        }
    }
    if (!full) {
        os << " <" << fixed_size << "/" << full_size << " nested nodes>";
    }
    os << "\n";
    const std::string child_prefix
        = prefix + (kind != group_kind::file ? (is_last ? "  " : "| ") : "");
    for (size_t i = 0; i < nodes.size(); ++i) {
        nodes[i]->dump(os, child_prefix, i + 1 == nodes.size(), full);
    }
}

void group_node::placeholde() {
    placeholder = true;
    fixed_size = 1;
}
