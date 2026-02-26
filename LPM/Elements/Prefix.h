#ifndef  PREFIX_H
#define  PREFIX_H

#include "Common.h"

using namespace std;

typedef struct Prefix {
    uint64_t ip6_upper;
    uint64_t ip6_lower;
    uint8_t  prefix_len;
    uint32_t port;
}Prefix;

#endif