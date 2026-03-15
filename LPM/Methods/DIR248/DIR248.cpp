#include "DIR248.h"

static void* linux_aligned_malloc_64(size_t size) {
    void* ptr = NULL;
    if (posix_memalign(&ptr, 64, size) != 0) {
        return NULL; 
    }
    return ptr;
}

#define linux_aligned_free_64(ptr) free(ptr)

bool cmp_for_dir248(const Prefix* a, const Prefix* b) {
    return a->prefix_len > b->prefix_len;
}

DirTable* create_dir_table(uint8_t stride) {
    if (stride == 0 || stride > 24) {
        return NULL;
    }

    DirTable* table = (DirTable*)linux_aligned_malloc_64(sizeof(DirTable));
    if (!table) {
        return NULL;
    }

    table->stride = stride;
    table->entry_count = 1ULL << stride;  
    table->mask = table->entry_count - 1; 
    table->entries = NULL;

    size_t entries_total_size = table->entry_count * sizeof(TableEntry);
    table->entries = (TableEntry*)linux_aligned_malloc_64(entries_total_size);
    if (!table->entries) {
        linux_aligned_free_64(table);  
        return NULL;
    }

    for (uint32_t i = 0; i < table->entry_count; ++i) {
        table->entries[i].next_hop = 0;          
        table->entries[i].has_subtable = 0;       
        table->entries[i].is_valid = 0;          
        table->entries[i].subtable = NULL;       
    }

    return table;
}

bool build_dir248(DirTable* current, vector<Prefix*>& prefixs, uint8_t current_len) {
    int prefixs_num = prefixs.size();
    if(prefixs_num == 0 || !current){
        cout << "DIR248::build_dir248() : prefixs is empty !!!" << endl;
        return false;
    }

    vector<Prefix*> next_level;
    uint32_t stride_mask = current->mask;
    uint8_t total_bits = current_len + current->stride;

    for(int i = 0; i < prefixs_num; i++){
        __uint128_t ip = trim_prefix( ((__uint128_t)((__uint128_t)prefixs[i]->ip6_upper << 64) | (__uint128_t)prefixs[i]->ip6_lower), 
                prefixs[i]->prefix_len);
        uint8_t prefix_len = prefixs[i]->prefix_len;

        if (prefix_len <= total_bits) {
            const uint32_t bits_diff = total_bits - prefix_len;
            const uint32_t coverage = 1 << bits_diff;
            const uint32_t base_idx = (ip >> (128 - total_bits)) & stride_mask;

            for (uint32_t j = 0; j < coverage; ++j) {
                const uint32_t idx = base_idx | j;
                TableEntry& entry = current->entries[idx];

                if (entry.is_valid) continue; 
                entry.is_valid = 1;
                entry.next_hop = prefixs[i]->port;
            }
        } else {
            const uint32_t idx = (ip >> (128 - total_bits)) & stride_mask;
            current->entries[idx].has_subtable = 1;
            next_level.push_back(prefixs[i]);
        }
    }

    for (uint32_t i = 0; i < current->entry_count; ++i) {
        TableEntry& entry = current->entries[i];
        if (!entry.has_subtable) continue;

        vector<Prefix*> sub_prefixes;
        for (const auto& prefix : next_level) {
            __uint128_t ip = trim_prefix( 
                ((__uint128_t)((__uint128_t)prefix->ip6_upper << 64) | (__uint128_t)prefix->ip6_lower), 
                prefix->prefix_len);

            if (((ip >> (128 - total_bits)) & stride_mask) == i) {
                sub_prefixes.push_back(prefix);
            }
        }

        if(sub_prefixes.empty()) continue;

        uint8_t new_stride = 8;
        if (total_bits + new_stride > 128) {
            new_stride = 128 - total_bits;
        }

        DirTable* sub_table = create_dir_table(new_stride);
        if (!sub_table || build_dir248(sub_table, sub_prefixes, total_bits) != true) {
            return false;
        }
        entry.subtable = sub_table;
    }

    return true;
}

