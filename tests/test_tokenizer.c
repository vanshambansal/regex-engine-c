// tests/test_tokenizer.c
#include <stdio.h>
#include <string.h>
#include "regex/token.h"

static int pass_count = 0;
static int fail_count = 0;

static void check_token(Token tok, TokenType expected_type, char expected_value, const char *label) {
    if (tok.type == expected_type && tok.value == expected_value) {
        printf("PASS: %s\n", label);
        pass_count++;
    } else {
        printf("FAIL: %s (got type=%d value=%c)\n", label, tok.type, tok.value);
        fail_count++;
    }
}

static void test_literals(void) {
    TokenList list;
    char err[256];
    tokenize("ab", &list, err);

    check_token(list.tokens[0], TOKEN_LITERAL, 'a', "literal 'a'");
    check_token(list.tokens[1], TOKEN_LITERAL, 'b', "literal 'b'");
    check_token(list.tokens[2], TOKEN_EOF, 0, "EOF after 'ab'");
}

static void test_symbols(void) {
    TokenList list;
    char err[256];
    tokenize("a*b+", &list, err);

    check_token(list.tokens[0], TOKEN_LITERAL, 'a', "literal 'a'");
    check_token(list.tokens[1], TOKEN_STAR, 0, "star");
    check_token(list.tokens[2], TOKEN_LITERAL, 'b', "literal 'b'");
    check_token(list.tokens[3], TOKEN_PLUS, 0, "plus");
}

static void test_shorthand(void) {
    TokenList list;
    char err[256];
    tokenize("\\d\\W", &list, err);

    check_token(list.tokens[0], TOKEN_SHORTHAND, 'd', "shorthand \\d");
    check_token(list.tokens[1], TOKEN_SHORTHAND, 'W', "shorthand \\W");
}

static void test_dangling_backslash(void) {
    TokenList list;
    char err[256];
    int ok = tokenize("a\\", &list, err);

    if (ok == 0) {
        printf("PASS: dangling backslash rejected (%s)\n", err);
        pass_count++;
    } else {
        printf("FAIL: dangling backslash should have failed\n");
        fail_count++;
    }
}

int main(void) {
    test_literals();
    test_symbols();
    test_shorthand();
    test_dangling_backslash();

    printf("\n%d passed, %d failed\n", pass_count, fail_count);
    return fail_count == 0 ? 0 : 1;
}