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

#include "reader.hpp"

#include <cassert>

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
       << ") <" << pos.line << ":" << pos.column << ">(\"" << word << "\")\n";
}

void token::dump(std::ostream& os) const noexcept { dump(os, "", true); }

reader::reader(
    const std::filesystem::path& path, const std::streamsize buffer_size
)
    : max_buffer_size(buffer_size) {
    ifs.open(path, std::ios::in | std::ios::binary);
    filename = path.string();
    if (!ifs.is_open()) {
        throw std::invalid_argument("cannot open file: " + filename);
    }
    ifs.seekg(0, std::ios::beg);
    file_offset = ifs.tellg();
    buffer.resize(static_cast<size_t>(max_buffer_size));
    reload_buffer();
}

reader::reader(std::string& data) noexcept
    : buffer(std::move(data)) {
    if (!buffer.empty()) {
        line = 0;
        column = 0;
    }
}

reader::~reader() {
    if (ifs.is_open()) {
        ifs.close();
    }
}

bool reader::is_valid() const noexcept {
    return !buffer.empty() && buffer_position < buffer.size();
}

char reader::peek_char() const noexcept { return buffer[buffer_position]; }

unsigned char reader::peek_uchar() const noexcept {
    return static_cast<unsigned char>(peek_char());
}

char reader::get_char() {
    const char current_char = peek_char();
    advance_char();
    return current_char;
}

void reader::advance_char() {
    assert(!buffer.empty());
    ++buffer_position;
    ++column;
    if (buffer_position >= buffer.size()) {
        reload_buffer();
    }
}

void reader::reload_buffer() {
    if (!ifs.is_open() || ifs.eof()) {
        return;
    }
    file_offset = ifs.tellg();
    buffer.resize(static_cast<size_t>(max_buffer_size));
    ifs.read(buffer.data(), max_buffer_size);
    const auto got = ifs.gcount();
    buffer.resize(static_cast<size_t>(got));
    buffer_position = 0;
}

void reader::read_whitespace(std::string& into) {
    into.clear();
    while (is_valid() && std::isspace(peek_uchar())) {
        if (peek_char() == '\n') {
            ++line;
            column = -1;
        }
        into += get_char();
    }
}

void reader::read_keyword(std::string& into) {
    into.clear();
    do {
        into += get_char();
    } while (is_valid() && (std::isalnum(peek_uchar()) || peek_char() == '_'));
}

void reader::read_comment(std::string& into) {
    assert(is_valid() && into.size() == 1 && into[0] == '/');
    into += get_char();
    const bool is_multiline = into.back() == '*';
    while (is_valid()) {
        const char current_char = get_char();
        if (is_multiline && current_char == '/' && into.back() == '*'
            && into.size() > 2) {
            into += current_char;
            return;
        }
        into += current_char;
        if (current_char == '\n') {
            ++line;
            column = -1;
            if (!is_multiline) {
                column = 0;
                break;
            }
        }
    }
    if (is_multiline) {
        throw make_error("missing closing comment delimiter");
    }
}

void reader::read_string(std::string& into) {
    into.clear();
    const char quote = get_char();
    bool escaped = false;
    while (is_valid()) {
        const char current_char = peek_char();
        if (escaped) {
            switch (current_char) {
            case '"':
                into += '"';
                break;
            case '\'':
                into += '\'';
                break;
            case '\\':
                into += '\\';
                break;
            case '/':
                into += '/';
                break;
            case 'b':
                into += '\b';
                break;
            case 'f':
                into += '\f';
                break;
            case 'n':
                into += '\n';
                break;
            case 'r':
                into += '\r';
                break;
            case 't':
                into += '\t';
                break;
            case 'u': {
                std::string hex;
                for (int i = 0; i < 4; ++i) {
                    advance_char();
                    if (!is_valid() || !std::isxdigit(peek_uchar())) {
                        throw make_error("invalid Unicode escape");
                    }
                    hex += peek_char();
                }
                // const int codepoint = std::stoi(hex, nullptr, 16);
                // std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t>
                //     converter;
                // into += converter.to_bytes(static_cast<char32_t>(codepoint));
                const auto codepoint
                    = static_cast<char32_t>(std::stoul(hex, nullptr, 16));
                auto encode = [](const char32_t cp) {
                    std::string out;
                    if (cp <= 0x7F) {
                        out += static_cast<char>(cp);
                    } else if (cp <= 0x7FF) {
                        out += static_cast<char>(0xC0 | (cp >> 6));
                        out += static_cast<char>(0x80 | (cp & 0x3F));
                    } else if (cp <= 0xFFFF) {
                        out += static_cast<char>(0xE0 | (cp >> 12));
                        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                        out += static_cast<char>(0x80 | (cp & 0x3F));
                    } else {
                        out += static_cast<char>(0xF0 | (cp >> 18));
                        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                        out += static_cast<char>(0x80 | (cp & 0x3F));
                    }
                    return out;
                };
                into += encode(codepoint);
                break;
            }
            default:
                throw make_error("invalid escape sequence");
            }
            escaped = false;
        } else if (current_char == '\\') {
            escaped = true;
        } else if (current_char == quote) {
            break;
        } else {
            into += current_char;
        }
        advance_char();
    }
    if (!is_valid() || peek_char() != quote) {
        throw make_error("missing closing quote");
    }
    advance_char();
}

