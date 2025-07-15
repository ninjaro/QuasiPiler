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

#ifndef READER_HPP
#define READER_HPP

#include <filesystem>
#include <fstream>
#include <source_location>

struct position {
    std::streamoff offset;
    int line;
    int column;
};

enum class token_kind {
    eof,
    open_bracket,
    close_bracket,
    separator,
    keyword,
    string,
    comment,
    whitespace,
    integer,
    floating,
    special_character
};

struct token {
    token_kind kind;
    position pos;
    std::string word;

    virtual ~token();

    virtual void dump(
        std::ostream& os, const std::string& prefix, bool is_last
    ) const noexcept;

    void dump(std::ostream& os) const noexcept;
};

using token_ptr = std::shared_ptr<token>;

class reader {
public:
    explicit reader(
        const std::filesystem::path& path, std::streamsize buffer_size = 4096
    );

    explicit reader(std::string& data) noexcept;

    ~reader();

    void next_token(token& out);

    void jump_to_position(position pos);

    void interrupt();

    position get_position() const;

private:
    std::ifstream ifs;
    std::string filename;
    std::string buffer;
    std::streamsize max_buffer_size {};
    std::streamoff file_offset {};
    int line { 0 };
    int column { 0 };
    size_t buffer_position { 0 };

    bool is_valid() const noexcept;

    char peek_char() const noexcept;

    unsigned char peek_uchar() const noexcept;

    char get_char();

    void advance_char();

    void reload_buffer();

    void read_whitespace(std::string& into);

    void read_keyword(std::string& into);

    void read_string(std::string& into);

    void read_comment(std::string& into);

    token_kind read_number(std::string& into);

    void init_token(token& t) const noexcept;

    [[nodiscard]] std::runtime_error make_error(
        const std::string& message,
        const std::source_location& location = std::source_location::current()
    ) const;
};

#endif // READER_HPP