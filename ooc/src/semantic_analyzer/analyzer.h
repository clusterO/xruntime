#ifndef _SEMANTIC_ANALYZER_
#define _SEMANTIC_ANALYZER_

#include "symbol_table.h"
#include "ast.h"

typedef struct {
    SymbolTable symbol_table;
    // Add more analyzer-specific variables as needed
} SemanticAnalyzer;

void analyzer_init(SemanticAnalyzer* analyzer);
void analyze_semantics(SemanticAnalyzer* analyzer, Node* root);
void analyze_node(SemanticAnalyzer* analyzer, Node* node);
void analyze_declaration(SemanticAnalyzer* analyzer, Node* node);
void analyze_expression(SemanticAnalyzer* analyzer, Node* node);

#endif  // SEMANTIC_ANALYZER_H
