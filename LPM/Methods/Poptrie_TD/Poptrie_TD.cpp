#include "Poptrie_TD.h"
#include <algorithm>
#include <cstring>
#include <unordered_map>

// 静态常量初始化：顶层节点处理的IP位数（固定16位，Poptrie标准设计）
uint8_t Poptrie_TD::TOP_LEVEL_STRIDE = 16;

/**
 * @brief 构造函数
 * 流程：
 * 1. 初始化总内存占用为0
 * 2. 将顶层结构的三个指针（索引表、直接结果表、内部节点数组）置空，避免野指针
 */
Poptrie_TD::Poptrie_TD() : totalMemory(0) {
    topLevel.indexTable = nullptr;    // 顶层索引表指针（后续存储范围→索引映射）
    topLevel.directResult = nullptr;  // 顶层直接结果表指针（存储无内部节点的匹配结果）
    topLevel.internalNodes = nullptr; // 顶层内部节点数组指针（存储有子节点的内部节点）
}

/**
 * @brief 析构函数
 * 流程：
 * 1. 调用clear()函数释放所有动态内存，避免内存泄漏
 * （析构函数仅负责触发释放，具体释放逻辑封装在clear()中，提高可维护性）
 */
Poptrie_TD::~Poptrie_TD() {
    clear();
}

/**
 * @brief 内存释放核心函数：释放所有动态分配的资源
 * 流程：
 * 1. 释放顶层结构的索引表（indexTable）
 * 2. 释放顶层结构的直接结果表（directResult）
 * 3. 递归释放顶层内部节点数组（internalNodes）：
 *    a. 先统计顶层内部节点数量（通过indexTable判断：最高位为0的是内部节点索引）
 *    b. 逐个调用destroyInternalNode()释放每个内部节点的子节点和资源
 *    c. 释放顶层内部节点数组本身
 * 4. 重置总内存占用为0
 */
void Poptrie_TD::clear() {
    // 步骤3：递归释放顶层内部节点数组（需先确保indexTable未被释放，否则无法统计节点数）
    if (topLevel.internalNodes != nullptr && topLevel.indexTable != nullptr) {
        uint32_t topLevelSize = 1ULL << TOP_LEVEL_STRIDE; // 顶层总范围数：2^16=65536
        uint32_t internalNodeCount = 0;
        
        // 步骤3a：统计顶层内部节点数量（indexTable中最高位为0的是内部节点索引）
        for (uint32_t i = 0; i < topLevelSize; ++i) {
            if (!(topLevel.indexTable[i] & (1ULL << 31))) {
                internalNodeCount++;
            }
        }
        
        // 步骤3b：逐个释放每个内部节点（递归释放子节点）
        for (uint32_t i = 0; i < internalNodeCount; ++i) {
            destroyInternalNode(&topLevel.internalNodes[i]);
        }
        
        // 步骤3c：释放顶层内部节点数组本身
        delete[] topLevel.internalNodes;
        topLevel.internalNodes = nullptr;
    }
    
    // 步骤1：释放顶层索引表（存储范围→索引的映射，大小为2^16=65536）
    if (topLevel.indexTable != nullptr) {
        delete[] topLevel.indexTable;
        topLevel.indexTable = nullptr;
    }
    
    // 步骤2：释放顶层直接结果表（存储无内部节点的范围匹配结果）
    if (topLevel.directResult != nullptr) {
        delete[] topLevel.directResult;
        topLevel.directResult = nullptr;
    }

    // 步骤4：重置总内存统计
    totalMemory = 0;
}

/**
 * @brief 递归释放单个内部节点的资源
 * 作用：处理内部节点的子节点和直接结果表，避免嵌套内存泄漏
 * 流程：
 * 1. 递归释放当前节点的子节点数组（childNodes）：
 *    a. 计算子节点数量（通过bitVector中1的个数，创建时子节点数=1的个数）
 *    b. 逐个递归调用destroyInternalNode()释放子节点
 *    c. 释放子节点数组本身
 * 2. 释放当前节点的直接结果表（directResult）
 * @param node 待释放的内部节点指针
 */
