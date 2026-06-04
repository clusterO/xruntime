#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

Node* create_node(NodeType type, const char* value) {
    Node* node = malloc(sizeof(Node));
    node->type = type;
    node->value = value ? strdup(value) : NULL;
    node->num_children = 0;
    node->capacity = 4;
    node->children = malloc(sizeof(Node*) * node->capacity);
    return node;
}

void node_add_child(Node* parent, Node* child) {
    if (parent->num_children == parent->capacity) {
        parent->capacity *= 2;
        parent->children = realloc(parent->children, sizeof(Node*) * parent->capacity);
    }
    parent->children[parent->num_children++] = child;
}

void ast_free(Node* node) {
    if (!node) return;
    for (int i = 0; i < node->num_children; i++) {
        ast_free(node->children[i]);
    }
    free(node->children);
    if (node->value) free(node->value);
    free(node);
}

void parser_init(Parser* parser, const char* source_code) {
    lexer_init(&parser->lexer, source_code);
    parser->current_token = lexer_next_token(&parser->lexer);
}

static void advance(Parser* parser) {
    token_free(&parser->current_token);
    parser->current_token = lexer_next_token(&parser->lexer);
}

static int check(Parser* parser, TokenType type, const char* lexeme) {
    if (parser->current_token.type != type) return 0;
    if (lexeme && strcmp(parser->current_token.lexeme, lexeme) != 0) return 0;
    return 1;
}

static Token consume(Parser* parser, TokenType type, const char* message) {
    if (parser->current_token.type == type) {
        Token t = parser->current_token;
        // Don't free lexeme yet as we use it
        parser->current_token = lexer_next_token(&parser->lexer);
        return t;
    }
    fprintf(stderr, "Error: %s at line %d\n", message, parser->current_token.line);
    exit(1);
}

static Node* parse_field(Parser* parser) {
    Token type = consume(parser, TOKEN_IDENTIFIER, "Expected field type");
    Token name = consume(parser, TOKEN_IDENTIFIER, "Expected field name");
    Node* field = create_node(NODE_FIELD, name.lexeme);
    node_add_child(field, create_node(NODE_TYPE, type.lexeme));
    return field;
}

static Node* parse_class(Parser* parser) {
    advance(parser); // consume 'class'
    Token name = consume(parser, TOKEN_IDENTIFIER, "Expected class name");
    Node* class_node = create_node(NODE_CLASS, name.lexeme);
    
    if (check(parser, TOKEN_SEPARATOR, ":")) {
        advance(parser); // consume ':'
        Token super = consume(parser, TOKEN_IDENTIFIER, "Expected superclass name");
        node_add_child(class_node, create_node(NODE_IDENTIFIER, super.lexeme));
    } else {
        node_add_child(class_node, create_node(NODE_IDENTIFIER, "Object"));
    }
    
    consume(parser, TOKEN_SEPARATOR, "{");
    while (!check(parser, TOKEN_SEPARATOR, "}")) {
        node_add_child(class_node, parse_field(parser));
        consume(parser, TOKEN_SEPARATOR, ";");
    }
    advance(parser); // consume '}'
    
    return class_node;
}

static Node* parse_variant(Parser* parser) {
    advance(parser); // consume 'variant'
    Token name = consume(parser, TOKEN_IDENTIFIER, "Expected variant name");
    Node* variant_node = create_node(NODE_VARIANT, name.lexeme);
    consume(parser, TOKEN_SEPARATOR, "{");
    while (!check(parser, TOKEN_SEPARATOR, "}")) {
        Token branch = consume(parser, TOKEN_IDENTIFIER, "Expected branch name");
        node_add_child(variant_node, create_node(NODE_VARIANT_BRANCH, branch.lexeme));
        if (check(parser, TOKEN_SEPARATOR, ",")) advance(parser);
    }
    advance(parser); // consume '}'
    return variant_node;
}

Node* parser_parse(Parser* parser) {
    Node* program = create_node(NODE_PROGRAM, NULL);
    while (parser->current_token.type != TOKEN_EOF) {
        if (check(parser, TOKEN_KEYWORD, "class")) {
            node_add_child(program, parse_class(parser));
        } else if (check(parser, TOKEN_KEYWORD, "variant")) {
            node_add_child(program, parse_variant(parser));
        } else {
            advance(parser);
        }
    }
    return program;
}
