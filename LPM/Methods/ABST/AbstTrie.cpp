#include "AbstTrie.h"

AbstTrieNode* AbstTrieNode_create() {
    AbstTrieNode* node = (AbstTrieNode*)malloc(sizeof(AbstTrieNode));
    node->entry = NULL;
    node->is_prefix = false;
    node->child[0] = NULL;
    node->child[1] = NULL;
    return node;
}


void AbstTrie_insert(AbstTrieNode* root, Entry entry) {
    AbstTrieNode* node = root;
    uint8_t dep = 0;
    
    while (dep != entry.prefix_len) {
        // 计算当前比特位（从左到右）
        uint8_t bit = (entry.prefix >> (127 - dep)) & 1;
        
        if (!node->child[bit]) {
            node->child[bit] = AbstTrieNode_create();
        }
        
        node = node->child[bit];
        dep++;
    }
    
    node->is_prefix = true;
    Entry* ret = (Entry*)malloc(sizeof(Entry));
    ret->label = entry.label;
    ret->port = entry.port;
    ret->prefix_len = entry.prefix_len;
    ret->prefix = entry.prefix;
    node->entry = ret;
}


Entry* AbstTrie_lookup_bmp(AbstTrieNode* root, Entry entry) {
    AbstTrieNode* node = root;
    uint8_t dep = 0;
    Entry* result = NULL;

    while (dep != entry.prefix_len) {
        if (node->is_prefix) {
            result = node->entry;
        }

        uint8_t bit = (entry.prefix >> (127 - dep)) & 1;
        
        if (!node->child[bit]) {
            return result;
        }
        
        node = node->child[bit];
        dep++;
    }

    if (node->is_prefix) {
        result = node->entry;
    }
    
    return result;
}

uint8_t AbstTrie_lookup_lpm(AbstTrieNode* root, __uint128_t addr) {
    AbstTrieNode* node = root;
    uint8_t dep = 0;
    uint8_t result = 0;

    while (dep != 128) {
        if (node->is_prefix) {
            result = node->entry->prefix_len;
        }

        uint8_t bit = (addr >> (127 - dep)) & 1;
        
        if (!node->child[bit]) {
            return result;
        }
        
        node = node->child[bit];
        dep++;
    }

    if (node->is_prefix) {
        result = node->entry->prefix_len;
    }
    
    return result;
}