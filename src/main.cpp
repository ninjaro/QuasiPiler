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

#include <cxxopts.hpp>
#include <iostream>

#include "reader.hpp"

int main(const int argc, char* argv[]) {
    std::filesystem::path path;
    try {
        cxxopts::Options options(
            "QuasiPiler", "the Hunchback Dragon of Compilers"
        );
        options
            .add_options()("i,input", "Input file", cxxopts::value<std::filesystem::path>(path))(
                "h,help", "show help"
            );
        options.parse_positional({ "input" });
        if (const auto result = options.parse(argc, argv);
            result.count("help")) {
            std::cout << options.help() << "\n";
            return 0;
        }
        if (path.empty() || !exists(path) || !is_regular_file(path)) {
            std::cerr << "input file is required.\n";
            return 1;
        }
    } catch (const cxxopts::exceptions::exception& e) {
        std::cerr << "error parsing options: " << e.what() << "\n";
        return 1;
    }

    reader r(path);

    return 0;
}