void Poptrie_TD::destroyInternalNode(InternalNode* node) {
    if (node == nullptr) return; // 防御性检查：空指针直接返回
    
    // 步骤1：递归释放子节点数组
    if (node->childNodes != nullptr) {
        uint32_t childCount = 0;
        // 步骤1a：计算子节点数量（bitVector共8个64位，统计所有1的个数）
        for (int i = 0; i < 8; ++i) {
            childCount += __builtin_popcountll(node->bitVector[i]);
        }
        
        // 步骤1b：逐个释放子节点（递归处理子节点的子节点）
        for (uint32_t i = 0; i < childCount; ++i) {
            destroyInternalNode(&node->childNodes[i]);
        }
        
        // 步骤1c：释放子节点数组本身
        delete[] node->childNodes;
        node->childNodes = nullptr;
    }
    
    // 步骤2：释放当前节点的直接结果表
    if (node->directResult != nullptr) {
        delete[] node->directResult;
        node->directResult = nullptr;
    }
}

/**
 * @brief 位提取工具函数：从128位IP中提取指定区间的位
 * 作用：分层匹配时，提取当前节点需要处理的IP位（如顶层提取前16位，内部节点提取6位）
 * 流程：
 * 1. 边界检查：若提取长度为0或区间超出128位，返回0
 * 2. 计算右移位数：128 - 起始偏移量（offset） - 提取长度（length）
 * 3. 提取目标位：右移后与掩码（(1<<length)-1）按位与，得到提取结果
 * @param ip 128位IP地址（IPv6）
 * @param offset 起始偏移量（从IP最高位开始计数，如offset=0表示最高位）
 * @param length 提取的位数（如length=16表示提取16位）
 * @return 提取的位组成的32位整数
 */
inline uint32_t Poptrie_TD::extractBits(__uint128_t ip, uint8_t offset, uint8_t length) const {
    if (length == 0 || offset + length > 128) return 0; // 边界检查：无效提取返回0
    // 右移到最低位 + 掩码过滤，得到提取结果
    return static_cast<uint32_t>((ip >> (128 - offset - length)) & ((1ULL << length) - 1));
}

/**
 * @brief 位向量检查工具函数：判断内部节点bitVector中指定位置是否为1
 * 作用：标记该位置对应的范围是否有子节点（1=有子节点，0=无）
 * 流程：
 * 1. 边界检查：若位置≥512（bitVector共8×64=512位），返回false
 * 2. 计算所在64位块的索引（vectorIndex = 位置 >> 6，即除以64）
 * 3. 计算块内偏移（bitIndex = 位置 & 63，即取余64）
 * 4. 检查该位是否为1：右移bitIndex位后与1按位与
 * @param node 待检查的内部节点
 * @param index 位向量中的位置索引（0~511）
 * @return true=该位为1（有子节点），false=该位为0（无子节点）
 */
inline bool Poptrie_TD::isBitSet(const InternalNode* node, uint32_t index) const {
    if (index >= 512) return false; // 边界检查：超出bitVector范围返回false
    uint8_t vectorIndex = index >> 6;  // 计算所在64位块（0~7）
    uint8_t bitIndex = index & 63;     // 计算块内位置（0~63）
    return (node->bitVector[vectorIndex] >> bitIndex) & 1ULL;
}

/**
 * @brief 位向量设置工具函数：将内部节点bitVector中指定位置设为1
 * 作用：标记该位置对应的范围需要创建子节点
 * 流程：
 * 1. 边界检查：若位置≥512，直接返回
 * 2. 计算所在64位块的索引和块内偏移（同isBitSet）
 * 3. 置1操作：与（1<<bitIndex）按位或，确保该位为1
 * @param node 待设置的内部节点
 * @param index 位向量中的位置索引（0~511）
 */
