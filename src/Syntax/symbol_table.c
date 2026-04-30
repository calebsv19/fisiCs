// SPDX-License-Identifier: Apache-2.0

#include "symbol_table.h"
#include "Utils/profiler.h"
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
    profiler_record_value("semantic_count_symbol_insert", 1);
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
    profiler_record_value("semantic_count_symbol_lookup", 1);
    unsigned int index = hashSymbolName(name);
    Symbol* current = table->buckets[index];
    while (current) {
        if (strcmp(current->name, name) == 0) {
            profiler_record_value("semantic_count_symbol_lookup_hit", 1);
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
            free(sym->signature.params);
            free(sym->cachedTypeInfo);
            free(sym);
            sym = next;
        }
        table->buckets[i] = NULL;
    }
}

void symbolAttachUnitsAnnotation(Symbol* sym,
                                 const FisicsUnitsAnnotation* annotation,
                                 size_t declaratorIndex) {
    if (!sym) return;
    sym->unitsAnnotation = annotation;
    sym->unitsDeclaratorIndex = annotation ? declaratorIndex : 0;
}

const FisicsUnitsAnnotation* symbolGetUnitsAnnotation(const Symbol* sym) {
    return sym ? sym->unitsAnnotation : NULL;
}

size_t symbolGetUnitsDeclaratorIndex(const Symbol* sym) {
    return sym ? sym->unitsDeclaratorIndex : 0;
}
