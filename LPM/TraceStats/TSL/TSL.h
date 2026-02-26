#ifndef  TSL_H
#define  TSL_H

#include "../../Elements/Elements.h"
#include "../../Tools/transition.h"

using namespace std;

typedef struct TopKItem{
    __uint128_t addr;
    uint32_t count;
    bool operator<(const TopKItem &other) const {
        return addr < other.addr;
    }
}TopKItem;

/**
 * @brief 流量频数矩阵，存储每个流量频数的矩阵
 */
class TSL{
public:
    static bool isTopKInited;
    static vector<TopKItem> TopKStats;
    static void InitTopKStats(vector<TFNode> TF);
    static double getTopKFreq(__uint128_t addr, uint8_t prefix_len);
    static size_t CalMemory();

    static void clear();
};
#endif