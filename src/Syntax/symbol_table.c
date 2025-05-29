#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

unsigned int hashSymbolName(const char* name) {
    unsigned int hash = 5381;
    while (*name)
        hash = ((hash << 5) + hash) + (unsigned char)(*name++);
    return hash % SYMBOL_TABLE_SIZE;
}

void initSymbolTable(SymbolTable* table) {
    for (int i = 0; i < SYMBOL_TABLE_SIZE; i++) {
        table->buckets[i] = NULL;
    }
}

bool insertSymbol(SymbolTable* table, Symbol* sym) {
    unsigned int index = hashSymbolName(sym->name);
    Symbol* current = table->buckets[index];

    while (current) {
        if (strcmp(current->name, sym->name) == 0) {
            // Already exists — shadowing not allowed in this table
            return false;
        }
        current = current->next;
    }

    sym->next = table->buckets[index];
    table->buckets[index] = sym;
    return true;
}

Symbol* lookupSymbol(SymbolTable* table, const char* name) {
    unsigned int index = hashSymbolName(name);
    Symbol* current = table->buckets[index];
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void freeSymbolTable(SymbolTable* table) {
    for (int i = 0; i < SYMBOL_TABLE_SIZE; i++) {
        Symbol* sym = table->buckets[i];
        while (sym) {
            Symbol* next = sym->next;
            free(sym->name);
            free(sym);
            sym = next;
        }
        table->buckets[i] = NULL;
    }
}

