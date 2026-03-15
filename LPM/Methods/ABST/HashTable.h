#ifndef ABST_HASHTABLE_H
#define ABST_HASHTABLE_H

#define HASH_TABLE_INITIAL_SIZE 65536
#define PREFIX 0x01
#define MARKER 0x02

#include "../../Elements/Elements.h"

typedef struct Entry {
    uint8_t label;
    uint32_t port;
    uint8_t prefix_len;
    __uint128_t prefix;
    struct Entry* bmp;
}Entry;

typedef struct HashNode {
    Entry entry;
    struct HashNode* next;  
} HashNode;  

typedef struct HashTable{
    HashNode** buckets;   
    int bucket_size;         
    int count;         
} HashTable;

HashTable* HashTable_create();
void HashTable_destroy(HashTable* table);
void HashTable_insert(HashTable* table, Entry entry);
Entry* HashTable_lookup(HashTable* table, __uint128_t prefix, ProgramState *ps);
Entry* HashTable_lookup(HashTable* table, __uint128_t prefix);
size_t HashTable_cal_memory(HashTable* table);
#endif