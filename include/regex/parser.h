// include/regex/parser.h
#ifndef REGEX_PARSER_H
#define REGEX_PARSER_H

#include "regex/token.h"
#include "regex/ast.h"

// Parses a token list into an AST. Returns NULL on error (writes message into error_msg).
ASTNode *parse(TokenList *tokens, char *error_msg);

// Recursively frees all heap-allocated nodes in the AST.
void free_ast(ASTNode *root);

#endif