inline void Poptrie_TD::setBit(InternalNode* node, uint32_t index) {
    if (index >= 512) return; // 边界检查：超出范围不操作
    uint8_t vectorIndex = index >> 6;
    uint8_t bitIndex = index & 63;
    node->bitVector[vectorIndex] |= (1ULL << bitIndex); // 该位置1
}

/**
 * @brief 位向量统计工具函数：统计bitVector中指定位置前的1的个数
 * 作用：计算子节点数组索引（有子节点的范围按1的顺序排列）和直接结果表索引
 * 流程：
 * 1. 边界处理：位置≥512按512处理
 * 2. 统计完整64位块的1的个数（fullBlocks = 位置 >> 6）
 * 3. 统计剩余部分的1的个数（remainingBits = 位置 & 63）
 * 4. 返回总个数
 * @param node 待统计的内部节点
 * @param index 统计的终止位置（前index位）
 * @return 前index位中1的总个数
 */
inline uint32_t Poptrie_TD::countSetBits(const InternalNode* node, uint32_t index) const {
    // 边界处理：超出511位按511处理（bitVector最大索引）
    if (index >= 512) {
        index = 512;
    }

    uint32_t count = 0;
    // 完整64位块的数量（例如index=65 → 65>>6=1，即第0块全统计）
    const uint8_t fullBlocks = static_cast<uint8_t>(index >> 6);

    // 步骤1：统计所有完整64位块中的1
    for (uint8_t blockIdx = 0; blockIdx < fullBlocks; ++blockIdx) {
        count += __builtin_popcountll(node->bitVector[blockIdx]);
    }

    // 步骤2：统计剩余部分（包含index所在位）的1
    const uint8_t remainingBits = static_cast<uint8_t>((index & 63) + 1); // [0, index]共remainingBits位
    uint64_t mask;

    if (remainingBits == 64) mask = ~0ULL; // 特殊处理：64位全1掩码（避免溢出）
    else mask = (1ULL << remainingBits) - 1; // 前remainingBits位为1，其余为0

    count += __builtin_popcountll(node->bitVector[fullBlocks] & mask);

    return count;
}

/**
 * @brief 前缀分组核心函数：将前缀按当前节点的范围分组（解决短前缀多范围覆盖问题）
 * 作用：为节点构建提供“范围→前缀数组”的映射，确保短前缀覆盖所有相关范围
 * 流程：
 * 1. 初始化映射表（rangeToPrefixes：key=范围索引，value=该范围的前缀数组）
 * 2. 遍历每个前缀，按以下逻辑分组：
 *    a. 计算有效位数：prefixBitsInNode = 前缀长度 - currentDepth（最多为当前节点步长）
 *    b. 提取固定位：baseRange = 前缀在当前节点区间的固定位（高prefixBitsInNode位）
 *    c. 计算覆盖范围区间：
 *       - variableBits = 步长 - 有效位数（短前缀的可变位>0，长前缀=0）
 *       - 范围起点 = 固定位 << 可变位长度（固定位左移至高位）
 *       - 范围终点 = 范围起点 + (1 << 可变位长度) - 1（覆盖所有可变位组合）
 *    d. 遍历区间加入映射：从起点到终点遍历所有范围，将前缀加入对应数组
 * 3. 返回分组映射表
 * @param prefixes 已按长度降序排序的前缀列表（确保最长匹配优先）
 * @param currentDepth 当前节点的起始深度（IP高位已匹配的位数，如顶层为0）
 * @param stride 当前节点的步长（处理的IP位数，如内部节点默认6位）
 * @return 范围→前缀数组的映射表
 */
