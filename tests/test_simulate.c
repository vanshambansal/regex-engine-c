#include <stdio.h>
#include "regex/token.h"
#include "regex/parser.h"
#include "regex/nfa_builder.h"
#include "regex/simulate.h"

static int pass_count = 0;
static int fail_count = 0;

static void check(int condition, const char *label) {
    if (condition) {
        printf("PASS: %s\n", label);
        pass_count++;
    } else {
        printf("FAIL: %s\n", label);
        fail_count++;
    }
}

static int match_pattern(const char *pattern, const char *text) {
    char err[256];
    TokenList tokens;
    if (!tokenize(pattern, &tokens, err)) {
        return 0;
    }

    ASTNode *ast = parse(&tokens, err);
    if (!ast) {
        return 0;
    }

    NFA nfa;
    build_nfa(ast, &nfa);
    return simulate_nfa(&nfa, text);
}

static void test_literals(void) {
    check(match_pattern("hello", "hello") == 1, "literal exact match");
    check(match_pattern("hello", "world") == 0, "literal mismatch rejected");
    check(match_pattern("hello", "hell") == 0, "partial match rejected");
}

static void test_dot(void) {
    check(match_pattern("a.c", "abc") == 1, "dot matches character");
    check(match_pattern("a.c", "a9c") == 1, "dot matches digit");
    check(match_pattern("a.c", "ac") == 0, "dot requires one character");
}

static void test_alternation(void) {
    check(match_pattern("cat|dog", "cat") == 1, "alt matches left branch");
    check(match_pattern("cat|dog", "dog") == 1, "alt matches right branch");
    check(match_pattern("cat|dog", "bird") == 0, "alt rejects unlisted");
}

static void test_star(void) {
    check(match_pattern("ab*c", "ac") == 1, "star matches zero repetitions");
    check(match_pattern("ab*c", "abc") == 1, "star matches one repetition");
    check(match_pattern("ab*c", "abbbbc") == 1, "star matches multiple repetitions");
    check(match_pattern("ab*c", "ab") == 0, "star requires trailing c");
}

static void test_plus(void) {
    check(match_pattern("ab+c", "abc") == 1, "plus matches one repetition");
    check(match_pattern("ab+c", "abbc") == 1, "plus matches multiple repetitions");
    check(match_pattern("ab+c", "ac") == 0, "plus rejects zero repetitions");
}

static void test_question(void) {
    check(match_pattern("colou?r", "color") == 1, "question matches zero occurrence");
    check(match_pattern("colou?r", "colour") == 1, "question matches one occurrence");
    check(match_pattern("colou?r", "colouur") == 0, "question rejects two occurrences");
}

static void test_char_class(void) {
    check(match_pattern("[0-9]+", "12345") == 1, "char class range matches digits");
    check(match_pattern("[^a-z]+", "123") == 1, "negated class matches non-lowercase");
    check(match_pattern("[^a-z]+", "12a") == 0, "negated class rejects lowercase");
}

static void test_shorthand(void) {
    check(match_pattern("\\d+\\w+", "42hello") == 1, "shorthand matches digits and words");
    check(match_pattern("\\s+", "   ") == 1, "shorthand matches whitespace");
    check(match_pattern("\\S+", "abc") == 1, "negated shorthand matches non-whitespace");
}

static void test_range(void) {
    check(match_pattern("a{2,4}", "aaa") == 1, "range matches within bounds");
    check(match_pattern("a{2,4}", "a") == 0, "range rejects below min");
    check(match_pattern("a{2,4}", "aaaaa") == 0, "range rejects above max");
}

static void test_groups(void) {
    check(match_pattern("(ab|cd)+ef", "abcdabef") == 1, "nested group repetition matches");
    check(match_pattern("(ab|cd)+ef", "ef") == 0, "nested group requires at least one match");
}

int main(void) {
    test_literals();
    test_dot();
    test_alternation();
    test_star();
    test_plus();
    test_question();
    test_char_class();
    test_shorthand();
    test_range();
    test_groups();

    printf("\n%d passed, %d failed\n", pass_count, fail_count);
    return fail_count == 0 ? 0 : 1;
}
