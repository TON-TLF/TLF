#ifndef TD_SUMMARY_H
#define TD_SUMMARY_H

#include "../../Elements/Elements.h"

// 常量规范：用明确宽度类型，避免int的平台差异
constexpr uint32_t TD_HASH_BITS = 12;                    // 哈希索引位数（低12位）
constexpr uint32_t TD_MAX_HASH = 1U << TD_HASH_BITS;     // 哈希表大小：2^12=4096（无符号避免溢出）
constexpr uint64_t TD_HASH_MASK = (1ULL << TD_HASH_BITS) - 1;  // 64位掩码：匹配哈希值类型

constexpr uint32_t TD_MAX_FLOW_NUM = 64;       // 最大流量计数（32位足够）
constexpr uint32_t TD_MAX_FLOW_SIZE = 5000;     // 流摘要最大容量（32位足够）
constexpr uint32_t TD_EXTRA_SIZE = 10;         // 额外空间冗余（32位足够）

class TDSSummary {
public:
    /**
     * @brief 构造函数
     * @param topK 需保留的Top-K重要流数量（uint32_t：规范计数类型）
     */
    explicit TDSSummary(uint32_t topK) : k_(topK) {
        clear();
    }

    /**
     * @brief 析构函数：释放动态分配的哈希对象
     */
    ~TDSSummary() {
    }

    /**
     * @brief 清空所有数据，恢复初始状态
     */
    void clear() {
        total_count_ = 0;
        free_id_count_ = TD_MAX_FLOW_SIZE;

        // 空闲ID数组：元素类型为uint32_t（节点ID是32位索引）
        for (uint16_t id = 0; id < TD_MAX_FLOW_SIZE + TD_EXTRA_SIZE; ++id) {
            free_ids_[id] = id;
        }

        // 头节点数组：成员为uint32_t（计数/索引类型）
        for (uint32_t i = 0; i < TD_MAX_FLOW_NUM + TD_EXTRA_SIZE; ++i) {
            head_nodes_[i].id = 0;
            head_nodes_[i].left = 0;
            head_nodes_[i].right = 0;
        }

        // 流量节点数组：成员为uint32_t（计数/索引类型）
        for (uint32_t i = 0; i < TD_MAX_FLOW_SIZE + TD_EXTRA_SIZE; ++i) {
            flow_nodes_[i].str.clear();
            flow_nodes_[i].logCount = 0;
            flow_nodes_[i].pre = 0;
            flow_nodes_[i].nxt = 0;
            flow_nodes_[i].Count = 0;
        }

        // 哈希表：数组元素为uint32_t（索引类型），memset按字节清零兼容
        std::memset(hash_heads_, 0, sizeof(hash_heads_));
        std::memset(hash_next_, 0, sizeof(hash_next_));
    }

    /**
     * @brief 计算当前数据结构占用的内存大小（字节）
     * @return 总内存大小（uint64_t：避免内存计算溢出）
     */
    uint64_t calculateMemory() const {
        uint64_t total_memory = 0;

        // 基本成员变量内存（按实际类型大小计算）
        total_memory += sizeof(k_);
        total_memory += sizeof(total_count_);
        total_memory += sizeof(free_id_count_);

        // 数组内存（按数组实际类型+大小计算）
        total_memory += sizeof(free_ids_);          // 空闲ID数组（uint32_t数组）
        total_memory += sizeof(head_nodes_);        // 头节点数组（HeadNode结构体数组）
        total_memory += sizeof(flow_nodes_);        // 流量节点数组（FlowNode结构体数组）
        total_memory += sizeof(hash_heads_);        // 哈希表头数组（uint32_t数组）
        total_memory += sizeof(hash_next_);         // 哈希表后继数组（uint32_t数组）

        return total_memory;
    }

    /**
     * @brief 头节点结构体：管理相同流量计数的节点链表
     * 成员均为uint32_t：id（节点索引）、left/right（计数索引）
     */
    struct HeadNode {
        uint16_t id;       // 当前计数对应的节点链表头ID（索引类型）
        uint16_t left;     // 左侧（计数更小）的头节点ID（计数类型）
        uint16_t right;    // 右侧（计数更大）的头节点ID（计数类型）
    };

    /**
     * @brief 流量节点结构体：存储单条流量的详细信息
     * logCount/start_sum（计数）、pre/nxt（节点索引）均为uint32_t
     */
    struct FlowNode {
        uint16_t pre;         // 链表前驱节点ID（索引类型）
        uint16_t nxt;         // 链表后继节点ID（索引类型）
        uint16_t hash;        // 流量的哈希值
        uint16_t logCount;    // 该流量的总条数（计数类型）
        uint32_t Count;       // 初始计数（计数类型）
        std::string str;      // 流量的字符串标识
    };

