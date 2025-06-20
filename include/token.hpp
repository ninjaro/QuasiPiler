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

#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <memory>
#include <ostream>
#include <source_location>
#include <streambuf>
#include <string>

/**
 * @brief Kinds of lexical tokens.
 */
enum class token_kind {
    eof, ///< End of file or input
    open_bracket, ///< One of '(', '[', '{'
    close_bracket, ///< One of ')', ']', '}'
    separator, ///< ',', ';' or ':'
    keyword, ///< Identifier or reserved keyword
    string, ///< Quoted string literal
    comment, ///< Single or multiline comment
    whitespace, ///< Sequence of whitespace characters
    integer, ///< Integer number
    floating, ///< Floating point number
    special_character ///< Any other single character
};

/**
 * @brief Describes a single lexical token extracted by the reader.
 */
struct token {
    token_kind kind {}; ///< Token type
    int line { 0 }; ///< Line where the token starts
    int column { 0 }; ///< Column where the token starts
    std::streamoff file_offset {}; ///< Position in the file
    std::string word; ///< Raw text of the token

    virtual ~token();

    /**
     * @brief Dump token information.
     *
     * Example:
     * @code
     * token t;
     * t.dump(std::cout);
     * @endcode
     */
    virtual void dump(std::ostream& os, const std::string& prefix, bool is_last)
        const noexcept;

    /// Convenience wrapper around dump(os, "", true)
    void dump(std::ostream& os) const noexcept;
};

using token_ptr = std::shared_ptr<token>;

#endif // TOKEN_HPP