bool build_dir248_td(DirTable* current, vector<Prefix*>& prefixs, uint8_t current_len) {
    int prefixs_num = prefixs.size();
    if(prefixs_num == 0 || !current){
        cout << "DIR248::build_dir248() : prefixs is empty !!!" << endl;
        return false;
    }

    vector<Prefix*> next_level;
    uint32_t stride_mask = current->mask;
    uint8_t total_bits = current_len + current->stride;
    double sub_table_num = 0;

    for(int i = 0; i < prefixs_num; i++){
        __uint128_t ip = trim_prefix( 
            ((__uint128_t)((__uint128_t)prefixs[i]->ip6_upper << 64) | (__uint128_t)prefixs[i]->ip6_lower), 
            prefixs[i]->prefix_len);
        uint8_t prefix_len = prefixs[i]->prefix_len;

        if (prefix_len <= total_bits) {
            const uint32_t bits_diff = total_bits - prefix_len;
            const uint32_t coverage = 1 << bits_diff;
            const uint32_t base_idx = (ip >> (128 - total_bits)) & stride_mask;

            for (uint32_t j = 0; j < coverage; ++j) {
                const uint32_t idx = base_idx | j;
                TableEntry& entry = current->entries[idx];

                if (entry.is_valid) continue; 
                entry.is_valid = 1;
                entry.next_hop = prefixs[i]->port;
            }
        } else {

            const uint32_t idx = (ip >> (128 - total_bits)) & stride_mask;
            current->entries[idx].has_subtable = 1;
            sub_table_num++;
            next_level.push_back(prefixs[i]);
        }
    }

    Prefix common_prefix;
    common_prefix.ip6_upper = prefixs[0]->ip6_upper;
    common_prefix.ip6_lower = prefixs[0]->ip6_lower;
    common_prefix.prefix_len = current_len;
    if(current_len <= 64){
        uint64_t mask = (current_len == 64) ? ~static_cast<uint64_t>(0) : (~static_cast<uint64_t>(0)) << (64 - current_len);
        common_prefix.ip6_upper &= mask;
        common_prefix.ip6_lower = 0;
    } else {
        uint8_t lower_bits = current_len - 64;
        uint64_t lower_mask = (lower_bits == 64) ? ~static_cast<uint64_t>(0) : (~static_cast<uint64_t>(0)) << (64 - lower_bits);
        common_prefix.ip6_lower &= lower_mask;
    }
    __uint128_t common_ip = trim_prefix( 
        ((__uint128_t)((__uint128_t)common_prefix.ip6_upper << 64) | (__uint128_t)common_prefix.ip6_lower), 
        common_prefix.prefix_len);

    for (uint32_t i = 0; i < current->entry_count; ++i) {
        TableEntry& entry = current->entries[i];
        if (!entry.has_subtable) continue;

        vector<Prefix*> sub_prefixes;
        for (const auto& prefix : next_level) {
            __uint128_t ip = trim_prefix( 
                ((__uint128_t)((__uint128_t)prefix->ip6_upper << 64) | (__uint128_t)prefix->ip6_lower), 
                prefix->prefix_len);

            if (((ip >> (128 - total_bits)) & stride_mask) == i) {
                sub_prefixes.push_back(prefix);
            }
        }

        if(sub_prefixes.empty()) continue;
        
        /* 计算流行度 TopK*/
        __uint128_t _ip = common_ip | ((__uint128_t)i << (128 - total_bits));
        double total_trace_size = TSL::getTopKFreq(common_ip, common_prefix.prefix_len);
        double sub_trace_size = TSL::getTopKFreq(_ip, total_bits);
        
        double expect_trace_size = 0, pop_score = 0;
        expect_trace_size = total_trace_size / sub_table_num;
        if(expect_trace_size != 0) pop_score = sub_trace_size / expect_trace_size;

        // 动态调整子表步长
        uint8_t new_stride = 8;
        if (pop_score > 1.5) new_stride = 12;
        else if (pop_score > 1.0) new_stride = 10;
        // else if (pop_score > 0.001 && pop_score < 0.2) new_stride = 6;
        // else if (pop_score <= 0.001) new_stride = 6;

        if (total_bits + new_stride > 128) {
            new_stride = 128 - total_bits;
        }

        // 创建子表并递归构建
        DirTable* sub_table = create_dir_table(new_stride);
        if (!sub_table || build_dir248_td(sub_table, sub_prefixes, total_bits) != true) {
            return false;
        }
        entry.subtable = sub_table;
    }

    return true;
}

uint64_t _CalMemory(DirTable* table) {
    if (table == NULL) {
        return 0;
    }

    uint64_t total_mem = 0;
    total_mem += sizeof(DirTable);
    total_mem += (uint64_t)(table->entry_count) * sizeof(TableEntry);

    for (uint32_t i = 0; i < table->entry_count; ++i) {
        TableEntry& entry = table->entries[i];
        if (entry.has_subtable && entry.subtable != NULL) {
            total_mem += _CalMemory(entry.subtable);
        }
    }

    return total_mem;
}

/***************************************
*                DIR248                *
***************************************/

DIR248::DIR248(){
    root = NULL;
}

