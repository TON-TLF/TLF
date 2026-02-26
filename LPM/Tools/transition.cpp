#include "transition.h"

using namespace std;

// 计算两个时间点之间的时间差，返回秒数
double GetRunTimeInSeconds(timeval timeval_start, timeval timeval_end) {
    uint64_t time_us = 1000000 * (timeval_end.tv_sec - timeval_start.tv_sec) + (timeval_end.tv_usec - timeval_start.tv_usec);
    return time_us / 1000000.0;  // 转换为秒并保留两位小数
}

double GetRunTimeInMicroSeconds(timeval timeval_start, timeval timeval_end) {
    uint64_t time_us = 1000000 * (timeval_end.tv_sec - timeval_start.tv_sec) + (timeval_end.tv_usec - timeval_start.tv_usec);
    return time_us;  
}

// 辅助函数：计算时间差，返回单位：秒 (Seconds)
// 适用于：构造时间、分析时间等较长的过程
double GetTimeInSeconds(struct timespec start, struct timespec end) {
    long seconds = end.tv_sec - start.tv_sec;
    long nanoseconds = end.tv_nsec - start.tv_nsec;
    return seconds + nanoseconds * 1e-9;
}

// 辅助函数：计算时间差，返回单位：微秒 (Microseconds)
// 适用于：查询延迟 (Lookup Latency) 等极短的过程
double GetTimeInMicroSeconds(struct timespec start, struct timespec end) {
    long seconds = end.tv_sec - start.tv_sec;
    long nanoseconds = end.tv_nsec - start.tv_nsec;
    // 秒转微秒(*1e6) + 纳秒转微秒(/1e3)
    return seconds * 1e6 + nanoseconds * 1e-3;
}

// 辅助函数声明
__uint128_t prefix_mask(uint8_t len) {
    if (len == 0) return (__uint128_t)0;  // 明确处理len=0的情况
    if (len >= 128) return ~((__uint128_t)0);
    return (~((__uint128_t)0)) << (128 - len);
}

// 修正后的前缀截断函数
__uint128_t trim_prefix(__uint128_t prefix, uint8_t len) {
    return (len == 0) ? (__uint128_t)0 : (prefix & prefix_mask(len));
}

/**
 * @brief 将IPv6地址字符串转换为128位整数形式
 * @param str 字符串形式的IPv6地址（纯十进制）
 * @return __uint128_t 128位整数形式的地址
 */
__uint128_t StrToUint128(const string& str) {
    __uint128_t result = 0;
    for (char c : str) {
        if (isdigit(c)) { // 只处理数字
            result = result * 10 + (c - '0');
        }
    }
    return result;
}

/**
 * @brief 将128位整数IP转换为字符串形式
 * @param ip 128位整数IP
 * @return 字符串形式的IP
 */
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

// 128位整数的近似对数
double Log2_128(__uint128_t value) {
    if (value == 0) return 0;
    
    // 计算最高位位置
    int bits = 0;
    __uint128_t tmp = value;
    while (tmp > 1) {
        bits++;
        tmp >>= 1;
    }
    return bits;  // 返回整数部分的对数
}

// 安全计算范围比例因子（避免128位整数计算溢出）
double SafeScopeFactor(__uint128_t low, __uint128_t high, double max_scope) {
    // 使用对数转换避免128位整数除法
    double log_range = Log2_128(high) - Log2_128(low);
    double log_max = log2(max_scope);
    return exp2(log_range - log_max);
}

string traceToString(const Trace* trace) {
    // 断言：防止空指针访问（提升代码安全性）
    assert(trace != nullptr && "traceToString: Trace 指针不可为空！");

    // IPv6 地址为 128 位 = 16 字节，字符串长度设为 16
    std::string result(16, '\0');
    char* ptr = &result[0];  // 指向字符串起始位置的指针

    // ------------------------------ 处理 IPv6 高 64 位（ip6_upper）：大端序存入前 8 字节 ------------------------------
    // 大端序：高字节先存（如 ip6_upper 的最高 8 位存入 result[0]，最低 8 位存入 result[7]）
    for (int i = 0; i < 8; ++i) {
        // 右移位数：64 - 8*(i+1)（如 i=0 时右移 56 位，取最高 8 位；i=7 时右移 0 位，取最低 8 位）
        uint64_t shift_bits = 64 - 8 * (i + 1);
        // 提取当前 8 位（用 uint8_t 确保无符号，避免 char 符号扩展问题）
        uint8_t byte = static_cast<uint8_t>((trace->ip6_upper >> shift_bits) & 0xFF);
        ptr[i] = static_cast<char>(byte);
    }
    ptr += 8;  // 指针移动到后 8 字节（用于存储 IPv6 低64位）

    // ------------------------------ 处理 IPv6 低 64 位（ip6_lower）：大端序存入后 8 字节 ------------------------------
    for (int i = 0; i < 8; ++i) {
        uint64_t shift_bits = 64 - 8 * (i + 1);
        uint8_t byte = static_cast<uint8_t>((trace->ip6_lower >> shift_bits) & 0xFF);
        ptr[i] = static_cast<char>(byte);
    }

    return result;
}

/**
 * 将字符串转回128位IPv6地址（Trace结构体）
 * @param s 输入字符串（必须是16字节，由traceToString生成）
 * @return 还原后的Trace结构体
 */
Trace stringToTrace(const std::string& s) {
    // 断言：输入字符串必须是16字节（否则无法还原）
    assert(s.size() == 16 && "stringToTrace: 输入字符串必须为16字节长度！");

    Trace trace{0,0};  // 初始化结构体（默认值0）

    // ------------------------------ 还原高64位（ip6_upper）------------------------------
    // 字符串前8字节（索引0-7）对应ip6_upper的大端序存储，需按顺序拼接
    for (int i = 0; i < 8; ++i) {
        // 将字符转换为无符号8位整数（避免符号扩展问题）
        uint8_t byte = static_cast<uint8_t>(s[i]);
        // 计算左移位数（大端序：第i个字节对应64位中的高[64-8*(i+1)]位）
        uint64_t shift_bits = 64 - 8 * (i + 1);
        // 拼接当前字节到ip6_upper
        trace.ip6_upper |= static_cast<uint64_t>(byte) << shift_bits;
    }

    // ------------------------------ 还原低64位（ip6_lower）------------------------------
    // 字符串后8字节（索引8-15）对应ip6_lower的大端序存储，同样按顺序拼接
    for (int i = 0; i < 8; ++i) {
        uint8_t byte = static_cast<uint8_t>(s[8 + i]);  // 取后8字节中的第i个
        uint64_t shift_bits = 64 - 8 * (i + 1);
        trace.ip6_lower |= static_cast<uint64_t>(byte) << shift_bits;
    }

    return trace;
}

__uint128_t traceTo128(const Trace* trace) {
    return static_cast<__uint128_t>(trace->ip6_upper) << 64 | trace->ip6_lower;
}

// 打印128位IPv6地址（辅助函数）
void print_ipv6(__uint128_t addr) {
    uint16_t seg[8];
    for (int i = 7; i >= 0; --i) {
        seg[i] = (addr >> (i * 16)) & 0xFFFF;  // 按16位段拆分
    }
    printf("%x:%x:%x:%x:%x:%x:%x:%x\n",
            seg[7], seg[6], seg[5], seg[4],
            seg[3], seg[2], seg[1], seg[0]);
}