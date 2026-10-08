#include <stdio.h>
#include <string.h>
#include "regex/json_export.h"

static const char *node_type_string(NodeType type) {
    switch (type) {
        case NODE_LITERAL: return "LITERAL";
        case NODE_DOT: return "DOT";
        case NODE_SEQ: return "SEQ";
        case NODE_ALT: return "ALT";
        case NODE_STAR: return "STAR";
        case NODE_PLUS: return "PLUS";
        case NODE_QUESTION: return "QUESTION";
        case NODE_GROUP: return "GROUP";
        case NODE_CHAR_CLASS: return "CHAR_CLASS";
        case NODE_SHORTHAND: return "SHORTHAND";
        case NODE_RANGE: return "RANGE";
        default: return "UNKNOWN";
    }
}

void export_ast_json(const ASTNode *root, FILE *out) {
    if (!root) {
        fprintf(out, "null");
        return;
    }

    fprintf(out, "{\"type\":\"%s\"", node_type_string(root->type));

    if (root->type == NODE_LITERAL || root->type == NODE_SHORTHAND) {
        if (root->value == '"' || root->value == '\\') {
            fprintf(out, ",\"value\":\"\\%c\"", root->value);
        } else {
            fprintf(out, ",\"value\":\"%c\"", root->value);
        }
    } else if (root->type == NODE_CHAR_CLASS) {
        fprintf(out, ",\"negate\":%s", root->negate ? "true" : "false");
        fprintf(out, ",\"chars\":\"");
        for (int i = 0; i < root->class_char_count; i++) {
            char c = root->class_chars[i];
            if (c == '"' || c == '\\') {
                fprintf(out, "\\%c", c);
            } else {
                fputc(c, out);
            }
        }
        fprintf(out, "\"");
    } else if (root->type == NODE_RANGE) {
        fprintf(out, ",\"min\":%d,\"max\":%d", root->min, root->max);
    }

    if (root->left) {
        fprintf(out, ",\"left\":");
        export_ast_json(root->left, out);
    }

    if (root->right) {
        fprintf(out, ",\"right\":");
        export_ast_json(root->right, out);
    }

    fprintf(out, "}");
}

static const char *match_type_string(MatchType type) {
    switch (type) {
        case MATCH_CHAR: return "CHAR";
        case MATCH_ANY: return "ANY";
        case MATCH_CLASS: return "CLASS";
        case MATCH_EPSILON: return "EPSILON";
        default: return "UNKNOWN";
    }
}

void export_nfa_json(const NFA *nfa, FILE *out) {
    if (!nfa) {
        fprintf(out, "null");
        return;
    }

    fprintf(out, "{\"start_state\":%d,\"accept_state\":%d,\"state_count\":%d,\"states\":[",
            nfa->start_state, nfa->accept_state, nfa->state_count);

    for (int s = 0; s < nfa->state_count; s++) {
        const NFAState *state = &nfa->states[s];
        fprintf(out, "%s{\"id\":%d,\"transitions\":[", (s > 0 ? "," : ""), state->id);

        for (int t = 0; t < state->transition_count; t++) {
            const Transition *tr = &state->transitions[t];
            fprintf(out, "%s{\"type\":\"%s\",\"target\":%d",
                    (t > 0 ? "," : ""), match_type_string(tr->match_type), tr->target_state);

            if (tr->match_type == MATCH_CHAR) {
                if (tr->match_char == '"' || tr->match_char == '\\') {
                    fprintf(out, ",\"char\":\"\\%c\"", tr->match_char);
                } else {
                    fprintf(out, ",\"char\":\"%c\"", tr->match_char);
                }
            } else if (tr->match_type == MATCH_CLASS) {
                fprintf(out, ",\"negate\":%s", tr->negate ? "true" : "false");
                fprintf(out, ",\"chars\":\"");
                for (int i = 0; i < tr->class_char_count; i++) {
                    char c = tr->class_chars[i];
                    if (c == '"' || c == '\\') {
                        fprintf(out, "\\%c", c);
                    } else {
                        fputc(c, out);
                    }
                }
                fprintf(out, "\"");
            }

            fprintf(out, "}");
        }

        fprintf(out, "]}");
    }

    fprintf(out, "]}");
}