void DIR248::Create(vector<Prefix*> &prefixs, ProgramState *ps){
    int prefixs_num = prefixs.size();
    if(prefixs_num == 0){
        cout << "DIR248::Create() : prefixs is empty !!!" << endl;
        return;
    }

    sort(prefixs.begin(), prefixs.end(), cmp_for_dir248);
    root = create_dir_table(24);

    if (build_dir248(root, prefixs, 0) == false) {
        cout << "DIR248::Create() : build_dir248 fail !!!" << endl;
    }
}

uint32_t DIR248::Lookup(Trace *trace, ProgramState *ps){
    __uint128_t ip = ((__uint128_t)trace->ip6_upper << 64) | trace->ip6_lower;
    DirTable* current_table = root;
    uint32_t best_match = 0;
    int8_t remaining_shift = 128;

    while (current_table) {
        ps->lookup_access.Addcount();
        ps->lookup_depth.Addcount();

        uint8_t stride = current_table->stride;
        remaining_shift -= stride;
        uint32_t idx = (ip >> remaining_shift) & current_table->mask;
        TableEntry& entry = current_table->entries[idx];

        if (entry.is_valid) {
            best_match = entry.next_hop;
        }
        current_table = entry.subtable;
    }

    ps->lookup_access.Cal();
    ps->lookup_depth.Cal();
    return best_match;
}

uint32_t DIR248::Lookup(Trace *trace){
    __uint128_t ip = ((__uint128_t)trace->ip6_upper << 64) | trace->ip6_lower;
    DirTable* current_table = root;
    uint32_t best_match = 0;
    int8_t remaining_shift = 128;

    while (current_table) {
        const uint8_t stride = current_table->stride;
        const uint32_t mask = current_table->mask;
        const TableEntry* entries = current_table->entries;  

        remaining_shift -= stride;
        const uint32_t idx = (ip >> remaining_shift) & mask;
        const TableEntry& entry = entries[idx];

        best_match = entry.is_valid ? entry.next_hop : best_match;
        current_table = entry.subtable;
    }
    return best_match;
}

uint64_t DIR248::CalMemory(){
    return _CalMemory(root);
}

DIR248::~DIR248(){}

/***************************************
*                DIR248_TD             *
***************************************/
DIR248_TD::DIR248_TD(){
    root = NULL;
}

void DIR248_TD::Create(vector<Prefix*> &prefixs, ProgramState *ps){ 
    int prefixs_num = prefixs.size();
    if(prefixs_num == 0){
        cout << "DIR248_TD::Create() : prefixs is empty !!!" << endl;
        return;
    }
    
    sort(prefixs.begin(), prefixs.end(), cmp_for_dir248);
    root = create_dir_table(24);

    if (build_dir248_td(root, prefixs, 0) == false) {
        cout << "DIR248_TD::Create() : build_dir248 fail !!!" << endl;
    }
}

uint32_t DIR248_TD::Lookup(Trace *trace, ProgramState *ps){
    __uint128_t ip = ((__uint128_t)trace->ip6_upper << 64) | trace->ip6_lower;
    DirTable* current_table = root;
    uint32_t best_match = 0;
    int8_t remaining_shift = 128;

    while (current_table) {
        ps->lookup_access.Addcount();
        ps->lookup_depth.Addcount();

        uint8_t stride = current_table->stride;
        remaining_shift -= stride;
        uint32_t idx = (ip >> remaining_shift) & current_table->mask;
        TableEntry& entry = current_table->entries[idx];

        if (entry.is_valid) {
            best_match = entry.next_hop;
        }
        current_table = entry.subtable;
    }

    ps->lookup_access.Cal();
    ps->lookup_depth.Cal();
    return best_match;
}

uint32_t DIR248_TD::Lookup(Trace *trace){
    __uint128_t ip = ((__uint128_t)trace->ip6_upper << 64) | trace->ip6_lower;
    DirTable* current_table = root;
    uint32_t best_match = 0;
    int8_t remaining_shift = 128;

    while (current_table) {
        const uint8_t stride = current_table->stride;
        const uint32_t mask = current_table->mask;
        const TableEntry* entries = current_table->entries;  

        remaining_shift -= stride;
        const uint32_t idx = (ip >> remaining_shift) & mask;
        const TableEntry& entry = entries[idx];

        best_match = entry.is_valid ? entry.next_hop : best_match;
        current_table = entry.subtable;
    }
    return best_match;
}

uint64_t DIR248_TD::CalMemory(){
    return _CalMemory(root);
}

DIR248_TD::~DIR248_TD(){}