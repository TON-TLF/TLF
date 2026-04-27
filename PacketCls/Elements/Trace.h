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

struct TraceFreq{
	Trace trace;
	double freq;
	bool operator<(const TraceFreq& tf) const {
		return freq > tf.freq;
    }
};

#endif