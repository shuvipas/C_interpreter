# C Interpreter 

This project is a minimal C interpreter implemented in C. It compiles and runs a limited subset of the C programming language using a custom stack-based virtual machine.

## Features

- Parses and executes simplified C programs
- Supports:
  - `int`, `char`, and `enum` types
  - Pointers and pointer arithmetic
  - Control flow: `if`, `else`, `while`, `return`
  - Function declarations and calls
  - Selected standard library functions: `printf`, `malloc`, `memset`, etc.
- Operator precedence parsing

## Project Structure

- `c_interpreter.c` – The interpreter source code
- `tests/` – C programs for testing
- `test_runner.sh` – Bash script for running and comparing test outputs

## Building

To compile the interpreter:

```bash
gcc -o cint c_interpreter.c
```

## Running

To run a C program using the interpreter:

```bash
./cint path/to/file.c
```

To enable **debug mode**, which prints executed VM instructions:

```bash
./cint -d path/to/file.c
```


## Automated Testing

The `test_runner.sh` script compiles and runs all `.c` files in the `tests/` directory using both the interpreter and native GCC execution. It compares the outputs and reports pass/fail results.

To run the test suite:

```bash
./test_runner.sh
```
## Language Subset

| Feature                               | Supported Details                   |
|---------------------------------------|-------------------------------------|
| Basic Types                           | `int`, `char`, `enum`               |
| Numeric Literals                      | Decimal (123), Hex (`0x1A`), Octal (`017`) |
| Pointers                              | Yes                                 |
| Control Flow                          | `if`, `else`, `while`, `return`     |
| Functions                             | Yes (with parameters and return values) |
| `sizeof` Operator                     | Yes                                 |
| Escape Sequences in Strings/Chars     | Only `\n`; no `\t`, `\r`, etc.      |
| Comments                              | Only `//` (no `/* ... */`)          |
| Preprocessor                          | No macros (`#define`) or `#include` |
| Variable Declarations                 | Must appear at start of each block/function; no mixed declaration+assignment (`int a; a = 2;`) |
| Unsupported C Constructs              | `for` loops, `switch`, `struct`, `union`, compound literals, block‑scope macros |


## Internals

- **Lexer**: Tokenizes the source code
- **Parser**: Recursive descent with operator precedence
- **VM**: Stack-based execution model
- **Memory Segments**:
  - `text`: Compiled instructions
  - `data`: Global/static memory
  - `stack`: Function call stack
  - `symbol_table`: Identifier metadata

