#ifndef REGEX_JSON_EXPORT_H
#define REGEX_JSON_EXPORT_H

#include <stdio.h>
#include "regex/ast.h"
#include "regex/nfa.h"

/* Exports the Abstract Syntax Tree as JSON */
void export_ast_json(const ASTNode *root, FILE *out);

/* Exports the NFA states and transitions as JSON */
void export_nfa_json(const NFA *nfa, FILE *out);

#endif
