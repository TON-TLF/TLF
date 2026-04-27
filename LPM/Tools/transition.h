#ifndef  TRANSITION_H
#define  TRANSITION_H

#include "../Elements/Elements.h"

using namespace std;

double GetTimeInSeconds(struct timespec start, struct timespec end);
double GetTimeInMicroSeconds(struct timespec start, struct timespec end);

__uint128_t trim_prefix(__uint128_t prefix, uint8_t len);
string traceToString(const Trace* trace);
Trace stringToTrace(const std::string& s);
__uint128_t traceTo128(const Trace* trace);
void print_ipv6(__uint128_t addr);
#endif