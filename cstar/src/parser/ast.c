#include "ast.h"

void parser_init(Parser *parser, const char *source_code)
{
    lexer_init(&parser->lexer, source_code);
    parser->current_token = lexer_next_token(&parser->lexer);
}

Node *parser_parse(Parser *parser)
{
    Node *program = create_node(NODE_PROGRAM);
    program->children = create_node_list();

    while (!is_token_type(parser->current_token, TOKEN_EOF))
    {
        Node *statement = parse_statement(parser);
        if (statement != NULL)
        {
            node_list_append(program->children, statement);
        }
        consume_token(parser, TOKEN_SEMICOLON);
    }

    return program;
}

void ast_free(Node *node)
{
    if (node == NULL)
    {
        return;
    }

    // Recursively free the children nodes
    for (int i = 0; i < node->num_children; i++)
    {
        ast_free(node->children[i]);
    }

    // Free the node itself
    free(node);
}

void parse_error(const char *message, int line, int column)
{
    fprintf(stderr, "Parse error at line %d, column %d: %s\n", line, column, message);
    exit(1);
}
