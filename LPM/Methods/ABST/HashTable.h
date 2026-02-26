#ifndef ABST_HASHTABLE_H
#define ABST_HASHTABLE_H

#define HASH_TABLE_INITIAL_SIZE 65536
/** @brief 标签常量：PREFIX表示普通前缀条目，MARKER表示辅助位图生成的中介条目 */
#define PREFIX 0x01
#define MARKER 0x02

#include "../../Elements/Elements.h"

/**
 * @struct Entry
 * @brief Cuckoo哈希表的条目结构
 */
typedef struct Entry {
    uint8_t label;
    uint32_t port;
    uint8_t prefix_len;
    __uint128_t prefix;
    struct Entry* bmp;
}Entry;

typedef struct HashNode {
    Entry entry;
    struct HashNode* next;  // 新增链表指针
} HashNode;  // 链表节点结构

typedef struct HashTable{
    HashNode** buckets;    // 哈希桶数组（改为链表结构）
    int bucket_size;          // 哈希表容量
    int count;         // 当前元素数量
} HashTable;

HashTable* HashTable_create();
void HashTable_destroy(HashTable* table);
void HashTable_insert(HashTable* table, Entry entry);
Entry* HashTable_lookup(HashTable* table, __uint128_t prefix, ProgramState *ps);
Entry* HashTable_lookup(HashTable* table, __uint128_t prefix);
size_t HashTable_cal_memory(HashTable* table);
#endif