    /**
     * @brief 计算哈希值对应的哈希表位置（位运算）
     * @param hash 预处理的哈希值（uint64_t：匹配哈希值的64位宽度）
     * @return 哈希表中的索引位置（uint32_t：索引类型，范围0~4095）
     */
    uint16_t getLocation(uint64_t hash) const {
        // 64位哈希值提取低12位
        return static_cast<uint16_t>(hash & TD_HASH_MASK);
    }

    /**
     * @brief 在哈希表中查找流量对应的节点ID
     * @param str 流量的字符串标识
     * @param hash 流量的哈希值（uint64_t：规范哈希值类型）
     * @return 找到的节点ID（uint32_t：索引类型），未找到则返回0
     */
    uint32_t findId(const std::string& str, uint64_t hash) const {
        for (uint32_t id = hash_heads_[getLocation(hash)]; id != 0; id = hash_next_[id]) {
            if (flow_nodes_[id].str == str) {
                return id;
            }
        }
        return 0;
    }

    /**
     * @brief 将头节点插入到头节点双向链表中（插入到pre之后）
     * @param pre 前驱头节点的计数（uint32_t：计数类型）
     * @param curr 当前头节点的计数（uint32_t：计数类型）
     */
    void linkHead(uint16_t pre, uint16_t curr) {
        head_nodes_[curr].left = pre;
        head_nodes_[curr].right = head_nodes_[pre].right;
        head_nodes_[pre].right = curr;
        head_nodes_[head_nodes_[curr].right].left = curr;
    }

    /**
     * @brief 从头节点双向链表中移除指定头节点
     * @param id 待移除的头节点计数（uint32_t：计数类型）
     */
    void cutHead(uint32_t id) {
        uint16_t left = head_nodes_[id].left;
        uint16_t right = head_nodes_[id].right;
        head_nodes_[left].right = right;
        head_nodes_[right].left = left;
    }

    /**
     * @brief 获取当前最小的有效流量计数
     * @return 最小计数（uint32_t：计数类型）
     */
    uint16_t getMinCount() const {
        if (head_nodes_[0].id != 0) {
            return 0;
        }
        return head_nodes_[0].right;
    }

    /**
     * @brief 将节点插入到对应计数的链表中
     * @param pre_count 前驱计数（uint32_t：计数类型）
     * @param node_id 待插入的节点ID（uint32_t：索引类型）
     */
    void linkNode(uint32_t preLogCount, uint32_t nodeId) {
        ++total_count_;
        uint16_t logCount = flow_nodes_[nodeId].logCount;

        // 如果 logCount == 0，使用单独的更新逻辑，因为 head_nodes_[0] 充当头节点，必须一直存在
        if(logCount == 0){
            flow_nodes_[nodeId].nxt = head_nodes_[0].id;
            if (flow_nodes_[nodeId].nxt != 0) {
                flow_nodes_[flow_nodes_[nodeId].nxt].pre = nodeId;
            }
            flow_nodes_[nodeId].pre = 0;
            head_nodes_[0].id = nodeId;
        } else {
            bool isNewList = (head_nodes_[logCount].id == 0);

            flow_nodes_[nodeId].nxt = head_nodes_[logCount].id;
            if (flow_nodes_[nodeId].nxt != 0) {
                flow_nodes_[flow_nodes_[nodeId].nxt].pre = nodeId;
            }
            flow_nodes_[nodeId].pre = 0;
            head_nodes_[logCount].id = nodeId;

            // 若为新链表，需将其插入头节点双向链表中
            if (isNewList) {
                // 优先插入到临近的较小计数后
                for (uint32_t j = logCount - 1; j > 0 && j > logCount - 10; --j) {
                    if (head_nodes_[j].id != 0) {
                        linkHead(j, logCount);
                        return;
                    }
                }
                linkHead(preLogCount, logCount);
            }
        }
    }

    /**
     * @brief 从链表中移除指定节点
     * @param node_id 待移除的节点ID（uint32_t：索引类型）
     */
    void cutNode(uint32_t node_id) {
        --total_count_;
        uint16_t logCount = flow_nodes_[node_id].logCount;

        // 更新链表头（若当前节点是头节点）
        if (head_nodes_[logCount].id == node_id) {
            head_nodes_[logCount].id = flow_nodes_[node_id].nxt;

            // 若链表为空，移除对应的头节点
            if (head_nodes_[logCount].id == 0 && logCount != 0) {
                cutHead(logCount);
                return;
            }
            flow_nodes_[head_nodes_[logCount].id].pre = 0;

        } else if(flow_nodes_[node_id].nxt == 0){
            uint16_t pre = flow_nodes_[node_id].pre;
            flow_nodes_[pre].nxt = 0;

        } else {
            uint16_t pre = flow_nodes_[node_id].pre;
            uint16_t nxt = flow_nodes_[node_id].nxt;
            flow_nodes_[pre].nxt = nxt;
            flow_nodes_[nxt].pre = pre;
        }
    }

