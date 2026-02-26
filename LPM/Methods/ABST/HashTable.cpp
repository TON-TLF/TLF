#include "HashTable.h"

#define PRIME64_1 11400714785074694791ULL
#define PRIME64_2 14029467366897019727ULL
#define PRIME64_3  1609587929392839161ULL
#define PRIME64_4  9650029242287828579ULL
#define PRIME64_5  2870177450012600261ULL
#define XXH_rotl64(x,r) ((x << r) | (x >> (64 - r)))

static inline uint64_t
clib_xxhash (uint64_t key)
{
    uint64_t k1, h64;

    k1 = key;
    h64 = 0x9e3779b97f4a7c13LL + PRIME64_5 + 8;
    k1 *= PRIME64_2;
    k1 = XXH_rotl64 (k1, 31);
    k1 *= PRIME64_1;
    h64 ^= k1;
    h64 = XXH_rotl64 (h64, 27) * PRIME64_1 + PRIME64_4;

    h64 ^= h64 >> 33;
    h64 *= PRIME64_2;
    h64 ^= h64 >> 29;
    h64 *= PRIME64_3;
    h64 ^= h64 >> 32;
    return h64;
}

const uint64_t mask = (~((uint64_t)0)) >> (64 - 16);

static int Abst_hash(__uint128_t prefix, int size) {
    uint64_t hash = clib_xxhash((uint64_t)((prefix >> 64) ^ prefix));
    return (int)(hash & mask);
}



HashTable* HashTable_create() {
    HashTable* table = (HashTable*)malloc(sizeof(HashTable));
    table->bucket_size = HASH_TABLE_INITIAL_SIZE;
    table->count = 0;
    table->buckets = (HashNode**)calloc(table->bucket_size, sizeof(HashNode*));
    return table;
}

void HashTable_destroy(HashTable* table) {
    for (int i = 0; i < table->bucket_size; i++) {
        HashNode* current = table->buckets[i];
        while (current) {
            HashNode* temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(table->buckets);
    free(table);
}

void HashTable_insert(HashTable* table, Entry entry) {
    int index = Abst_hash(entry.prefix, table->bucket_size);
    // if(entry.prefix_len == 0)
    //     printf("HashTable_insert: %016llx%016llx\n",
    //         (unsigned long long)(entry.prefix >> 64), 
    //         (unsigned long long)(entry.prefix & 0xFFFFFFFFFFFFFFFF));
    // if(entry.prefix_len == 0)
    //     printf("index:%d\n",index);

    // 检查是否已存在
    HashNode* current = table->buckets[index];
    while (current) {
        if (current->entry.prefix == entry.prefix) {
            // 更新现有条目
            current->entry.label |= entry.label;
            if(entry.label == PREFIX){
                current->entry.port = entry.port;
            } 
            // if(entry.prefix_len == 0)
            //     printf("current->entry.port:%d\n",current->entry.port);
            return;
        }
        current = current->next;
    }
    
    // 创建新节点（头插法）
    HashNode* newNode = (HashNode*)malloc(sizeof(HashNode));
    newNode->entry = entry;
    newNode->next = table->buckets[index];
    table->buckets[index] = newNode;
    table->count++;
    // if(entry.prefix_len == 0)
    //     printf("current->entry.port:%d\n",current->entry.port);
}

Entry* HashTable_lookup(HashTable* table, __uint128_t prefix, ProgramState *ps) {
    int index = Abst_hash(prefix, table->bucket_size);
    
    // printf("Looking up prefix: %016llx%016llx\n",
    //     (unsigned long long)(prefix >> 64), 
    //     (unsigned long long)(prefix & 0xFFFFFFFFFFFFFFFF));
    
    HashNode* current = table->buckets[index];
    while (current) {
        // printf("  Compare with stored: %016llx%016llx\n",
        //         (unsigned long long)(current->entry.prefix >> 64),
        //         (unsigned long long)(current->entry.prefix & 0xFFFFFFFFFFFFFFFF)
        //         );
        ps->lookup_access.Addcount();
        ps->lookup_access_entry.Addcount();
        if (current->entry.prefix == prefix) {
            return &current->entry;
        }
        current = current->next;
    }
    return NULL;
}

Entry* HashTable_lookup(HashTable* table, __uint128_t prefix) {
    int index = Abst_hash(prefix, table->bucket_size);
    
    HashNode* current = table->buckets[index];
    while (current) {
        if (current->entry.prefix == prefix) {
            return &current->entry;
        }
        current = current->next;
    }
    return NULL;
}

// 计算哈希表的总存储开销（单位：字节）
size_t HashTable_cal_memory(HashTable* table) {
    if (table == NULL) {
        return 0;
    }
    
    size_t total = 0;
    // 1. 哈希表结构体本身的大小
    total += sizeof(HashTable);
    // 2. 桶数组的大小（指针数组）
    total += table->bucket_size * sizeof(HashNode*);
    // 3. 所有HashNode节点的总大小（包含其内部的Entry）
    total += table->count * sizeof(HashNode);
    
    return total;
}