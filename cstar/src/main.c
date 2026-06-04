#include <stdio.h>
#include <stdlib.h>
#include "parser/ast.h"
#include "generator/generator.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: cstar <source_file>\n");
        return 1;
    }

    FILE* f = fopen(argv[1], "r");
    if (!f) {
        perror("fopen");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* source = malloc(size + 1);
    fread(source, 1, size, f);
    source[size] = '\0';
    fclose(f);

    Parser parser;
    parser_init(&parser, source);
    Node* program = parser_parse(&parser);

    printf("Generating code...\n");
    generate_code(program);

    ast_free(program);
    free(source);
    printf("Done.\n");
    return 0;
}
