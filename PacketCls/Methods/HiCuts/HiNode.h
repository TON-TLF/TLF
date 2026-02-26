#ifndef  HINODE_H
#define  HINODE_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../TreeNode.h"
#include "HiCuts.h"

using namespace std;

/**
 * @brief 表示HiCuts树中的一个节点。
 */
class HiNode : public TreeNode {
public:    
    vector<Rule*> rules;   /** 指向规则的指针向量 */ 
    int rules_num;         /** 规则数量 */ 
    bool leaf_node;        /** 是否为叶子节点 */ 
    uint32_t bounds[5][2]; /** 节点范围 */ 

    int cut;               /** 切割数量 */ 
    char dim;              /** 切割维度 */ 
    uint64_t width;        /** 切割宽度 */ 

    HiNode** child_arr;    /** 子节点数组 */
    int child_num;         /** 子节点数量 */

    HiNode();
    void Create(vector<Rule*> &rules, ProgramState *ps, int depth){};  
    void Create(ProgramState *ps, int depth);                          /** 根据给定的规则和深度创建节点 */
    uint64_t CalMemory();                                              /** 计算节点使用的内存大小 */
    void HasBounds();                                                  /** 检查节点的范围是否合法 */
    void RemoveRedund();
    void CalDimToCuts();
    void CalNumCuts();
    bool NodeContainRule(Rule* rule);
    ~HiNode();                                                       /** 释放节点内存 */
};

/**
 * @brief 两个节点是否包含相同的规则集
 * @param node1 节点1指针
 * @param node2 节点2指针
 * @return bool 返回判断结果
 */
bool SameEffiRules(HiNode *node1, HiNode *node2);
#endif