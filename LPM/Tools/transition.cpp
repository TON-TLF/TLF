#include "transition.h"

using namespace std;

double GetRunTimeInSeconds(timeval timeval_start, timeval timeval_end) {
    uint64_t time_us = 1000000 * (timeval_end.tv_sec - timeval_start.tv_sec) + (timeval_end.tv_usec - timeval_start.tv_usec);
    return time_us / 1000000.0;  
}

double GetRunTimeInMicroSeconds(timeval timeval_start, timeval timeval_end) {
    uint64_t time_us = 1000000 * (timeval_end.tv_sec - timeval_start.tv_sec) + (timeval_end.tv_usec - timeval_start.tv_usec);
    return time_us;  
}

double GetTimeInSeconds(struct timespec start, struct timespec end) {
    long seconds = end.tv_sec - start.tv_sec;
    long nanoseconds = end.tv_nsec - start.tv_nsec;
    return seconds + nanoseconds * 1e-9;
}

double GetTimeInMicroSeconds(struct timespec start, struct timespec end) {
    long seconds = end.tv_sec - start.tv_sec;
    long nanoseconds = end.tv_nsec - start.tv_nsec;
    return seconds * 1e6 + nanoseconds * 1e-3;
}

__uint128_t prefix_mask(uint8_t len) {
    if (len == 0) return (__uint128_t)0;  
    if (len >= 128) return ~((__uint128_t)0);
    return (~((__uint128_t)0)) << (128 - len);
}

__uint128_t trim_prefix(__uint128_t prefix, uint8_t len) {
    return (len == 0) ? (__uint128_t)0 : (prefix & prefix_mask(len));
}

__uint128_t StrToUint128(const string& str) {
    __uint128_t result = 0;
    for (char c : str) {
        if (isdigit(c)) { 
            result = result * 10 + (c - '0');
        }
    }
    return result;
}

string Uint128ToStr(__uint128_t ip) {
    if (ip == 0) return "0";
    
    string result;
    while (ip) {
        result = char('0' + ip % 10) + result;
        ip /= 10;
    }
    return result;
}

__uint128_t GetLargestPowerOfTwo(__uint128_t n) {
    if (n == 0) return 0;
    
    __uint128_t value = 1;
    while (value <= n / 2) {
        value *= 2;
    }
    return value;
}

int log2_floor(__uint128_t value) {
    if (value == 0) return 0;
    int bits = 0;
    __uint128_t temp = value;
    while (temp > 1) {
        bits++;
        temp >>= 1;
    }
    return bits;
}
double Log2_128(__uint128_t value) {
    if (value == 0) return 0;
    
    int bits = 0;
    __uint128_t tmp = value;
    while (tmp > 1) {
        bits++;
        tmp >>= 1;
    }
    return bits;  
}

double SafeScopeFactor(__uint128_t low, __uint128_t high, double max_scope) {
    double log_range = Log2_128(high) - Log2_128(low);
    double log_max = log2(max_scope);
    return exp2(log_range - log_max);
}

string traceToString(const Trace* trace) {
    assert(trace != nullptr && "traceToString: Trace 指针不可为空！");

    std::string result(16, '\0');
    char* ptr = &result[0];  

    for (int i = 0; i < 8; ++i) {
        uint64_t shift_bits = 64 - 8 * (i + 1);
        uint8_t byte = static_cast<uint8_t>((trace->ip6_upper >> shift_bits) & 0xFF);
        ptr[i] = static_cast<char>(byte);
    }
    ptr += 8;  

    for (int i = 0; i < 8; ++i) {
        uint64_t shift_bits = 64 - 8 * (i + 1);
        uint8_t byte = static_cast<uint8_t>((trace->ip6_lower >> shift_bits) & 0xFF);
        ptr[i] = static_cast<char>(byte);
    }

    return result;
}

Trace stringToTrace(const std::string& s) {
    assert(s.size() == 16 && "stringToTrace error！");

    Trace trace{0,0};  

    for (int i = 0; i < 8; ++i) {
        uint8_t byte = static_cast<uint8_t>(s[i]);
        uint64_t shift_bits = 64 - 8 * (i + 1);
        trace.ip6_upper |= static_cast<uint64_t>(byte) << shift_bits;
    }

    for (int i = 0; i < 8; ++i) {
        uint8_t byte = static_cast<uint8_t>(s[8 + i]); 
        uint64_t shift_bits = 64 - 8 * (i + 1);
        trace.ip6_lower |= static_cast<uint64_t>(byte) << shift_bits;
    }

    return trace;
}

__uint128_t traceTo128(const Trace* trace) {
    return static_cast<__uint128_t>(trace->ip6_upper) << 64 | trace->ip6_lower;
}

void print_ipv6(__uint128_t addr) {
    uint16_t seg[8];
    for (int i = 7; i >= 0; --i) {
        seg[i] = (addr >> (i * 16)) & 0xFFFF;  
    }
    printf("%x:%x:%x:%x:%x:%x:%x:%x\n",
            seg[7], seg[6], seg[5], seg[4],
            seg[3], seg[2], seg[1], seg[0]);
}