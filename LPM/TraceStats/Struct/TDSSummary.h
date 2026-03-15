#ifndef TD_SUMMARY_H
#define TD_SUMMARY_H

#include "../../Elements/Elements.h"

constexpr uint32_t TD_HASH_BITS = 12;                    
constexpr uint32_t TD_MAX_HASH = 1U << TD_HASH_BITS;     
constexpr uint64_t TD_HASH_MASK = (1ULL << TD_HASH_BITS) - 1;  

constexpr uint32_t TD_MAX_FLOW_NUM = 64;       
constexpr uint32_t TD_MAX_FLOW_SIZE = 5000;     
constexpr uint32_t TD_EXTRA_SIZE = 10;         

class TDSSummary {
public:
    explicit TDSSummary(uint32_t topK) : k_(topK) {
        clear();
    }

    ~TDSSummary() {
    }

    void clear() {
        total_count_ = 0;
        free_id_count_ = TD_MAX_FLOW_SIZE;

        for (uint16_t id = 0; id < TD_MAX_FLOW_SIZE + TD_EXTRA_SIZE; ++id) {
            free_ids_[id] = id;
        }

        for (uint32_t i = 0; i < TD_MAX_FLOW_NUM + TD_EXTRA_SIZE; ++i) {
            head_nodes_[i].id = 0;
            head_nodes_[i].left = 0;
            head_nodes_[i].right = 0;
        }

        for (uint32_t i = 0; i < TD_MAX_FLOW_SIZE + TD_EXTRA_SIZE; ++i) {
            flow_nodes_[i].str.clear();
            flow_nodes_[i].logCount = 0;
            flow_nodes_[i].pre = 0;
            flow_nodes_[i].nxt = 0;
            flow_nodes_[i].Count = 0;
        }

        std::memset(hash_heads_, 0, sizeof(hash_heads_));
        std::memset(hash_next_, 0, sizeof(hash_next_));
    }

    uint64_t calculateMemory() const {
        uint64_t total_memory = 0;

        total_memory += sizeof(k_);
        total_memory += sizeof(total_count_);
        total_memory += sizeof(free_id_count_);

        total_memory += sizeof(free_ids_);         
        total_memory += sizeof(head_nodes_);        
        total_memory += sizeof(flow_nodes_);        
        total_memory += sizeof(hash_heads_);       
        total_memory += sizeof(hash_next_);        

        return total_memory;
    }

    struct HeadNode {
        uint16_t id;       
        uint16_t left;     
        uint16_t right;    
    };

    struct FlowNode {
        uint16_t pre;        
        uint16_t nxt;        
        uint16_t hash;       
        uint16_t logCount;    
        uint32_t Count;      
        std::string str;    
    };

    uint16_t getLocation(uint64_t hash) const {
        return static_cast<uint16_t>(hash & TD_HASH_MASK);
    }

    uint32_t findId(const std::string& str, uint64_t hash) const {
        for (uint32_t id = hash_heads_[getLocation(hash)]; id != 0; id = hash_next_[id]) {
            if (flow_nodes_[id].str == str) {
                return id;
            }
        }
        return 0;
    }

    void linkHead(uint16_t pre, uint16_t curr) {
        head_nodes_[curr].left = pre;
        head_nodes_[curr].right = head_nodes_[pre].right;
        head_nodes_[pre].right = curr;
        head_nodes_[head_nodes_[curr].right].left = curr;
    }

    void cutHead(uint32_t id) {
        uint16_t left = head_nodes_[id].left;
        uint16_t right = head_nodes_[id].right;
        head_nodes_[left].right = right;
        head_nodes_[right].left = left;
    }

    uint16_t getMinCount() const {
        if (head_nodes_[0].id != 0) {
            return 0;
        }
        return head_nodes_[0].right;
    }

    void linkNode(uint32_t preLogCount, uint32_t nodeId) {
        ++total_count_;
        uint16_t logCount = flow_nodes_[nodeId].logCount;

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

            if (isNewList) {
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

    void cutNode(uint32_t node_id) {
        --total_count_;
        uint16_t logCount = flow_nodes_[node_id].logCount;

        if (head_nodes_[logCount].id == node_id) {
            head_nodes_[logCount].id = flow_nodes_[node_id].nxt;

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

    uint16_t createNode(const std::string& flowStr, uint64_t hash, 
                        uint16_t logCount, uint32_t originalCount) {
        removeMinNode();
        if (free_id_count_ == 0) {
            std::cerr << "TDSSummary: ID over！" << std::endl;
            exit(1);
        }

        uint16_t nodeId = free_ids_[free_id_count_--];
        flow_nodes_[nodeId].str = flowStr;
        flow_nodes_[nodeId].logCount = logCount;
        flow_nodes_[nodeId].Count = originalCount;
        flow_nodes_[nodeId].hash = getLocation(hash);
        flow_nodes_[nodeId].pre = 0;
        flow_nodes_[nodeId].nxt = 0;
        hash_next_[nodeId] = 0;

        uint16_t pos = flow_nodes_[nodeId].hash;
        hash_next_[nodeId] = hash_heads_[pos];
        hash_heads_[pos] = nodeId;
        linkNode(0, nodeId);
        return nodeId;
    }

    void updateNodeCounts(uint32_t nodeId, uint32_t newLogCount, uint32_t newCount) {
        uint32_t oldLogCount = flow_nodes_[nodeId].logCount;
        if (oldLogCount == newLogCount) {
            flow_nodes_[nodeId].Count = newCount;
            return;
        }

        uint32_t preLogCount = head_nodes_[oldLogCount].left;
        cutNode(nodeId);  

        flow_nodes_[nodeId].logCount = newLogCount;
        flow_nodes_[nodeId].Count = newCount;

        if (head_nodes_[oldLogCount].id != 0) {
            preLogCount = oldLogCount;
        }
        linkNode(preLogCount, nodeId);  
    }

    void removeMinNode() {
        while (total_count_ >= k_) {
            // cout<<"removeMinNode  total_count_ = "<<total_count_<<"  k = "<<k_<<endl;
            uint32_t minCount = getMinCount();
            uint16_t minNodeId = head_nodes_[minCount].id;
            if (minNodeId == 0){
                std::cerr << "TDSSummary: 无法移除节点，链表为空！" << std::endl;
                exit(1);
            } 

            cutNode(minNodeId);

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

    uint32_t k_;                          
    uint32_t total_count_;                
    uint32_t free_id_count_;             
    uint16_t free_ids_[TD_MAX_FLOW_SIZE + TD_EXTRA_SIZE];  

    HeadNode head_nodes_[TD_MAX_FLOW_NUM + TD_EXTRA_SIZE];  
    FlowNode flow_nodes_[TD_MAX_FLOW_SIZE + TD_EXTRA_SIZE];  

    uint16_t hash_heads_[TD_MAX_HASH + TD_EXTRA_SIZE];    
    uint16_t hash_next_[TD_MAX_FLOW_SIZE + TD_EXTRA_SIZE]; 
};

#endif  // TD_SUMMARY_H