unordered_map<uint32_t, vector<Prefix*>> Poptrie_TD::groupPrefixesByRange(
    const vector<Prefix*>& prefixes, uint8_t currentDepth, uint8_t stride) {
    unordered_map<uint32_t, vector<Prefix*>> rangeToPrefixes;

    // 遍历每个前缀，计算其覆盖的范围区间并加入映射
    for (const auto& prefix : prefixes) {
        // 步骤2a：计算前缀在当前节点的有效范围（最多不超过 2^stride）
        uint8_t prefixBitsInNode = prefix->prefix_len - currentDepth;
        if (prefix->prefix_len < currentDepth) prefixBitsInNode = 0;  // 短前缀：覆盖所有范围
        if (prefixBitsInNode > stride) prefixBitsInNode = stride;     // 长前缀：覆盖单个范围

        // 步骤2b：提取前缀在当前节点的固定位（高prefixBitsInNode位）
        __uint128_t prefixIp = ((__uint128_t)prefix->ip6_upper << 64) | prefix->ip6_lower;
        uint32_t baseRange = extractBits(prefixIp, currentDepth, prefixBitsInNode);

        // 步骤2c：计算覆盖范围的起点和终点
        const uint8_t variableBits = stride - prefixBitsInNode;
        const uint32_t rangeStart = baseRange << variableBits;  // 范围起点
        const uint32_t rangeEnd = rangeStart + ((1ULL << variableBits) - 1);  // 范围终点

        // 步骤2d：遍历范围区间，将前缀加入所有覆盖的范围
        for (uint32_t range = rangeStart; range <= rangeEnd; ++range) {
            rangeToPrefixes[range].push_back(const_cast<Prefix*>(prefix));
        }
    }

    return rangeToPrefixes;
}


/**
 * @brief 内部节点构建函数：创建单个内部节点的结构（递归调用）
 * 作用：为每个内部节点初始化bitVector、directResult、childNodes，实现分层匹配
 * 流程：
 * 1. 确定当前节点步长：
 *    a. 默认步长6位（Poptrie标准设计）
 *    b. 若当前深度+6>128，调整为128-currentDepth（避免超出IPv6的128位）
 * 2. 前缀分组：调用groupPrefixesByRange()获取当前节点的“范围→前缀数组”映射
 * 3. 初始化bitVector并统计子节点数量：
 *    a. 清空bitVector（所有位置0）
 *    b. 遍历每个范围，若该范围有长于当前节点覆盖范围的前缀（需子节点），则置1并计数
 * 4. 分配内存：
 *    a. directResult：大小=总范围数-子节点数（存储无子女的匹配结果）
 *    b. childNodes：大小=子节点数（存储有子女的内部节点）
 * 5. 处理每个范围：
 *    a. 若无子节点：从前缀数组取第一个元素（最长匹配），写入directResult
 *    b. 若有子节点：筛选长前缀，递归调用buildInternalNode()创建子节点
 * @param node 待构建的内部节点指针
 * @param prefixes 已按长度降序排序的前缀列表
 * @param currentDepth 当前节点的起始深度（已处理的深度）
 */
