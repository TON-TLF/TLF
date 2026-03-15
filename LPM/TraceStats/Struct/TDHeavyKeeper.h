#ifndef TD_HEAVYKEEPER_H
#define TD_HEAVYKEEPER_H

#include "../../Elements/Elements.h"
#include "TDSSummary.h"
#include "BOBHASH64.h"

constexpr uint32_t HK_ROWS = 2;            
constexpr double HK_DECAY_FACTOR = 1.08;   
constexpr uint32_t HK_EXTRA_SIZE = 10;     
constexpr uint32_t HK_FP_SHIFT = 56;     

class TDHeavyKeeper {
public:
    TDHeavyKeeper() 
        : ss_(nullptr), hkTable_(nullptr), bobhash_(nullptr), k_(0), maxMem_(0) {}

    TDHeavyKeeper(uint32_t topK, uint32_t maxMem) 
        : k_(topK), maxMem_(maxMem), ss_(nullptr), hkTable_(nullptr), bobhash_(nullptr) {
        ss_ = new TDSSummary(topK);

        hkTable_ = new HKNode*[HK_ROWS];
        for (uint32_t i = 0; i < HK_ROWS; ++i) {
            hkTable_[i] = new HKNode[maxMem + HK_EXTRA_SIZE](); 
        }

        bobhash_ = new BOBHash64(1005);
    }

    ~TDHeavyKeeper() {
        if (hkTable_ != nullptr) {
            for (uint32_t i = 0; i < HK_ROWS; ++i) {
                delete[] hkTable_[i];
                hkTable_[i] = nullptr;
            }
            delete[] hkTable_;
            hkTable_ = nullptr;
        }

        delete ss_;
        ss_ = nullptr;
        delete bobhash_;
        bobhash_ = nullptr;
    }

    void clear() {
        assert(ss_ != nullptr);
        ss_->clear();

        for (uint32_t i = 0; i < HK_ROWS; ++i) {
            for (uint32_t j = 0; j < maxMem_ + HK_EXTRA_SIZE; ++j) {
                hkTable_[i][j].C = 0;
                hkTable_[i][j].FP = 0;
            }
        }
    }

    void insert(const std::string& flowStr) {
        uint32_t maxCount = 0;
        const uint64_t hashValue = hashString(flowStr);  
        const uint32_t fingerprint = static_cast<uint32_t>(hashValue >> HK_FP_SHIFT); 

        const uint32_t ssNodeId = ss_->findId(flowStr, hashValue);
        const bool isInSummary = (ssNodeId != 0);

        for (uint32_t row = 0; row < HK_ROWS; ++row) {
            const uint32_t index = calculateTableIndex(hashValue, row);
            HKNode& currentNode = hkTable_[row][index];
            const uint32_t currentCount = currentNode.C;

            if (currentNode.FP == fingerprint) {
                if (isInSummary || currentCount <= pow2(ss_->getMinCount() + 1)) {
                    currentNode.C++;
                }
                maxCount = std::max(maxCount, currentNode.C);
            } else {
                const int decayProbInt = std::pow(HK_DECAY_FACTOR, currentCount);
                if (decayProbInt <= 0) {
                    std::cerr << "TDHeavyKeeper: factor=" << HK_DECAY_FACTOR << ", count=" << currentCount << std::endl;
                    std::exit(1);
                }

                if (std::rand() % decayProbInt == 0) {
                    if (currentNode.C > 0) {
                        currentNode.C--;
                    }
                    if (currentNode.C == 0) {
                        currentNode.FP = fingerprint;
                        currentNode.C = 1;
                        maxCount = std::max(maxCount, static_cast<uint32_t>(1));
                    }
                }
            }
        }
        
        if(maxCount == 0) return;

        const uint32_t logMaxCount = log2(maxCount);
        // cout<<"maxCount = "<<maxCount<<".  logMaxCount = "<<logMaxCount<<endl;
        if (!isInSummary) {
            if ((ss_->total_count_ < k_) || (logMaxCount - ss_->getMinCount() == 1)) {
                // cout<<ss_->total_count_<<" "<<k_<<endl;
                ss_->createNode(flowStr, hashValue, logMaxCount, maxCount);
            }
        } else if (maxCount > ss_->flow_nodes_[ssNodeId].Count){
            ss_->updateNodeCounts(ssNodeId, logMaxCount, maxCount);
        }
    }