token_kind reader::read_number(std::string& into) {
    into.clear();
    bool is_float = false;
    if (is_valid() && peek_char() == '0') {
        into += get_char();
        if (is_valid() && std::isdigit(peek_uchar())) {
            throw make_error("leading zeros not allowed");
        }
    } else if (is_valid() && std::isdigit(peek_uchar())) {
        do {
            into += get_char();
        } while (is_valid() && std::isdigit(peek_uchar()));
    } else {
        throw make_error("expected digit");
    }

    if (is_valid() && peek_char() == '.') {
        is_float = true;
        into += get_char();
        if (!is_valid() || !std::isdigit(peek_uchar())) {
            throw make_error("digit expected after decimal");
        }
        while (is_valid() && std::isdigit(peek_uchar())) {
            into += get_char();
        }
    }

    if (is_valid() && (peek_char() == 'e' || peek_char() == 'E')) {
        is_float = true;
        into += get_char();
        if (is_valid() && (peek_char() == '+' || peek_char() == '-')) {
            into += get_char();
        }
        if (!is_valid() || !std::isdigit(peek_uchar())) {
            throw make_error("digit expected after exponent");
        }
        while (is_valid() && std::isdigit(peek_uchar())) {
            into += get_char();
        }
    }
    return is_float ? token_kind::floating : token_kind::integer;
}

void reader::init_token(token& t) const noexcept {
    t.word.clear();
    t.pos = get_position();
}

std::runtime_error reader::make_error(
    const std::string& message, const std::source_location& location
) const {
    std::ostringstream oss;
    oss << "[Reader-Error] " << message << ". ";
#ifndef NDEBUG
    if (!ifs.is_open()) {
        oss << "no file open. ";
    }
    if (!is_valid()) {
        oss << filename << " is open. ";
        oss << "position is out of range. line: " << (line + 1)
            << ", column: " << (column + 1) << " exceeds available input. ";
    } else {
        const char current_char = peek_char();
        oss << "character '" << current_char
            << "' (ASCII: " << static_cast<unsigned>(current_char)
            << ") was found at line " << (line + 1) << ", column "
            << (column + 1) << ". ";
    }
    oss << "in file: " << location.file_name() << '(' << location.line() << ':'
        << location.column() << ") `" << location.function_name() << "`";
    oss << std::endl << buffer;
#endif
    return std::runtime_error(oss.str());
}

void reader::next_token(token& out) {
    init_token(out);
    out.kind = token_kind::special_character;

    if (!is_valid()) {
        out.kind = token_kind::eof;
        out.word.clear();
        return;
    }
    switch (const char current_char = peek_char()) {
    case '(':
    case '[':
    case '{':
        out.kind = token_kind::open_bracket;
        out.word = std::string(1, get_char());
        break;
    case ')':
    case ']':
    case '}':
        out.kind = token_kind::close_bracket;
        out.word = std::string(1, get_char());
        break;
    case ',':
    case ';':
    case ':':
        out.kind = token_kind::separator;
        out.word = std::string(1, get_char());
        break;
    case '/':
        out.word = std::string(1, get_char());
        if (is_valid() && (peek_char() == '/' || peek_char() == '*')) {
            read_comment(out.word);
            out.kind = token_kind::comment;
        }
        break;
    default:
        if (std::isalpha(static_cast<unsigned char>(current_char))
            || current_char == '_') {
            read_keyword(out.word);
            out.kind = token_kind::keyword;
        } else if (std::isdigit(static_cast<unsigned char>(current_char))) {
            out.kind = read_number(out.word);
        } else if (current_char == '"' || current_char == '\'') {
            read_string(out.word);
            out.kind = token_kind::string;
        } else if (std::isspace(static_cast<unsigned char>(current_char))) {
            read_whitespace(out.word);
            out.kind = token_kind::whitespace;
        } else {
            out.word = std::string(1, get_char());
        }
    }
}

void reader::jump_to_position(const position pos) {
    if (pos.offset < 0) {
        throw make_error("position is out of range");
    }
    if (!ifs.is_open()) {
        buffer_position = static_cast<size_t>(pos.offset);
        if (buffer_position > buffer.size()) {
            throw make_error("position is out of range");
        }
    } else {
        ifs.clear();
        ifs.seekg(pos.offset, std::ios::beg);
        reload_buffer();
    }
    this->line = pos.line;
    this->column = pos.column;
}

void reader::interrupt() {
    if (ifs.is_open() && ifs.eof()) {
        return;
    }
    throw make_error("interrupted");
}

position reader::get_position() const {
    return { file_offset + static_cast<std::streamoff>(buffer_position), line,
             column };
}
