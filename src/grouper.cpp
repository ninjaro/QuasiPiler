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
            if (append_command(group, top, kind)) {
                return;
            }
        } else if (current.kind == token_kind::open_bracket) {
            append_wrapped(top);
        } else if (current.kind == token_kind::close_bracket
                   || current.kind == token_kind::eof) {
            close_wrapped(group, top, kind);
            return;
        } else {
            auto tk = std::make_shared<token_node>();
            tk->value = current;
            append(top, tk);
        }
    }
}

void grouper::append(
    const group_ptr& parent, const ast_node_ptr& node,
    const std::source_location& location
) const {
    try {
        parent->append(node, src);
    } catch (const std::runtime_error& e) {
        std::stringstream msg;
        msg << "failed to append node: \n";
        node->dump(msg, "", true, false);
        msg << e.what();
        throw make_error(msg.str(), parent, location);
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

group_ptr grouper::identify_subgroup(const group_ptr& group) const {
    group_ptr inode;
    const auto kind = group->kind;
    if (kind == group_kind::body || kind == group_kind::list
        || kind == group_kind::paren) {
        inode = std::make_shared<wrapped_node>();
    } else {
        inode = std::make_shared<group_node>();
    }
    inode->limit = limit;
    inode->kind = kind;
    identify(group, inode);
    return inode;
}

bool grouper::handle_chain(
    const group_ptr& result, const group_ptr& inode
) const {
    const auto first = inode->nodes.front();
    std::string kw;
    if (const auto ctrl = std::dynamic_pointer_cast<control_node>(first)) {
        kw = ctrl->value.word;
    } else if (const auto cond
               = std::dynamic_pointer_cast<condition_node>(first)) {
        kw = cond->value.word;
    }
    if (kw == "else" || kw == "elif" || kw == "catch" || kw == "finally") {
        if (result->empty()) {
            throw make_error("orphan secondary keyword: " + kw, inode);
        }
        const auto prev
            = std::dynamic_pointer_cast<group_node>(result->nodes.back());
        if (!prev || prev->nodes.empty() || prev->kind != group_kind::command) {
            throw make_error("invalid predecessor for keyword: " + kw, inode);
        }
        const auto last = prev->nodes.back();
        std::string prev_kw;
        if (const auto ctrl = std::dynamic_pointer_cast<control_node>(last)) {
            prev_kw = ctrl->value.word;
        } else if (const auto cond
                   = std::dynamic_pointer_cast<condition_node>(last)) {
            prev_kw = cond->value.word;
        } else {
            throw make_error("invalid predecessor for keyword: " + kw, inode);
        }
        bool allowed = false;
        if (kw == "else" || kw == "elif") {
            allowed = (prev_kw == "if" || prev_kw == "elif");
        } else if (kw == "catch" || kw == "finally") {
            allowed = (prev_kw == "try" || prev_kw == "catch");
        }
        if (!allowed) {
            throw make_error(
                "unexpected keyword order: " + prev_kw + " before " + kw, inode
            );
        }
        result->pop_back();
        for (auto& ch : inode->nodes) {
            append(prev, ch);
        }
        append(result, prev);
        return true;
    }
    return false;
}

bool grouper::append_group(
    const group_ptr& result, const ast_node_ptr& node, bool& wait_for_condition,
    bool& wait_for_body, const group_kind kind
) const {
    if (!result->empty()) {
        const auto top = result->nodes.back();
        result->pop_back();
        if (const auto cond = std::dynamic_pointer_cast<condition_node>(top);
            cond && kind == group_kind::paren) {
            cond->set_paren(node);
            append(result, cond);
            wait_for_condition = false;
            wait_for_body = true;
            return true;
        }
        if (const auto ctrl = std::dynamic_pointer_cast<control_node>(top);
            ctrl && kind == group_kind::body) {
            wait_for_body = false;
            ctrl->set_body(node);
            append(result, ctrl);
            return true;
        }
        if (const auto callexp = std::dynamic_pointer_cast<callexp_node>(top);
            callexp && kind == group_kind::body) {
            const auto fundecl = std::make_shared<fundecl_node>(callexp);
            fundecl->set_body(node);
            append(result, fundecl);
            return true;
        }
        const auto tok = std::dynamic_pointer_cast<token_node>(top);
        if (tok && tok->value.kind == token_kind::keyword
            && kind == group_kind::paren) {
            const auto callexp = std::make_shared<callexp_node>(tok->value);
            callexp->set_paren(node);
            append(result, callexp);
            return true;
        }
        append(result, top);
    }
    return false;
}

void grouper::identify_body(const group_ptr& group) const {
    const auto body = std::make_shared<group_node>();
    body->limit = limit;
    while (!group->empty()) {
        auto top = group->nodes.back();
        group->pop_back();
        if (auto tok = std::dynamic_pointer_cast<token_node>(top)) {
            if (const auto ctrl
                = std::dynamic_pointer_cast<control_node>(tok)) {
                ctrl->set_body(body);
                append(group, ctrl);
                break;
            }
            if (auto callexp = std::dynamic_pointer_cast<callexp_node>(tok)) {
                const auto fundecl = std::make_shared<fundecl_node>(callexp);
                fundecl->set_body(body);
                append(group, fundecl);
                break;
            }
        }
        append(body, top);
    }
}

void grouper::identify(const group_ptr& group, const group_ptr& result) const {
    bool wait_for_condition = false;
    bool wait_for_body = false;

    for (auto& node : group->nodes) {
        bool is_group = false;
        group_kind kind {};

        if (auto sub = std::dynamic_pointer_cast<group_node>(node)) {
            kind = sub->kind;
            auto inode = identify_subgroup(sub);
            node = inode;
            is_group = true;

            if ((kind == group_kind::halt || kind == group_kind::command)
                && !inode->nodes.empty()) {
                if (handle_chain(result, inode)) {
                    continue;
                }
            }
        }
        if (wait_for_condition && (!is_group || kind != group_kind::paren)) {
            throw make_error("expected condition after control keyword");
        }
        if (is_group) {
            if (append_group(
                    result, node, wait_for_condition, wait_for_body, kind
                )) {
                continue;
            }
        }
        if (const auto tok = std::dynamic_pointer_cast<token_node>(node)) {
            if (tok->value.kind == token_kind::keyword) {
                const auto& w = tok->value.word;
                if (w == "if" || w == "elif" || w == "while" || w == "for"
                    || w == "catch") {
                    wait_for_condition = true;
                    auto cond = std::make_shared<condition_node>(tok->value);
                    append(result, cond);
                    continue;
                }
                if (w == "else" || w == "try" || w == "finally") {
                    wait_for_body = true;
                    auto ctrl = std::make_shared<control_node>(tok->value);
                    append(result, ctrl);
                    continue;
                }
                if (w == "return" || w == "continue" || w == "break"
                    || w == "goto") {
                    auto jmp = std::make_shared<jump_node>(tok->value);
                    append(result, jmp);
                    wait_for_body = (w != "continue" && w != "break");
                    continue;
                }
            }
        }
        append(result, node);
    }
    if (wait_for_body) {
        identify_body(result);
    }
}

bool grouper::append_command(
    group_ptr& group, group_ptr& top, const group_kind kind
) const {
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
            return true;
        }
        append(group, top);
        throw make_error(
            "wrong group kind. expected: " + std::string(group_kind_name(kind))
                + ", got: " + group_kind_name(group->kind),
            group
        );
    }
    append(group, top);
    top = std::make_shared<group_node>();
    top->limit = limit;
    return false;
}

void grouper::append_wrapped(const group_ptr& top) {
    group_kind sub_kind;
    if (current.word == "{") {
        sub_kind = group_kind::body;
    } else if (current.word == "[") {
        sub_kind = group_kind::list;
    } else if (current.word == "(") {
        sub_kind = group_kind::paren;
    } else {
        throw make_error("unexpected open bracket: " + current.word, top);
    }
    const auto wn = std::make_shared<wrapped_node>();
    wn->start = pos;
    wn->limit = limit;
    wn->kind = sub_kind;
    auto gr = std::dynamic_pointer_cast<group_node>(wn);
    parse_group(sub_kind, gr);
    append(top, gr);
}

void grouper::close_wrapped(
    const group_ptr& group, group_ptr& top, const group_kind kind
) {
    append(group, top);
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
        throw make_error("unexpected close bracket: " + current.word, group);
    }
    if (kind == group_kind::halt) {
        reuse = true;
        return;
    }
    if (group->kind == kind) {
        return;
    }
    throw make_error(
        "wrong group kind. expected: " + std::string(group_kind_name(kind))
            + ", got: " + group_kind_name(group->kind),
        group
    );
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
