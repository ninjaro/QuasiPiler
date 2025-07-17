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

#include <array>
#include <string_view>
#include <unordered_map>

const std::unordered_map<std::string, std::pair<int, bool>>
    expression::binary_ops = { { "=", { 1, true } },    { "+=", { 1, true } },
                               { "-=", { 1, true } },   { "*=", { 1, true } },
                               { "/=", { 1, true } },   { "%=", { 1, true } },
                               { "^=", { 1, true } },   { "|=", { 1, true } },
                               { "&=", { 1, true } },   { "<<=", { 1, true } },
                               { ">>=", { 1, true } },  { "||", { 3, false } },
                               { "&&", { 4, false } },  { "|", { 5, false } },
                               { "^", { 6, false } },   { "&", { 7, false } },
                               { "==", { 8, false } },  { "!=", { 8, false } },
                               { "<", { 9, false } },   { "<=", { 9, false } },
                               { ">", { 9, false } },   { ">=", { 9, false } },
                               { "<<", { 10, false } }, { ">>", { 10, false } },
                               { "+", { 11, false } },  { "-", { 11, false } },
                               { "*", { 12, false } },  { "/", { 12, false } },
                               { "%", { 12, false } } };

const std::unordered_map<std::string, int> expression::prefix_ops
    = { { "+", 13 }, { "-", 13 },  { "!", 13 },
        { "~", 13 }, { "++", 13 }, { "--", 13 } };

const std::unordered_map<std::string, int> expression::postfix_ops
    = { { "++", 14 }, { "--", 14 } };

token expression::make_token(const token_node& tn, std::string_view word) {
    token t = tn.value;
    t.word = word;
    return t;
}

bool expression::match_op(
    const std::vector<ast_node_ptr>& nodes, size_t pos, std::string_view op
) {
    if (pos + op.size() > nodes.size()) {
        return false;
    }
    size_t i = 0;
    for (char c : op) {
        const auto tn = std::dynamic_pointer_cast<token_node>(nodes[pos + i]);
        if (!tn || tn->value.word != std::string(1, c)) {
            return false;
        }
        ++i;
    }
    return true;
}

std::vector<expression::item>
expression::make_items(const std::vector<ast_node_ptr>& nodes) {
    std::vector<item> res;
    for (size_t i = 0; i < nodes.size();) {
        if (auto tn = std::dynamic_pointer_cast<token_node>(nodes[i])) {
            if (tn->value.kind == token_kind::special_character
                || tn->value.kind == token_kind::separator) {
                static constexpr std::array<std::string_view, 20> multi_ops {
                    "<<=", ">>=", "++", "--", "+=", "-=", "*=",
                    "/=",  "%=",  "^=", "|=", "&=", "==", "!=",
                    "<=",  ">=",  "<<", ">>", "&&", "||"
                };

                std::string_view op = tn->value.word;
                size_t len = 1;
                for (auto candidate : multi_ops) {
                    if (match_op(nodes, i, candidate)) {
                        op = candidate;
                        len = candidate.size();
                        break;
                    }
                }

                token tok = make_token(*tn, op);
                res.push_back({ true, tok, {} });
                i += len;
                continue;
            }
        }
        res.push_back({ false, {}, nodes[i] });
        ++i;
    }
    return res;
}

ast_node_ptr expression::parse_expression(
    std::vector<item>& items, size_t& idx, const int min_prec
) {
    auto left = parse_prefix(items, idx);
    while (idx < items.size()) {
        if (!items[idx].is_op) {
            break;
        }
        auto op = items[idx].tok.word;
        if (op == "?") {
            int prec = 2;
            if (prec < min_prec) {
                break;
            }
            token qtok = items[idx].tok;
            ++idx;
            auto middle = parse_expression(items, idx, 0);
            if (idx >= items.size() || !items[idx].is_op
                || items[idx].tok.word != ":") {
                throw std::runtime_error("expected ':' in ternary expression");
            }
            token ctok = items[idx].tok;
            ++idx;
            auto right = parse_expression(items, idx, prec);
            left = std::make_shared<ternary_node>(
                qtok, ctok, left, middle, right, prec
            );
            continue;
        }
        auto it = binary_ops.find(op);
        if (it == binary_ops.end()) {
            break;
        }
        int prec = it->second.first;
        const bool right = it->second.second;
        if (prec < min_prec) {
            break;
        }
        token optok = items[idx].tok;
        ++idx;
        auto rhs = parse_expression(items, idx, prec + (right ? 0 : 1));
        left = std::make_shared<binary_node>(optok, left, rhs, prec);
    }
    return left;
}

ast_node_ptr expression::parse_prefix(std::vector<item>& items, size_t& idx) {
    if (idx < items.size() && items[idx].is_op) {
        if (const auto it = prefix_ops.find(items[idx].tok.word);
            it != prefix_ops.end()) {
            token tok = items[idx].tok;
            int prec = it->second;
            ++idx;
            auto operand = parse_prefix(items, idx);
            return std::make_shared<unary_node>(tok, operand, true, prec);
        }
    }
    if (idx >= items.size()) {
        throw std::runtime_error("unexpected end");
    }
    auto node = items[idx].node;
    ++idx;
    while (idx < items.size() && items[idx].is_op) {
        auto it = postfix_ops.find(items[idx].tok.word);
        if (it == postfix_ops.end()) {
            break;
        }
        token tok = items[idx].tok;
        int prec = it->second;
        ++idx;
        node = std::make_shared<unary_node>(tok, node, false, prec);
    }
    return node;
}
