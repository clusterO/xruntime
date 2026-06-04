#ifndef _LEXER_
#define _LEXER_

#include <stddef.h>

typedef enum {
    TOKEN_KEYWORD,
    TOKEN_IDENTIFIER,
    TOKEN_LITERAL,
    TOKEN_OPERATOR,
    TOKEN_SEPARATOR,
    TOKEN_EOF,
    TOKEN_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char* lexeme;
    int line;
    int column;
} Token;

typedef struct {
    const char* input;
    size_t input_length;
    size_t current;
    int line;
    int column;
} Lexer;

void lexer_init(Lexer *lexer, const char *source_code);
Token lexer_next_token(Lexer *lexer);
void token_free(Token *token);
const char *token_type_to_string(TokenType type);

#endif
