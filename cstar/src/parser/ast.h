#ifndef _AST_
#define _AST_

#include "../lexer/lexer.h"

typedef enum {
    NODE_PROGRAM,
    NODE_CLASS,
    NODE_METHOD,
    NODE_FIELD,
    NODE_IDENTIFIER,
    NODE_TYPE,
    NODE_VARIANT,
    NODE_VARIANT_BRANCH
} NodeType;

typedef struct Node {
    NodeType type;
    char* value;
    struct Node** children;
    int num_children;
    int capacity;
} Node;

typedef struct {
    Lexer lexer;
    Token current_token;
} Parser;

void parser_init(Parser* parser, const char* source_code);
Node* parser_parse(Parser* parser);
void ast_free(Node* node);

Node* create_node(NodeType type, const char* value);
void node_add_child(Node* parent, Node* child);

#endif
