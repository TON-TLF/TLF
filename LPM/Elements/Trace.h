#ifndef  TRACE_H
#define  TRACE_H

#include "Common.h"

using namespace std;

typedef struct Trace {
    uint64_t ip6_upper;
    uint64_t ip6_lower;
}Trace;

typedef struct TFNode{
	string str;
	uint32_t count;

    TFNode(string s, uint32_t c) 
        : str(move(s)), count(c) {}  // 用move优化字符串传递效率

	bool operator<(const TFNode& tfnode) const {
		return count > tfnode.count;
    }
}TFNode;
#endif