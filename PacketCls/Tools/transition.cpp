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

/**
 * @brief 将 TFNode 转换回 TraceFreq，逆转 traceToString 的逻辑
 */
TraceFreq getTraceFreq(const TFNode &tmp) {
    TraceFreq ans;
    ans.freq = static_cast<double>(tmp.freq); // 频率赋值

    // 必须确保字符串长度足够，虽然理论上 sketch 出来的都是 13 字节
    if (tmp.str.size() < 13) {
        // 这里可以做一些错误处理，或者直接返回空
        memset(ans.trace.key, 0, sizeof(ans.trace.key));
        cout<<"Error: TFNode string size less than 13 bytes!"<<endl;
        return ans;
    }

    // 获取字符串数据的无符号指针，避免位运算时的符号扩展问题
    const unsigned char* ptr = reinterpret_cast<const unsigned char*>(tmp.str.data());

    // 1. 恢复源IP (Bytes 0-3) -> key[0]
    // 之前是大端序写入：ptr[0]是最高位
    ans.trace.key[0] = (static_cast<uint32_t>(ptr[0]) << 24) |
                       (static_cast<uint32_t>(ptr[1]) << 16) |
                       (static_cast<uint32_t>(ptr[2]) << 8)  |
                       (static_cast<uint32_t>(ptr[3]));

    // 2. 恢复目的IP (Bytes 4-7) -> key[1]
    ans.trace.key[1] = (static_cast<uint32_t>(ptr[4]) << 24) |
                       (static_cast<uint32_t>(ptr[5]) << 16) |
                       (static_cast<uint32_t>(ptr[6]) << 8)  |
                       (static_cast<uint32_t>(ptr[7]));

    // 3. 恢复源端口 (Bytes 8-9) -> key[2]
    // 之前只取了低16位存入，现在恢复回来
    ans.trace.key[2] = (static_cast<uint32_t>(ptr[8]) << 8) |
                       (static_cast<uint32_t>(ptr[9]));

    // 4. 恢复目的端口 (Bytes 10-11) -> key[3]
    ans.trace.key[3] = (static_cast<uint32_t>(ptr[10]) << 8) |
                       (static_cast<uint32_t>(ptr[11]));

    // 5. 恢复协议 (Byte 12) -> key[4]
    ans.trace.key[4] = static_cast<uint32_t>(ptr[12]);

    return ans;
}

/**
 * @brief 批量转换函数
 */
vector<TraceFreq> TFNodeToTraceFreq(vector<TFNode> hkTF){
    vector<TraceFreq> result;
    // 使用 reserve 预分配内存以提高性能
    result.reserve(hkTF.size());
    
    int hkTF_len = hkTF.size();
    for(int i = 0; i < hkTF_len; i++){
        result.push_back(getTraceFreq(hkTF[i]));
    }
    return result;
}

string traceToString(Trace *trace) {
    // 创建13字节的字符串
    string result(13, '\0');
    char* ptr = &result[0];  // 指向字符串的指针

    // 源IP（4字节，大端序）
    for (int i = 0; i < 4; ++i) {
        ptr[i] = static_cast<char>((trace->key[0] >> (24 - 8 * i)) & 0xFF);
    }
    ptr += 4;

    // 目的IP（4字节，大端序）
    for (int i = 0; i < 4; ++i) {
        ptr[i] = static_cast<char>((trace->key[1] >> (24 - 8 * i)) & 0xFF);
    }
    ptr += 4;

    // 源端口（2字节，大端序），取key[2]的低16位
    *ptr++ = static_cast<char>((trace->key[2] >> 8) & 0xFF);  // 高字节
    *ptr++ = static_cast<char>(trace->key[2] & 0xFF);        // 低字节

    // 目的端口（2字节，大端序），取key[3]的低16位
    *ptr++ = static_cast<char>((trace->key[3] >> 8) & 0xFF); // 高字节
    *ptr++ = static_cast<char>(trace->key[3] & 0xFF);       // 低字节

    // 协议（1字节），取key[4]的最低字节
    *ptr = static_cast<char>(trace->key[4] & 0xFF);

    return result;
}