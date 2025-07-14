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

group_ptr grouper::parse(const group_kind kind) {
    group_ptr group, result;
    if (kind == group_kind::body || kind == group_kind::list
        || kind == group_kind::paren) {
        group = std::make_shared<wrapped_node>();
        result = std::make_shared<wrapped_node>();
    } else {
        group = std::make_shared<group_node>();
        result = std::make_shared<group_node>();
    }
    group->limit = limit;
    group->kind = kind;
    result->limit = limit;
    result->kind = kind;
    parse_group(kind, group);
    identify(group, result);
    return result;
}

void grouper::parse_group(const group_kind kind, group_ptr& group) {
    auto top = std::make_shared<group_node>();
    top->limit = limit;
    while (true) {
        peek();
        if (current.kind == token_kind::separator) {
            if (current.word == ":") {
                top->kind = group_kind::key;
            } else if (current.word == ",") {
                top->kind = group_kind::item;
            } else if (current.word == ";") {
                top->kind = group_kind::command;
            } else {
                throw make_error("unexpected separator: " + current.word, top);
            }
            if (top->kind == kind) {
                if (group->empty()) {
                    group = top;
                    return;
                }
                try {
                    group->append(top, src);
                } catch (const std::runtime_error& e) {
                    std::stringstream msg;
                    msg << e.what() << ". group limit exceeded";
                    throw make_error(msg.str());
                }
                throw make_error(
                    "wrong group kind. expected: "
                        + std::string(group_kind_name(kind))
                        + ", got: " + group_kind_name(group->kind),
                    group
                );
            }
            try {
                group->append(top, src);
            } catch (const std::runtime_error& e) {
                std::stringstream msg;
                msg << e.what() << ". group limit exceeded";
                throw make_error(msg.str());
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
                    "unexpected open bracket: " + current.word, top
                );
            }
            try {
                wrapped_ptr wn = std::make_shared<wrapped_node>();
                wn->start = pos;
                wn->limit = limit;
                wn->kind = sub_kind;
                auto gr = std::dynamic_pointer_cast<group_node>(wn);
                parse_group(sub_kind, gr);
                top->append(gr, src);
            } catch (const std::runtime_error& e) {
                std::stringstream msg;
                msg << e.what() << ". group limit exceeded";
                throw make_error(msg.str());
            }
        } else if (current.kind == token_kind::close_bracket
                   || current.kind == token_kind::eof) {
            try {
                group->append(top, src);
            } catch (const std::runtime_error& e) {
                std::stringstream msg;
                msg << e.what() << ". group limit exceeded";
                throw make_error(msg.str());
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
                    "unexpected close bracket: " + current.word, group
                );
            }
            if (kind == group_kind::halt) {
                reuse = true;
                return;
            }
            if (group->kind == kind) {
                return;
            }
            throw make_error(
                "wrong group kind. expected: "
                    + std::string(group_kind_name(kind))
                    + ", got: " + group_kind_name(group->kind),
                group
            );
        } else {
            auto tk = std::make_shared<token_node>();
            tk->value = current;
            try {
                top->append(tk, src);
            } catch (const std::runtime_error& e) {
                std::stringstream msg;
                msg << e.what() << ". group limit exceeded";
                throw make_error(msg.str());
            }
        }
    }
}

void grouper::peek() {
    if (reuse) {
        reuse = false;
        return;
    }
    do {
        pos = src.get_position();
        src.next_token(current);
    } while (current.kind == token_kind::whitespace
             || current.kind == token_kind::comment);
}

void grouper::identify(const group_ptr& group, const group_ptr& result) const {
    bool wait_for_condition = false;
    bool wait_for_body = false;

    for (auto& node : group->nodes) {
        bool is_group = false;
        group_kind kind {};

        if (auto sub = std::dynamic_pointer_cast<group_node>(node)) {
            group_ptr inode;
            kind = sub->kind;
            if (kind == group_kind::body || kind == group_kind::list
                || kind == group_kind::paren) {
                inode = std::make_shared<wrapped_node>();
            } else {
                inode = std::make_shared<group_node>();
            }
            inode->limit = limit;
            inode->kind = kind;
            identify(sub, inode);
            node = inode;
            is_group = true;
        }
        if (wait_for_condition && (!is_group || kind != group_kind::paren)) {
            throw make_error("expected condition after control keyword");
        }
        if (is_group) {
            if (!result->empty()) {
                auto top = result->nodes.back();
                result->pop_back();
                if (auto tok = std::dynamic_pointer_cast<token_node>(top)) {
                    if (kind == group_kind::paren) {
                        if (auto cond
                            = std::dynamic_pointer_cast<condition_node>(tok)) {
                            cond->set_paren(node);
                            result->append(cond, src);
                            wait_for_condition = false;
                            wait_for_body = true;
                            continue;
                        }
                        auto callexp
                            = std::make_shared<callexp_node>(tok->value);
                        callexp->set_paren(node);
                        result->append(callexp, src);
                        continue;
                    }
                    if (kind == group_kind::body) {
                        if (auto ctrl
                            = std::dynamic_pointer_cast<control_node>(tok)) {
                            wait_for_body = false;
                            ctrl->set_body(node);
                            result->append(ctrl, src);
                            continue;
                        }
                        if (auto callexp
                            = std::dynamic_pointer_cast<callexp_node>(tok)) {
                            auto fundecl
                                = std::make_shared<fundecl_node>(callexp);
                            fundecl->set_body(node);
                            result->append(fundecl, src);
                            continue;
                        }
                    }
                }
                result->append(top, src);
                // continue;
            }
        }
        if (auto tok = std::dynamic_pointer_cast<token_node>(node)) {
            if (tok->value.kind == token_kind::keyword) {
                const auto& w = tok->value.word;
                if (w == "if" || w == "elif" || w == "while" || w == "for"
                    || w == "catch") {
                    wait_for_condition = true;
                    auto cond = std::make_shared<condition_node>(tok->value);
                    result->append(cond, src);
                    continue;
                }
                if (w == "else" || w == "try" || w == "finally") {
                    wait_for_body = true;
                    auto ctrl = std::make_shared<control_node>(tok->value);
                    result->append(ctrl, src);
                    continue;
                }
                if (w == "return" || w == "continue" || w == "break") {
                    auto jmp = std::make_shared<jump_node>(tok->value);
                    result->append(jmp, src);
                    wait_for_body = (w == "return" || w == "throw");
                    continue;
                }
            }
        }
        result->append(node, src);
    }
    if (wait_for_body) {
        auto body = std::make_shared<group_node>();
        body->limit = limit;
        while (!result->empty()) {
            auto top = result->nodes.back();
            result->pop_back();
            if (auto tok = std::dynamic_pointer_cast<token_node>(top)) {
                if (auto ctrl = std::dynamic_pointer_cast<control_node>(tok)) {
                    ctrl->set_body(body);
                    result->append(ctrl, src);
                    break;
                }
                if (auto callexp
                    = std::dynamic_pointer_cast<callexp_node>(tok)) {
                    auto fundecl = std::make_shared<fundecl_node>(callexp);
                    fundecl->set_body(body);
                    result->append(fundecl, src);
                    break;
                }
            }
            body->append(top, src);
        }
    }
}

std::runtime_error grouper::make_error(
    const std::string& message, const group_ptr& context,
    const std::source_location& location
) const {
    std::ostringstream oss;
    oss << "[Grouper-Error] " << message << ". " << std::endl;
    if (context) {
        oss << "during parsing of group:" << std::endl;
        context->dump(oss, "\t", true, false);
    }
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
