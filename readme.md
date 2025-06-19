![QuasiPiler Logo Light](logo/llogo.svg#gh-light-mode-only)![QuasiPiler Logo Dark](logo/dlogo.svg#gh-dark-mode-only)
## _— the Hunchback Dragon of Compilers_


[//]: # ([![version]&#40;https://img.shields.io/github/v/release/YaRiabtsev/QuasiPiler?include_prereleases&#41;]&#40;https://github.com/YaRiabtsev/QuasiPiler/releases/latest&#41;)
[//]: # ([![Checks]&#40;https://github.com/YaRiabtsev/QuasiPiler/actions/workflows/tests.yml/badge.svg&#41;]&#40;https://github.com/YaRiabtsev/QuasiPiler/actions/workflows/tests.yml&#41;)
[//]: # ([![Docs & Coverage]&#40;https://github.com/YaRiabtsev/QuasiPiler/actions/workflows/html.yml/badge.svg&#41;]&#40;https://github.com/YaRiabtsev/QuasiPiler/actions/workflows/html.yml&#41;)
[![codecov](https://codecov.io/gh/YaRiabtsev/QuasiPiler/graph/badge.svg?token=MCNEJFWMDU)](https://codecov.io/gh/YaRiabtsev/QuasiPiler)
[![license](https://img.shields.io/github/license/YaRiabtsev/QuasiPiler?color=e6e6e6)](https://github.com/YaRiabtsev/QuasiPiler/blob/master/license)

> “A one-eyed transpiler is much more incomplete than a blind transpiler, for he knows what it is that’s lacking.”  
> — Victor-Marie of Gugle Inc. (1998–2017)

This repo is my sanctuary under license — it begs mercy, not stars. I’ll bell when (or if) it works.
“Documentation and Contributing” is a friendly suggestion, not a Martin Luther pinboard.

## Setup and Installation

### Requirements

* **C++20** compiler
* **cxxopts**: for command line options
* **GTest**: for unit tests
* **lcov**: for code coverage reports
* **doxygen** and **graphviz**: for generating documentation

### Building the Application

1. Build with CMake in Release mode:
    ```bash
    $ cmake -DBUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release -B build -S .
    $ cmake --build build
    ```
2. Run the Application:
    ```bash
    $ qpiler [options] <inputfile>
    ```
   * `<inputfile>`: path to your QuasiCode file

## QuasiLang Syntax

### Basics

- **Comments**
   - Line comments begin with `//`.
   - Block comments are enclosed in `/*` and `*/`.
- **Whitespace** is ignored except as a separator.
- **Identifiers** use letters, digits and underscores and may not start with a digit.
- **Literals**
   - Numbers support integer and floating point forms (with optional exponent).
   - Strings can use either single `'` or double `"` quotes and support common escape sequences.

[//]: # (- **Separators and grouping**)

[//]: # (   - `,` comma, `;` semicolon and `:` colon act as separators.)

[//]: # (   - `&#40;&#41;` parentheses, `[]` brackets and `{}` braces form groups.)

[//]: # (   - Nested groups are used for lists, code blocks and expressions.)

[//]: # ()
[//]: # (### Expressions)

[//]: # ()
[//]: # (- Standard arithmetic and assignment operators are recognized: `+`, `-`, `*`, `/`, `%`, `=` and their compound forms &#40;`+=`, `-=`, `*=`, `/=`, `%=`&#41;.)

[//]: # (- Comparison and logical operators include `==`, `!=`, `<`, `<=`, `>`, `>=`, `&&`, `||`, `!`.)

[//]: # (- Bitwise operators: `&`, `|`, `^`, `<<`, `>>` and their compound assignments.)

[//]: # (- Increment and decrement operators `++` and `--` are supported in prefix and postfix form.)

[//]: # (- Member access uses `.` and indexing uses `[expr]`. Slice syntax `[start:end:step]` is available.)

[//]: # (- Function calls use the form `name&#40;arg1, arg2&#41;`.)

### Statements and Declarations

[//]: # (- **Variable assignment** follows `name = expression;`.)

[//]: # (- **Function declarations** use `name&#40;param1, param2&#41; { ... }`.)

[//]: # (- **Control flow**)

[//]: # (   - Conditional statements: `if &#40;cond&#41; { ... }`, optional `else` or `elif` blocks.)

[//]: # (   - Loops: `while &#40;cond&#41; { ... }` and `for&#40;init; cond; step&#41; { ... }`.)

[//]: # (   - `break`, `continue`, `return` and `goto` appear as standalone keywords and may take an optional expression for `return`.)

[//]: # (   - `try { ... } catch { ... }` for exception handling.)

[//]: # (- **Labels** can be defined with `label_name:` and referenced via `goto label_name`.)

### Data Structures

[//]: # (- **Lists** use `[item1, item2, ...]`.)

[//]: # (- **Dictionaries/objects** use `{ "key": value }`.)

## Examples

```qc
// todo: Example of a simple QuasiLang program
```

## Documentation and Contributing

To build and run tests, enable debug mode, or generate coverage reports:

1. **Build with Debug and Coverage:**
   ```bash
   $ cmake -B build CMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DCOVERAGE=ON
   ```
2. **Generate Coverage Report and HTML:**
   ```bash
   $ cmake --build build --target coverage
   ```

For detailed documentation, see the [Documentation](https://yariabtsev.github.io/QuasiPiler/doc/) and for the latest
coverage report, see [Coverage](https://yariabtsev.github.io/QuasiPiler/cov/).

## Security Policy

Please report any security issues using GitHub's private vulnerability reporting
or by emailing [yaroslav.riabtsev@rwth-aachen.de](mailto:yaroslav.riabtsev@rwth-aachen.de).
See the [security policy](.github/SECURITY.md) for full details.

## License

This project is open-source and available under the MIT License.