#ifndef  TRANSITION_H
#define  TRANSITION_H

#include "../Elements/Elements.h"

using namespace std;

double GetTimeInSeconds(struct timespec start, struct timespec end);
double GetTimeInMicroSeconds(struct timespec start, struct timespec end);
string traceToString(Trace *trace);

#endif