void Poptrie_TD::buildInternalNode(InternalNode* node, const vector<Prefix*>& prefixes, 
                                uint8_t currentDepth, __uint128_t common_ip, double pop_score) {
    // 步骤1：确定当前节点步长（默认6位，避免超出128位）
    /* 精确统计 */
    // node->stride = 6;
    // if (pop_score > 2) node->stride = 8;
    // else if (pop_score > 1.2) node->stride = 7;
    // else if (pop_score > 0.1 && pop_score <= 0.8) node->stride = 5;
    // else if (pop_score <= 0.1) node->stride = 4;

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
    uint32_t totalRangeCount = 1ULL << stride;  // 当前节点总范围数（2^步长）
    
    // 步骤2：前缀分组（获取范围→前缀数组的映射）
    auto rangeToPrefixes = groupPrefixesByRange(prefixes, currentDepth, stride);
    
    // 步骤3：初始化bitVector并统计子节点数量
    memset(node->bitVector, 0, sizeof(node->bitVector)); // 清空bitVector
    uint32_t childNodeCount = 0;                         // 子节点总数
    
    for (uint32_t range = 0; range < totalRangeCount; ++range) {
        auto it = rangeToPrefixes.find(range);
        if (it != rangeToPrefixes.end()) {
            const auto& rangePrefixes = it->second;
            // 若该范围有长于当前节点覆盖范围的前缀（需子节点），则置1并计数
            if (!rangePrefixes.empty() && rangePrefixes[0]->prefix_len > currentDepth + stride) {
                setBit(node, range);       // 标记该范围有子节点
                childNodeCount++;          // 子节点数量+1
            }
        }
    }
    
    // 步骤4：分配内存（directResult和childNodes）
    uint32_t directResultSize = totalRangeCount - childNodeCount; // 直接结果表大小
    node->directResult = new uint32_t[directResultSize]();        // 初始化为0（无匹配时返回0）
    totalMemory += sizeof(uint32_t) * directResultSize;           // 累加内存统计
    
    node->childNodes = nullptr;
    if (childNodeCount > 0) {
        node->childNodes = new InternalNode[childNodeCount]();    // 分配子节点数组
        totalMemory += sizeof(InternalNode) * childNodeCount;     // 累加内存统计
    }
    
    // 步骤5：处理每个范围（填充directResult或构建子节点）
    uint32_t currentChildIdx = 0; // 子节点数组的当前索引
    for (uint32_t range = 0; range < totalRangeCount; ++range) {
        // 获取当前范围的前缀数组（无则为空）
        auto it = rangeToPrefixes.find(range);
        const auto& rangePrefixes = (it != rangeToPrefixes.end()) ? it->second : vector<Prefix*>();
        
        // 步骤5a：处理无子节点的范围（填充directResult）
        if (childNodeCount == 0 || !isBitSet(node, range)) {
            // 计算直接结果表的索引（公式：范围 - 前range位中1的个数）
            uint32_t directIndex = range - countSetBits(node, range);

            // 最佳匹配是前缀数组的第一个元素（已降序排序，最长优先）
            uint32_t bestPort = 0;
            if (!rangePrefixes.empty()) {
                bestPort = rangePrefixes[0]->port;
            }
            // 写入直接结果表（防御性检查：避免索引越界）
            if (directIndex < directResultSize) {
                node->directResult[directIndex] = bestPort;
            }
            continue;
        }
        
        // 步骤5b：处理有子节点的范围（递归构建子节点）
        if (node->childNodes != nullptr && currentChildIdx < childNodeCount) {
            /* 计算流行度 精确值*/
            // __uint128_t _ip = common_ip | ((__uint128_t)range << (128 - currentDepth - stride));
            // double total_trace_size = TSL::ipv6Trie.query((uint64_t)(common_ip >> 64), (uint64_t)common_ip, currentDepth);
            // double sub_trace_size = TSL::ipv6Trie.query((uint64_t)(_ip >> 64), (uint64_t)_ip, currentDepth + stride);
            
            /* 计算流行度 TopK*/
            __uint128_t _ip = common_ip | ((__uint128_t)range << (128 - currentDepth - stride));
            double total_trace_size = TSL::getTopKFreq(common_ip, currentDepth);
            double sub_trace_size = TSL::getTopKFreq(_ip, currentDepth + stride);
            
            double expect_trace_size = 0, son_pop_score = 0;
            expect_trace_size = total_trace_size / childNodeCount;
            if(expect_trace_size != 0) son_pop_score = sub_trace_size / expect_trace_size;

            // 递归构建子节点（子节点的起始深度=当前深度+步长）
            buildInternalNode(&node->childNodes[currentChildIdx], rangePrefixes, currentDepth + stride, _ip, son_pop_score);
            currentChildIdx++; // 子节点索引+1
        }
    }
}

/**
 * @brief 顶层结构构建函数：创建Poptrie的顶层索引（入口节点）
 * 作用：初始化顶层的indexTable、directResult、internalNodes，连接后续内部节点
 * 流程：
 * 1. 计算顶层总范围数（2^16=65536）
 * 2. 前缀分组：调用groupPrefixesByRange()获取顶层的“范围→前缀数组”映射
 * 3. 统计顶层内部节点数量：遍历分组结果，若范围有长于16位的前缀，计数+1
 * 4. 分配顶层内存：
 *    a. indexTable：大小=65536（存储范围→索引映射）
 *    b. directResult：大小=65536-内部节点数（存储无内部节点的匹配结果）
 *    c. internalNodes：大小=内部节点数（存储顶层内部节点）
 * 5. 处理每个顶层范围：
 *    a. 若无内部节点：配置indexTable（最高位=1），写入directResult
 *    b. 若有内部节点：配置indexTable（最高位=0），调用buildInternalNode()创建内部节点
 * @param prefixes 已按长度降序排序的前缀列表
 */
