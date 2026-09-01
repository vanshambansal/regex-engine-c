// src/parser.c
#include <stdio.h>
#include <stdlib.h>
#include "regex/parser.h"

typedef struct
{
    TokenList *tokens;
    int pos;
    
} ParserState;

static ASTNode *parse_atom(ParserState *ps, char *error_msg);
static ASTNode *parse_repeat(ParserState *ps, char *error_msg);
static ASTNode *parse_seq(ParserState *ps, char *error_msg);
static ASTNode *parse_alt(ParserState *ps, char *error_msg);

static ASTNode *make_node(NodeType type)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = type;
    node->left = NULL;
    node->right = NULL;
    node->min = 0;
    node->max = 0;
    node->negate = 0;
    node->class_char_count = 0;
    return node;
}

static Token current(ParserState *ps)
{
    return ps->tokens->tokens[ps->pos];
}

static void advance(ParserState *ps)
{
    ps->pos++;
}

static ASTNode *parse_atom(ParserState *ps, char *error_msg)
{
    Token tok = current(ps);

    if (tok.type == TOKEN_LITERAL)
    {
        ASTNode *node = make_node(NODE_LITERAL);
        node->value = tok.value;
        advance(ps);
        return node;
    }

    if (tok.type == TOKEN_DOT)
    {
        ASTNode *node = make_node(NODE_DOT);
        advance(ps);
        return node;
    }

    if (tok.type == TOKEN_LPAREN)
    {
        advance(ps);

        ASTNode *inner = parse_alt(ps, error_msg);
        if (inner == NULL)
        {
            return NULL;
        }

        if (current(ps).type != TOKEN_RPAREN)
        {
            sprintf(error_msg, "Expected closing ')'");
            return NULL;
        }
        advance(ps);

        ASTNode *node = make_node(NODE_GROUP);
        node->left = inner;
        return node;
    }
    if (tok.type == TOKEN_LBRACKET)
    {
        advance(ps);

        ASTNode *node = make_node(NODE_CHAR_CLASS);

        if (current(ps).type == TOKEN_CARET)
        {
            node->negate = 1;
            advance(ps);
        }

        while (current(ps).type != TOKEN_RBRACKET)
        {
            if (current(ps).type == TOKEN_EOF)
            {
                sprintf(error_msg, "Unterminated character class, missing ']'");
                return NULL;
            }

            char start_char = current(ps).value;
            advance(ps);

            if (current(ps).type == TOKEN_LITERAL && current(ps).value == '-')
            {
                advance(ps);
                char end_char = current(ps).value;
                advance(ps);

                for (char c = start_char; c <= end_char; c++)
                {
                    node->class_chars[node->class_char_count] = c;
                    node->class_char_count++;
                }
            }
            else
            {
                node->class_chars[node->class_char_count] = start_char;
                node->class_char_count++;
            }
        }

        advance(ps);
        return node;
    }
    if (tok.type == TOKEN_SHORTHAND)
    {
        ASTNode *node = make_node(NODE_SHORTHAND);
        node->value = tok.value;
        advance(ps);
        return node;
    }

    sprintf(error_msg, "Unexpected token in pattern");
    return NULL;
}


static int parse_number(ParserState *ps) {
    int value = 0;
    while (current(ps).type == TOKEN_LITERAL &&
           current(ps).value >= '0' && current(ps).value <= '9') {
        value = value * 10 + (current(ps).value - '0');
        advance(ps);
    }
    return value;
}

static ASTNode *parse_repeat(ParserState *ps, char *error_msg) {
    ASTNode *node = parse_atom(ps, error_msg);
    if (node == NULL) return NULL;

    for (;;) {
        Token tok = current(ps);

        if (tok.type == TOKEN_STAR || tok.type == TOKEN_PLUS || tok.type == TOKEN_QUESTION) {
            NodeType rep_type = (tok.type == TOKEN_STAR) ? NODE_STAR :
                                 (tok.type == TOKEN_PLUS) ? NODE_PLUS : NODE_QUESTION;
            ASTNode *rep = make_node(rep_type);
            rep->left = node;
            node = rep;
            advance(ps);

        } else if (tok.type == TOKEN_LBRACE) {
            advance(ps);
            int min = parse_number(ps);
            int max;

            if (current(ps).type == TOKEN_COMMA) {
                advance(ps);
                max = (current(ps).type == TOKEN_RBRACE) ? -1 : parse_number(ps);
            } else {
                max = min;
            }

            if (current(ps).type != TOKEN_RBRACE) {
                sprintf(error_msg, "Expected closing '}'");
                return NULL;
            }
            advance(ps);

            ASTNode *rep = make_node(NODE_RANGE);
            rep->left = node;
            rep->min = min;
            rep->max = max;
            node = rep;

        } else {
            break;
        }
    }

    return node;
}

static ASTNode *parse_seq(ParserState *ps, char *error_msg) {
    ASTNode *left = NULL;

    while (current(ps).type != TOKEN_EOF &&
           current(ps).type != TOKEN_PIPE &&
           current(ps).type != TOKEN_RPAREN) {

        ASTNode *next = parse_repeat(ps, error_msg);
        if (next == NULL) return NULL;

        if (left == NULL) {
            left = next;
        } else {
            ASTNode *seq = make_node(NODE_SEQ);
            seq->left = left;
            seq->right = next;
            left = seq;
        }
    }

    if (left == NULL) {
        sprintf(error_msg, "Empty pattern where content was expected");
        return NULL;
    }

    return left;
}

static ASTNode *parse_alt(ParserState *ps, char *error_msg) {
    ASTNode *left = parse_seq(ps, error_msg);
    if (left == NULL) return NULL;

    while (current(ps).type == TOKEN_PIPE) {
        advance(ps);
        ASTNode *right = parse_seq(ps, error_msg);
        if (right == NULL) return NULL;

        ASTNode *alt = make_node(NODE_ALT);
        alt->left = left;
        alt->right = right;
        left = alt;
    }

    return left;
}

ASTNode *parse(TokenList *tokens, char *error_msg) {
    ParserState ps;
    ps.tokens = tokens;
    ps.pos = 0;

    ASTNode *result = parse_alt(&ps, error_msg);
    if (result == NULL) return NULL;

    if (current(&ps).type != TOKEN_EOF) {
        sprintf(error_msg, "Unexpected trailing input");
        return NULL;
    }

    return result;
}