    std::vector<TFNode> work() const {
        assert(ss_ != nullptr);
        std::vector<TFNode> result;

        for (uint32_t count = TD_MAX_FLOW_NUM; count != 0 ; count--) {
            for (uint32_t nodeId = ss_->head_nodes_[count].id; nodeId != 0; nodeId = ss_->flow_nodes_[nodeId].nxt) {
                result.emplace_back(
                    ss_->flow_nodes_[nodeId].str, 
                    ss_->flow_nodes_[nodeId].Count
                );
            }
        }

        std::sort(result.begin(), result.end());
        return result;
    }

    uint64_t calculateMemory() const {
        uint64_t totalMemory = 0;

        totalMemory += sizeof(k_);
        totalMemory += sizeof(maxMem_);
        totalMemory += sizeof(ss_);      
        totalMemory += sizeof(bobhash_);  
        totalMemory += sizeof(hkTable_); 

        if (ss_ != nullptr) {
            totalMemory += ss_->calculateMemory();
        }

        if (hkTable_ != nullptr) {
            totalMemory += sizeof(HKNode*) * HK_ROWS;  
            for (uint32_t i = 0; i < HK_ROWS; ++i) {
                if (hkTable_[i] != nullptr) {
                    totalMemory += sizeof(HKNode) * (maxMem_ + HK_EXTRA_SIZE);  
                }
            }
        }

        if (bobhash_ != nullptr) {
            totalMemory += sizeof(*bobhash_);
        }

        return totalMemory;
    }

    void printSummary() const {
        assert(ss_ != nullptr);

        for (uint32_t count = 8; count >= 0; count--) {
            std::cout << "---------------------------------------" << std::endl;
            std::cout << "Count Level: " << count 
                      << ", Head ID: " << ss_->head_nodes_[count].id
                      << ", Left: " << ss_->head_nodes_[count].left
                      << ", Right: " << ss_->head_nodes_[count].right << std::endl;
            
            for (uint32_t nodeId = ss_->head_nodes_[count].id; nodeId != 0; nodeId = ss_->flow_nodes_[nodeId].nxt) {
                std::cout << "Flow: " << ss_->flow_nodes_[nodeId].str
                          << ", Log Count: " << ss_->flow_nodes_[nodeId].logCount
                          << ", Original Count: " << ss_->flow_nodes_[nodeId].Count << std::endl;
            }
            std::cout << "---------------------------------------" << std::endl;
        }
    }

private:
    struct HKNode {
        uint32_t C;   
        uint32_t FP;  
    };

    uint64_t hashString(const std::string& flowStr) const {
        return bobhash_->run(flowStr.c_str(), flowStr.size());
    }

    uint32_t pow2(uint32_t x) const {
        assert(x <= 31);
        return 1U << x;
    }

    uint32_t log2(uint32_t x) const {
        assert(x > 0);
        uint32_t left = 0, right = 31;
        while (left < right) {
            const uint32_t mid = (left + right + 1) >> 1;
            if ((x >> mid) > 0) {
                left = mid;
            } else {
                right = mid - 1;
            }
        }
        return left;
    }

    uint32_t calculateTableIndex(uint64_t hashValue, uint32_t row) const {
        const uint32_t divisor = maxMem_ - 2 * HK_ROWS + 2 * row + 3;
        assert(divisor > 0);
        return static_cast<uint32_t>(hashValue % divisor);
    }

    uint32_t k_;                 
    uint32_t maxMem_;            
    TDSSummary* ss_;            
    BOBHash64* bobhash_;        
    HKNode** hkTable_;           
};

#endif  // TD_HEAVYKEEPER_H