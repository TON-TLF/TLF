#ifndef ABST_TRTE_H
#define ABST_TRTE_H

#include "../../Elements/Elements.h"
#include "HashTable.h"

typedef struct AbstTrieNode {
    Entry* entry;
    bool is_prefix;
    struct AbstTrieNode* child[2];
} AbstTrieNode;

AbstTrieNode* AbstTrieNode_create();
void AbstTrie_insert(AbstTrieNode* root, Entry entry);
Entry* AbstTrie_lookup_bmp(AbstTrieNode* root, Entry entry);
uint8_t AbstTrie_lookup_lpm(AbstTrieNode* root, __uint128_t addr);
#endif