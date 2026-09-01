// include/regex/nfa_builder.h
#ifndef REGEX_NFA_BUILDER_H
#define REGEX_NFA_BUILDER_H

#include "regex/ast.h"
#include "regex/nfa.h"

// Builds an NFA from an AST using Thompson Construction.
void build_nfa(ASTNode *root, NFA *nfa);

#endif