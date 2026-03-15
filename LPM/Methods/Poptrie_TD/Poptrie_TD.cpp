#include "Poptrie_TD.h"
#include <algorithm>
#include <cstring>
#include <unordered_map>

uint8_t Poptrie_TD::TOP_LEVEL_STRIDE = 16;

Poptrie_TD::Poptrie_TD() : totalMemory(0) {
    topLevel.indexTable = nullptr;    
    topLevel.directResult = nullptr;  
    topLevel.internalNodes = nullptr; 
}

Poptrie_TD::~Poptrie_TD() {
    clear();
}

void Poptrie_TD::clear() {
    if (topLevel.internalNodes != nullptr && topLevel.indexTable != nullptr) {
        uint32_t topLevelSize = 1ULL << TOP_LEVEL_STRIDE; 
        uint32_t internalNodeCount = 0;
        
        for (uint32_t i = 0; i < topLevelSize; ++i) {
            if (!(topLevel.indexTable[i] & (1ULL << 31))) {
                internalNodeCount++;
            }
        }
        
        for (uint32_t i = 0; i < internalNodeCount; ++i) {
            destroyInternalNode(&topLevel.internalNodes[i]);
        }
        
        delete[] topLevel.internalNodes;
        topLevel.internalNodes = nullptr;
    }
    
    if (topLevel.indexTable != nullptr) {
        delete[] topLevel.indexTable;
        topLevel.indexTable = nullptr;
    }
    
    if (topLevel.directResult != nullptr) {
        delete[] topLevel.directResult;
        topLevel.directResult = nullptr;
    }

    totalMemory = 0;
}


void Poptrie_TD::destroyInternalNode(InternalNode* node) {
    if (node == nullptr) return; 
    
    if (node->childNodes != nullptr) {
        uint32_t childCount = 0;
        for (int i = 0; i < 8; ++i) {
            childCount += __builtin_popcountll(node->bitVector[i]);
        }
        
        for (uint32_t i = 0; i < childCount; ++i) {
            destroyInternalNode(&node->childNodes[i]);
        }

        delete[] node->childNodes;
        node->childNodes = nullptr;
    }
    
    if (node->directResult != nullptr) {
        delete[] node->directResult;
        node->directResult = nullptr;
    }
}

inline uint32_t Poptrie_TD::extractBits(__uint128_t ip, uint8_t offset, uint8_t length) const {
    if (length == 0 || offset + length > 128) return 0; 
    return static_cast<uint32_t>((ip >> (128 - offset - length)) & ((1ULL << length) - 1));
}

inline bool Poptrie_TD::isBitSet(const InternalNode* node, uint32_t index) const {
    if (index >= 512) return false; 
    uint8_t vectorIndex = index >> 6;  
    uint8_t bitIndex = index & 63;     
    return (node->bitVector[vectorIndex] >> bitIndex) & 1ULL;
}

inline void Poptrie_TD::setBit(InternalNode* node, uint32_t index) {
    if (index >= 512) return; 
    uint8_t vectorIndex = index >> 6;
    uint8_t bitIndex = index & 63;
    node->bitVector[vectorIndex] |= (1ULL << bitIndex); 
}

inline uint32_t Poptrie_TD::countSetBits(const InternalNode* node, uint32_t index) const {
    if (index >= 512) {
        index = 512;
    }

    uint32_t count = 0;
    const uint8_t fullBlocks = static_cast<uint8_t>(index >> 6);

    for (uint8_t blockIdx = 0; blockIdx < fullBlocks; ++blockIdx) {
        count += __builtin_popcountll(node->bitVector[blockIdx]);
    }

    const uint8_t remainingBits = static_cast<uint8_t>((index & 63) + 1); 
    uint64_t mask;

    if (remainingBits == 64) mask = ~0ULL; 
    else mask = (1ULL << remainingBits) - 1; 

    count += __builtin_popcountll(node->bitVector[fullBlocks] & mask);

    return count;
}

unordered_map<uint32_t, vector<Prefix*>> Poptrie_TD::groupPrefixesByRange(
    const vector<Prefix*>& prefixes, uint8_t currentDepth, uint8_t stride) {
    unordered_map<uint32_t, vector<Prefix*>> rangeToPrefixes;

    for (const auto& prefix : prefixes) {
        uint8_t prefixBitsInNode = prefix->prefix_len - currentDepth;
        if (prefix->prefix_len < currentDepth) prefixBitsInNode = 0; 
        if (prefixBitsInNode > stride) prefixBitsInNode = stride;    

        __uint128_t prefixIp = ((__uint128_t)prefix->ip6_upper << 64) | prefix->ip6_lower;
        uint32_t baseRange = extractBits(prefixIp, currentDepth, prefixBitsInNode);

        const uint8_t variableBits = stride - prefixBitsInNode;
        const uint32_t rangeStart = baseRange << variableBits;  
        const uint32_t rangeEnd = rangeStart + ((1ULL << variableBits) - 1); 

        for (uint32_t range = rangeStart; range <= rangeEnd; ++range) {
            rangeToPrefixes[range].push_back(const_cast<Prefix*>(prefix));
        }
    }

    return rangeToPrefixes;
}

