# C Lexer

A C++ implementation of a lexical analyzer for a practical subset of the C language. The lexer reads source code, recognizes tokens, and writes the token stream to an output file.

## Features

- C keywords and identifiers
- Decimal, octal, hexadecimal, and common floating-point constants
- String and character literals with escape-sequence handling
- Single-line (`//`) and multi-line (`/* ... */`) comments
- Common C operators and punctuators, using longest-match recognition
- Whitespace tokens
- Invalid-token reporting
- Source line and column information
- Configurable input and output file paths

## Requirements

- A C++ compiler supporting C++11 or newer (for example, GCC or Clang)

## Build

```bash
g++ -std=c++11 -Wall -Wextra -pedantic lexer.cpp -o lexer
```

On Windows with MinGW:

```powershell
g++ -std=c++11 -Wall -Wextra -pedantic lexer.cpp -o lexer.exe
```

## Run

By default, the program reads `source.txt` and writes tokens to `output.txt`:

```bash
./lexer
```

You can also provide custom paths:

```bash
./lexer input.c tokens.txt
```

On Windows:

```powershell
.\lexer.exe source.txt output.txt
```

The included `source.txt` is a small example input. The token stream is printed to the terminal and saved to the output file.

## Token categories

| Category | Description |
|---|---|
| `KEYWORD` | Recognized C language keywords |
| `IDENTIFIER` | User-defined names |
| `DECIMAL_INTEGER` | Decimal integer constants |
| `OCTAL_INTEGER` | Octal integer constants |
| `HEX_INTEGER` | Hexadecimal integer constants |
| `FLOAT` | Supported floating-point constants |
| `STRING` | String literals |
| `CHARACTER` | Single-character character constants |
| `COMMENT` | C-style comments |
| `OPERATOR` | Arithmetic, relational, logical, assignment, and related operators |
| `PUNCTUATOR` | Delimiters and other punctuation |
| `WHITESPACE` | Spaces, tabs, and line breaks |
| `INVALID` | Input that does not match the supported token forms |

Each output line includes the token category, token text, and its starting line and column.

## Scope and limitations

This is an educational lexer, not a complete implementation of the C standard. It does not perform syntax analysis or semantic analysis. Preprocessor directives are tokenized, but they are not expanded. Some advanced or implementation-specific numeric and literal forms may not be supported.
