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

/**
 * @brief Byte and line location within the input stream.
 */
struct position {
    std::streamoff offset; ///< absolute offset from the beginning of the file
    int line; ///< zero based line number
    int column; ///< zero based column number
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

struct token final {
    token_kind kind;
    position pos;
    std::string word;

    ~token();

    void dump(
        std::ostream& os, const std::string& prefix, bool is_last
    ) const noexcept;

    void dump(std::ostream& os) const noexcept;
};

using token_ptr = std::shared_ptr<token>;

/**
 * @brief Lightweight tokenizer for QuasiLang source code.
 *
 * The reader reads from either a file or a memory buffer and produces
 * tokens on demand via next_token(). Position information is tracked so
 * callers can report meaningful diagnostics.
 */
class reader {
public:
    explicit reader(
        const std::filesystem::path& path, std::streamsize buffer_size = 4096
    );

    explicit reader(std::string& data) noexcept;

    ~reader();
    /**
     * @brief Read the next token from the input stream.
     *
     * @param out Token object to be filled with the parsed data.
     */
    void next_token(token& out);

    void jump_to_position(position pos);
    /**
     * @brief Throw an exception with the current position information.
     *
     * Used by parsers to abort processing while preserving diagnostics.
     */
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
    /**
     * @brief Read a quoted string literal with escape handling.
     *
     * Supports common escape sequences and Unicode escapes of the
     * form <tt>\uXXXX</tt>. The resulting decoded text is stored in
     * @p into without the surrounding quotes.
     * @throw std::runtime_error on malformed input.
     */
    void read_string(std::string& into);

    void read_comment(std::string& into);
    /**
     * @brief Parse an integer or floating point literal.
     *
     * Digits are consumed according to the QuasiLang grammar. If a
     * fractional part or exponent is present the returned kind is
     * token_kind::floating.
     */
    token_kind read_number(std::string& into);

    void init_token(token& t) const noexcept;
    /**
     * @brief Helper to create formatted runtime errors.
     *
     * In debug builds the message includes context information such
     * as the current position and originating source location.
     */
    [[nodiscard]] std::runtime_error make_error(
        const std::string& message,
        const std::source_location& location = std::source_location::current()
    ) const;
};

#endif // READER_HPP