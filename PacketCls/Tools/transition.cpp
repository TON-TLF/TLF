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

TraceFreq getTraceFreq(const TFNode &tmp) {
    TraceFreq ans;
    ans.freq = static_cast<double>(tmp.freq); 

    if (tmp.str.size() < 13) {
        memset(ans.trace.key, 0, sizeof(ans.trace.key));
        cout<<"Error: TFNode string size less than 13 bytes!"<<endl;
        return ans;
    }

    const unsigned char* ptr = reinterpret_cast<const unsigned char*>(tmp.str.data());

    ans.trace.key[0] = (static_cast<uint32_t>(ptr[0]) << 24) |
                       (static_cast<uint32_t>(ptr[1]) << 16) |
                       (static_cast<uint32_t>(ptr[2]) << 8)  |
                       (static_cast<uint32_t>(ptr[3]));

    ans.trace.key[1] = (static_cast<uint32_t>(ptr[4]) << 24) |
                       (static_cast<uint32_t>(ptr[5]) << 16) |
                       (static_cast<uint32_t>(ptr[6]) << 8)  |
                       (static_cast<uint32_t>(ptr[7]));

    ans.trace.key[2] = (static_cast<uint32_t>(ptr[8]) << 8) |
                       (static_cast<uint32_t>(ptr[9]));

    ans.trace.key[3] = (static_cast<uint32_t>(ptr[10]) << 8) |
                       (static_cast<uint32_t>(ptr[11]));

    ans.trace.key[4] = static_cast<uint32_t>(ptr[12]);

    return ans;
}

vector<TraceFreq> TFNodeToTraceFreq(vector<TFNode> hkTF){
    vector<TraceFreq> result;
    result.reserve(hkTF.size());
    
    int hkTF_len = hkTF.size();
    for(int i = 0; i < hkTF_len; i++){
        result.push_back(getTraceFreq(hkTF[i]));
    }
    return result;
}

string traceToString(Trace *trace) {
    string result(13, '\0');
    char* ptr = &result[0];  
    for (int i = 0; i < 4; ++i) {
        ptr[i] = static_cast<char>((trace->key[0] >> (24 - 8 * i)) & 0xFF);
    }
    ptr += 4;

    for (int i = 0; i < 4; ++i) {
        ptr[i] = static_cast<char>((trace->key[1] >> (24 - 8 * i)) & 0xFF);
    }
    ptr += 4;

    *ptr++ = static_cast<char>((trace->key[2] >> 8) & 0xFF);  
    *ptr++ = static_cast<char>(trace->key[2] & 0xFF);        

    *ptr++ = static_cast<char>((trace->key[3] >> 8) & 0xFF); 
    *ptr++ = static_cast<char>(trace->key[3] & 0xFF);       

    *ptr = static_cast<char>(trace->key[4] & 0xFF);

    return result;
}