    /**
     * 创建新节点并添加到Summary
     * @param flowStr 流量字符串
     * @param hash 流量哈希值
     * @param logCount 对数计数（sum）
     * @param originalCount 原始计数（start_sum）
     * @param preCount 插入链表的前驱计数（通常为0）
     * @return 新节点ID（失败返回0）
     */
    uint16_t createNode(const std::string& flowStr, uint64_t hash, 
                        uint16_t logCount, uint32_t originalCount) {
        removeMinNode();
        if (free_id_count_ == 0) {
            std::cerr << "TDSSummary: 节点ID耗尽！" << std::endl;
            exit(1);
        }

        // 1. 分配空闲ID
        uint16_t nodeId = free_ids_[free_id_count_--];

        // 2. 初始化节点数据
        flow_nodes_[nodeId].str = flowStr;
        flow_nodes_[nodeId].logCount = logCount;
        flow_nodes_[nodeId].Count = originalCount;
        flow_nodes_[nodeId].hash = getLocation(hash);
        flow_nodes_[nodeId].pre = 0;
        flow_nodes_[nodeId].nxt = 0;
        hash_next_[nodeId] = 0;

        // 3. 添加到哈希表
        uint16_t pos = flow_nodes_[nodeId].hash;
        hash_next_[nodeId] = hash_heads_[pos];
        hash_heads_[pos] = nodeId;

        // 4. 插入到计数链表
        linkNode(0, nodeId);
        return nodeId;
    }

    /**
     * 更新节点的计数（sum和start_sum）并调整链表位置
     * @param nodeId 节点ID
     * @param newLogCount 新的对数计数
     * @param newOriginalCount 新的原始计数
     */
    void updateNodeCounts(uint32_t nodeId, uint32_t newLogCount, uint32_t newCount) {
        uint32_t oldLogCount = flow_nodes_[nodeId].logCount;
        if (oldLogCount == newLogCount) {
            // 计数未变，仅更新原始计数
            flow_nodes_[nodeId].Count = newCount;
            return;
        }

        // 1. 从原计数链表中移除
        uint32_t preLogCount = head_nodes_[oldLogCount].left;
        cutNode(nodeId);  // 调用内部cutNode

        // 2. 更新计数
        flow_nodes_[nodeId].logCount = newLogCount;
        flow_nodes_[nodeId].Count = newCount;

        // 3. 插入新计数链表
        if (head_nodes_[oldLogCount].id != 0) {
            preLogCount = oldLogCount;
        }
        linkNode(preLogCount, nodeId);  // 调用内部linkNode
    }

    /**
     * 移除并回收最小计数的节点（用于Top-K修剪）
     */
    void removeMinNode() {
        while (total_count_ >= k_) {
            // cout<<"removeMinNode  total_count_ = "<<total_count_<<"  k = "<<k_<<endl;
            uint32_t minCount = getMinCount();
            uint16_t minNodeId = head_nodes_[minCount].id;
            if (minNodeId == 0){
                std::cerr << "TDSSummary: 无法移除节点，链表为空！" << std::endl;
                exit(1);
            } 

            // 1. 从链表中移除
            cutNode(minNodeId);

            // 2. 从哈希表中移除并回收ID
            std::string flowStr = flow_nodes_[minNodeId].str;
            uint16_t pos = flow_nodes_[minNodeId].hash;
            if (hash_heads_[pos] == minNodeId) {
                hash_heads_[pos] = hash_next_[minNodeId];
            } else {
                for (uint32_t j = hash_heads_[pos]; j != 0; j = hash_next_[j]) {
                    if (hash_next_[j] == minNodeId) {
                        hash_next_[j] = hash_next_[minNodeId];
                        break;
                    }
                }
            }
            free_ids_[++free_id_count_] = minNodeId;
        }
    }

    // 成员变量规范：按用途匹配明确宽度类型
    uint32_t k_;                          // 需保留的Top-K流量数量（计数类型）
    uint32_t total_count_;                // 总流量条数（计数类型）
    uint32_t free_id_count_;              // 空闲节点ID数量（计数类型）
    uint16_t free_ids_[TD_MAX_FLOW_SIZE + TD_EXTRA_SIZE];  // 空闲节点ID池（索引数组）

    HeadNode head_nodes_[TD_MAX_FLOW_NUM + TD_EXTRA_SIZE];  // 头节点数组（计数组织）
    FlowNode flow_nodes_[TD_MAX_FLOW_SIZE + TD_EXTRA_SIZE];  // 流量节点数组（数据存储）

    uint16_t hash_heads_[TD_MAX_HASH + TD_EXTRA_SIZE];     // 哈希表头数组（索引数组）
    uint16_t hash_next_[TD_MAX_FLOW_SIZE + TD_EXTRA_SIZE]; // 哈希表后继数组（索引数组）
};

#endif  // TD_SUMMARY_H