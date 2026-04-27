#include "transition.h"

using namespace std;

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