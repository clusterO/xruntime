#include "lexer.h"

void lexer_init(Lexer *lexer, const char *source_code)
{
    lexer->input = source_code;
    lexer->input_length = strlen(source_code);
    lexer->current = 0;
}

Token lexer_next_token(Lexer *lexer)
{
    Token token;

    // Initialize token values
    token.type = TOKEN_UNKNOWN;
    token.lexeme = NULL;

    // Check if lexer has reached the end of input
    if (lexer->current >= lexer->input_length)
    {
        token.type = TOKEN_EOF;
        return token;
    }

    // Skip whitespace characters
    while (is_whitespace(lexer->input[lexer->current]))
    {
        lexer->current++;
    }

    // Check for end of input after skipping whitespace
    if (lexer->current >= lexer->input_length)
    {
        token.type = TOKEN_EOF;
        return token;
    }

    // Get the current character
    char current_char = lexer->input[lexer->current];

    // Determine the token type based on the current character
    if (is_alpha(current_char))
    {
        token = lexer_identifier(lexer);
    }
    else if (is_digit(current_char))
    {
        token = lexer_number(lexer);
    }
    else
    {
        // Handle other token types such as operators, punctuation, etc.
        // Implement your logic here based on the language grammar
        // and return the appropriate token.
        // Example:
        // token = lexer_operator(lexer);
        // token = lexer_punctuation(lexer);
    }

    return token;
}

void token_free(Token *token)
{
    free(token->lexeme);
    token->lexeme = NULL;
    token->type = TOKEN_UNKNOWN;
}

const char *token_type_to_string(TokenType type)
{
    switch (type)
    {
    case TOKEN_IDENTIFIER:
        return "IDENTIFIER";
    case TOKEN_KEYWORD:
        return "KEYWORD";
    case TOKEN_LITERAL:
        return "LITERAL";
    case TOKEN_OPERATOR:
        return "OPERATOR";
    case TOKEN_SEPARATOR:
        return "SEPARATOR";
    case TOKEN_COMMENT:
        return "COMMENT";
    case TOKEN_UNKNOWN:
    default:
        return "UNKNOWN";
    }
}
