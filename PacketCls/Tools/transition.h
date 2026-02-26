#ifndef  TRANSITION_H
#define  TRANSITION_H

#include "../Elements/Elements.h"

using namespace std;

double GetRunTimeInSeconds(timeval timeval_start, timeval timeval_end);
double GetRunTimeInMicroSeconds(timeval timeval_start, timeval timeval_end);

double GetTimeInSeconds(struct timespec start, struct timespec end);
double GetTimeInMicroSeconds(struct timespec start, struct timespec end);

vector<TraceFreq> TFNodeToTraceFreq(vector<TFNode> hkTF);
string traceToString(Trace *trace);
#endif