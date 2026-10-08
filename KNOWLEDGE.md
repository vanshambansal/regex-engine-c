# KNOWLEDGE.md - Understanding the Regex Engine

This document explains the theory, design, and implementation behind this regular expression engine. It is intended to be a long-term reference for understanding both regular expressions in general and how this project processes them using C.

The goal is not merely to know how to run the engine, but to understand what happens internally, why each stage exists, and how the stages work together.

---

# Table of Contents

1. [Project Overview](#1-project-overview)
2. [What Is a Regular Expression?](#2-what-is-a-regular-expression)
3. [Alphabets, Strings, Languages, and Regular Languages](#3-alphabets-strings-languages-and-regular-languages)
4. [Regex Operators and Their Meaning](#4-regex-operators-and-their-meaning)
5. [How a Regex Engine Processes a Pattern](#5-how-a-regex-engine-processes-a-pattern)
6. [Tokenization and Lexical Analysis](#6-tokenization-and-lexical-analysis)
7. [Parsing and Grammar](#7-parsing-and-grammar)
8. [Operator Precedence and Associativity](#8-operator-precedence-and-associativity)
9. [Recursive Descent Parsing](#9-recursive-descent-parsing)
10. [Abstract Syntax Tree (AST)](#10-abstract-syntax-tree-ast)
11. [Finite State Machines](#11-finite-state-machines)
12. [Nondeterministic Finite Automata (NFA)](#12-nondeterministic-finite-automata-nfa)
13. [Epsilon Transitions and Epsilon Closure](#13-epsilon-transitions-and-epsilon-closure)
14. [Thompson's Construction](#14-thompsons-construction)
15. [NFA Simulation and Matching](#15-nfa-simulation-and-matching)
16. [Character Classes, Shorthands, and Quantifiers](#16-character-classes-shorthands-and-quantifiers)
17. [Matching Semantics and Limitations](#17-matching-semantics-and-limitations)
18. [Time and Space Complexity](#18-time-and-space-complexity)
19. [Understanding the C Implementation](#19-understanding-the-c-implementation)
20. [Memory Management and Error Handling](#20-memory-management-and-error-handling)
21. [End-to-End Example](#21-end-to-end-example)
22. [Project Architecture and File Responsibilities](#22-project-architecture-and-file-responsibilities)
23. [Current Scope and Possible Extensions](#23-current-scope-and-possible-extensions)
24. [Glossary](#24-glossary)

---

<a id="1-project-overview"></a>
# 1. Project Overview

## 1.1 What Are We Building?

This project implements a regular expression engine from scratch in C.

A regular expression describes a pattern. Given a pattern and an input string, the engine determines whether the input matches that pattern.

For example:

```text
Pattern: a(b|c)*d
Input:   abcbcd
Result:  MATCH
```

The pattern means:

- The string must begin with `a`.
- It may contain zero or more occurrences of either `b` or `c`.
- It must end with `d`.

The string `abcbcd` satisfies these conditions.

The string `abcbc` does not, because it does not end with `d`.

Instead of using an existing regex library, this project implements the processing stages itself.

## 1.2 Why Build It from Scratch?

A regex engine combines several important computer science concepts:

- **Compiler construction:** converting source text into a structured representation.
- **Formal languages:** describing sets of valid strings.
- **Parsing:** determining how operators and expressions are grouped.
- **Automata theory:** representing patterns as state machines.
- **Algorithms:** simulating an automaton against input.
- **Systems programming:** managing data structures and memory in C.

Understanding the complete pipeline makes the behavior of a regex engine much less mysterious.

## 1.3 The Complete Processing Pipeline

The architecture is:

```text
Regex Pattern
     |
     v
Tokenizer / Lexer
     |
     v
Token List
     |
     v
Recursive Descent Parser
     |
     v
Abstract Syntax Tree (AST)
     |
     v
Thompson NFA Builder
     |
     v
Epsilon-NFA
     |
     v
NFA Simulator
     |
     v
MATCH / NO MATCH
```

Each stage has a separate responsibility.

| Stage | Responsibility |
|---|---|
| Tokenizer | Recognizes meaningful symbols in the pattern |
| Parser | Determines how the symbols are structured |
| AST | Stores the logical structure of the pattern |
| NFA builder | Converts the AST into a finite-state machine |
| Simulator | Executes the machine against the input |
| CLI | Accepts input and presents results |
| JSON exporter | Serializes internal structures for external tools |

The tokenizer does not decide whether a string matches. The parser does not simulate the input. The NFA builder does not handle command-line arguments.

This separation keeps the engine modular and easier to test.

---

<a id="2-what-is-a-regular-expression"></a>
# 2. What Is a Regular Expression?

A **regular expression**, or regex, is a notation for describing a pattern of strings.

For example:

```text
cat
```

describes the exact string `cat`.

```text
a|b
```

describes either `a` or `b`.

```text
ab*
```

describes `a` followed by zero or more occurrences of `b`.

It can therefore match:

```text
a
ab
abb
abbb
```

But not:

```text
b
ba
ac
```

## 2.1 Regex as a Language Description

A regex does not generally describe just one string. It describes a **set of strings**.

For example:

```text
a*
```

describes:

```text
""
"a"
"aa"
"aaa"
"aaaa"
...
```

The empty string is written as \(\varepsilon\) in formal-language theory. It contains zero characters.

Similarly:

```text
a|b
```

describes the set:

\[
\{a,b\}
\]

And:

```text
(ab)*
```

describes:

\[
\{\varepsilon, ab, abab, ababab,\ldots\}
\]

This viewpoint is important: the engine is recognizing whether a string belongs to the language described by a pattern.

## 2.2 Matching Is Not the Same as Searching

In this project, the basic matching operation is intended to check whether the **entire input string** matches the pattern.

For example:

```text
Pattern: cat
Input:   cat
Result:  MATCH
```

But:

```text
Pattern: cat
Input:   cats
Result:  NO MATCH
```

Even though `cats` contains `cat`, the entire string is not equal to the language described by the pattern `cat`.

A search operation would have different semantics: it would look for a matching substring inside a larger input.

Unless a search mode is explicitly implemented, do not assume the engine searches for substrings automatically.

---

<a id="3-alphabets-strings-languages-and-regular-languages"></a>
# 3. Alphabets, Strings, Languages, and Regular Languages

These concepts form the mathematical foundation of the engine.

## 3.1 Alphabet

An **alphabet**, usually represented by \(\Sigma\), is a set of symbols.

For a simple example:

\[
\Sigma = \{a,b\}
\]

The symbols available are `a` and `b`.

An alphabet does not have to contain only letters. It may contain digits, punctuation, or other symbols.

For this project, character-level processing is used, with the intended scope focused on ASCII rather than full Unicode character properties.

## 3.2 String

A string is a finite sequence of symbols from an alphabet.

If:

\[
\Sigma = \{a,b\}
\]

then these are valid strings:

```text
a
b
ab
ba
aab
bbab
```

The empty string, \(\varepsilon\), is also a valid string.

The length of a string \(w\) is written as \(|w|\).

For example:

\[
|abba| = 4
\]

while:

\[
|\varepsilon| = 0
\]

## 3.3 Language

A **language** is a set of strings over an alphabet.

For example:

\[
L = \{a,ab,abb,abbb,\ldots\}
\]

This language contains `a` followed by zero or more `b` characters.

The regex:

```text
ab*
```

describes this language.

A language may contain finitely many strings or infinitely many strings.

## 3.4 Regular Languages

A language is regular if it can be described by a regular expression and recognized by a finite automaton.

For example:

```text
a*
```

describes a regular language.

So does:

```text
(a|b)*abb
```

The latter describes strings over `{a,b}` that end in `abb`, assuming full-string matching.

The important connection is:

```text
Regular Expression
        |
        v
Equivalent Finite Automaton
        |
        v
Recognizes the Same Language
```

This equivalence is the theoretical foundation of our project.

Instead of interpreting the regex repeatedly as raw text, we transform it into an automaton and execute that automaton.

---

<a id="4-regex-operators-and-their-meaning"></a>
# 4. Regex Operators and Their Meaning

The engine recognizes different kinds of operators. Understanding their semantics is essential before studying the parser and NFA builder.

## 4.1 Literals

A literal represents itself.

```text
cat
```

means the exact sequence:

```text
c -> a -> t
```

Each literal must match the corresponding input character.

## 4.2 Concatenation

Concatenation means one expression is followed by another.

```text
ab
```

means `a` followed by `b`.

The regex notation does not require an explicit concatenation operator. The sequence itself implies concatenation.

For example:

```text
abcd
```

is a concatenation of four literals.

Concatenation is usually represented explicitly in the AST as a `SEQ` node.

## 4.3 Alternation

The `|` operator means either the expression on its left or the expression on its right.

```text
cat|dog
```

matches either `cat` or `dog`.

For example:

```text
Pattern: cat|dog
Input:   cat
Result:  MATCH
```

```text
Pattern: cat|dog
Input:   dog
Result:  MATCH
```

```text
Pattern: cat|dog
Input:   cow
Result:  NO MATCH
```

Alternation is represented by an `ALT` node in the AST.

## 4.4 Kleene Star

The `*` operator means zero or more repetitions of the preceding expression.

```text
ab*
```

means:

```text
a
ab
abb
abbb
...
```

The star applies only to the expression immediately before it, unless grouping changes that structure.

Compare:

```text
ab*
```

with:

```text
(ab)*
```

The first means `a` followed by zero or more `b` characters.

The second means zero or more repetitions of the sequence `ab`.

The distinction is fundamental to parsing.

## 4.5 Plus

The `+` operator means one or more repetitions.

```text
a+
```

matches:

```text
a
aa
aaa
...
```

Unlike `a*`, it does not allow zero occurrences of `a`.

## 4.6 Optional

The `?` operator means zero or one occurrence.

```text
colou?r
```

matches:

```text
color
colour
```

The `u` is optional.

## 4.7 Grouping

Parentheses group an expression:

```text
(ab)*
```

The star applies to the entire sequence `ab`.

Parentheses also control the scope of alternation:

```text
a(b|c)d
```

means:

```text
abd
acd
```

The AST represents grouping so that the parser preserves the intended structure.

## 4.8 Character Classes

A character class describes a set of characters, usually matching one character from that set.

```text
[abc]
```

matches exactly one of:

```text
a
b
c
```

It does not mean the sequence `abc`.

A range can be written as:

```text
[a-z]
```

which represents lowercase ASCII letters from `a` through `z`.

Multiple ranges can be combined:

```text
[a-zA-Z0-9]
```

This represents the corresponding lowercase letters, uppercase letters, and digits.

A negated character class begins with `^`:

```text
[^0-9]
```

This represents a character outside the listed digit set, subject to the engine's character-set semantics.

## 4.9 Shorthand Classes

Common shorthand forms are:

| Syntax | Intended meaning |
|---|---|
| `\d` | Digit |
| `\D` | Non-digit |
| `\w` | Word character |
| `\W` | Non-word character |
| `\s` | Whitespace |
| `\S` | Non-whitespace |

For example:

```text
\d+
```

matches one or more digits.

```text
\w+
```

matches one or more word characters according to the engine's definition.

The exact definitions depend on the implementation. They should not automatically be assumed to match the behavior of every programming language's regex engine.

## 4.10 Quantifier Ranges

A range quantifier specifies how many repetitions are allowed.

```text
a{3}
```

means exactly three occurrences of `a`.

```text
a{2,4}
```

means two, three, or four occurrences.

```text
a{2,}
```

means at least two occurrences.

The parser stores the bounds in the AST. The NFA builder must then represent the corresponding repetition behavior.

## 4.11 Anchors

Many regex dialects use:

```text
^
$
```

for start-of-input and end-of-input assertions.

However, recognizing these symbols as tokens is not sufficient to implement anchor semantics. The parser and matching engine must explicitly support their intended behavior.

In this project, anchor handling should be considered supported only if the relevant implementation has been completed and tested. Do not assume that token definitions alone make anchors functional.

---

<a id="5-how-a-regex-engine-processes-a-pattern"></a>
# 5. How a Regex Engine Processes a Pattern

Consider:

```text
a(b|c)*d
```

and the input:

```text
abcbcd
```

The engine does not directly jump from the pattern string to a match result.

Instead, each stage transforms the representation.

## Stage 1: Raw Pattern

The pattern initially exists as a C string:

```text
a(b|c)*d
```

At this stage, it is simply a sequence of characters.

## Stage 2: Tokenization

The tokenizer recognizes symbols such as literals, parentheses, alternation, and repetition.

Conceptually:

```text
LITERAL('a')
LPAREN
LITERAL('b')
PIPE
LITERAL('c')
RPAREN
STAR
LITERAL('d')
EOF
```

## Stage 3: Parsing

The parser determines the grammatical structure:

- `b|c` is an alternation.
- `(b|c)` is a grouped expression.
- `(b|c)*` repeats that group zero or more times.
- `a` is concatenated with the repeated group.
- `d` follows the preceding expression.

## Stage 4: AST

The structure is represented as a tree.

```text
             SEQ
            /   \
          SEQ    d
         /   \
        a    STAR
              |
             GROUP
               |
              ALT
             /   \
            b     c
```

## Stage 5: NFA Construction

The AST is converted into a graph of states and transitions.

The graph contains:

- States for literal transitions
- ε-transitions for branching and repetition
- A designated start state
- A designated accepting state

## Stage 6: Simulation

The simulator begins at the NFA start state.

It processes the input character by character, tracking all states reachable after each consumed character and the relevant ε-transitions.

## Stage 7: Acceptance

After all input characters have been consumed, the input is accepted if the accepting state is reachable in the final active-state set.

For full-string matching, reaching the accepting state before consuming the entire input is not enough.

---

<a id="6-tokenization-and-lexical-analysis"></a>
# 6. Tokenization and Lexical Analysis

## 6.1 What Is Tokenization?

Tokenization, also called lexical analysis, converts a sequence of raw characters into meaningful units called **tokens**.

For a programming language, tokens might include identifiers, keywords, operators, and numbers.

For our regex language, tokens include:

```text
TOKEN_LITERAL
TOKEN_DOT
TOKEN_STAR
TOKEN_PLUS
TOKEN_QUESTION
TOKEN_PIPE
TOKEN_LPAREN
TOKEN_RPAREN
TOKEN_SHORTHAND
TOKEN_EOF
```

The complete token types are defined in `include/regex/token.h`.

## 6.2 Why Do We Need Tokens?

Consider:

```text
a(b|c)*
```

The parser should not need to repeatedly ask whether each raw character is `(`, `|`, `)`, or `*`.

Instead, the tokenizer classifies the characters first.

The parser can then reason about token types and grammatical structure.

This separation makes both components simpler.

## 6.3 Token Type and Token Value

The project defines a token using a structure conceptually equivalent to:

```c
typedef struct {
    TokenType type;
    char value;
} Token;
```

The two fields serve different purposes.

- `type` identifies the token category.
- `value` stores the actual character when that information matters.

For example:

```text
Token:
    type  = TOKEN_LITERAL
    value = 'a'
```

A star token has no literal character value that the parser needs to interpret, so its `value` can be initialized to zero.

## 6.4 Token Lists

The project stores tokens in a `TokenList`, which contains an array and a count.

The array has a fixed maximum capacity:

```c
#define MAX_TOKENS 256
```

This makes the tokenizer's storage predictable, but it also imposes a limit on the number of tokens that can be represented.

The tokenizer must check the capacity before writing another token.

The final `TOKEN_EOF` token marks the end of the token stream. It is a parser sentinel, not a character from the original pattern.

## 6.5 Escape Sequences

The backslash introduces an escape sequence.

For example:

```text
\d
```

is read as a shorthand token rather than a literal backslash followed by `d`.

The tokenizer must therefore advance past the backslash and inspect the following character.

If the pattern ends immediately after a backslash, the sequence is incomplete and should produce an error.

The exact accepted escapes are determined by the tokenizer implementation.

## 6.6 Connection to This Project

The main implementation is in:

```text
include/regex/token.h
src/tokenizer.c
```

The public interface is:

```c
int tokenize(const char *pattern, TokenList *out, char *error_msg);
```

The pattern is read as a C string. The tokenizer fills the caller-provided token list and returns a success/failure result.

This interface allows `main.c` to create a token list and pass it to the tokenizer without the tokenizer needing to allocate and return a separate token-list object.

---

<a id="7-parsing-and-grammar"></a>
# 7. Parsing and Grammar

Tokenization identifies individual symbols. Parsing determines how those symbols form a valid expression.

Consider:

```text
a|bc
```

The tokenizer can identify the literal characters and the alternation symbol, but it does not determine the intended grouping.

The parser does.

The correct interpretation is:

```text
a | (bc)
```

because concatenation has higher precedence than alternation.

## 7.1 What Is a Grammar?

A grammar describes how valid expressions can be constructed from smaller expressions.

A simplified grammar for the project's core syntax is:

```text
expression  := sequence ("|" sequence)*
sequence    := repetition+
repetition  := atom (quantifier)*
atom        := literal
             | "."
             | "(" expression ")"
             | character_class
             | shorthand

quantifier  := "*" | "+" | "?" | range_quantifier
```

This is a conceptual grammar. The implementation contains additional details for character classes, range quantifiers, and error handling.

The important idea is that a complex expression is built from smaller expressions according to defined rules.

## 7.2 Why Parsing Must Be Separate from Tokenization

Consider:

```text
a(b|c)*d
```

Tokenization tells us which symbols appear.

Parsing tells us:

- The parentheses contain an alternation.
- The star applies to the entire grouped expression.
- The outer expression is concatenated in sequence.

These are structural relationships, not simply character classifications.

The AST records those relationships so that the NFA builder can use them.

---

<a id="8-operator-precedence-and-associativity"></a>
# 8. Operator Precedence and Associativity

Precedence determines which operator binds more tightly when parentheses do not specify the grouping explicitly.

For the supported core syntax:

```text
Highest precedence
    Atoms and groups
    Repetition (*, +, ?, ranges)
    Concatenation
    Alternation (|)
Lowest precedence
```

## 8.1 Repetition Before Concatenation

Consider:

```text
ab*
```

The star applies to `b`, not to the entire sequence `ab`.

The structure is:

```text
SEQ
├── a
└── STAR
    └── b
```

But:

```text
(ab)*
```

has a star whose child is the grouped sequence `ab`.

Parentheses change the structure.

## 8.2 Concatenation Before Alternation

Consider:

```text
a|bc
```

The correct structure is:

```text
ALT
├── a
└── SEQ
    ├── b
    └── c
```

It is not:

```text
SEQ
├── ALT
│   ├── a
│   └── b
└── c
```

The second tree would represent a different expression.

## 8.3 Associativity

Repeated concatenation is commonly grouped from the left:

```text
abc
```

becomes:

```text
SEQ
├── SEQ
│   ├── a
│   └── b
└── c
```

Repeated alternation is also grouped from the left in this parser:

```text
a|b|c
```

becomes:

```text
ALT
├── ALT
│   ├── a
│   └── b
└── c
```

This is known as left associativity.

For ordinary alternation, this grouping preserves the same set of accepted strings, although it determines the shape of the AST.

## 8.4 Why This Matters

If precedence is implemented incorrectly, the engine can build a valid AST for the wrong expression.

The NFA builder may then work perfectly while the final matching behavior is still incorrect.

Correct parsing is therefore essential to the entire pipeline.

---

<a id="9-recursive-descent-parsing"></a>
# 9. Recursive Descent Parsing

Recursive descent is a parsing technique in which functions implement grammar rules and call one another to parse nested structures.

Our parser is organized around four central functions:

```text
parse_alt()
    |
    v
parse_seq()
    |
    v
parse_repeat()
    |
    v
parse_atom()
```

## 9.1 `parse_atom()`

An atom is the smallest unit handled by the parser.

Examples include:

```text
a
.
(...)
[abc]
\d
```

The function inspects the current token and constructs the corresponding AST node.

When it encounters an opening parenthesis, it recursively parses the expression inside the group until the closing parenthesis is found.

## 9.2 `parse_repeat()`

This function parses an atom and then checks whether repetition operators follow it.

For example:

```text
a*
```

is processed by:

1. Parsing `a` as an atom.
2. Seeing the `*` token.
3. Creating a `NODE_STAR` node.
4. Making the literal node its child.

The same function handles the other supported postfix quantifiers.

## 9.3 `parse_seq()`

This function parses one or more repetition expressions and combines them using `NODE_SEQ`.

For:

```text
abc
```

it constructs the equivalent of:

```text
SEQ(SEQ(a, b), c)
```

It stops when it encounters a token that ends the current sequence, such as `TOKEN_PIPE`, `TOKEN_RPAREN`, or `TOKEN_EOF`.

## 9.4 `parse_alt()`

This function parses a sequence and then processes any following alternation operators.

For:

```text
a|b
```

it constructs:

```text
ALT(a, b)
```

The right side of an alternation is itself parsed as a sequence, so expressions such as `a|bc` receive the correct precedence.

## 9.5 Parser State

The parser maintains a small state structure:

```c
typedef struct {
    TokenList *tokens;
    int pos;
} ParserState;
```

`tokens` points to the token list, and `pos` identifies the current token.

The helper `current()` returns the current token, while `advance()` moves the cursor forward.

This is effectively a cursor over an already-tokenized sequence.

## 9.6 Errors During Parsing

Examples of syntax errors include:

```text
a(
(a|b
a{2
```

The parser must detect invalid or incomplete structures and return an error instead of passing an invalid AST to the NFA builder.

A parser error is different from a valid pattern that simply fails to match an input string.

---

<a id="10-abstract-syntax-tree-ast"></a>
# 10. Abstract Syntax Tree (AST)

An **Abstract Syntax Tree** represents the logical structure of an expression.

It omits unnecessary details of the original text and focuses on relationships between operations.

## 10.1 Why an AST Is Needed

Consider:

```text
a(b|c)*d
```

The raw string contains eight characters, but those characters have a more meaningful structure:

- `a` is a literal.
- `b|c` is an alternation.
- The alternation is grouped.
- The group is repeated.
- The entire result is concatenated with `a` and `d`.

An AST makes this structure explicit.

## 10.2 AST Node Types

The project's `NodeType` enum includes categories such as:

```text
NODE_LITERAL
NODE_DOT
NODE_SEQ
NODE_ALT
NODE_STAR
NODE_PLUS
NODE_QUESTION
NODE_GROUP
NODE_CHAR_CLASS
NODE_SHORTHAND
NODE_RANGE
```

Each type identifies a different operation or expression element.

## 10.3 AST Node Structure

The project represents nodes using an `ASTNode` structure with fields such as:

```c
typedef struct ASTNode {
    NodeType type;
    char value;
    struct ASTNode *left;
    struct ASTNode *right;
    int min;
    int max;
    int negate;
    char class_chars[128];
    int class_char_count;
} ASTNode;
```

Different node types use different fields.

| Node type | Important fields |
|---|---|
| `NODE_LITERAL` | `value` |
| `NODE_DOT` | `type` |
| `NODE_SEQ` | `left`, `right` |
| `NODE_ALT` | `left`, `right` |
| `NODE_STAR` | `left` |
| `NODE_PLUS` | `left` |
| `NODE_QUESTION` | `left` |
| `NODE_GROUP` | `left` |
| `NODE_SHORTHAND` | `value` |
| `NODE_CHAR_CLASS` | `class_chars`, `class_char_count`, `negate` |
| `NODE_RANGE` | `left`, `min`, `max` |

A binary node such as `SEQ` or `ALT` has two children. A unary node such as `STAR` has one child. A literal is a leaf.

## 10.4 Why Nodes Use Pointers

The number and shape of nodes depend on the pattern, so the AST is represented as a dynamically allocated tree.

The `left` and `right` pointers refer to other AST nodes.

For example:

```text
SEQ
├── a
└── STAR
    └── b
```

the `SEQ` node points to the literal `a` and the `STAR` node. The `STAR` node points to the literal `b`.

This is why C pointer and memory-management concepts matter to this project.

## 10.5 AST and the NFA Builder

The AST is an intermediate representation between parsing and automata construction.

The parser answers:

> What does this pattern mean structurally?

The NFA builder answers:

> How can that structure be represented as a state machine?

This separation lets the NFA builder work without needing to understand the original regex string or tokenization rules.

---

<a id="11-finite-state-machines"></a>
# 11. Finite State Machines

A **finite state machine**, or FSM, is a computational model with a finite number of states and transitions.

A machine processes input by moving between states according to its transition rules.

## 11.1 Basic Components

A finite automaton contains:

- A finite set of states
- An input alphabet
- A transition relation or transition function
- A start state
- One or more accepting states

A state represents the machine's current progress toward recognizing a pattern.

## 11.2 Example: Recognizing `ab`

A simple machine for the exact pattern `ab` can be represented as:

```text
(start) --a--> (q1) --b--> ((accept))
```

The machine starts at the first state.

After reading `a`, it moves to `q1`.

After reading `b`, it reaches the accepting state.

For full-string matching:

```text
Input: ab
Result: MATCH
```

But:

```text
Input: ac
Result: NO MATCH
```

because the required transition for `b` cannot be followed.

Likewise, `abc` does not match the exact pattern `ab` when the entire input must be consumed.

## 11.3 Accepting States

An accepting state indicates that the consumed input can represent a valid match.

However, reaching an accepting state before the input ends does not automatically mean the entire input matches.

For full-string matching, the input must be consumed and the final active-state set must contain an accepting state.

## 11.4 NFA and DFA

Two important kinds of finite automata are:

- Deterministic Finite Automata (DFA)
- Nondeterministic Finite Automata (NFA)

Both can recognize regular languages, but their transition rules differ.

---

<a id="12-nondeterministic-finite-automata-nfa"></a>
# 12. Nondeterministic Finite Automata (NFA)

An NFA allows a state and input symbol to lead to multiple possible next states. It may also have transitions that consume no input.

This makes NFAs convenient for representing regex operations such as alternation and repetition.

## 12.1 Nondeterminism

Consider:

```text
a|b
```

The machine needs to recognize either alternative.

Conceptually:

```text
             --> [a] --
            /          \
START -----              ----> ACCEPT
            \          /
             --> [b] --
```

An NFA can branch into multiple possible paths.

The simulator does not need to choose one path and discard the others. It can track the set of all reachable states.

## 12.2 Transition Representation

An NFA transition contains information such as:

- The transition type
- The target state
- The character or condition associated with the transition

A character transition consumes input.

An epsilon transition does not.

The exact structure of transitions is defined in the project's NFA headers.

## 12.3 Why Use an NFA?

Thompson's construction gives a systematic way to create an NFA from the AST.

Each regex operation can be represented by a small graph, and those graphs can be combined.

This is simpler than trying to create a separate, manually designed state machine for every possible regex.

---

<a id="13-epsilon-transitions-and-epsilon-closure"></a>
# 13. Epsilon Transitions and Epsilon Closure

## 13.1 What Is an Epsilon Transition?

An epsilon transition, written \(\varepsilon\), moves the automaton from one state to another without consuming an input character.

For example:

```text
q0 --ε--> q1
```

means the machine can move from `q0` to `q1` without advancing the input position.

This is useful for representing choices, optional expressions, and repetition.

## 13.2 Why Epsilon Transitions Are Useful

For:

```text
a|b
```

the NFA can use epsilon transitions to branch into two alternatives without consuming a character to make the choice.

The actual character is consumed only by the transition representing `a` or `b`.

This allows Thompson's construction to combine smaller automata without requiring complicated transition rules.

## 13.3 Epsilon Closure

The **epsilon closure** of a set of states is the set containing:

1. Every state already in the set.
2. Every state reachable from those states using zero or more epsilon transitions.

For example:

```text
q0 --ε--> q1 --ε--> q2
```

The epsilon closure of `{q0}` is:

```text
{q0, q1, q2}
```

The closure includes the original state because zero epsilon transitions are allowed.

It also includes `q1` and `q2` because they are reachable without consuming input.

## 13.4 How to Compute Epsilon Closure

A common algorithm uses a worklist:

1. Put the initial states into a result set and a worklist.
2. Remove one state from the worklist.
3. Examine its outgoing epsilon transitions.
4. For every target not already in the result set, add it to the result set and worklist.
5. Continue until the worklist is empty.

Pseudocode:

```text
epsilon_closure(initial_states):
    result = initial_states
    worklist = initial_states

    while worklist is not empty:
        state = remove_one(worklist)

        for each epsilon_transition from state:
            target = transition.target

            if target is not in result:
                add target to result
                add target to worklist

    return result
```

Tracking visited states prevents cycles from causing infinite traversal.

## 13.5 When Must Closure Be Computed?

The simulator needs the epsilon closure:

- Before consuming the first input character.
- After following character transitions for an input character.
- Whenever it needs to determine all states reachable without consuming more input.

The simulator's exact API determines how this operation is exposed, but the underlying requirement is the same.

---

<a id="14-thompsons-construction"></a>
# 14. Thompson's Construction

Thompson's construction is an algorithm for converting a regular expression into an equivalent epsilon-NFA.

The key idea is to build small automaton fragments for basic expressions and combine them to represent larger expressions.

## 14.1 NFA Fragments

A fragment represents part of the final NFA.

Conceptually, it has:

- An entry state
- An exit or patch point
- A graph of transitions between them

When fragments are combined, their entry and exit relationships are connected according to the regex operator.

Some implementations use explicit outgoing transition lists that are patched later. Others use different state-linking representations. The exact representation depends on the builder design.

The important invariant is that each fragment must expose enough information for the parent operation to connect it correctly.

## 14.2 Literal Construction

For a literal `a`, create two states with a character transition:

```text
START --a--> ACCEPT
```

The transition consumes `a`.

## 14.3 Concatenation

For:

```text
ab
```

construct a fragment for `a` and a fragment for `b`, then connect the exit of the first to the entry of the second.

Conceptually:

```text
START --a--> q1 --b--> ACCEPT
```

The first expression must be matched before the second.

## 14.4 Alternation

For:

```text
a|b
```

create a new start and exit structure:

```text
                 ε --> [a] --> ε
                /               \
START ---------                   -------- ACCEPT
                \               /
                 ε --> [b] --> ε
```

The epsilon transitions allow the machine to take either branch.

The machine accepts if either alternative leads to the accepting state.

## 14.5 Kleene Star

For:

```text
a*
```

the machine must allow zero or more occurrences.

Conceptually:

```text
               ε ------------------+
              /                    |
START ------> [a] -----------------+
   |                               |
   +------------- ε --------------> ACCEPT
```

The epsilon paths allow the machine to:

- Skip the expression entirely.
- Enter the expression and repeat it.
- Exit after any completed repetition.

The exact state layout depends on the construction, but the accepted language must be equivalent to `a*`.

## 14.6 Plus and Optional

The `+` operator requires at least one occurrence before additional repetitions are allowed.

The `?` operator allows the expression to be skipped or executed once.

Both can be implemented by combining fragments and epsilon transitions.

## 14.7 Grouping

Grouping affects how the parser structures the expression.

Once the AST has been created, a group node can often be compiled by compiling its child expression. Parentheses usually do not require a special kind of NFA transition by themselves.

## 14.8 Bounded Repetition

For:

```text
a{2,4}
```

the resulting automaton must allow two, three, or four occurrences of `a`.

For:

```text
a{3}
```

it must allow exactly three occurrences.

For:

```text
a{2,}
```

it must allow two or more occurrences.

The AST stores the repetition bounds, but the NFA builder must implement them correctly. A parsed range is not automatically a working range quantifier until construction supports it.

## 14.9 Why Thompson Construction Fits This Project

The AST already separates expressions into nodes such as:

```text
LITERAL
SEQ
ALT
STAR
PLUS
QUESTION
GROUP
CHAR_CLASS
SHORTHAND
RANGE
```

The NFA builder can recursively process each node type and create the corresponding fragment.

This makes the conversion systematic:

```text
AST Node
   |
   v
Compile its children
   |
   v
Combine their NFA fragments
   |
   v
Return the resulting fragment
```

The builder is therefore the bridge between the syntax tree and the executable state machine.

---

<a id="15-nfa-simulation-and-matching"></a>
# 15. NFA Simulation and Matching

After constructing the NFA, the engine needs to determine whether the input is accepted.

This is the simulator's responsibility.

## 15.1 Active State Sets

Because an NFA can branch into several possible paths, the simulator maintains a set of active states.

Instead of asking:

> Which single state should the machine enter next?

it asks:

> Which states could the machine be in after processing this input prefix?

This is the central idea of NFA simulation.

## 15.2 Simulation Algorithm

Let:

- \(Q\) be the set of NFA states.
- \(S\) be the current active-state set.
- \(c\) be the next input character.
- \(\delta(S,c)\) be the set of targets reachable from states in \(S\) through transitions that consume \(c\).

The simulator repeatedly computes the next set and its epsilon closure.

Conceptually:

```text
active = epsilon_closure({start_state})

for each character c in input:
    next_states = empty set

    for each state in active:
        for each matching character transition:
            add its target to next_states

    active = epsilon_closure(next_states)

if accept_state is in active:
    MATCH
else:
    NO MATCH
```

This is the core matching algorithm.

The exact implementation may use arrays, bitsets, or another state-set representation, but the logic is the same.

## 15.3 Dry Run: Pattern `ab`

Suppose the NFA is:

```text
q0 --a--> q1 --b--> q2
```

where `q0` is the start and `q2` is accepting.

Input:

```text
ab
```

Initially:

```text
Active states: {q0}
```

Read `a`:

```text
q0 --a--> q1
```

New active set:

```text
{q1}
```

Read `b`:

```text
q1 --b--> q2
```

New active set:

```text
{q2}
```

No input remains, and `q2` is accepting.

Result:

```text
MATCH
```

For input `ac`, there is no transition matching `c` from the active state after reading `a`. The active set becomes empty, so the input cannot be accepted.

## 15.4 Why Epsilon Closure Is Essential

Consider:

```text
a*
```

The NFA must be able to accept the empty string without consuming a character.

If the simulator checks only character transitions, it may fail to discover the accepting state reachable through epsilon transitions.

Epsilon closure ensures that every state reachable without consuming input is considered.

## 15.5 Full-String Acceptance

For full-string matching, the acceptance check happens after the input has been consumed.

The rule is:

```text
All input consumed
        AND
Accept state is active
        =
MATCH
```

If the accepting state becomes active halfway through the input but the remaining characters cannot be processed, the entire input does not match.

## 15.6 Why This Avoids Recursive Backtracking

A backtracking engine may explore one path, discover that it fails, and return to an earlier choice to try another path.

The NFA simulator instead keeps all currently reachable states together.

For example, if an alternation creates two possible paths, the simulator can track both rather than recursively exploring one path at a time.

This avoids the exponential path-exploration behavior associated with catastrophic backtracking for the supported regular-language features.

---

<a id="16-character-classes-shorthands-and-quantifiers"></a>
# 16. Character Classes, Shorthands, and Quantifiers

These features need special attention because the parser can recognize their syntax, but the NFA builder and simulator must also implement their behavior.

## 16.1 Character Classes

A character class such as:

```text
[abc]
```

means one character from the set `{a,b,c}`.

The parser stores class information in the AST node.

The NFA builder and simulator must then represent and evaluate the class correctly.

A class should not be confused with concatenation: `[abc]` represents one character chosen from a set, not the three-character string `abc`.

## 16.2 Character Ranges

A range such as:

```text
[a-d]
```

represents:

```text
a, b, c, d
```

In the current parser design, ranges are expanded into a stored character list.

That is a representation choice, not a requirement of regex theory. Another implementation could store range boundaries and check membership when matching.

The current representation needs bounds checking because the AST's character array has a finite capacity.

## 16.3 Negated Classes

A class such as:

```text
[^abc]
```

matches a character that is not in the listed set, according to the engine's supported character domain.

The `negate` field records that the class is negated.

The simulator must interpret that flag consistently with the chosen character domain.

## 16.4 Shorthands

The tokenizer recognizes shorthand escapes, and the AST stores their shorthand character.

The matching implementation must define the actual membership test.

For example, if `\d` means ASCII digits, its set is:

```text
0 1 2 3 4 5 6 7 8 9
```

The complement `\D` should be defined relative to the engine's supported character domain, not assumed to mean every possible Unicode character.

## 16.5 Quantifier Ranges

The parser stores the minimum and maximum repetition counts in a `NODE_RANGE` node.

For an exact range:

```text
a{3}
```

the bounds are:

```text
min = 3
max = 3
```

For an open-ended range:

```text
a{2,}
```

the project uses:

```text
min = 2
max = -1
```

where `-1` indicates no finite upper bound.

The NFA builder must interpret these values correctly when constructing the repeated fragment.

---

<a id="17-matching-semantics-and-limitations"></a>
# 17. Matching Semantics and Limitations

A regex engine's behavior depends not only on syntax, but also on its matching semantics.

## 17.1 Full Match vs Search

Full matching asks:

> Does the entire input belong to the language described by this pattern?

Search asks:

> Does some substring of the input match the pattern?

For example, pattern:

```text
cat
```

and input:

```text
wildcat
```

produce:

- Full match: `NO MATCH`
- Substring search: `MATCH`, if search semantics are implemented

This project should be understood according to the behavior implemented by its CLI and simulator.

## 17.2 Empty Expressions

The empty string is a valid concept in formal-language theory, and some regex engines support empty expressions.

However, a parser may intentionally reject an empty sequence.

The current parser's `parse_seq()` reports an error when no expression is parsed where content is expected.

Consequently, empty patterns or empty alternatives should not be assumed to work unless explicitly supported and tested.

This is an implementation choice rather than a limitation of finite automata themselves.

## 17.3 Backreferences

A backreference refers to text matched earlier by a capture group.

For example, a regex dialect might allow a pattern conceptually like:

```text
(word)\1
```

to match a repeated occurrence of the same captured text.

Ordinary finite automata do not store arbitrary-length captured substrings for later comparison. Backreferences therefore fall outside the regular-language model used by this project.

They are deliberately excluded.

## 17.4 Lookahead and Lookbehind

Lookaround assertions test surrounding input without consuming it as ordinary characters.

Examples include:

```text
(?=...)
(?!...)
(?<=...)
(?<!...)
```

These features introduce additional matching semantics and are not part of the project's intended core dialect.

## 17.5 Unicode

ASCII-oriented character classes are much simpler than full Unicode support.

Unicode introduces questions about character properties, case folding, encoding, and what counts as one character.

This project focuses on the core engine and automata algorithms rather than implementing a complete Unicode regex dialect.

## 17.6 Syntax Recognition Is Not Full Feature Support

A token type or AST node does not prove that a feature works end to end.

For a feature to be supported, the implementation needs to handle it throughout the relevant pipeline:

```text
Tokenizer
    |
    v
Parser
    |
    v
AST
    |
    v
NFA Builder
    |
    v
Simulator
    |
    v
Tests
```

For example, recognizing `^` as `TOKEN_CARET` does not by itself implement start-of-input anchoring.

This distinction is important when documenting the project's supported syntax.

---

<a id="18-time-and-space-complexity"></a>
# 18. Time and Space Complexity

Complexity describes how the amount of work or memory changes as the regex and input grow.

Let:

- \(m\) be the size of the regex or, more precisely for simulation, the number of NFA states.
- \(n\) be the length of the input string.

For Thompson construction, the generated NFA is linear in the size of the supported regex representation, subject to implementation details such as how bounded repetitions are expanded.

## 18.1 NFA Simulation

A straightforward NFA simulator tracks active states while processing each input character.

In a typical implementation, processing one character can examine transitions associated with many states.

A common upper-bound description is:

\[
O(mn)
\]

This is the basis for the predictable behavior of Thompson-style NFA simulation.

Strict complexity claims should reflect the actual implementation, including state-set operations, duplicate-state handling, character-class checks, and repetition expansion.

## 18.2 Space Complexity

The NFA requires storage for its states and transitions.

The simulator also needs storage for active and next-state sets and any worklist used for epsilon closure.

For a straightforward implementation, these structures can be bounded in terms of the number of NFA states:

\[
O(m)
\]

excluding input storage and other separately allocated structures.

## 18.3 Comparison with Backtracking

Some backtracking regex engines can revisit the same input positions through many different paths. Certain patterns and inputs can therefore produce exponential worst-case behavior.

A properly implemented Thompson NFA simulator tracks reachable states rather than repeatedly exploring equivalent paths.

This is why it is a useful foundation for predictable regex matching.

---

<a id="19-understanding-the-c-implementation"></a>
# 19. Understanding the C Implementation

This project is written in C, so understanding how data is represented and passed between modules is essential.

## 19.1 Header Files and Source Files

The project separates declarations from implementations.

For example:

```text
include/regex/token.h
src/tokenizer.c
```

The header defines token types, structures, and the public tokenizer function.

The source file implements the tokenizer.

This separation lets other source files use the tokenizer without needing to know the details of its implementation.

## 19.2 Structures

A C `struct` groups related data into one object.

The project uses structures to represent:

- Tokens
- Token lists
- Parser state
- AST nodes
- NFA states
- Transitions
- NFA metadata
- State sets

For example, an AST node groups the node type, value, child pointers, and any extra information required by the node.

## 19.3 Pointers

A pointer stores the address of an object.

Pointers are used extensively because the AST and NFA contain relationships between objects.

For example:

```c
ASTNode *left;
ASTNode *right;
```

store addresses of child nodes.

The arrow operator:

```c
node->left
```

accesses a member through a pointer.

It is equivalent to:

```c
(*node).left
```

Pointers allow the parser to connect dynamically allocated AST nodes without copying entire subtrees.

## 19.4 Dynamic Memory Allocation

AST nodes are created dynamically using `malloc()`.

For example:

```c
ASTNode *node = malloc(sizeof(ASTNode));
```

This reserves enough memory for an `ASTNode` and returns a pointer to it, or `NULL` if allocation fails.

The allocated memory remains reserved until it is released using `free()`.

A correct implementation must handle allocation failures and release nodes when they are no longer needed.

## 19.5 Recursive Functions

The parser is recursive because groups can contain expressions that contain further groups.

For example:

```text
a(b(c|d)*|e)f
```

requires the parser to enter nested expressions.

The NFA builder may also be recursive because each AST node can contain child expressions that must be compiled before the parent operation is assembled.

Recursion here reflects the nested structure of the input, rather than being used merely for convenience.

## 19.6 Enumerations

Enums define a set of named integer constants.

The tokenizer uses `TokenType`, while the AST uses `NodeType`.

These enums describe different concepts:

- `TokenType` represents the symbols recognized in the source pattern.
- `NodeType` represents the operations and expressions identified by parsing.

For example:

```text
TOKEN_PIPE
    |
    v
Parser recognizes alternation
    |
    v
NODE_ALT
```

A token is a unit of syntax; an AST node is part of the parsed structure.

## 19.7 Fixed Arrays

The tokenizer's `TokenList` uses a fixed-size array.

The AST character-class representation also uses a fixed-size character array.

Fixed arrays simplify allocation and storage, but every write must respect the array's capacity.

## 19.8 Return Values and Error Buffers

The tokenizer returns a success/failure indicator and writes diagnostic information into a caller-provided error buffer.

The parser returns an `ASTNode *` and uses `NULL` to indicate failure.

These are different API conventions, but both allow the caller to detect an error and avoid proceeding with invalid data.

---

<a id="20-memory-management-and-error-handling"></a>
# 20. Memory Management and Error Handling

C does not automatically reclaim dynamically allocated objects.

The project must therefore manage ownership and cleanup explicitly.

## 20.1 AST Ownership

When the parser creates a node using `malloc()`, that node needs a clear owner.

For an AST, the root node provides access to the complete tree.

If each child belongs to its parent tree, cleanup can recursively visit the children and then free the parent.

Conceptually:

```text
free_ast(node):
    if node is NULL:
        return

    free_ast(node.left)
    free_ast(node.right)

    free(node)
```

This is post-order deallocation: children are freed before their parent.

The actual implementation should match the project's declared memory-management API.

## 20.2 Error Paths

Consider a parser that allocates several nodes and then encounters an invalid token.

Returning `NULL` alone does not automatically free the nodes that were allocated earlier.

A robust implementation must ensure partially constructed structures are cleaned up on failure.

The same principle applies to NFA construction and JSON serialization if those operations allocate memory.

## 20.3 Allocation Failure

`malloc()` can fail.

Code that dereferences its result without checking for `NULL` risks undefined behavior.

Allocation failure should be handled deliberately, particularly in functions that create nodes or grow state collections.

## 20.4 Buffer Bounds

Fixed-size arrays and error buffers need bounds checks.

For example, the parser expands ranges into `class_chars`. A sufficiently large expansion must not write beyond the array.

Similarly, formatting an error message into a buffer must respect that buffer's capacity.

## 20.5 Why This Matters

Memory management is part of correctness, not just cleanup.

A regex engine can produce correct results on ordinary inputs while still leaking memory or accessing invalid memory on error paths.

Testing and debugging should therefore cover both matching behavior and resource management.

---

<a id="21-end-to-end-example"></a>
# 21. End-to-End Example

This section follows a complete example through the conceptual pipeline.

Consider:

```text
Pattern: a(b|c)*d
Input:   abcbcd
```

## Step 1: Understand the Pattern

The pattern requires:

1. An `a`.
2. Zero or more occurrences of `b` or `c`.
3. A final `d`.

The input:

```text
abcbcd
```

has exactly this structure.

## Step 2: Tokenization

The tokenizer produces tokens corresponding to:

```text
a
(
b
|
c
)
*
d
EOF
```

The token types distinguish literals, grouping, alternation, and repetition.

## Step 3: Parsing

The parser applies the grammar rules.

The `b|c` expression becomes an alternation node.

The star wraps the grouped alternation.

The entire expression becomes a concatenation of `a`, the repeated group, and `d`.

## Step 4: AST

The AST is:

```text
             SEQ
            /   \
          SEQ    d
         /   \
        a    STAR
              |
             GROUP
               |
              ALT
             /   \
            b     c
```

## Step 5: NFA Construction

The NFA builder compiles the AST recursively.

It creates fragments for:

- Literal `a`
- Literal `b`
- Literal `c`
- Alternation between `b` and `c`
- Repetition of that alternation
- Literal `d`
- Concatenation of all parts

Epsilon transitions connect the alternatives and repetition paths.

The final graph has a designated start state and accepting state.

## Step 6: Initial State Set

The simulator begins from the start state and computes its epsilon closure.

The active set contains all states reachable without consuming an input character.

## Step 7: Consume `a`

The simulator follows transitions matching `a` and computes the epsilon closure of the resulting states.

The active set now represents every valid state the NFA could occupy after reading the first character.

## Step 8: Consume `b`, `c`, `b`, `c`

The simulator processes each character in turn.

The repeated alternation permits each of these characters to be matched by either the `b` or `c` branch, as appropriate.

The epsilon transitions also preserve the ability to repeat the group or exit the repetition.

## Step 9: Consume `d`

The `d` transition moves the machine toward the accepting state.

After the final input character, the simulator computes the required closure.

## Step 10: Decide the Result

The entire input has been consumed, and the accepting state is reachable.

Therefore:

```text
[MATCH] Pattern 'a(b|c)*d' matches input 'abcbcd'
```

Now consider:

```text
Pattern: a(b|c)*d
Input:   abcbc
```

The input ends without consuming the required final `d`.

The accepting state is not active after all input has been processed.

Therefore:

```text
[NO MATCH] Pattern 'a(b|c)*d' does not match input 'abcbc'
```

This example illustrates how each stage contributes to the final result.

---

<a id="22-project-architecture-and-file-responsibilities"></a>
# 22. Project Architecture and File Responsibilities

The project separates public declarations, implementations, tests, and build configuration.

The following describes the intended responsibility of each module. The exact file list should be kept synchronized with the repository.

## 22.1 Tokenizer

```text
include/regex/token.h
src/tokenizer.c
```

Defines tokens, token types, the token-list representation, and the tokenizer implementation.

## 22.2 AST

```text
include/regex/ast.h
```

Defines the AST node types and the structure used to represent parsed expressions.

## 22.3 Parser

```text
include/regex/parser.h
src/parser.c
```

Defines the parser interface and implements recursive descent parsing, including precedence, grouping, quantifiers, and syntax validation.

AST cleanup belongs to the memory-management interface exposed by the project.

## 22.4 NFA Builder

```text
include/regex/nfa.h
include/regex/nfa_builder.h
src/nfa_builder.c
```

Defines NFA-related structures and the interface for converting an AST into a Thompson NFA.

## 22.5 Simulator

```text
include/regex/simulate.h
src/simulate.c
```

Defines the simulator interface and the state-set operations required for epsilon closure and input processing.

## 22.6 CLI

```text
src/main.c
```

Handles command-line arguments, invokes the relevant modules, and displays results.

## 22.7 JSON Export

```text
include/regex/json_export.h
src/json_export.c
```

Serializes selected internal representations into JSON so that external tools can inspect the AST or NFA.

## 22.8 Tests

```text
tests/
```

Contains automated tests for the individual modules and end-to-end matching behavior.

Testing modules separately helps identify whether a problem originates in tokenization, parsing, construction, or simulation.

## 22.9 Makefile

```text
Makefile
```

Defines build and test commands so that the project can be compiled consistently without manually repeating the full compiler command.

---

<a id="23-current-scope-and-possible-extensions"></a>
# 23. Current Scope and Possible Extensions

The project's primary purpose is to implement and understand the core regex-to-automaton pipeline.

## Core Scope

The core architecture includes:

- Tokenization
- Recursive descent parsing
- AST construction
- Thompson NFA construction
- Epsilon closure
- NFA simulation
- Command-line matching
- Error handling
- Automated tests

Diagnostic modes and JSON export make the internal behavior easier to inspect.

## Possible Extensions

Future additions could include:

- A visual representation of AST nodes and NFA states.
- An interactive step-through mode for simulation.
- DFA construction from the NFA.
- More character-class features.
- Improved diagnostics for malformed patterns.
- Broader Unicode support.
- Fuzz testing and additional memory-safety checks.
- Additional regex syntax where appropriate.

These extensions should preserve the separation between parsing, automata construction, simulation, and presentation.

---

<a id="24-glossary"></a>
# 24. Glossary

| Term | Meaning |
|---|---|
| Regex | A notation for describing a pattern of strings |
| Alphabet | A set of symbols |
| String | A finite sequence of symbols |
| Language | A set of strings |
| Regular language | A language recognizable by a finite automaton |
| Token | A classified unit of syntax |
| Lexer / tokenizer | The component that converts source characters into tokens |
| Parser | The component that determines grammatical structure |
| Grammar | Rules describing valid expressions |
| Precedence | The priority of operators when grouping expressions |
| Associativity | The grouping direction for repeated operators of equal precedence |
| AST | A tree representing the structure of an expression |
| FSM | Finite State Machine |
| DFA | Deterministic Finite Automaton |
| NFA | Nondeterministic Finite Automaton |
| State | A position or configuration in an automaton |
| Transition | A permitted movement between states |
| Accepting state | A state indicating that the consumed input can be accepted |
| Epsilon transition | A transition that consumes no input |
| Epsilon closure | All states reachable through zero or more epsilon transitions |
| Thompson construction | An algorithm that converts a regex into an epsilon-NFA |
| Active-state set | The states the NFA may occupy after consuming an input prefix |
| Backtracking | A matching strategy that revisits earlier choices when a path fails |
| Full-string matching | Checking whether the entire input matches the pattern |
| Serialization | Converting internal data structures into a format such as JSON |
| Complexity | How execution time or memory use scales with input size |

---

# Final Mental Model

The most important concept to remember is that the engine does not match a regex by repeatedly guessing what the original string means.

It transforms the pattern through several representations:

```text
Raw Text
   |
   | Tokenization
   v
Tokens
   |
   | Parsing
   v
AST
   |
   | Thompson Construction
   v
Epsilon-NFA
   |
   | NFA Simulation
   v
Match Result
```

Each representation solves a different problem:

- **Tokens** identify the syntax.
- **The AST** identifies the structure.
- **The NFA** represents the pattern as a state machine.
- **The simulator** determines whether the input is accepted.

Understanding how and why these representations change is the key to understanding the entire project.
