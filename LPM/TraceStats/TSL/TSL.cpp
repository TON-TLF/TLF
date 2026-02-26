#include "TSL.h"

using namespace std;

bool TSL::isTopKInited = false;
vector<TopKItem> TSL::TopKStats;
void TSL::InitTopKStats(vector<TFNode> TF){
    int tf_size = TF.size();
    TSL::TopKStats.clear();
    for (int i = 0; i < tf_size; ++i){
        Trace trace = stringToTrace(TF[i].str);
        __uint128_t addr = traceTo128(&trace);
        TSL::TopKStats.push_back({
            .addr = addr,
            .count = TF[i].count
        });
    }
    sort(TSL::TopKStats.begin(), TSL::TopKStats.end());
    // for(int i = 0; i < (int)TSL::TopKStats.size() && i < 10; ++i){
    //     print_ipv6(TSL::TopKStats[i].addr);
    //     std::cout << " : " << TSL::TopKStats[i].count << std::endl;
    // }
    TSL::isTopKInited = true;
}

double TSL::getTopKFreq(__uint128_t addr, uint8_t prefix_len){
    if(prefix_len == 0){
        double total_count = 0.0;
        for(auto it : TSL::TopKStats){
            total_count += it.count;
        }
        return total_count;
    }

    TopKItem start = {.addr = addr, .count = 0};
    TopKItem end = {.addr = addr | (((__uint128_t)1 << (128 - prefix_len)) - 1), .count = 0};

    // 使用lower_bound和upper_bound定位范围
    auto it = lower_bound(TSL::TopKStats.begin(), TSL::TopKStats.end(), start); // 第一个不小于start的元素
    auto it_end = upper_bound(TSL::TopKStats.begin(), TSL::TopKStats.end(), end); // 第一个大于end的元素
    
    double total_count = 0.0;
    for (; it != it_end; ++it){
        total_count += it->count;   
    }

    return total_count;
}

size_t TSL::CalMemory(){
    size_t total_memory = 0;

    // 计算ipv6Trie的内存
    // total_memory += TSL::ipv6Trie.CalMemory();

    // 计算TopKStats的内存
    total_memory += TSL::TopKStats.size() * sizeof(TopKItem);

    return total_memory;
}

void TSL::clear(){
    TSL::TopKStats.clear();
}