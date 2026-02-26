#ifndef  TRACE_H
#define  TRACE_H

#include "Common.h"

using namespace std;

struct Trace{
	uint32_t key[5];  
	bool operator<(const Trace& trace) const {
		for (int i = 0; i < 4; ++i)
			if (key[i] != trace.key[i])
				return key[i] < trace.key[i];
		return key[4] < trace.key[4];
    }

    bool operator==(const Trace& trace) const {
		for (int i = 0; i < 5; ++i)
			if (key[i] != trace.key[i])
					return false;
		return true;
    }
};

struct TFNode{
	string str;
	uint64_t freq;
	uint64_t thre;
	bool operator<(const TFNode& tfnode) const {
		return freq > tfnode.freq;
    }
};

struct TraceFreq{
	Trace trace;
	double freq;
};

#endif