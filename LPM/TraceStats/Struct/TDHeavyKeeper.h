#ifndef TD_HEAVYKEEPER_H
#define TD_HEAVYKEEPER_H

#include "../../Elements/Elements.h"
#include "TDSSummary.h"
#include "BOBHASH64.h"

constexpr uint32_t HK_ROWS = 2;            // 哈希表行数
constexpr double HK_DECAY_FACTOR = 1.08;   // 计数器衰减因子
constexpr uint32_t HK_EXTRA_SIZE = 10;     // 哈希表列冗余
constexpr uint32_t HK_FP_SHIFT = 56;       // 指纹提取：64位哈希值右移56位（取高8位）

class TDHeavyKeeper {
public:
    /**
     * @brief 默认构造函数
     */
    TDHeavyKeeper() 
        : ss_(nullptr), hkTable_(nullptr), bobhash_(nullptr), k_(0), maxMem_(0) {}

    /**
     * @brief 带参构造函数：初始化HeavyKeeper核心组件
     * @param topK 保留的Top-K重流数量
     * @param maxMem 哈希表单行最大列数（总大小：HK_ROWS × (maxMem + HK_EXTRA_SIZE)）
     */
    TDHeavyKeeper(uint32_t topK, uint32_t maxMem) 
        : k_(topK), maxMem_(maxMem), ss_(nullptr), hkTable_(nullptr), bobhash_(nullptr) {
        ss_ = new TDSSummary(topK);

        // 初始化哈希表：二维数组（HK_ROWS行，maxMem+HK_EXTRA_SIZE列）
        hkTable_ = new HKNode*[HK_ROWS];
        for (uint32_t i = 0; i < HK_ROWS; ++i) {
            hkTable_[i] = new HKNode[maxMem + HK_EXTRA_SIZE](); // 零初始化：C=0, FP=0
        }

        // 初始化64位哈希对象
        bobhash_ = new BOBHash64(1005);
    }

    /**
     * @brief 析构函数：释放所有动态内存，避免泄漏
     */
    ~TDHeavyKeeper() {
        // 释放哈希表：先释放列数组，再释放行指针
        if (hkTable_ != nullptr) {
            for (uint32_t i = 0; i < HK_ROWS; ++i) {
                delete[] hkTable_[i];
                hkTable_[i] = nullptr;
            }
            delete[] hkTable_;
            hkTable_ = nullptr;
        }

        // 释放TDSSummary和哈希对象
        delete ss_;
        ss_ = nullptr;
        delete bobhash_;
        bobhash_ = nullptr;
    }

    /**
     * @brief 清空所有数据：重置哈希表计数器/指纹 + 清空TDSSummary
     */
    void clear() {
        assert(ss_ != nullptr && "TDSSummary未初始化");
        ss_->clear();

        // 重置哈希表所有节点的计数器和指纹
        for (uint32_t i = 0; i < HK_ROWS; ++i) {
            for (uint32_t j = 0; j < maxMem_ + HK_EXTRA_SIZE; ++j) {
                hkTable_[i][j].C = 0;
                hkTable_[i][j].FP = 0;
            }
        }
    }