void Poptrie_TD::buildTopLevel(const vector<Prefix*>& prefixes) {
    uint32_t topLevelSize = 1ULL << TOP_LEVEL_STRIDE; // 顶层总范围数：2^16=65536
    
    // 步骤2：顶层前缀分组（起始深度0，步长16）
    auto rangeToPrefixes = groupPrefixesByRange(prefixes, 0, TOP_LEVEL_STRIDE);
    
    // 步骤3：统计顶层内部节点数量（需处理长于16位的前缀）
    uint32_t internalNodeCount = 0;
    for (const auto& pair : rangeToPrefixes) {
        const auto& rangePrefixes = pair.second;
        if (!rangePrefixes.empty() && rangePrefixes[0]->prefix_len > TOP_LEVEL_STRIDE) {
            internalNodeCount++;
        }
    }
    
    // 步骤4：分配顶层内存
    topLevel.indexTable = new uint32_t[topLevelSize]();  // 顶层索引表（65536个元素）
    totalMemory += sizeof(uint32_t) * topLevelSize;      // 累加内存统计
    
    uint32_t directResultSize = topLevelSize - internalNodeCount; // 顶层直接结果表大小
    topLevel.directResult = new uint32_t[directResultSize]();     // 初始化为0
    totalMemory += sizeof(uint32_t) * directResultSize;           // 累加内存统计
    
    topLevel.internalNodes = nullptr;
    if (internalNodeCount > 0) {
        topLevel.internalNodes = new InternalNode[internalNodeCount](); // 顶层内部节点数组
        totalMemory += sizeof(InternalNode) * internalNodeCount;        // 累加内存统计
    }

    // 计算当前节点的公共前缀
    __uint128_t common_ip = 0;

    // 步骤5：处理每个顶层范围（配置索引表+构建内部节点）
    uint32_t currentInternalIdx = 0; // 顶层内部节点数组的当前索引
    for (uint32_t range = 0; range < topLevelSize; ++range) {
        // 获取当前范围的前缀数组（无则为空）
        auto it = rangeToPrefixes.find(range);
        const auto& rangePrefixes = (it != rangeToPrefixes.end()) ? it->second : vector<Prefix*>();
        
        // 步骤5a：处理无内部节点的范围（直接匹配）
        if (rangePrefixes.empty() || rangePrefixes[0]->prefix_len <= TOP_LEVEL_STRIDE) {
            // 计算顶层直接结果表的索引（公式：范围 - 已处理的内部节点数）
            uint32_t directIndex = range - currentInternalIdx;

            // 最佳匹配是前缀数组的第一个元素（无则为0）
            uint32_t bestPort = 0;
            if (!rangePrefixes.empty()) {
                bestPort = rangePrefixes[0]->port;
            }

            // 配置索引表：最高位=1（标记直接匹配），低31位=directIndex
            topLevel.indexTable[range] = directIndex | (1ULL << 31);
            // 写入直接结果表（防御性检查）
            if (directIndex < directResultSize) {
                topLevel.directResult[directIndex] = bestPort;
            }
            continue;
        }
        
        // 步骤5b：处理有内部节点的范围（构建内部节点）
        if (topLevel.internalNodes != nullptr && currentInternalIdx < internalNodeCount) {
            /* 计算流行度 精确值*/
            // __uint128_t _ip = common_ip | ((__uint128_t)range << (128 - TOP_LEVEL_STRIDE));
            // double total_trace_size = TSL::ipv6Trie.query(0, 0, 0);
            // double sub_trace_size = TSL::ipv6Trie.query((uint64_t)(_ip >> 64), (uint64_t)_ip, TOP_LEVEL_STRIDE);

            /* 计算流行度 TopK*/
            __uint128_t _ip = common_ip | ((__uint128_t)range << (128 - TOP_LEVEL_STRIDE));
            double total_trace_size = TSL::getTopKFreq(0, 0);
            double sub_trace_size = TSL::getTopKFreq(_ip, TOP_LEVEL_STRIDE);
            
            double expect_trace_size = 0, pop_score = 0;
            expect_trace_size = total_trace_size / internalNodeCount;
            if(expect_trace_size != 0) pop_score = sub_trace_size / expect_trace_size;
            
            // 配置索引表：最高位=0（标记内部节点），值=currentInternalIdx
            topLevel.indexTable[range] = currentInternalIdx;
            // 构建顶层内部节点（起始深度=16）
            buildInternalNode(&topLevel.internalNodes[currentInternalIdx], rangePrefixes, TOP_LEVEL_STRIDE, _ip, pop_score);
            currentInternalIdx++; // 内部节点索引+1
        }
    }
}

