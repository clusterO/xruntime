#ifndef _LEXER_
#define _LEXER_

typedef enum {
    TOKEN_KEYWORD,
    TOKEN_IDENTIFIER,
    TOKEN_LITERAL,
    TOKEN_OPERATOR,
    // Add more token types as needed
} TokenType;

typedef struct {
    TokenType type;
    char* value;
    int line;
    int column;
} Token;

typedef struct {
    char* source_code;
    int source_length;
    int current_index;
    int current_line;
    int current_column;
    // Add more lexer-specific variables as needed
} Lexer;

Token lexer_next_token(Lexer* lexer);
void lexer_init(Lexer* lexer, const char* source_code);
void token_free(Token* token);
const char* token_type_to_string(TokenType type);

#endif
