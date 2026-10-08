# regex-engine-c

A fast, memory-safe Regular Expression Engine built from scratch in **C17** (compiled with GCC 13.2.0). 

It converts regex patterns into state machines using **Thompson's NFA construction** and simulates them in parallel. This guarantees predictable, linear matching time and prevents the CPU freezes (ReDoS) common in backtracking engines like Python and Java.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Language: C17](https://img.shields.io/badge/Language-C17-blue.svg)](https://en.wikipedia.org/wiki/C17_(C_standard_revision))
[![Compiler: GCC 13.2.0](https://img.shields.io/badge/Compiler-GCC%2013.2.0-orange.svg)]()
[![Tests](https://img.shields.io/badge/Tests-83%20Passed-brightgreen.svg)]()

> [!TIP]
> **Complete Theoretical Guide:** For an in-depth 24-chapter exploration of formal language theory, Chomsky hierarchy, Kleene's theorem, Thompson's construction, epsilon closures, memory safety in C, and end-to-end execution walks, see [KNOWLEDGE.md](KNOWLEDGE.md).

---

## Table of Contents

- [Part I: Project Proposal](#part-i-project-proposal)
  - [1. Project Description](#1-project-description)
  - [2. Motivation and Goals](#2-motivation-and-goals)
  - [3. Specifications and Scope](#3-specifications-and-scope)
  - [4. Architectural Design](#4-architectural-design)
- [Part II: Getting Started and User Guide](#part-ii-getting-started-and-user-guide)
  - [Prerequisites](#prerequisites)
  - [Compilation](#compilation)
  - [Running the Test Suite](#running-the-test-suite)
  - [Command-Line Usage](#command-line-usage)
- [Part III: Project Layout and Architecture Reference](#part-iii-project-layout-and-architecture-reference)
  - [Quick Comparison: Backtracking vs. Thompson NFA](#quick-comparison-backtracking-vs-thompson-nfa)
  - [License](#license)
- [Theory Guide: KNOWLEDGE.md](KNOWLEDGE.md)

---

<a id="part-i-project-proposal"></a>
# Part I: Project Proposal

<a id="1-project-description"></a>
## 1. Project Description
**`regex-engine-c`** is a standalone regular expression engine and command-line tool written in pure C17 without relying on external libraries or `<regex.h>`.

Instead of using recursive backtracking, this project compiles a regex pattern through a clean compiler pipeline:
1. Breaks raw text into tokens (Lexer)
2. Builds an Abstract Syntax Tree (Parser)
3. Converts the tree into a state machine graph (Thompson NFA Builder)
4. Simulates all candidate states at the same time (Virtual Machine Simulator)

The core engine is completely decoupled from I/O and printing, making it easy to embed, test, or export data to external tools and web frontends.

---

<a id="2-motivation-and-goals"></a>
<a id="2-motivation-goals"></a>
## 2. Motivation and Goals

### Why Build This?
Most built-in regex engines (like those in Python, Java, or JavaScript) use recursive backtracking. On tricky patterns, such as `(a+)+$` matching against a long string of `a`'s, they try millions of branch combinations sequentially. This causes exponential slowdowns **O(2^n)** and can freeze an entire program (known as a Regular Expression Denial of Service, or ReDoS).

### Core Goals:
1. **Guaranteed Linear Matching Time**:
   - Use Thompson's algorithm to track all possible branches simultaneously.
   - Guarantee matching finishes in **O(m * n)** time (where `m` is pattern length and `n` is text length), ensuring it never freezes.
2. **Clean Compiler Design**:
   - Write a recursive descent parser that naturally respects operator precedence (`Alternation` < `Sequence` < `Repetition` < `Atoms`).
3. **Memory Safety in C**:
   - Use fixed-size stack arrays for tokens and states to prevent memory leaks and heap fragmentation.
   - Implement `free_ast` to clean up heap-allocated tree nodes so the engine runs with zero memory leaks.
4. **Developer Tools**:
   - Built-in terminal visualization tools: `--tree` to see the syntax tree, `--trace` to watch state transitions live, and `--json` to export data for web visualizers.

---

<a id="3-specifications-and-scope"></a>
<a id="3-specifications-scope"></a>
## 3. Specifications and Scope

### Supported Regex Features

| Category | Syntax | Example | Description |
| :--- | :--- | :--- | :--- |
| **Literals** | `a`, `b`, `1` | `cat` | Exact character matching |
| **Wildcard** | `.` | `a.c` | Matches any single character |
| **Alternation** | `\|` | `cat\|dog` | Matches either left or right choice |
| **Kleene Star** | `*` | `ab*c` | Matches 0 or more repetitions |
| **Plus** | `+` | `ab+c` | Matches 1 or more repetitions |
| **Optional** | `?` | `colou?r` | Matches 0 or 1 occurrence |
| **Character Sets** | `[...]` | `[a-zA-Z0-9]` | Matches any character in the given range or list |
| **Negated Sets** | `[^...]` | `[^0-9]` | Matches any character NOT in the set |
| **Shorthands** | `\d`, `\w`, `\s` | `\d+-\w+` | Digits (`0-9`), Word chars (`a-z`, `A-Z`, `0-9`, `_`), Whitespace |
| **Negated Shorthands** | `\D`, `\W`, `\S` | `\D+` | Non-digits, non-word chars, non-whitespace |
| **Quantifier Ranges** | `{m,n}`, `{m}`, `{m,}` | `a{2,4}` | Bounded repetitions |
| **Grouping & Nesting** | `(...)` | `(ab\|cd)+` | Parentheses for precedence and nested groups |

### Features Intentionally Left Out (And Why)
To keep the engine fast, predictable, and within standard regular grammar limits:
- **Backreferences (`\1`, `\2`)**: Checking if a later part of a string matches an earlier group requires remembering text of arbitrary length. This forces engines into slow backtracking and exponential time. Modern production engines like Google RE2 and Rust's `regex` crate also omit backreferences for this exact reason.
- **Lookaround Assertions (`(?=...)`)**: Checking ahead or behind without consuming characters breaks single-pass linear simulation.
- **Full Unicode Tables**: Sticking to standard ASCII (0-127) keeps the C code small, readable, and easy to explain without requiring thousands of lines of Unicode tables.

---

<a id="4-architectural-design"></a>
## 4. Architectural Design

The engine compiles and runs patterns in four modular steps:

```text
Pattern String: "a(b|c)*d"
      │
      ▼
┌──────────────┐
│  Tokenizer   │  Turns text into typed tokens (LITERAL, LPAREN, STAR, etc.)
└──────┬───────┘
       │  TokenList
       ▼
┌──────────────┐
│    Parser    │  Builds an Abstract Syntax Tree (ASTNode) respecting precedence
└──────┬───────┘
       │  ASTNode* (Root)
       ▼
┌──────────────┐
│ NFA Builder  │  Turns AST into state-machine fragments linked by epsilon-moves
└──────┬───────┘
       │  NFA (Flat State Table)
       ▼
┌──────────────┐
│  Simulator   │  Walks all candidate states in parallel on each character
└──────┬───────┘
       │
       ▼
 MATCH (0) / NO MATCH (1) + Visual Output (--tree, --trace, --json)
```

### Module Breakdown:
1. **Tokenizer (`include/regex/token.h`, `src/tokenizer.c`)**:
   - Scans characters into a fixed `TokenList` array on the stack. No heap allocation is used during lexing.
   - Detects escape characters (`\d`, `\w`, `\s`) and catches errors like dangling backslashes.
2. **Parser & AST (`include/regex/ast.h`, `include/regex/parser.h`, `src/parser.c`)**:
   - Uses recursive descent to build a clean binary syntax tree:
     ```text
     parse_alt()  -->  parse_seq()  -->  parse_repeat()  -->  parse_atom()
     ```
   - Includes `free_ast(root)` to free all allocated nodes after execution.
3. **Thompson NFA Builder (`include/regex/nfa.h`, `include/regex/nfa_builder.h`, `src/nfa_builder.c`)**:
   - Converts tree nodes into reusable NFA fragments (each with one start and one accept state).
   - Links fragments with free epsilon-transitions (teleporters).
   - Stores all states in a flat array (`nfa->states`) indexed by numbers (`0, 1, 2...`) to avoid pointer chasing.
4. **Simulator (`include/regex/simulate.h`, `src/simulate.c`)**:
   - Tracks a set of active states (`StateSet`) at each step.
   - Computes the epsilon-closure using a simple worklist loop to find all reachable free moves.
   - Steps characters across all active states simultaneously in linear time.
5. **CLI & Serialization (`src/main.c`, `src/json_export.c`)**:
   - Handles terminal flags: `--tree` (indented AST view), `--trace` (live state machine walk), and `--json` (exports AST and NFA data).

---

<a id="part-ii-getting-started-and-user-guide"></a>
<a id="part-ii-getting-started-user-guide"></a>
# Part II: Getting Started and User Guide

<a id="prerequisites"></a>
## Prerequisites
- **C Compiler**: Any standard C17 compiler (`gcc` 13.2.0 or newer recommended, or `clang`).
- **Build System**: GNU Make (`make` on Linux/macOS, `mingw32-make` on Windows).

---

<a id="compilation"></a>
## Compilation

Build the executable with a single command:

```bash
# On Linux / macOS:
make

# On Windows (MinGW):
mingw32-make
```

This compiles all source files with `-Wall -Wextra -std=c17 -Iinclude` and creates the `regex_engine` executable.

To compile manually with GCC directly:
```bash
gcc -Wall -Wextra -std=c17 -Iinclude src/tokenizer.c src/parser.c src/nfa_builder.c src/simulate.c src/json_export.c src/main.c -o regex_engine
```

---

<a id="running-the-test-suite"></a>
## Running the Test Suite

The project includes an automated test suite with **83 unit tests** checking every phase:

```bash
# Run all 83 unit tests:
make test          # (or mingw32-make test)
```

### Individual Test Suites:
```bash
make test_tokenizer   # Tokenizer & escape handling (10 tests)
make test_parser      # AST parsing & precedence (23 tests)
make test_nfa         # Thompson NFA fragment construction (20 tests)
make test_simulate    # String matching & edge cases (30 tests)
```

Clean build outputs:
```bash
make clean         # (or mingw32-make clean)
```

---

<a id="command-line-usage"></a>
## Command-Line Usage

The executable takes a regular expression pattern, an optional string to match against, and optional display flags.

### 1. Basic String Matching
Returns exit code `0` on match, and `1` on no match (standard Unix `grep` behavior):

```bash
$ ./regex_engine "cat|dog" "dog"
[MATCH] Pattern 'cat|dog' matches input 'dog'

$ ./regex_engine "cat|dog" "bird"
[NO MATCH] Pattern 'cat|dog' does not match input 'bird'
```

Check exit codes in PowerShell:
```powershell
.\regex_engine.exe "cat|dog" "dog"
echo $LASTEXITCODE    # 0

.\regex_engine.exe "cat|dog" "bird"
echo $LASTEXITCODE    # 1
```

---

### 2. Syntax Tree View (`--tree`)
Prints the Abstract Syntax Tree with branch lines and human-readable labels:

```bash
$ ./regex_engine "a(b|c)*d" --tree
Syntax Tree (AST):
\-- SEQ (Concatenation: left then right)
    +-- SEQ (Concatenation: left then right)
    |   +-- LITERAL 'a'
    |   \-- STAR (Repetition: 0 or more times)
    |       \-- GROUP (Parentheses)
    |           \-- ALT (Choice: left OR right)
    |               +-- LITERAL 'b'
    |               \-- LITERAL 'c'
    \-- LITERAL 'd'
```

---

### 3. Step-by-Step State Trace (`--trace`)
Shows active NFA states and free transitions as each character is processed:

```bash
$ ./regex_engine "a(b|c)*d" "acd" --trace

=== NFA Simulation Trace ===
Pattern:      a(b|c)*d
Input Text:   "acd"
Start State:  0
Accept State: 11

[Init]       Active states after e-closure: { 0 }
[Char 'a']   Active states after e-closure: { 1, 8, 6, 9, 2, 4, 10 }
[Char 'c']   Active states after e-closure: { 5, 7, 6, 9, 2, 4, 10 }
[Char 'd']   Active states after e-closure: { 11 }

[Result]     Accept state 11 reached: YES

[MATCH] Pattern 'a(b|c)*d' matches input 'acd'
```

When input does not match:
```bash
$ ./regex_engine "a(b|c)*d" "xyz" --trace

=== NFA Simulation Trace ===
Pattern:      a(b|c)*d
Input Text:   "xyz"
Start State:  0
Accept State: 11

[Init]       Active states after e-closure: { 0 }
[Char 'x']   Active states after e-closure: { EMPTY }
[Char 'y']   Active states after e-closure: { EMPTY }
[Char 'z']   Active states after e-closure: { EMPTY }

[Result]     Accept state 11 reached: NO

[NO MATCH] Pattern 'a(b|c)*d' does not match input 'xyz'
```

---

### 4. Machine-Readable JSON Export (`--json`)
Exports the AST and NFA state machine for external visualizers (e.g., D3.js graph renderers):

```bash
$ ./regex_engine "a|b" --json
{"pattern":"a|b","ast":{"type":"ALT","left":{"type":"LITERAL","value":"a"},"right":{"type":"LITERAL","value":"b"}},"nfa":{"start_state":0,"accept_state":5,"state_count":6,"states":[{"id":0,"transitions":[{"type":"EPSILON","target":1},{"type":"EPSILON","target":3}]},{"id":1,"transitions":[{"type":"CHAR","target":2,"char":"a"}]},{"id":2,"transitions":[{"type":"EPSILON","target":5}]},{"id":3,"transitions":[{"type":"CHAR","target":4,"char":"b"}]},{"id":4,"transitions":[{"type":"EPSILON","target":5}]},{"id":5,"transitions":[]}]}}
```

---

<a id="part-iii-project-layout-and-architecture-reference"></a>
<a id="part-iii-project-layout-architecture-reference"></a>
# Part III: Project Layout and Architecture Reference

```text
regex-engine-c/
├── include/regex/          # Header files (Definitions & Interfaces)
│   ├── token.h             # TokenType enum, Token, TokenList
│   ├── ast.h               # NodeType enum, ASTNode structure
│   ├── parser.h            # parse() and free_ast() prototypes
│   ├── nfa.h               # MatchType, Transition, NFAState, NFA structures
│   ├── nfa_builder.h       # build_nfa() prototype
│   ├── simulate.h          # StateSet, epsilon-closure, simulate_nfa() prototypes
│   └── json_export.h       # export_ast_json() and export_nfa_json() prototypes
├── src/                    # Source files (Core logic)
│   ├── tokenizer.c         # Lexical scanner
│   ├── parser.c            # Recursive descent parser & free_ast()
│   ├── nfa_builder.c       # Thompson fragment builder
│   ├── simulate.c          # Multi-state simulator & epsilon closure
│   ├── json_export.c       # JSON serialization functions
│   └── main.c              # CLI interface with --tree, --trace, --json
├── tests/                  # Unit test suites
│   ├── test_tokenizer.c    # Tokenizer tests (10 tests)
│   ├── test_parser.c       # Parser & memory cleanup tests (23 tests)
│   ├── test_nfa.c          # Thompson NFA construction tests (20 tests)
│   └── test_simulate.c     # String matching tests (30 tests)
├── Makefile                # Build automation file (configured for C17)
├── .gitignore              # Ignores compiled binaries and build artifacts
├── LICENSE                 # MIT Open-Source License
├── KNOWLEDGE.md            # Deep-dive theory, automata, and compiler guide (24 chapters)
└── README.md               # Complete Project Proposal & Documentation
```

---

<a id="quick-comparison-backtracking-vs-thompson-nfa"></a>
## Quick Comparison: Backtracking vs. Thompson NFA

```text
┌────────────────────┬───────────────────────────────┬───────────────────────────────┐
│ Metric / Property  │ Backtracking (Java/Python)    │ Thompson NFA (Our Engine)     │
├────────────────────┼───────────────────────────────┼───────────────────────────────┤
│ Execution Strategy │ Single branch at a time       │ All candidate states at once  │
│ Worst-case Runtime │ O(2^n) - Exponential (Slow)   │ O(m * n) - Always Linear      │
│ State Machine      │ Implicit call stack           │ Explicit NFA graph with jumps │
│ Memory Layout      │ Recursive stack frames        │ Flat static array             │
│ Performance Risk   │ Freezes on pathological input │ Never freezes or hangs        │
└────────────────────┴───────────────────────────────┴───────────────────────────────┘
```

---

<a id="license"></a>
## License

This project is licensed under the [MIT License](LICENSE).