/**
 * @brief Poptrie创建入口函数：初始化整个Poptrie结构
 * 作用：对外提供统一的创建接口，衔接前缀排序、内存清空、顶层构建
 * 流程：
 * 1. 调用clear()清空之前的结构（避免重复创建导致内存泄漏）
 * 2. 边界检查：若前缀列表为空，打印提示并返回
 * 3. 前缀排序：按长度降序排序（确保分组后每个范围的第一个前缀是最长匹配）
 * 4. 构建顶层结构：调用buildTopLevel()创建顶层索引和内部节点
 * @param prefixes 待处理的前缀列表（未排序）
 * @param ps 程序状态对象（当前未使用，预留扩展）
 */
void Poptrie_TD::Create(vector<Prefix*> &prefixes, ProgramState *ps) {
    // 步骤1：清空之前的结构（避免内存泄漏）
    clear();
    
    // 步骤2：边界检查：前缀列表为空
    if (prefixes.empty()) {
        cout << "Poptrie_TD::Create() : 前缀列表为空！" << endl;
        return;
    }
    
    // 步骤3：前缀排序：按长度降序（核心！确保最长匹配优先）
    sort(prefixes.begin(), prefixes.end(), 
        [](const Prefix* a, const Prefix* b) {
            return a->prefix_len > b->prefix_len;
        });

    // 步骤4：构建顶层结构（触发后续内部节点的递归创建）
    buildTopLevel(prefixes);
}

/**
 * @brief 带统计的查找函数：实现IP匹配并记录访存次数和深度
 * 作用：用于性能测试，统计每次查找的内存访问次数和匹配深度
 * 流程：
 * 1. 边界检查：若顶层索引表为空，返回0（无匹配）
 * 2. 提取128位IP（IPv6的upper和lower拼接）
 * 3. 顶层查找：
 *    a. 提取顶层范围（IP前16位）
 *    b. 访问indexTable（访存次数+1），获取索引
 *    c. 若索引最高位=1：访问directResult（访存+1），返回结果（深度=1）
 *    d. 若索引最高位=0：进入内部节点查找
 * 4. 内部节点递归查找：
 *    a. 访问当前节点（访存+1，深度+1）
 *    b. 提取当前节点的范围（IP对应区间的位）
 *    c. 若有子节点：访问childNodes（访存+1），更新当前节点和深度
 *    d. 若无子节点：访问directResult（访存+1），返回结果
 * 5. 统计计算：更新ps中的访存次数和深度统计
 * @param trace 待查找的流量（包含IPv6地址）
 * @param ps 程序状态对象（存储统计信息）
 * @return 匹配到的端口号（无匹配返回0）
 */
