#ifndef  HCNODE_H
#define  HCNODE_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../TreeNode.h"
#include "HyperCuts.h"

using namespace std;

/**
 * @brief 表示HyperCuts树中的一个节点。
 */
class HcNode : public TreeNode {
public:    
    vector<Rule*> rules;   /** 指向规则的指针向量 */ 
    int rules_num;         /** 规则数量 */ 
    bool leaf_node;        /** 是否为叶子节点 */ 
    uint32_t bounds[5][2]; /** 节点范围 */ 

    int cuts[2];           /** 切割数量 */ 
    char dims[2];          /** 切割维度 */ 
    char dims_num;         /** 切割维度数量 */ 
    uint64_t width[2];     /** 切割宽度 */ 

    HcNode** child_arr;    /** 子节点数组 */
    int child_num;         /** 子节点数量 */

    HcNode();
    void Create(vector<Rule*> &rules, ProgramState *ps, int depth){};  
    void Create(ProgramState *ps, int depth);                          /** 根据给定的规则和深度创建节点 */
    uint64_t CalMemory();                                              /** 计算节点使用的内存大小 */
    void HasBounds();                                                  /** 检查节点的范围是否合法 */
    void RemoveRedund();
    void CalDimToCuts();
    void CalNumCuts();
    bool NodeContainRule(Rule* rule);
    ~HcNode();                                                       
};

/**
 * @brief 两个节点是否包含相同的规则集
 * @param node1 节点1指针
 * @param node2 节点2指针
 * @return bool 返回判断结果
 */
bool SameEffiRules(HcNode *node1, HcNode *node2);
#endif