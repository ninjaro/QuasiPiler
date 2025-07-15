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

#ifndef EXPRESSION_HPP
#define EXPRESSION_HPP

#include "ast.hpp"
#include <unordered_map>
#include <vector>

class expression {
public:
    struct item {
        bool is_op { false };
        token tok;
        ast_node_ptr node;
    };

    static std::vector<item> make_items(const std::vector<ast_node_ptr>& nodes);

    static ast_node_ptr
    parse_expression(std::vector<item>& items, size_t& idx, int min_prec);

    static ast_node_ptr parse_prefix(std::vector<item>& items, size_t& idx);

private:
    static token make_token(const token_node& tn, const std::string& word);

    static bool match_op(
        const std::vector<ast_node_ptr>& nodes, size_t pos,
        const std::string& op
    );

    static const std::unordered_map<std::string, std::pair<int, bool>>
        binary_ops;
    static const std::unordered_map<std::string, int> prefix_ops;
    static const std::unordered_map<std::string, int> postfix_ops;
};

#endif // EXPRESSION_HPP
