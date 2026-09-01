#ifndef REGEX_TOKEN_H
#define REGEX_TOKEN_H

typedef enum {
    TOKEN_LITERAL,       // a, b, 5, etc.
    TOKEN_DOT,           // .
    TOKEN_STAR,          // *
    TOKEN_PLUS,          // +
    TOKEN_QUESTION,      // ?
    TOKEN_PIPE,          // |
    TOKEN_LPAREN,        // (
    TOKEN_RPAREN,        // )
    TOKEN_LBRACKET,      // [
    TOKEN_RBRACKET,      // ]
    TOKEN_CARET,         // ^  (anchor, or negation inside [^...])
    TOKEN_DOLLAR,        // $
    TOKEN_LBRACE,        // {
    TOKEN_RBRACE,        // }
    TOKEN_COMMA,         // ,  (inside {m,n})
    TOKEN_SHORTHAND,     // \d \w \s \D \W \S
    TOKEN_EOF            // end of pattern
} TokenType;

typedef struct {
    TokenType type;
    char value;   // literal char, or shorthand letter (d/w/s/D/W/S). Unused otherwise.
} Token;

#define MAX_TOKENS 256

typedef struct {
    Token tokens[MAX_TOKENS];
    int count;
} TokenList;

int tokenize(const char *pattern, TokenList *out, char *error_msg);

#endif