    /**
     * @brief 插入流量字符串到HeavyKeeper
     * @param flowStr 待插入的流量字符串（如IP、端口组合）
     */
    void insert(const std::string& flowStr) {
        uint32_t maxCount = 0;
        const uint64_t hashValue = hashString(flowStr);  // 计算64位哈希值
        const uint32_t fingerprint = static_cast<uint32_t>(hashValue >> HK_FP_SHIFT);  // 提取高8位指纹

        // 检查流量是否已在TDSSummary中
        const uint32_t ssNodeId = ss_->findId(flowStr, hashValue);
        const bool isInSummary = (ssNodeId != 0);

        // 遍历所有哈希表行，更新计数器
        for (uint32_t row = 0; row < HK_ROWS; ++row) {
            const uint32_t index = calculateTableIndex(hashValue, row);
            HKNode& currentNode = hkTable_[row][index];
            const uint32_t currentCount = currentNode.C;

            if (currentNode.FP == fingerprint) {
                // 指纹匹配：更新计数器
                if (isInSummary || currentCount <= pow2(ss_->getMinCount() + 1)) {
                    currentNode.C++;
                }
                maxCount = std::max(maxCount, currentNode.C);
            } else {
                // 指纹不匹配：按概率衰减计数器
                const int decayProbInt = std::pow(HK_DECAY_FACTOR, currentCount);
                // 检查概率计算有效性（避免除以0）
                if (decayProbInt <= 0) {
                    std::cerr << "TDHeavyKeeper: 衰减概率计算错误！factor=" 
                              << HK_DECAY_FACTOR << ", count=" << currentCount << std::endl;
                    std::exit(1);
                }

                // 以1/decayProbInt的概率衰减计数器
                if (std::rand() % decayProbInt == 0) {
                    if (currentNode.C > 0) {
                        currentNode.C--;
                    }
                    // 计数器归零时，替换为当前流量的指纹和初始计数
                    if (currentNode.C == 0) {
                        currentNode.FP = fingerprint;
                        currentNode.C = 1;
                        maxCount = std::max(maxCount, static_cast<uint32_t>(1));
                    }
                }
            }
        }
        
        if(maxCount == 0) return;

        // 计算计数的对数（用于TDSSummary排序）
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

    /**
     * @brief 导出所有重流结果（按字符串排序）
     * @return 包含流量字符串和原始计数的向量
     */
    std::vector<TFNode> work() const {
        assert(ss_ != nullptr && "TDSSummary未初始化");
        std::vector<TFNode> result;

        // 遍历TDSSummary所有有效节点（直接访问head/node成员，与TDSSummary对齐）
        for (uint32_t count = TD_MAX_FLOW_NUM; count != 0 ; count--) {
            for (uint32_t nodeId = ss_->head_nodes_[count].id; nodeId != 0; nodeId = ss_->flow_nodes_[nodeId].nxt) {
                result.emplace_back(
                    ss_->flow_nodes_[nodeId].str, 
                    ss_->flow_nodes_[nodeId].Count
                );
            }
        }

        // 按流量字符串排序（保持原逻辑）
        std::sort(result.begin(), result.end());
        return result;
    }

    /**
     * @brief 计算当前数据结构总内存占用（字节）
     * @return 总内存大小（包含TDSSummary、哈希表、成员变量）
     */
    uint64_t calculateMemory() const {
        uint64_t totalMemory = 0;

        // 1. 基本成员变量内存（含指针本身）
        totalMemory += sizeof(k_);
        totalMemory += sizeof(maxMem_);
        totalMemory += sizeof(ss_);       // TDSSummary指针
        totalMemory += sizeof(bobhash_);  // BOBHash64指针
        totalMemory += sizeof(hkTable_);  // 哈希表指针

        // 2. TDSSummary内存（调用TDSSummary::calculateMemory）
        if (ss_ != nullptr) {
            totalMemory += ss_->calculateMemory();
        }

        // 3. HeavyKeeper哈希表内存（HK_ROWS行 × (maxMem_+HK_EXTRA_SIZE)列）
        if (hkTable_ != nullptr) {
            totalMemory += sizeof(HKNode*) * HK_ROWS;  // 行指针数组
            for (uint32_t i = 0; i < HK_ROWS; ++i) {
                if (hkTable_[i] != nullptr) {
                    totalMemory += sizeof(HKNode) * (maxMem_ + HK_EXTRA_SIZE);  // 每行列数组
                }
            }
        }

        // 4. BOBHash64对象内存（指针指向的对象大小）
        if (bobhash_ != nullptr) {
            totalMemory += sizeof(*bobhash_);
        }

        return totalMemory;
    }

    /**
     * @brief 调试用：打印TDSSummary中的所有节点（与TDSSummary成员对齐）
     */
    void printSummary() const {
        assert(ss_ != nullptr && "TDSSummary未初始化");

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
        uint32_t C;   // 计数器：记录流量出现次数
        uint32_t FP;  // 指纹：判断哈希冲突（与流量哈希值关联）
    };

    /**
     * @brief 计算字符串的64位哈希值（调用BOBHASH64）
     * @param flowStr 待哈希的流量字符串
     * @return 64位无符号哈希值
     */
    uint64_t hashString(const std::string& flowStr) const {
        return bobhash_->run(flowStr.c_str(), flowStr.size());
    }

    /**
     * @brief 计算2的幂（x≤31，避免uint32_t溢出）
     * @param x 指数（0≤x≤31）
     * @return 2^x的结果
     */
    uint32_t pow2(uint32_t x) const {
        assert(x <= 31 && "pow2指数超过31，会导致uint32_t溢出");
        return 1U << x;
    }

    /**
     * @brief 计算整数的对数（以2为底，向上取整）
     * @param x 输入整数（x≥0）
     * @return log2(x)的结果（x=0时返回0）
     */
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

    /**
     * @brief 计算当前行的哈希表索引
     * @param hashValue 流量的64位哈希值
     * @param row 当前哈希表行号
     * @return 非负索引值
     */
    uint32_t calculateTableIndex(uint64_t hashValue, uint32_t row) const {
        const uint32_t divisor = maxMem_ - 2 * HK_ROWS + 2 * row + 3;
        assert(divisor > 0 && "HeavyKeeper哈希表除数为负，需调整maxMem大小");
        return static_cast<uint32_t>(hashValue % divisor);
    }

    // ------------------------------ 私有成员变量（规范命名+类型） ------------------------------
    uint32_t k_;                  // 保留的Top-K重流数量
    uint32_t maxMem_;             // 哈希表单行列数
    TDSSummary* ss_;              // 指向内部TDSSummary的指针
    BOBHash64* bobhash_;          // 64位哈希计算对象
    HKNode** hkTable_;            // HeavyKeeper哈希表
};

#endif  // TD_HEAVYKEEPER_H