#ifndef _SYMBOL_TABLE_
#define _SYMBOL_TABLE_

#include <stdbool.h>

typedef struct {
    char* name;
    // Add more fields as needed
} SymbolEntry;

typedef struct {
    SymbolEntry** entries;
    int size;
    int capacity;
} SymbolTable;

void symbol_table_init(SymbolTable* table);
void symbol_table_insert(SymbolTable* table, const char* name);
bool symbol_table_exists(SymbolTable* table, const char* name);
void symbol_table_remove(SymbolTable* table, const char* name);
void symbol_table_destroy(SymbolTable* table);

#endif