uint32_t Poptrie_TD::Lookup(Trace *trace, ProgramState *ps) {
    if (topLevel.indexTable == nullptr) return 0; // 边界检查：无结构返回0
    
    // 步骤2：提取128位IPv6地址
    __uint128_t ip = ((__uint128_t)trace->ip6_upper << 64) | trace->ip6_lower;
    uint32_t resultPort = 0; // 匹配结果（端口号）
    
    // 步骤3：顶层查找
    ps->lookup_access.Addcount(); // 访存1：访问顶层indexTable
    // 提取顶层范围（IP前16位：upper的高16位）
    uint32_t topRange = static_cast<uint32_t>(trace->ip6_upper >> (64 - TOP_LEVEL_STRIDE));
    uint32_t topIndex = topLevel.indexTable[topRange];
    
    if (topIndex & (1ULL << 31)) {
        // 步骤3c：顶层直接匹配（无内部节点）
        ps->lookup_access.Addcount(); // 访存2：访问顶层directResult
        uint32_t directIndex = topIndex & ((1ULL << 31) - 1); // 提取低31位索引
        resultPort = topLevel.directResult[directIndex];
        ps->lookup_depth.Addcount(); // 深度1：仅顶层
    } else {
        // 步骤3d：进入内部节点查找
        InternalNode* currentNode = &topLevel.internalNodes[topIndex]; // 顶层内部节点
        uint8_t currentDepth = TOP_LEVEL_STRIDE; // 内部节点起始深度=16
        ps->lookup_depth.Addcount(); // 深度1：顶层
        
        while (true) {
            ps->lookup_access.Addcount(); // 访存：访问当前节点（bitVector/stride）
            // 步骤4b：提取当前节点的范围（IP对应区间的位）
            uint32_t range = extractBits(ip, currentDepth, currentNode->stride);
            if (isBitSet(currentNode, range)) {
                // 步骤4c：有子节点，继续深入
                ps->lookup_access.Addcount(); // 访存：访问childNodes
                uint32_t childIndex = countSetBits(currentNode, range) - 1; // 子节点索引
                currentDepth += currentNode->stride;                        // 更新深度
                currentNode = &currentNode->childNodes[childIndex];         // 更新当前节点
                ps->lookup_depth.Addcount();                                // 深度+1
            } else {
                // 步骤4d：无子节点，返回直接结果
                ps->lookup_access.Addcount(); // 访存：访问directResult
                uint32_t directIndex = range - countSetBits(currentNode, range);
                resultPort = currentNode->directResult[directIndex];
                break; // 退出循环
            }
        }
    }
    
    // 步骤5：统计计算（更新平均访存次数和深度）
    ps->lookup_access.Cal();
    ps->lookup_depth.Cal();
    return resultPort;
}

/**
 * @brief 不带统计的查找接口实现（Classifier基类纯虚函数）
 * 作用：实现基类接口，对外提供统一的无统计查找入口
 * @param trace 待查找的流量
 * @return 匹配到的端口号
 */
uint32_t Poptrie_TD::Lookup(Trace *trace) {
    if (topLevel.indexTable == nullptr) return 0; // 边界检查：无结构返回0
    
    // 提取128位IPv6地址
    __uint128_t ip = ((__uint128_t)trace->ip6_upper << 64) | trace->ip6_lower;
    
    // 顶层查找
    uint32_t topRange = static_cast<uint32_t>(trace->ip6_upper >> (64 - TOP_LEVEL_STRIDE));
    uint32_t topIndex = topLevel.indexTable[topRange];
    
    if (topIndex & (1ULL << 31)) {
        // 顶层直接匹配
        uint32_t directIndex = topIndex & ((1ULL << 31) - 1);
        return topLevel.directResult[directIndex];
    } else {
        // 内部节点查找
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

/**
 * @brief 内存统计接口：返回Poptrie的总内存占用
 * 作用：对外提供内存占用查询，用于性能对比
 * 原理：totalMemory在内存分配时实时累加（new时 += 分配字节数）
 * @return 总内存占用（字节）
 */
uint64_t Poptrie_TD::CalMemory() {
    return totalMemory;
}