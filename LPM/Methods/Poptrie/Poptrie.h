#ifndef POPTRIE_H
#define POPTRIE_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../Classifier.h"

using namespace std;

/**
 * @brief Poptrie算法实现类，继承自分类器基类
 * 实现高效的IPv6前缀查找，采用分层索引结构和位向量优化查找性能
 */
class Poptrie : public Classifier {
public:
    Poptrie();
    ~Poptrie();

    // 实现Classifier接口
    void Create(vector<Prefix*> &prefixes, ProgramState *ps) override;
    uint32_t Lookup(Trace *trace, ProgramState *ps) override;
    uint32_t Lookup(Trace *trace) override;
    uint64_t CalMemory() override;

private:
    /**
     * @brief Poptrie内部节点结构
     * 每个节点包含位向量、步长和两级结果表
     */
    struct InternalNode {
        uint8_t stride;             // 当前节点的步长
        uint64_t bitVector[8];      // 位向量，标记存在子节点的路径（共512位）
        uint32_t* directResult;     // 直接匹配结果表（无子女的路径）
        InternalNode* childNodes;   // 子节点数组（有子女的路径）
    };

    /**
     * @brief 顶层索引结构
     * 包含直接索引表、直接匹配结果表和内部节点指针表
     */
    struct DirectPointer {
        uint32_t* indexTable;       // 顶层索引表
        uint32_t* directResult;     // 直接匹配结果表
        InternalNode* internalNodes;// 内部节点数组
    } topLevel;

    // 常量定义
    static const uint8_t TOP_LEVEL_STRIDE;  // 顶层步长
    uint64_t totalMemory;                   // 总内存占用（字节）

    // 内存管理
    void destroyInternalNode(InternalNode* node);
    void clear();

    // 构建辅助函数
    void buildTopLevel(const vector<Prefix*>& prefixes);
    void buildInternalNode(InternalNode* node, const vector<Prefix*>& prefixes, uint8_t currentDepth);
    unordered_map<uint32_t, vector<Prefix*>> groupPrefixesByRange(
        const vector<Prefix*>& prefixes, uint8_t currentDepth, uint8_t stride);

    // 位操作工具函数
    inline uint32_t extractBits(__uint128_t ip, uint8_t offset, uint8_t length) const;
    inline bool isBitSet(const InternalNode* node, uint32_t index) const;
    inline void setBit(InternalNode* node, uint32_t index);
    inline uint32_t countSetBits(const InternalNode* node, uint32_t index) const;
};

#endif // POPTRIE_H