void Poptrie_TD::buildInternalNode(InternalNode* node, const vector<Prefix*>& prefixes, 
                                uint8_t currentDepth, __uint128_t common_ip, double pop_score) {
    /* TopK */
    node->stride = 6;
    if (pop_score > 1.2) node->stride = 8;
    else if (pop_score > 1.0) node->stride = 8;
    // else if (pop_score > 0.01 && pop_score <= 0.1) node->stride = 4;
    else if (pop_score <= 0.01) node->stride = 4;

    if (currentDepth + node->stride > 128) {
        node->stride = 128 - currentDepth;
    }
    uint32_t stride = node->stride;
    uint32_t totalRangeCount = 1ULL << stride;  
    
    auto rangeToPrefixes = groupPrefixesByRange(prefixes, currentDepth, stride);
    
    memset(node->bitVector, 0, sizeof(node->bitVector)); 
    uint32_t childNodeCount = 0;                         
    
    for (uint32_t range = 0; range < totalRangeCount; ++range) {
        auto it = rangeToPrefixes.find(range);
        if (it != rangeToPrefixes.end()) {
            const auto& rangePrefixes = it->second;
            if (!rangePrefixes.empty() && rangePrefixes[0]->prefix_len > currentDepth + stride) {
                setBit(node, range);       
                childNodeCount++;         
            }
        }
    }

    uint32_t directResultSize = totalRangeCount - childNodeCount; 
    node->directResult = new uint32_t[directResultSize]();        
    totalMemory += sizeof(uint32_t) * directResultSize;          
    
    node->childNodes = nullptr;
    if (childNodeCount > 0) {
        node->childNodes = new InternalNode[childNodeCount]();   
        totalMemory += sizeof(InternalNode) * childNodeCount;    
    }
    
    uint32_t currentChildIdx = 0; 
    for (uint32_t range = 0; range < totalRangeCount; ++range) {
        auto it = rangeToPrefixes.find(range);
        const auto& rangePrefixes = (it != rangeToPrefixes.end()) ? it->second : vector<Prefix*>();
        
        if (childNodeCount == 0 || !isBitSet(node, range)) {
            uint32_t directIndex = range - countSetBits(node, range);
            uint32_t bestPort = 0;
            if (!rangePrefixes.empty()) {
                bestPort = rangePrefixes[0]->port;
            }
            if (directIndex < directResultSize) {
                node->directResult[directIndex] = bestPort;
            }
            continue;
        }
        
        if (node->childNodes != nullptr && currentChildIdx < childNodeCount) {
            /* 计算流行度 TopK*/
            __uint128_t _ip = common_ip | ((__uint128_t)range << (128 - currentDepth - stride));
            double total_trace_size = TSL::getTopKFreq(common_ip, currentDepth);
            double sub_trace_size = TSL::getTopKFreq(_ip, currentDepth + stride);
            
            double expect_trace_size = 0, son_pop_score = 0;
            expect_trace_size = total_trace_size / childNodeCount;
            if(expect_trace_size != 0) son_pop_score = sub_trace_size / expect_trace_size;

            buildInternalNode(&node->childNodes[currentChildIdx], rangePrefixes, currentDepth + stride, _ip, son_pop_score);
            currentChildIdx++; 
        }
    }
}

void Poptrie_TD::buildTopLevel(const vector<Prefix*>& prefixes) {
    uint32_t topLevelSize = 1ULL << TOP_LEVEL_STRIDE; 
    
    auto rangeToPrefixes = groupPrefixesByRange(prefixes, 0, TOP_LEVEL_STRIDE);
    
    uint32_t internalNodeCount = 0;
    for (const auto& pair : rangeToPrefixes) {
        const auto& rangePrefixes = pair.second;
        if (!rangePrefixes.empty() && rangePrefixes[0]->prefix_len > TOP_LEVEL_STRIDE) {
            internalNodeCount++;
        }
    }
    
    topLevel.indexTable = new uint32_t[topLevelSize](); 
    totalMemory += sizeof(uint32_t) * topLevelSize;     
    
    uint32_t directResultSize = topLevelSize - internalNodeCount; 
    topLevel.directResult = new uint32_t[directResultSize]();     
    totalMemory += sizeof(uint32_t) * directResultSize;          
    
    topLevel.internalNodes = nullptr;
    if (internalNodeCount > 0) {
        topLevel.internalNodes = new InternalNode[internalNodeCount](); 
        totalMemory += sizeof(InternalNode) * internalNodeCount;      
    }

    __uint128_t common_ip = 0;

    uint32_t currentInternalIdx = 0; 
    for (uint32_t range = 0; range < topLevelSize; ++range) {
        auto it = rangeToPrefixes.find(range);
        const auto& rangePrefixes = (it != rangeToPrefixes.end()) ? it->second : vector<Prefix*>();
        
        if (rangePrefixes.empty() || rangePrefixes[0]->prefix_len <= TOP_LEVEL_STRIDE) {
            uint32_t directIndex = range - currentInternalIdx;

            uint32_t bestPort = 0;
            if (!rangePrefixes.empty()) {
                bestPort = rangePrefixes[0]->port;
            }

            topLevel.indexTable[range] = directIndex | (1ULL << 31);
            if (directIndex < directResultSize) {
                topLevel.directResult[directIndex] = bestPort;
            }
            continue;
        }
        
        if (topLevel.internalNodes != nullptr && currentInternalIdx < internalNodeCount) {
            __uint128_t _ip = common_ip | ((__uint128_t)range << (128 - TOP_LEVEL_STRIDE));
            double total_trace_size = TSL::getTopKFreq(0, 0);
            double sub_trace_size = TSL::getTopKFreq(_ip, TOP_LEVEL_STRIDE);
            
            double expect_trace_size = 0, pop_score = 0;
            expect_trace_size = total_trace_size / internalNodeCount;
            if(expect_trace_size != 0) pop_score = sub_trace_size / expect_trace_size;
            
            topLevel.indexTable[range] = currentInternalIdx;
            buildInternalNode(&topLevel.internalNodes[currentInternalIdx], rangePrefixes, TOP_LEVEL_STRIDE, _ip, pop_score);
            currentInternalIdx++;
        }
    }
}

