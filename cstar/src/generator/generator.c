#include <stdio.h>
#include <string.h>
#include "../parser/ast.h"

void generate_h(Node* class_node) {
    char filename[256];
    sprintf(filename, "cstar/coop/src/%s.h", class_node->value);
    FILE* f = fopen(filename, "w");
    if (!f) return;

    fprintf(f, "#ifndef %s_H\n", class_node->value);
    fprintf(f, "#define %s_H\n\n", class_node->value);
    fprintf(f, "const void * %s(void);\n\n", class_node->value);
    fprintf(f, "#endif\n");
    fclose(f);
}

void generate_r(Node* class_node) {
    char filename[256];
    sprintf(filename, "cstar/coop/src/%s.r", class_node->value);
    FILE* f = fopen(filename, "w");
    if (!f) return;

    fprintf(f, "#ifndef %s_R\n", class_node->value);
    fprintf(f, "#define %s_R\n\n", class_node->value);
    
    const char* super = class_node->children[0]->value;
    if (strcmp(super, "Object") != 0) {
        fprintf(f, "#include \"%s.r\"\n\n", super);
    }
    
    fprintf(f, "struct %s {\n", class_node->value);
    if (strcmp(super, "Object") == 0) {
        fprintf(f, "    const void * class;\n");
    } else {
        fprintf(f, "    struct %s super;\n", super);
    }
    
    for (int i = 1; i < class_node->num_children; i++) {
        fprintf(f, "    %s %s;\n", class_node->children[i]->children[0]->value, class_node->children[i]->value);
    }
    fprintf(f, "};\n\n");
    fprintf(f, "#endif\n");
    fclose(f);
}

void generate_c(Node* class_node) {
    char filename[256];
    sprintf(filename, "cstar/coop/src/%s.c", class_node->value);
    FILE* f = fopen(filename, "w");
    if (!f) return;

    fprintf(f, "#include \"%s.h\"\n", class_node->value);
    fprintf(f, "#include \"%s.r\"\n", class_node->value);
    fprintf(f, "#include \"new.h\"\n");
    fprintf(f, "#include \"Object.h\"\n\n");
    
    // Class descriptor
    fprintf(f, "static struct Class _%s = {\n", class_node->value);
    fprintf(f, "    & _Class, %s(), sizeof(struct %s),\n", class_node->children[0]->value, class_node->value);
    fprintf(f, "    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL\n");
    fprintf(f, "};\n\n");
    
    // Functional accessor
    fprintf(f, "const void * %s(void) {\n", class_node->value);
    fprintf(f, "    static const void * _%s_ptr = 0;\n", class_node->value);
    fprintf(f, "    if (!_%s_ptr) _%s_ptr = &_%s;\n", class_node->value, class_node->value, class_node->value);
    fprintf(f, "    return _%s_ptr;\n", class_node->value);
    fprintf(f, "}\n");
    fclose(f);
}

void generate_variant(Node* variant_node) {
    char filename[256];
    sprintf(filename, "cstar/cfp/src/%s.h", variant_node->value);
    FILE* f = fopen(filename, "w");
    if (!f) return;

    fprintf(f, "#ifndef %s_H\n", variant_node->value);
    fprintf(f, "#define %s_H\n\n", variant_node->value);
    
    fprintf(f, "typedef enum { ");
    for (int i = 0; i < variant_node->num_children; i++) {
        fprintf(f, "TAG_%s%s", variant_node->children[i]->value, i == variant_node->num_children - 1 ? "" : ", ");
    }
    fprintf(f, " } %sTag;\n\n", variant_node->value);
    
    fprintf(f, "typedef struct {\n    %sTag tag;\n} %s;\n\n", variant_node->value, variant_node->value);

    // Pattern matching macro
    fprintf(f, "#define match_%s(m, ", variant_node->value);
    for (int i = 0; i < variant_node->num_children; i++) {
        fprintf(f, "case_%s%s", variant_node->children[i]->value, i == variant_node->num_children - 1 ? "" : ", ");
    }
    fprintf(f, ") \\\n    switch ((m).tag) { \\\n");
    for (int i = 0; i < variant_node->num_children; i++) {
        fprintf(f, "        case TAG_%s: case_%s(); break; \\\n", variant_node->children[i]->value, variant_node->children[i]->value);
    }
    fprintf(f, "    }\n\n");
    
    fprintf(f, "#endif\n");
    fclose(f);
}

void generate_code(Node* program) {
    for (int i = 0; i < program->num_children; i++) {
        Node* n = program->children[i];
        if (n->type == NODE_CLASS) {
            generate_h(n);
            generate_r(n);
            generate_c(n);
        } else if (n->type == NODE_VARIANT) {
            generate_variant(n);
        }
    }
}
