// src/tokenizer.c
#include <stdio.h>
#include <string.h>
#include "regex/token.h"

static int is_shorthand_letter(char c) {
    return c == 'd' || c == 'D' || c == 'w' || c == 'W' || c == 's' || c == 'S';
}

int tokenize(const char *pattern, TokenList *out, char *error_msg) {
    out->count = 0;
    int i = 0;

    while (pattern[i] != '\0') {
        char c = pattern[i];

        if (out->count >= MAX_TOKENS) {
            sprintf(error_msg, "Pattern too long (max %d tokens)", MAX_TOKENS);
            return 0;
        }

        Token tok;

        switch (c) {
            case '.': tok.type = TOKEN_DOT; tok.value = 0; break;
            case '*': tok.type = TOKEN_STAR; tok.value = 0; break;
            case '+': tok.type = TOKEN_PLUS; tok.value = 0; break;
            case '?': tok.type = TOKEN_QUESTION; tok.value = 0; break;
            case '|': tok.type = TOKEN_PIPE; tok.value = 0; break;
            case '(': tok.type = TOKEN_LPAREN; tok.value = 0; break;
            case ')': tok.type = TOKEN_RPAREN; tok.value = 0; break;
            case '[': tok.type = TOKEN_LBRACKET; tok.value = 0; break;
            case ']': tok.type = TOKEN_RBRACKET; tok.value = 0; break;
            case '^': tok.type = TOKEN_CARET; tok.value = 0; break;
            case '$': tok.type = TOKEN_DOLLAR; tok.value = 0; break;
            case '{': tok.type = TOKEN_LBRACE; tok.value = 0; break;
            case '}': tok.type = TOKEN_RBRACE; tok.value = 0; break;
            case ',': tok.type = TOKEN_COMMA; tok.value = 0; break;

            case '\\':
                i++;
                if (pattern[i] == '\0') {
                    sprintf(error_msg, "Dangling backslash at end of pattern");
                    return 0;
                }
                if (!is_shorthand_letter(pattern[i])) {
                    sprintf(error_msg, "Unknown escape sequence: \\%c", pattern[i]);
                    return 0;
                }
                tok.type = TOKEN_SHORTHAND;
                tok.value = pattern[i];
                break;

            default:
                tok.type = TOKEN_LITERAL;
                tok.value = c;
                break;
        }

        out->tokens[out->count] = tok;
        out->count++;
        i++;
    }

    Token eof_tok;
    eof_tok.type = TOKEN_EOF;
    eof_tok.value = 0;
    out->tokens[out->count] = eof_tok;
    out->count++;

    return 1;
}