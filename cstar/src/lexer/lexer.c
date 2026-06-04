#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "lexer.h"

static int is_alpha(char c) {
    return isalpha(c) || c == '_';
}

static int is_digit(char c) {
    return isdigit(c);
}

static int is_whitespace(char c) {
    return isspace(c);
}

void lexer_init(Lexer *lexer, const char *source_code) {
    lexer->input = source_code;
    lexer->input_length = strlen(source_code);
    lexer->current = 0;
    lexer->line = 1;
    lexer->column = 1;
}

static Token lexer_identifier(Lexer *lexer) {
    size_t start = lexer->current;
    while (lexer->current < lexer->input_length && (is_alpha(lexer->input[lexer->current]) || is_digit(lexer->input[lexer->current]))) {
        lexer->current++;
        lexer->column++;
    }

    size_t length = lexer->current - start;
    char *lexeme = malloc(length + 1);
    strncpy(lexeme, lexer->input + start, length);
    lexeme[length] = '\0';

    Token token;
    token.lexeme = lexeme;
    token.line = lexer->line;
    token.column = lexer->column - (int)length;

    // Check for keywords
    if (strcmp(lexeme, "class") == 0 || strcmp(lexeme, "extends") == 0 || strcmp(lexeme, "method") == 0 || strcmp(lexeme, "variant") == 0) {
        token.type = TOKEN_KEYWORD;
    } else {
        token.type = TOKEN_IDENTIFIER;
    }

    return token;
}

static Token lexer_number(Lexer *lexer) {
    size_t start = lexer->current;
    while (lexer->current < lexer->input_length && is_digit(lexer->input[lexer->current])) {
        lexer->current++;
        lexer->column++;
    }

    size_t length = lexer->current - start;
    char *lexeme = malloc(length + 1);
    strncpy(lexeme, lexer->input + start, length);
    lexeme[length] = '\0';

    Token token;
    token.type = TOKEN_LITERAL;
    token.lexeme = lexeme;
    token.line = lexer->line;
    token.column = lexer->column - (int)length;

    return token;
}

Token lexer_next_token(Lexer *lexer) {
    while (lexer->current < lexer->input_length && is_whitespace(lexer->input[lexer->current])) {
        if (lexer->input[lexer->current] == '\n') {
            lexer->line++;
            lexer->column = 1;
        } else {
            lexer->column++;
        }
        lexer->current++;
    }

    if (lexer->current >= lexer->input_length) {
        Token token = { TOKEN_EOF, NULL, lexer->line, lexer->column };
        return token;
    }

    char c = lexer->input[lexer->current];

    if (is_alpha(c)) return lexer_identifier(lexer);
    if (is_digit(c)) return lexer_number(lexer);

    // Operators and Separators
    char *lexeme = malloc(2);
    lexeme[0] = c;
    lexeme[1] = '\0';
    lexer->current++;
    lexer->column++;

    TokenType type = TOKEN_UNKNOWN;
    if (strchr("{}:;,", c)) type = TOKEN_SEPARATOR;
    else if (strchr("+-*/=", c)) type = TOKEN_OPERATOR;

    Token token = { type, lexeme, lexer->line, lexer->column - 1 };
    return token;
}

void token_free(Token *token) {
    if (token->lexeme) free(token->lexeme);
}

const char *token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_KEYWORD: return "KEYWORD";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_LITERAL: return "LITERAL";
        case TOKEN_OPERATOR: return "OPERATOR";
        case TOKEN_SEPARATOR: return "SEPARATOR";
        case TOKEN_EOF: return "EOF";
        default: return "UNKNOWN";
    }
}
