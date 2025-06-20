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

#include "grouper.hpp"

grouper::grouper(reader& r, const size_t limit)
    : src(r)
    , limit(limit) {
    if (limit < 2) {
        throw make_error("minimum limit is 2");
    }
}

group_ptr grouper::parse_group(const group_kind kind) {
    auto group = std::make_shared<group_node>();
    group->limit = limit;
    auto top = std::make_shared<group_node>();
    top->limit = limit;
    while (true) {
        const token current = peek();
        if (current.kind == token_kind::separator) {
            if (current.word == ":") {
                top->kind = group_kind::key;
            } else if (current.word == ",") {
                top->kind = group_kind::item;
            } else if (current.word == ";") {
                top->kind = group_kind::command;
            } else {
                throw make_error(
                    "unexpected separator: " + current.word
                ); // todo: top->dump()
            }
            if (top->kind == kind) {
                if (group->empty()) {
                    return top;
                }
                try {
                    group->append(top);
                } catch (const std::runtime_error&) {
                    throw make_error("group limit exceeded");
                }
                throw make_error("wrong group kind"); // todo: group->dump()
            }
            try {
                group->append(top);
            } catch (const std::runtime_error&) {
                throw make_error("group limit exceeded");
            }
            top = std::make_shared<group_node>();
            top->limit = limit;
        } else if (current.kind == token_kind::open_bracket) {
            group_kind sub_kind;
            if (current.word == "{") {
                sub_kind = group_kind::body;
            } else if (current.word == "[") {
                sub_kind = group_kind::list;
            } else if (current.word == "(") {
                sub_kind = group_kind::paren;
            } else {
                throw make_error(
                    "unexpected open bracket: " + current.word
                ); // todo: top->dump()
            }
            try {
                top->append(parse_group(sub_kind));
            } catch (const std::runtime_error&) {
                throw make_error("group limit exceeded");
            }
        } else if (current.kind == token_kind::close_bracket
                   || current.kind == token_kind::eof) {
            try {
                group->append(top);
            } catch (const std::runtime_error&) {
                throw make_error("group limit exceeded");
            }
            top = std::make_shared<group_node>();
            top->limit = limit;
            if (current.kind == token_kind::eof) {
                group->kind = group_kind::file;
            } else if (current.word == "}") {
                group->kind = group_kind::body;
            } else if (current.word == "]") {
                group->kind = group_kind::list;
            } else if (current.word == ")") {
                group->kind = group_kind::paren;
            } else {
                throw make_error(
                    "unexpected close bracket: " + current.word
                ); // todo: group->dump()
            }
            if (group->kind == kind) {
                return group;
            }
            throw make_error("wrong group kind");
        } else {
            auto tk = std::make_shared<token_node>();
            tk->value = current;
            try {
                top->append(tk);
            } catch (const std::runtime_error&) {
                throw make_error("group limit exceeded");
            }
        }
    }
}

token grouper::peek() const {
    token current;
    do {
        src.next_token(current);
    } while (current.kind == token_kind::whitespace
             || current.kind == token_kind::comment);
    return current;
}

std::runtime_error grouper::make_error(
    const std::string& message, const std::source_location& location
) const {
    std::ostringstream oss;
    oss << "[Grouper-Error] " << message << ". " << std::endl;
    // oss << "during parsing of group:" << std::endl;
    // obj->dump(oss, "\t", true, false);
    // oss << "\n";
    oss << "in file: " << location.file_name() << '(' << location.line() << ':'
        << location.column() << ") `" << location.function_name() << "`"
        << std::endl;
    try {
        src.interrupt();
    } catch (const std::runtime_error& e) {
        oss << e.what();
    }
    return std::runtime_error(oss.str());
}
