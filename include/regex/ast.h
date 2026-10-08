// include/regex/ast.h
#ifndef REGEX_AST_H
#define REGEX_AST_H

typedef enum {
    NODE_LITERAL,      // a
    NODE_DOT,          // .
    NODE_SEQ,          // ab  (left then right)
    NODE_ALT,          // a|b (left OR right)
    NODE_STAR,         // a*
    NODE_PLUS,         // a+
    NODE_QUESTION,     // a?
    NODE_GROUP,        // (a)
    NODE_CHAR_CLASS,   // [a-z] or [^a-z]
    NODE_SHORTHAND,    // \d \w \s \D \W \S
    NODE_RANGE         // a{m,n}
} NodeType;

typedef struct ASTNode {
    NodeType type;

    char value;              // used by LITERAL, SHORTHAND

    struct ASTNode *left;     // used by SEQ, ALT, STAR, PLUS, QUESTION, GROUP, RANGE
    struct ASTNode *right;    // used only by SEQ, ALT

    int min;                  // used by RANGE ({m,...})
    int max;                  // used by RANGE ({...,n}), -1 means "no upper bound"

    int negate;                // used by CHAR_CLASS ([^...] vs [...])
    char class_chars[128];      // used by CHAR_CLASS - which characters are in the set
    int class_char_count;       // how many entries in class_chars are filled

} ASTNode;

#endif