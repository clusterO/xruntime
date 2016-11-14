#ifndef _PARSER_
#define _PARSER_

#include "lexer.h"

// AST node types
typedef enum {
    NODE_PROGRAM,
    NODE_STATEMENT,
    NODE_EXPRESSION,
    // Add more node types as needed
} NodeType;

// AST node structure
typedef struct Node {
    NodeType type;
    Token token;
    struct Node* left;
    struct Node* right;
    // Add more node-specific variables as needed
} Node;

// Parser structure
typedef struct {
    Lexer lexer;
    Token current_token;
    // Add more parser-specific variables as needed
} Parser;

Node* parser_parse(Parser* parser);
void parser_init(Parser* parser, const char* source_code);
void ast_free(Node* node);
void parse_error(const char* message, int line, int column);

#endif