void Poptrie_TD::Create(vector<Prefix*> &prefixes, ProgramState *ps) {
    clear();

    if (prefixes.empty()) {
        cout << "Poptrie_TD::Create() : prefixes is empty" << endl;
        return;
    }

    sort(prefixes.begin(), prefixes.end(), 
        [](const Prefix* a, const Prefix* b) {
            return a->prefix_len > b->prefix_len;
        });

    buildTopLevel(prefixes);
}

uint32_t Poptrie_TD::Lookup(Trace *trace, ProgramState *ps) {
    if (topLevel.indexTable == nullptr) return 0; 

    __uint128_t ip = ((__uint128_t)trace->ip6_upper << 64) | trace->ip6_lower;
    uint32_t resultPort = 0;
    
    ps->lookup_access.Addcount(); 
    uint32_t topRange = static_cast<uint32_t>(trace->ip6_upper >> (64 - TOP_LEVEL_STRIDE));
    uint32_t topIndex = topLevel.indexTable[topRange];
    
    if (topIndex & (1ULL << 31)) {
        ps->lookup_access.Addcount();
        uint32_t directIndex = topIndex & ((1ULL << 31) - 1); 
        resultPort = topLevel.directResult[directIndex];
        ps->lookup_depth.Addcount();
    } else {
        InternalNode* currentNode = &topLevel.internalNodes[topIndex]; 
        uint8_t currentDepth = TOP_LEVEL_STRIDE; 
        ps->lookup_depth.Addcount();
        
        while (true) {
            ps->lookup_access.Addcount(); 
            uint32_t range = extractBits(ip, currentDepth, currentNode->stride);
            if (isBitSet(currentNode, range)) {
                ps->lookup_access.Addcount();
                uint32_t childIndex = countSetBits(currentNode, range) - 1; 
                currentDepth += currentNode->stride;                        
                currentNode = &currentNode->childNodes[childIndex];         
                ps->lookup_depth.Addcount();                                
            } else {
                ps->lookup_access.Addcount(); 
                uint32_t directIndex = range - countSetBits(currentNode, range);
                resultPort = currentNode->directResult[directIndex];
                break; 
            }
        }
    }
    
    ps->lookup_access.Cal();
    ps->lookup_depth.Cal();
    return resultPort;
}

uint32_t Poptrie_TD::Lookup(Trace *trace) {
    if (topLevel.indexTable == nullptr) return 0; 
    
    __uint128_t ip = ((__uint128_t)trace->ip6_upper << 64) | trace->ip6_lower;
    
    uint32_t topRange = static_cast<uint32_t>(trace->ip6_upper >> (64 - TOP_LEVEL_STRIDE));
    uint32_t topIndex = topLevel.indexTable[topRange];
    
    if (topIndex & (1ULL << 31)) {
        uint32_t directIndex = topIndex & ((1ULL << 31) - 1);
        return topLevel.directResult[directIndex];
    } else {
        InternalNode* currentNode = &topLevel.internalNodes[topIndex];
        uint8_t currentDepth = TOP_LEVEL_STRIDE;
        
        while (true) {
            uint32_t range = extractBits(ip, currentDepth, currentNode->stride);
            
            if (isBitSet(currentNode, range)) {
                uint32_t childIndex = countSetBits(currentNode, range) - 1;
                currentDepth += currentNode->stride;
                currentNode = &currentNode->childNodes[childIndex];
            } else {
                uint32_t directIndex = range - countSetBits(currentNode, range);
                return currentNode->directResult[directIndex];
            }
        }
    }
}

uint64_t Poptrie_TD::CalMemory() {
    return totalMemory;
}