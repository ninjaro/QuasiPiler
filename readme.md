![QuasiPiler Logo Light](logo/llogo.svg#gh-light-mode-only)![QuasiPiler Logo Dark](logo/dlogo.svg#gh-dark-mode-only)
## _— the Hunchback Dragon of Compilers_


[![version](https://img.shields.io/github/v/release/ninjaro/QuasiPiler?include_prereleases)](https://github.com/ninjaro/QuasiPiler/releases/latest)
[![Checks](https://github.com/ninjaro/QuasiPiler/actions/workflows/tests.yml/badge.svg)](https://github.com/ninjaro/QuasiPiler/actions/workflows/tests.yml)
[![Deploy](https://github.com/ninjaro/QuasiPiler/actions/workflows/html.yml/badge.svg)](https://github.com/ninjaro/QuasiPiler/actions/workflows/html.yml)
[![codecov](https://codecov.io/gh/ninjaro/QuasiPiler/graph/badge.svg?token=MCNEJFWMDU)](https://codecov.io/gh/ninjaro/QuasiPiler)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/940dcf5e3cf64e759ce6ad17176d31f4)](https://app.codacy.com/gh/ninjaro/QuasiPiler/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
[![license](https://img.shields.io/github/license/ninjaro/QuasiPiler?color=e6e6e6)](https://github.com/ninjaro/QuasiPiler/blob/master/license)

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

## QuasiLang Syntax Guide

### Basics

* **Comments**

    * Line comments: `// This is a comment`
    * Block comments:

      ```qc
      /* 
        This is a block comment
      */
      ```

* **Whitespace** is ignored except as a separator.

* **Identifiers** consist of letters, digits, and underscores, but cannot start with a digit.

* **Literals**

    * **Numbers:** Support integer and floating-point (with optional exponent).
    * **Strings:** Can use single `'` or double `"` quotes. Common escape sequences (like `\n`, `\t`, `\\`, etc.) are supported.

* **Separators and Grouping**

    * Separators: `,` (comma), `;` (semicolon), `:` (colon)
    * Grouping:

        * `()` for expressions and function parameters
        * `[]` for lists and indexing
        * `{}` for code blocks and objects

---

### Expressions

* **Arithmetic & Assignment:**
  `+`, `-`, `*`, `/`, `%`, `=`, `+=`, `-=`, `*=`, `/=`, `%=`
* **Comparison & Logic:**
  `==`, `!=`, `<`, `<=`, `>`, `>=`, `&&`, `||`, `!`
* **Bitwise:**
  `&`, `|`, `^`, `<<`, `>>`, and compound assignments (`&=`, `|=`, etc.)
* **Increment/Decrement:**
  `++`, `--` (prefix and postfix)
* **Member Access:**
  `obj.key` or `obj["key"]`
* **Indexing & Slicing:**
  `arr[2]`, `arr[1:4]`, `arr[::2]`
* **Function Calls:**
  `func(arg1, arg2)`

---

### Statements and Declarations

* **Variable Assignment:**
  `name = expression;`
* **Function Declaration:**
  `name(param1, param2) { ... }`
* **Control Flow:**

    * **Conditional:**

      ```qc
      if (cond) { ... }
      elif (other) { ... }
      else { ... }
      ```
    * **Loops:**

        * While: `while (cond) { ... }`
        * For: `for (init; cond; step) { ... }`
    * **Jump Statements:**

        * `break;`
        * `continue;`
        * `return;` or `return expr;`
        * `goto label_name;`
    * **Exception Handling:**

      ```qc
      try {
          // code
      } catch (err) {
          // handler
      } finally {
          // cleanup
      }
      ```
* **Labels:**
  Define with `label_name:` and use with `goto label_name;`

---

### Data Structures

* **Lists:**
  `[item1, item2, ...]`
* **Dictionaries/Objects:**
  `{ "key": value, "other": 42 }`

---

### Example Programs

**Hello World**

```qc
// hello.qc
greeting = "Hello, world!";
print(greeting);
```

**Basic Control Flow**

```qc
numbers = [1, 2, 3, 4, 5];
sum = 0;

for(i = 0; i < len(numbers); i++) {
    sum += numbers[i];
}

if (sum > 15) {
    print("Sum is greater than 15");
} elif (sum > 10) {
    print("Sum is greater than 10");
} else {
    print("Sum is", sum);
}
```

**Functions and Error Handling**

```qc
safe_div(a, b) {
    try {
        return a / b;
    } catch (...) {
        print("Error: Division by zero");
        return null;
    }
}

result = safe_div(10, 0);
```

**Labels and Goto (Rare, but Supported)**

```qc
counter = 0;
start:
    counter++;
    if (counter < 3) {
        goto start;
    }
print("Done!");
```

---

**See `include/frontend` and `src/frontend` for implementation, and check `tests/frontend` for more examples.**


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

For detailed documentation, see the [Documentation](https://ninjaro.github.io/QuasiPiler/doc/) and for the latest
coverage report, see [Coverage](https://ninjaro.github.io/QuasiPiler/cov/).

## Security Policy

Please report any security issues using GitHub's private vulnerability reporting
or by emailing [yaroslav.riabtsev@rwth-aachen.de](mailto:yaroslav.riabtsev@rwth-aachen.de).
See the [security policy](.github/SECURITY.md) for full details.

## License

This project is open-source and available under the MIT License.