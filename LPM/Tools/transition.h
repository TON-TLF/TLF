#ifndef  TRANSITION_H
#define  TRANSITION_H

#include "../Elements/Elements.h"

using namespace std;

double GetRunTimeInSeconds(timeval timeval_start, timeval timeval_end);
double GetRunTimeInMicroSeconds(timeval timeval_start, timeval timeval_end);

double GetTimeInSeconds(struct timespec start, struct timespec end);
double GetTimeInMicroSeconds(struct timespec start, struct timespec end);

__uint128_t prefix_mask(uint8_t len);
__uint128_t trim_prefix(__uint128_t prefix, uint8_t len);

__uint128_t StrToUint128(const string& str);
string Uint128ToStr(__uint128_t ip);

__uint128_t GetLargestPowerOfTwo(__uint128_t n);
int log2_floor(__uint128_t value);
double SafeScopeFactor(__uint128_t low, __uint128_t high, double max_scope);
string traceToString(const Trace* trace);
Trace stringToTrace(const std::string& s);
__uint128_t traceTo128(const Trace* trace);
// 打印128位IPv6地址（辅助函数）
void print_ipv6(__uint128_t addr);
#endif